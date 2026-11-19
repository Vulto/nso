/* starshipmod.cpp - Star Fox 64 (Starship, repos/n64/starship-src) as an NSO
 * in-process module (embed/nsomod.h). The port's tree is never edited: its
 * objects are linked here (Makefile) in two libraries, libsf64game.so (every
 * decompiled C object: the game's variables) and libstarshipmod.so (libultraship,
 * the port glue and this file). libultraship runs a hidden SDL window of its own;
 * its frame, audio and pad hooks (weak symbols in the checked-out libultraship)
 * land in the functions below. A suspend point is the game library's
 * .data/.bss, gzip'd, with its pointers moved on load (the libraries load at
 * another address every run). */
#include <embed/nsomod.h>

#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "port/Engine.h"
#include "port/resource/type/audio/SoundFont.h"
#include "public/bridge/windowbridge.h"
#include <libultraship.h>

/* X11's headers #define None (libultraship has an enum of that name), so the four
 * GLX calls this file needs are declared by hand. */
typedef struct _XDisplay Display;
typedef unsigned long GLXDrawable;
typedef struct __GLXcontextRec* GLXContext;
extern "C" {
Display* glXGetCurrentDisplay(void);
GLXDrawable glXGetCurrentDrawable(void);
GLXContext glXGetCurrentContext(void);
int glXMakeCurrent(Display* dpy, GLXDrawable drawable, GLXContext ctx);
}

extern "C" {
void Main_SetVIMode(void);
void Main_Initialize(void);
void Main_ThreadEntry(void* arg);
void Lib_FillScreen(u8 setFill);
}
void push_frame();   /* port/Game.cpp */

