/* lang.h - UI strings (English default, Portuguese pt-BR) + language config.
 * No raylib: unit-tested in lang_test.c. T(id) returns the active language's
 * string; NsoMsg keeps a status as (id, args) so it re-renders when the
 * language changes at runtime. */
#ifndef NSO_LANG_H
#define NSO_LANG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { LANG_EN = 0, LANG_PT, LANG_COUNT } NsoLang;

typedef enum {
    S_RAW = 0,                 /* NsoMsg only: show arg verbatim */
    S_MOSAIC_INFO, S_SIDEBAR_INFO, S_INFO_CANCEL, S_MANAGE_INFO, S_MOVE_INFO,
    /* scraping */
    S_SCRAPE_DONE, S_SCRAPE_DONE_MISSED, S_SCRAPE_NO_SOURCE, S_SCRAPE_DOWNLOAD_FAILED,
    S_SCRAPE_INDEX, S_SCRAPE_NO_CURL, S_SCRAPE_CANCELLED, S_SCRAPE_INDEX_TOO_LARGE,
    S_SCRAPE_NO_INDEX,
    /* gear screen */
    S_MANAGE_COVERS, S_MANAGE_MISSING, S_MANAGE_REPLACE, S_MANAGE_REPOS, S_MANAGE_NO_REPOS,
    S_MANAGE_ALL_SYSTEM, S_MANAGE_ALL, S_MANAGE_QUEUED_N, S_MANAGE_SETTINGS, S_MANAGE_LANGUAGE,
    S_LANG_NAME,
    /* repositories */
    S_REPO_NOT_CLONED, S_REPO_LOCAL_COPY, S_REPO_LOCAL_READY, S_REPO_LOCAL_UNBUILT,
    S_REPO_NOT_BUILT, S_REPO_ROM_NEEDED, S_REPO_READY, S_REPO_UP_TO_DATE, S_REPO_MISSING_DEPS,
    S_REPO_CLONING, S_REPO_CLONE_FAILED, S_REPO_CHECKING, S_REPO_FETCH_FAILED,
    S_REPO_BAD_CHECKOUT, S_REPO_NO_UPSTREAM, S_REPO_UPDATING, S_REPO_UPDATE_FAILED,
    S_REPO_SUBMODULE_FAILED, S_REPO_BUILDING, S_REPO_BUILD_FAILED, S_REPO_BUILD_NO_BINARY,
    S_REPO_DOWNLOAD_FAILED, S_REPO_NO_SH, S_REPO_FAILED, S_REPO_QUEUED, S_REPO_START_FAILED,
    /* install watch (notice, gear row, Details button) */
    S_REPO_INSTALLABLE, S_NOTE_INSTALL, S_NOTE_ROM, S_NOTE_INSTALLED, S_DET_INSTALL,
    /* save states */
    S_SS_RENAME_CANCELLED, S_SS_INVALID_NAME, S_SS_RENAME_FAILED, S_SS_RENAMED,
    S_SS_PATH_TOO_LONG, S_SS_CONFIRM_DELETE, S_SS_DELETE_FAILED, S_SS_DELETED,
    S_SS_COPY_FAILED, S_SS_COPIED, S_SS_TITLE, S_SS_EMPTY, S_SS_NO_IMAGE,
    S_SS_INFO_RENAME, S_SS_INFO,
    /* details panel (grid X) */
    S_DET_SYSTEM, S_DET_SOURCE, S_DET_STATUS, S_DET_SAVES, S_DET_NO_SAVES,
    S_DET_INSTALLED, S_DET_NOT_INSTALLED, S_DET_UNAVAILABLE, S_DET_INFO,
    /* in-system sidebar (grid B/O menu) and its Settings panel */
    S_SB_SINGLE, S_SB_TOGETHER, S_SB_ONLINE, S_SB_SETTINGS,
    S_SET_VERSIONS, S_SET_VER_NA, S_SET_VER_NA_EU, S_SET_RUMBLE, S_SET_ENABLED,
    S_SET_CONTROLS, S_SET_SHOW, S_SET_SCREEN, S_SET_43, S_SET_PIXEL, S_SET_CRT, S_SET_CTRL_DISPLAY,
    /* embedded game pause menu (embed.h) */
    S_EMB_CONTINUE, S_EMB_RESTART, S_EMB_QUIT,
    S_COUNT
} StrId;