namespace {
constexpr int kW = 640, kH = 480;
constexpr int kInRate = 32000, kHostRate = 48000;
constexpr size_t kAudioCap = 32768;

struct Mod {
    std::array<uint8_t, kW * kH * 3> rgb{};
    std::deque<std::array<int16_t, 2>> audio;
    std::mutex audioMutex;
    double audioPos = 0.0;
    NsomodInput in{};
    SDL_Window* win = nullptr;
    SDL_GLContext ctx = nullptr;
    Display* hostDpy = nullptr;
    GLXDrawable hostDraw = 0;
    GLXContext hostCtx = nullptr;
    bool live = false;
    std::string path;
} M;

bool HostRestore() {
    return M.hostDpy && M.hostCtx && M.hostDraw && glXMakeCurrent(M.hostDpy, M.hostDraw, M.hostCtx) != 0;
}
void GameCurrent() {
    if (M.win && M.ctx) SDL_GL_MakeCurrent(M.win, M.ctx);
}
void GameRelease() {
    SDL_GL_MakeCurrent(nullptr, nullptr);
    HostRestore();
}

bool ModInit(const NsomodConfig* cfg) {
    if (!cfg || !cfg->data_path || !cfg->data_path[0]) return false;
    if (M.live) return M.path == cfg->data_path && (M.hostDpy = glXGetCurrentDisplay(), M.hostDraw = glXGetCurrentDrawable(),
                                                    M.hostCtx = glXGetCurrentContext(), HostRestore());
    M.hostDpy = glXGetCurrentDisplay();
    M.hostDraw = glXGetCurrentDrawable();
    M.hostCtx = glXGetCurrentContext();
    if (!M.hostDpy || !M.hostCtx || !M.hostDraw) return false;
    setenv("SHIP_HOME", cfg->data_path, 1);
    setenv("SHIP_APP_DIR", cfg->data_path, 1);   /* starship.o2r beside sf64.o2r */
    setenv("NSO_EMBED_MODE", "1", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);   /* NSO plays the captured samples */
    GameEngine::Create();
    M.win = SDL_GL_GetCurrentWindow();
    M.ctx = SDL_GL_GetCurrentContext();
    if (!M.win || !M.ctx) { HostRestore(); return false; }
    Main_SetVIMode();
    Lib_FillScreen(1);
    Main_Initialize();
    Main_ThreadEntry(nullptr);
    GameRelease();
    M.path = cfg->data_path;
    M.live = true;
    return true;
}

NsomodFrame ModFrame(const NsomodInput* in) {
    M.in = in ? *in : NsomodInput{};
    if (M.live) {
        GameCurrent();
        WindowIsRunning();
        push_frame();
        GameRelease();
    }
    return { M.rgb.data(), kW, kH };
}

size_t ModAudioPull(int16_t* out, size_t max) {
    std::lock_guard<std::mutex> lock(M.audioMutex);
    size_t n = 0;
    const double step = double(kInRate) / kHostRate;
    while (n < max) {
        const size_t i = size_t(M.audioPos);
        if (i + 1 >= M.audio.size()) break;
        const double f = M.audioPos - double(i);
        for (int c = 0; c < 2; ++c) {
            const double s = M.audio[i][c] + (M.audio[i + 1][c] - M.audio[i][c]) * f;
            out[n * 2 + c] = int16_t(std::clamp(s, -32768.0, 32767.0));
        }
        ++n;
        M.audioPos += step;
    }
    const size_t used = size_t(M.audioPos);
    for (size_t i = 0; i < used && !M.audio.empty(); ++i) M.audio.pop_front();
    M.audioPos -= double(used);
    return n;
}

/* ---- suspend points (see the header comment) ---- */
struct Lib {
    const char* name = nullptr;
    uintptr_t lo = 0, hi = 0;
    uintptr_t rw[4][2] = {};   /* start, size */
    unsigned nrw = 0;
    uint8_t id[20] = {};
};
int LibCb(struct dl_phdr_info* info, size_t, void* data) {
    Lib* L = static_cast<Lib*>(data);
    if (!info->dlpi_name || strcmp(info->dlpi_name, L->name) != 0) return 0;
    uintptr_t relroLo = 0, relroHi = 0;
    L->lo = UINTPTR_MAX;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
        const uintptr_t a = info->dlpi_addr + ph->p_vaddr;
        if (ph->p_type == PT_GNU_RELRO) { relroLo = a; relroHi = a + ph->p_memsz; }
        if (ph->p_type == PT_LOAD) {
            L->lo = std::min(L->lo, a);
            L->hi = std::max(L->hi, a + ph->p_memsz);
        }
        if (ph->p_type == PT_NOTE) {
            const uint8_t *p = reinterpret_cast<const uint8_t*>(a), *end = p + ph->p_memsz;
            while (p + sizeof(ElfW(Nhdr)) <= end) {
                const ElfW(Nhdr)* nh = reinterpret_cast<const ElfW(Nhdr)*>(p);
                const uint8_t* desc = p + sizeof *nh + ((nh->n_namesz + 3) & ~3u);
                if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4 && !memcmp(p + sizeof *nh, "GNU", 4))
                    memcpy(L->id, desc, nh->n_descsz < 20 ? nh->n_descsz : 20);
                p = desc + ((nh->n_descsz + 3) & ~3u);
            }
        }
    }
    for (int i = 0; i < info->dlpi_phnum && L->nrw < 4; i++) {
        const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
        if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_W)) continue;
        uintptr_t a = info->dlpi_addr + ph->p_vaddr;
        const uintptr_t b = a + ph->p_memsz;
        if (relroHi > a && relroHi <= b && relroLo <= a) a = relroHi;   /* RELRO opens the segment */
        if (a < b) { L->rw[L->nrw][0] = a; L->rw[L->nrw][1] = b - a; L->nrw++; }
    }
    return 1;
}
bool LibFind(Lib* L, const void* sym) {
    Dl_info di;
    *L = Lib{};
    if (!dladdr(sym, &di) || !di.dli_fname) return false;
    L->name = di.dli_fname;
    return dl_iterate_phdr(LibCb, L) && L->nrw > 0;
}

/* Pointers the game keeps into libultraship's heap (the data of every loaded
 * resource, GameEngine_Malloc blocks) mean nothing in another process. A state
 * remembers which words of the game's memory hold one (and which resource, by
 * path, or which block it is) and the load points them at this run's copy. The
 * game library is linked with --wrap on GameEngine_Malloc (Makefile) so its
 * blocks are known. */
struct Reg { int kind; std::string name; size_t size; };   /* kind 0 resource, 1 malloc'd block */
std::unordered_map<const void*, Reg> gReg;
std::mutex gRegMutex;

}
extern "C" void* __wrap_GameEngine_Malloc(size_t size);
namespace {
struct StateHead {
    char magic[8];
    uint8_t id[20], modId[20];
    uint32_t nrw;
    uint64_t gameLo, gameSize, modLo, modSize;
    uint64_t rw[4][2];   /* offset from gameLo, size */
};
const char kMagic[8] = "NSOSF64";

bool Head(StateHead* h, Lib* g, Lib* m) {
    if (!LibFind(g, reinterpret_cast<const void*>(&Main_Initialize)) ||
        !LibFind(m, reinterpret_cast<const void*>(&ModFrame)) || g->lo == m->lo) return false;
    memset(h, 0, sizeof *h);
    memcpy(h->magic, kMagic, 8);
    memcpy(h->id, g->id, 20);
    memcpy(h->modId, m->id, 20);
    h->nrw = g->nrw;
    h->gameLo = g->lo; h->gameSize = g->hi - g->lo;
    h->modLo = m->lo; h->modSize = m->hi - m->lo;
    for (unsigned i = 0; i < g->nrw; i++) { h->rw[i][0] = g->rw[i][0] - g->lo; h->rw[i][1] = g->rw[i][1]; }
    return true;
}

void Relocate(uint8_t* p, size_t n, const uint64_t lo[2], const uint64_t size[2], const uint64_t d[2]) {
    if (!d[0] && !d[1]) return;
    for (size_t k = 0; k + 8 <= n;) {
        uint64_t v;
        memcpy(&v, p + k, 8);
        const int i = v - lo[0] <= size[0] ? 0 : v - lo[1] <= size[1] ? 1 : -1;
        if (i < 0) { k += 4; continue; }
        v += d[i];
        memcpy(p + k, &v, 8);
        k += 8;
    }
}

/* Where the game's memory holds a pointer into a registered heap block. */
struct Fix { uint64_t off; uint32_t reg; uint64_t delta; };

bool GzPut(gzFile z, const void* p, size_t n) { return gzwrite(z, p, unsigned(n)) == int(n); }
bool GzGet(gzFile z, void* p, size_t n) { return gzread(z, p, unsigned(n)) == int(n); }

bool ModStateSave(const char* path) {
    StateHead h;
    Lib g, m;
    if (!M.live || !path || !Head(&h, &g, &m)) return false;
    struct Range { uintptr_t lo, hi; uint32_t reg; };
    std::vector<Reg> regs;
    std::vector<Range> ranges;
    {
        std::lock_guard<std::mutex> lock(gRegMutex);
        Ship::Context::GetInstance()->GetResourceManager()->ForEachCachedResource(
            [&](const std::string& name, Ship::IResource* res) {
                void* p = res->GetRawPointer();
                if (!p) return;
                const size_t n = res->GetPointerSize();
                ranges.push_back({ reinterpret_cast<uintptr_t>(p), reinterpret_cast<uintptr_t>(p) + n, uint32_t(regs.size()) });
                regs.push_back(Reg{ 0, name, n });
                /* the game copies a font's header into its own memory, keeping the
                 * font's instrument and drum arrays (vectors inside the resource) */
                if (auto* font = dynamic_cast<SF64::SoundFont*>(res)) {
                    if (!font->mInstruments.empty())
                        regs.push_back(Reg{ 2, name, font->mInstruments.size() * sizeof(void*) }),
                        ranges.push_back({ reinterpret_cast<uintptr_t>(font->mInstruments.data()),
                            reinterpret_cast<uintptr_t>(font->mInstruments.data()) + regs.back().size, uint32_t(regs.size() - 1) });
                    if (!font->mDrums.empty())
                        regs.push_back(Reg{ 3, name, font->mDrums.size() * sizeof(void*) }),
                        ranges.push_back({ reinterpret_cast<uintptr_t>(font->mDrums.data()),
                            reinterpret_cast<uintptr_t>(font->mDrums.data()) + regs.back().size, uint32_t(regs.size() - 1) });
                }
            });
        for (auto& e : gReg) {
            Reg r = e.second;
            if (r.kind == 0) continue;
            ranges.push_back({ reinterpret_cast<uintptr_t>(e.first), reinterpret_cast<uintptr_t>(e.first) + r.size,
                               uint32_t(regs.size()) });
            regs.push_back(std::move(r));
        }
    }
    std::sort(ranges.begin(), ranges.end(), [](const Range& a, const Range& b) { return a.lo < b.lo; });
    std::vector<Fix> fixes;
    uint64_t base = 0;
    for (unsigned i = 0; i < g.nrw; i++) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(g.rw[i][0]);
        for (size_t k = 0; k + 8 <= g.rw[i][1]; k += 4) {
            uint64_t v;
            memcpy(&v, p + k, 8);
            auto it = std::upper_bound(ranges.begin(), ranges.end(), v, [](uint64_t x, const Range& r) { return x < r.lo; });
            if (it == ranges.begin()) continue;
            --it;
            if (v >= it->lo && v <= it->hi) fixes.push_back({ base + k, it->reg, v - it->lo });
        }
        base += g.rw[i][1];
    }
    gzFile z = gzopen(path, "wb1");
    if (!z) return false;
    bool ok = GzPut(z, &h, sizeof h);
    for (unsigned i = 0; ok && i < g.nrw; i++) ok = GzPut(z, reinterpret_cast<const void*>(g.rw[i][0]), g.rw[i][1]);
    const uint32_t counts[2] = { uint32_t(regs.size()), uint32_t(fixes.size()) };
    ok = ok && GzPut(z, counts, sizeof counts);
    for (const Reg& r : regs) {
        const uint32_t head[2] = { uint32_t(r.kind), uint32_t(r.name.size()) };
        const uint64_t size = r.size;
        ok = ok && GzPut(z, head, sizeof head) && GzPut(z, &size, sizeof size) && GzPut(z, r.name.data(), r.name.size());
    }
    ok = ok && (fixes.empty() || GzPut(z, fixes.data(), fixes.size() * sizeof(Fix)));
    return gzclose(z) == Z_OK && ok;
}