typedef struct { int id; int n; char arg[160]; } NsoMsg;   /* id < 0 = empty */

extern const char *const nso_strings[LANG_COUNT][S_COUNT];

void Lang_Set(NsoLang lang);
NsoLang Lang_Get(void);
const char *T(StrId id);
/* Config text -> language; anything missing / unknown = English. */
NsoLang Lang_Parse(const char *text);
NsoLang Lang_Load(const char *path);
bool Lang_Save(const char *path, NsoLang lang);   /* keeps the file's other keys */

/* config/nso.cfg is "key=value" lines. Cfg_Get copies key's value (false if
 * absent); Cfg_Put writes text with key set (replaced in place or appended),
 * keeping every other line (false if out is too small). Cfg_SaveKey does a
 * read-modify-write of the file. */
bool Cfg_Get(const char *text, const char *key, char *dst, size_t n);
bool Cfg_Put(const char *text, const char *key, const char *value, char *out, size_t n);
bool Cfg_SaveKey(const char *path, const char *key, const char *value);

void Msg_Clear(NsoMsg *m);
void Msg_Set(NsoMsg *m, int id, int n, const char *arg);
/* "%d" uses n, "%s" uses arg; otherwise a non-empty arg is appended as ": arg". */
void Msg_Format(const NsoMsg *m, char *dst, size_t size);

#endif /* NSO_LANG_H */

#ifdef LANG_IMPLEMENTATION
#ifndef LANG_IMPLEMENTATION_ONCE
#define LANG_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <string.h>

const char *const nso_strings[LANG_COUNT][S_COUNT] = {
    [LANG_EN] = {
        [S_RAW] = "%s",
        [S_MOSAIC_INFO] = "    🅨  Move (Press and hold to sort)      🅧 Details      🅐 Start      🅑 Back   ",
        [S_SIDEBAR_INFO] = "    🅑 Back      🅐 OK   ",
        [S_INFO_CANCEL] = "    🅑 Cancel   ",
        [S_MANAGE_INFO] = "    🅐 Select      🅑 Back   ",
        [S_MOVE_INFO] = "    🅐 Confirm      🅑 Cancel Move   ",
        [S_SCRAPE_DONE] = "Done",
        [S_SCRAPE_DONE_MISSED] = "Done, %d missed",
        [S_SCRAPE_NO_SOURCE] = "No art source",
        [S_SCRAPE_DOWNLOAD_FAILED] = "Download failed",
        [S_SCRAPE_INDEX] = "Index",
        [S_SCRAPE_NO_CURL] = "curl not found",
        [S_SCRAPE_CANCELLED] = "Cancelled",
        [S_SCRAPE_INDEX_TOO_LARGE] = "Index too large",
        [S_SCRAPE_NO_INDEX] = "No index",
        [S_MANAGE_COVERS] = "Image scraping",
        [S_MANAGE_MISSING] = "Missing box art",
        [S_MANAGE_REPLACE] = "Replace box art",
        [S_MANAGE_REPOS] = "Repository download",
        [S_MANAGE_NO_REPOS] = "No repositories for this system yet",
        [S_MANAGE_ALL_SYSTEM] = "Download all from %s",
        [S_MANAGE_ALL] = "Download all (all systems)",
        [S_MANAGE_QUEUED_N] = "%d queued",
        [S_MANAGE_SETTINGS] = "Settings",
        [S_MANAGE_LANGUAGE] = "Language",
        [S_LANG_NAME] = "English",
        [S_REPO_NOT_CLONED] = "Not downloaded",
        [S_REPO_LOCAL_COPY] = "Local copy",
        [S_REPO_LOCAL_READY] = "Local copy, ready",
        [S_REPO_LOCAL_UNBUILT] = "Local copy, not built",
        [S_REPO_NOT_BUILT] = "Not built",
        [S_REPO_ROM_NEEDED] = "ROM needed",
        [S_REPO_READY] = "Ready",
        [S_REPO_UP_TO_DATE] = "Up to date",
        [S_REPO_MISSING_DEPS] = "Missing deps",
        [S_REPO_CLONING] = "Cloning",
        [S_REPO_CLONE_FAILED] = "Clone failed",
        [S_REPO_CHECKING] = "Checking for updates",
        [S_REPO_FETCH_FAILED] = "Fetch failed",
        [S_REPO_BAD_CHECKOUT] = "Bad checkout",
        [S_REPO_NO_UPSTREAM] = "No upstream branch",
        [S_REPO_UPDATING] = "Updating",
        [S_REPO_UPDATE_FAILED] = "Update failed (local changes?)",
        [S_REPO_SUBMODULE_FAILED] = "Submodule update failed",
        [S_REPO_BUILDING] = "Building",
        [S_REPO_BUILD_FAILED] = "Build failed (see log)",
        [S_REPO_BUILD_NO_BINARY] = "Build made no binary",
        [S_REPO_DOWNLOAD_FAILED] = "Download failed",
        [S_REPO_NO_SH] = "sh not found",
        [S_REPO_FAILED] = "Failed",
        [S_REPO_QUEUED] = "Queued",
        [S_REPO_START_FAILED] = "Could not start sh",
        [S_REPO_INSTALLABLE] = "Ready to install",
        [S_NOTE_INSTALL] = "%s is ready - press 🅐 to install",
        [S_NOTE_ROM] = "%s downloaded - put %s in %s",
        [S_NOTE_INSTALLED] = "%s is installed - ready to play",
        [S_DET_INSTALL] = "Install",
        [S_SS_RENAME_CANCELLED] = "Rename cancelled",
        [S_SS_INVALID_NAME] = "Invalid name (letters, digits, space . - ( ))",
        [S_SS_RENAME_FAILED] = "Rename failed (name taken?)",
        [S_SS_RENAMED] = "Renamed",
        [S_SS_PATH_TOO_LONG] = "Path too long",
        [S_SS_CONFIRM_DELETE] = "Press Delete again to delete: %s",
        [S_SS_DELETE_FAILED] = "Delete failed",
        [S_SS_DELETED] = "Deleted",
        [S_SS_COPY_FAILED] = "Copy failed",
        [S_SS_COPIED] = "Copied as: %s",
        [S_SS_TITLE] = "Save States - %s",
        [S_SS_EMPTY] = "No save states yet. In game press F12 (or Home) to save and return.",
        [S_SS_NO_IMAGE] = "No image",
        [S_SS_INFO_RENAME] = "    Type a name    Enter Save    Tab Cancel   ",
        [S_SS_INFO] = "    🅐 Load    🅧 Delete    🅨 Copy    R Rename    🅑 Back   ",
        [S_DET_SYSTEM] = "System",
        [S_DET_SOURCE] = "Source",
        [S_DET_STATUS] = "Status",
        [S_DET_SAVES] = "Save States",
        [S_DET_NO_SAVES] = "No save states for this game",
        [S_DET_INSTALLED] = "Installed",
        [S_DET_NOT_INSTALLED] = "Not installed",
        [S_DET_UNAVAILABLE] = "Not available yet",
        [S_DET_INFO] = "    🅐 Open      🅑 Back   ",
        [S_SB_SINGLE] = "Single player",
        [S_SB_TOGETHER] = "Play together",
        [S_SB_ONLINE] = "Play online",
        [S_SB_SETTINGS] = "Settings",
        [S_SET_VERSIONS] = "Playable versions",
        [S_SET_VER_NA] = "North American versions",
        [S_SET_VER_NA_EU] = "North American and European versions",
        [S_SET_RUMBLE] = "Rumble",
        [S_SET_ENABLED] = "Enabled",
        [S_SET_CONTROLS] = "Show controls in game",
        [S_SET_SHOW] = "Show",
        [S_SET_SCREEN] = "Game Screen",
        [S_SET_43] = "4:3",
        [S_SET_PIXEL] = "Pixel Perfect",
        [S_SET_CRT] = "CRT Filter",
        [S_SET_CTRL_DISPLAY] = "Control Display",
        [S_EMB_CONTINUE] = "Continue",
        [S_EMB_RESTART] = "Reset Game",
        [S_EMB_QUIT] = "To Game Selection",
    },
    [LANG_PT] = {
        [S_RAW] = "%s",
        [S_MOSAIC_INFO] = "    🅨  Mover (segure para ordenar)      🅧 Detalhes      🅐 Iniciar      🅑 Voltar   ",
        [S_SIDEBAR_INFO] = "    🅑 Voltar      🅐 OK   ",
        [S_INFO_CANCEL] = "    🅑 Cancelar   ",
        [S_MANAGE_INFO] = "    🅐 Selecionar      🅑 Voltar   ",
        [S_MOVE_INFO] = "    🅐 Confirmar      🅑 Cancelar movimento   ",
        [S_SCRAPE_DONE] = "Concluído",
        [S_SCRAPE_DONE_MISSED] = "Concluído, %d sem capa",
        [S_SCRAPE_NO_SOURCE] = "Sem fonte de capas",
        [S_SCRAPE_DOWNLOAD_FAILED] = "Falha no download",
        [S_SCRAPE_INDEX] = "Índice",
        [S_SCRAPE_NO_CURL] = "curl não encontrado",
        [S_SCRAPE_CANCELLED] = "Cancelado",
        [S_SCRAPE_INDEX_TOO_LARGE] = "Índice grande demais",
        [S_SCRAPE_NO_INDEX] = "Sem índice",
        [S_MANAGE_COVERS] = "Capas dos jogos",
        [S_MANAGE_MISSING] = "Baixar capas que faltam",
        [S_MANAGE_REPLACE] = "Substituir todas as capas",
        [S_MANAGE_REPOS] = "Repositórios",
        [S_MANAGE_NO_REPOS] = "Nenhum repositório para este sistema ainda",
        [S_MANAGE_ALL_SYSTEM] = "Baixar todos do %s",
        [S_MANAGE_ALL] = "Baixar todos (todos os sistemas)",
        [S_MANAGE_QUEUED_N] = "%d na fila",
        [S_MANAGE_SETTINGS] = "Configurações",
        [S_MANAGE_LANGUAGE] = "Idioma",
        [S_LANG_NAME] = "Português (Brasil)",
        [S_REPO_NOT_CLONED] = "Não baixado",
        [S_REPO_LOCAL_COPY] = "Cópia local",
        [S_REPO_LOCAL_READY] = "Cópia local, pronta",
        [S_REPO_LOCAL_UNBUILT] = "Cópia local, sem build",
        [S_REPO_NOT_BUILT] = "Sem build",
        [S_REPO_ROM_NEEDED] = "Precisa da ROM",
        [S_REPO_READY] = "Pronto",
        [S_REPO_UP_TO_DATE] = "Atualizado",
        [S_REPO_MISSING_DEPS] = "Faltam dependências",
        [S_REPO_CLONING] = "Clonando",
        [S_REPO_CLONE_FAILED] = "Falha ao clonar",
        [S_REPO_CHECKING] = "Verificando atualizações",
        [S_REPO_FETCH_FAILED] = "Falha ao verificar",
        [S_REPO_BAD_CHECKOUT] = "Checkout inválido",
        [S_REPO_NO_UPSTREAM] = "Sem branch remoto",
        [S_REPO_UPDATING] = "Atualizando",
        [S_REPO_UPDATE_FAILED] = "Falha ao atualizar (mudanças locais?)",
        [S_REPO_SUBMODULE_FAILED] = "Falha nos submódulos",
        [S_REPO_BUILDING] = "Compilando",
        [S_REPO_BUILD_FAILED] = "Falha na compilação (ver log)",
        [S_REPO_BUILD_NO_BINARY] = "Compilação não gerou o binário",
        [S_REPO_DOWNLOAD_FAILED] = "Falha no download",
        [S_REPO_NO_SH] = "sh não encontrado",
        [S_REPO_FAILED] = "Falhou",
        [S_REPO_QUEUED] = "Na fila",
        [S_REPO_START_FAILED] = "Falha ao iniciar sh",
        [S_REPO_INSTALLABLE] = "Pronto para instalar",
        [S_NOTE_INSTALL] = "%s está pronto - aperte 🅐 para instalar",
        [S_NOTE_ROM] = "%s baixado - coloque %s em %s",
        [S_NOTE_INSTALLED] = "%s está instalado - pronto para jogar",
        [S_DET_INSTALL] = "Instalar",
        [S_SS_RENAME_CANCELLED] = "Renomear cancelado",
        [S_SS_INVALID_NAME] = "Nome inválido (letras, números, espaço . - ( ))",
        [S_SS_RENAME_FAILED] = "Falha ao renomear (nome em uso?)",
        [S_SS_RENAMED] = "Renomeado",
        [S_SS_PATH_TOO_LONG] = "Caminho longo demais",
        [S_SS_CONFIRM_DELETE] = "Aperte Delete de novo para apagar: %s",
        [S_SS_DELETE_FAILED] = "Falha ao apagar",
        [S_SS_DELETED] = "Apagado",
        [S_SS_COPY_FAILED] = "Falha ao copiar",
        [S_SS_COPIED] = "Copiado como: %s",
        [S_SS_TITLE] = "Estados salvos - %s",
        [S_SS_EMPTY] = "Nenhum estado salvo ainda. No jogo, aperte F12 (ou Home) para salvar e voltar.",
        [S_SS_NO_IMAGE] = "Sem imagem",
        [S_SS_INFO_RENAME] = "    Digite um nome    Enter Salvar    Tab Cancelar   ",
        [S_SS_INFO] = "    🅐 Carregar    🅧 Apagar    🅨 Copiar    R Renomear    🅑 Voltar   ",
        [S_DET_SYSTEM] = "Sistema",
        [S_DET_SOURCE] = "Origem",
        [S_DET_STATUS] = "Situação",
        [S_DET_SAVES] = "Estados salvos",
        [S_DET_NO_SAVES] = "Este jogo não tem estados salvos",
        [S_DET_INSTALLED] = "Instalado",
        [S_DET_NOT_INSTALLED] = "Não instalado",
        [S_DET_UNAVAILABLE] = "Ainda não disponível",
        [S_DET_INFO] = "    🅐 Abrir      🅑 Voltar   ",
        [S_SB_SINGLE] = "Um jogador",
        [S_SB_TOGETHER] = "Jogar junto",
        [S_SB_ONLINE] = "Jogar online",
        [S_SB_SETTINGS] = "Configurações",
        [S_SET_VERSIONS] = "Versões jogáveis",
        [S_SET_VER_NA] = "Versões norte-americanas",
        [S_SET_VER_NA_EU] = "Versões norte-americanas e europeias",
        [S_SET_RUMBLE] = "Função de vibração",
        [S_SET_ENABLED] = "Ativado",
        [S_SET_CONTROLS] = "Mostrar controles no jogo",
        [S_SET_SHOW] = "Mostrar",
        [S_SET_SCREEN] = "Tela do jogo",
        [S_SET_43] = "4:3",
        [S_SET_PIXEL] = "Pixel Perfect",
        [S_SET_CRT] = "Filtro CRT",
        [S_SET_CTRL_DISPLAY] = "Exibição de controles",
        [S_EMB_CONTINUE] = "Continuar",
        [S_EMB_RESTART] = "Reiniciar jogo",
        [S_EMB_QUIT] = "Voltar à seleção de jogos",
    },
};