bool ModStateLoad(const char* path) {
    StateHead now, h;
    Lib g, m;
    if (!M.live || !path || access(path, R_OK) != 0 || !Head(&now, &g, &m)) return false;
    gzFile z = gzopen(path, "rb");
    if (!z) return false;
    bool ok = GzGet(z, &h, sizeof h) && !memcmp(h.magic, kMagic, 8) && !memcmp(h.id, now.id, 20) &&
              !memcmp(h.modId, now.modId, 20) && h.nrw == now.nrw && h.gameSize == now.gameSize &&
              h.modSize == now.modSize && !memcmp(h.rw, now.rw, sizeof h.rw);
    size_t total = 0;
    for (unsigned i = 0; ok && i < g.nrw; i++) total += g.rw[i][1];
    uint8_t* buf = ok ? static_cast<uint8_t*>(malloc(total)) : nullptr;
    size_t at = 0;
    for (unsigned i = 0; buf && i < g.nrw && ok; i++) {   /* all read before any byte of the game changes */
        ok = GzGet(z, buf + at, g.rw[i][1]);
        at += g.rw[i][1];
    }
    uint32_t counts[2] = { 0, 0 };
    std::vector<Reg> regs;
    std::vector<Fix> fixes;
    ok = ok && buf && GzGet(z, counts, sizeof counts) && counts[0] < (1u << 24) && counts[1] < (1u << 26);
    for (uint32_t i = 0; ok && i < counts[0]; i++) {
        uint32_t head[2];
        uint64_t size;
        ok = GzGet(z, head, sizeof head) && GzGet(z, &size, sizeof size) && head[1] < 4096;
        Reg r{ int(head[0]), std::string(ok ? head[1] : 0, '\0'), size_t(size) };
        ok = ok && GzGet(z, r.name.data(), head[1]);
        regs.push_back(std::move(r));
    }
    fixes.resize(ok ? counts[1] : 0);
    ok = ok && (fixes.empty() || GzGet(z, fixes.data(), fixes.size() * sizeof(Fix)));
    gzclose(z);
    if (!buf || !ok) { free(buf); return false; }
    /* this run's copy of every heap block the state points into */
    GameCurrent();
    std::vector<uint8_t*> now_at(regs.size(), nullptr);
    std::vector<char> wanted(regs.size(), 0);
    for (const Fix& f : fixes) if (f.reg < regs.size()) wanted[f.reg] = 1;
    for (size_t i = 0; ok && i < regs.size(); i++) {
        if (!wanted[i]) continue;
        if (regs[i].kind == 1) {
            now_at[i] = static_cast<uint8_t*>(__wrap_GameEngine_Malloc(regs[i].size));
        } else if (regs[i].kind == 0) {
            now_at[i] = static_cast<uint8_t*>(ResourceGetDataByName(regs[i].name.c_str()));
        } else {
            auto* font = dynamic_cast<SF64::SoundFont*>(ResourceLoad(regs[i].name.c_str()).get());
            now_at[i] = !font ? nullptr : regs[i].kind == 2 ? reinterpret_cast<uint8_t*>(font->mInstruments.data())
                                                            : reinterpret_cast<uint8_t*>(font->mDrums.data());
        }
        ok = now_at[i] != nullptr;
    }
    GameRelease();
    if (!ok) { free(buf); return false; }
    const uint64_t lo[2] = { h.gameLo, h.modLo }, size[2] = { h.gameSize, h.modSize };
    const uint64_t d[2] = { now.gameLo - h.gameLo, now.modLo - h.modLo };
    for (unsigned i = 0, off = 0; i < g.nrw; off += g.rw[i][1], i++)
        Relocate(buf + off, g.rw[i][1], lo, size, d);
    for (const Fix& f : fixes) {
        if (f.reg >= regs.size() || f.off + 8 > total) continue;
        const uint64_t v = reinterpret_cast<uint64_t>(now_at[f.reg]) + f.delta;
        memcpy(buf + f.off, &v, 8);
    }
    at = 0;
    for (unsigned i = 0; i < g.nrw; i++) {
        memcpy(reinterpret_cast<void*>(g.rw[i][0]), buf + at, g.rw[i][1]);
        at += g.rw[i][1];
    }
    free(buf);
    std::lock_guard<std::mutex> lock(M.audioMutex);   /* the old position's sound */
    M.audio.clear();
    M.audioPos = 0.0;
    return true;
}