static NsoLang lang_current = LANG_EN;

void Lang_Set(NsoLang lang) { lang_current = (lang >= 0 && lang < LANG_COUNT) ? lang : LANG_EN; }
NsoLang Lang_Get(void) { return lang_current; }

const char *T(StrId id)
{
    const char *s;
    if (id < 0 || id >= S_COUNT) return "";
    s = nso_strings[lang_current][id];
    if (!s || !s[0]) s = nso_strings[LANG_EN][id];
    return s ? s : "";
}

NsoLang Lang_Parse(const char *text)
{
    const char *p = text;
    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        while (len && (p[len - 1] == '\r' || p[len - 1] == ' ')) len--;
        if (len >= 9 && strncmp(p, "language=", 9) == 0) {
            const char *v = p + 9;
            size_t vl = len - 9;
            if ((vl == 2 && strncmp(v, "pt", 2) == 0) || (vl == 5 && strncmp(v, "pt-BR", 5) == 0)) return LANG_PT;
            return LANG_EN;
        }
        p = e ? e + 1 : NULL;
    }
    return LANG_EN;
}

NsoLang Lang_Load(const char *path)
{
    char buf[512];
    size_t n;
    FILE *f = path ? fopen(path, "r") : NULL;
    if (!f) return LANG_EN;
    n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return Lang_Parse(buf);
}

bool Lang_Save(const char *path, NsoLang lang)
{
    return Cfg_SaveKey(path, "language", lang == LANG_PT ? "pt-BR" : "en");
}

/* Length of line p (without '\n') with trailing CR / spaces trimmed. */
static size_t cfg_line_len(const char *p, const char **next)
{
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    *next = e ? e + 1 : NULL;
    while (len && (p[len - 1] == '\r' || p[len - 1] == ' ')) len--;
    return len;
}

static bool cfg_line_is(const char *p, size_t len, const char *key, size_t kl)
{
    return len > kl && strncmp(p, key, kl) == 0 && p[kl] == '=';
}

bool Cfg_Get(const char *text, const char *key, char *dst, size_t n)
{
    size_t kl = key ? strlen(key) : 0;
    const char *p = text;
    if (!kl || !dst || !n) return false;
    while (p && *p) {
        const char *next;
        size_t len = cfg_line_len(p, &next);
        if (cfg_line_is(p, len, key, kl)) {
            size_t vl = len - kl - 1;
            if (vl >= n) vl = n - 1;
            memcpy(dst, p + kl + 1, vl);
            dst[vl] = '\0';
            return true;
        }
        p = next;
    }
    return false;
}

bool Cfg_Put(const char *text, const char *key, const char *value, char *out, size_t n)
{
    size_t kl = key ? strlen(key) : 0, used = 0;
    bool done = false;
    const char *p = text ? text : "";
    if (!kl || !out || !n) return false;
    out[0] = '\0';
    while (*p) {
        const char *next;
        size_t len = cfg_line_len(p, &next);
        int w;
        if (cfg_line_is(p, len, key, kl)) {
            if (done) { p = next ? next : ""; continue; }   /* drop duplicates */
            w = snprintf(out + used, n - used, "%s=%s\n", key, value ? value : "");
            done = true;
        } else {
            w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
        }
        if (w < 0 || (size_t)w >= n - used) return false;
        used += (size_t)w;
        p = next ? next : "";
    }
    if (!done) {
        int w = snprintf(out + used, n - used, "%s=%s\n", key, value ? value : "");
        if (w < 0 || (size_t)w >= n - used) return false;
    }
    return true;
}