void ModShutdown() {
    if (!M.live) return;
    GameCurrent();
    GameEngine::Instance->Destroy();
    M.win = nullptr;
    M.ctx = nullptr;
    HostRestore();
    M.audio.clear();
    M.audioPos = 0.0;
    M.live = false;
    M.path.clear();
}

const NsomodApi kApi = {
    NSOMOD_API_VERSION, kW, kH, 30.0, 2, ModInit, ModFrame, ModAudioPull, ModStateSave, ModStateLoad, ModShutdown, false,
};
}  // namespace

extern "C" const NsomodApi* nsomod_get_api(void) { return &kApi; }

/* libultraship hooks (weak references in the checked-out tree) */
extern "C" void NsoLusCaptureFrame(const uint8_t* rgba, int width, int height) {
    if (!rgba || width <= 0 || height <= 0) return;
    for (int y = 0; y < kH; ++y) {
        const int sy = height - 1 - (y * height / kH);   /* GL rows go bottom-up */
        for (int x = 0; x < kW; ++x) {
            const size_t s = (size_t(sy) * width + size_t(x * width / kW)) * 4;
            const size_t t = (size_t(y) * kW + x) * 3;
            M.rgb[t] = rgba[s];
            M.rgb[t + 1] = rgba[s + 1];
            M.rgb[t + 2] = rgba[s + 2];
        }
    }
}