bool Cfg_SaveKey(const char *path, const char *key, const char *value)
{
    char in[4096], out[4200];
    size_t len = 0;
    FILE *f = path ? fopen(path, "r") : NULL;
    if (f) { len = fread(in, 1, sizeof(in) - 1, f); fclose(f); }
    in[len] = '\0';
    if (!Cfg_Put(in, key, value, out, sizeof(out))) return false;
    f = path ? fopen(path, "w") : NULL;
    if (!f) return false;
    bool ok = fputs(out, f) >= 0;
    return (fclose(f) == 0) && ok;
}

void Msg_Clear(NsoMsg *m) { m->id = -1; m->n = 0; m->arg[0] = '\0'; }

void Msg_Set(NsoMsg *m, int id, int n, const char *arg)
{
    m->id = id;
    m->n = n;
    snprintf(m->arg, sizeof(m->arg), "%s", arg ? arg : "");
}

void Msg_Format(const NsoMsg *m, char *dst, size_t size)
{
    const char *fmt;
    if (!dst || size == 0) return;
    dst[0] = '\0';
    if (!m || m->id < 0 || m->id >= S_COUNT) return;
    fmt = T((StrId)m->id);
    if (strstr(fmt, "%d")) {
        snprintf(dst, size, fmt, m->n);
    } else if (strstr(fmt, "%s")) {
        snprintf(dst, size, fmt, m->arg);
    } else if (m->arg[0]) {
        snprintf(dst, size, "%s: %s", fmt, m->arg);
    } else {
        snprintf(dst, size, "%s", fmt);
    }
}

#endif /* LANG_IMPLEMENTATION_ONCE */
#endif /* LANG_IMPLEMENTATION */