extern "C" int NsoLusAudioBuffered(void) {
    std::lock_guard<std::mutex> lock(M.audioMutex);
    return int(M.audio.size() * sizeof(int16_t) * 2);
}

extern "C" bool NsoLusCaptureAudio(const uint8_t* bytes, size_t length) {
    if (!bytes || length < sizeof(int16_t) * 2) return false;
    const int16_t* s = reinterpret_cast<const int16_t*>(bytes);
    const size_t frames = length / (sizeof(int16_t) * 2);
    std::lock_guard<std::mutex> lock(M.audioMutex);
    for (size_t i = 0; i < frames; ++i) {
        if (M.audio.size() >= kAudioCap) {
            M.audio.pop_front();
            M.audioPos = std::max(0.0, M.audioPos - 1.0);
        }
        M.audio.push_back({ s[i * 2], s[i * 2 + 1] });
    }
    return true;
}

extern "C" bool NsoLusWriteControllerInput(OSContPad* pad) {
    if (!pad) return false;
    const NsomodInput& p = M.in;
    auto axis = [](float v, bool neg, bool pos) -> int8_t {
        if (v > 0.15f || v < -0.15f) return int8_t(std::clamp(v * 80.0f, -80.0f, 80.0f));
        return int8_t(pos ? 80 : neg ? -80 : 0);
    };
    const bool analog = p.lx > 0.15f || p.lx < -0.15f || p.ly > 0.15f || p.ly < -0.15f;
    memset(pad, 0, sizeof(OSContPad));
    pad->button = uint16_t((p.a ? A_BUTTON : 0) | (p.b ? B_BUTTON : 0) | (p.z ? Z_TRIG : 0) |
                           (p.start ? START_BUTTON : 0) | (p.l ? L_TRIG : 0) | (p.r ? R_TRIG : 0) |
                           (p.ry < -0.5f ? U_CBUTTONS : 0) | (p.ry > 0.5f ? D_CBUTTONS : 0) |
                           (p.rx < -0.5f ? L_CBUTTONS : 0) | (p.rx > 0.5f ? R_CBUTTONS : 0));
    pad->stick_x = analog ? axis(p.lx, false, false) : axis(0, p.left, p.right);
    pad->stick_y = analog ? axis(-p.ly, false, false) : axis(0, p.down, p.up);   /* N64 y is up */
    return true;
}

/* The game library's calls (--wrap in the Makefile) */

/* Rumble Pak (sys_joybus.c Controller_Rumble, once per game frame): the motor goes
 * to NSO (nsomod.h), never to libultraship's own SDL rumble, so the setting rules. */
static std::atomic<bool> gMotorOn{false};
extern "C" int32_t __wrap___osMotorAccess(OSPfs* pfs, uint32_t vibrate) {
    if (pfs && pfs->channel == 0) gMotorOn.store(vibrate != 0, std::memory_order_relaxed);
    return 0;
}
extern "C" __attribute__((visibility("default"))) float nsomod_rumble(void) {
    return M.live && gMotorOn.load(std::memory_order_relaxed) ? 1.0f : 0.0f;
}

extern "C" void* __wrap_GameEngine_Malloc(size_t size) {
    void* p = GameEngine_Malloc(size);
    std::lock_guard<std::mutex> lock(gRegMutex);
    gReg[p] = Reg{ 1, "", size };
    return p;
}
