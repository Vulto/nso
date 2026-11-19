/* norumble.c - nso-norumble.so: LD_PRELOADed into an external game (launched
 * through the menu launcher) when its system's Rumble setting is off. Every
 * SDL2 / SDL3 rumble and haptic-play entry point is answered "done" without
 * touching the device, so a port's own rumble (N64Recomp runtimes, the
 * libultraship ports, sm64ex's SDL haptics) stays silent. Built by `make`
 * next to nso; needs no SDL headers (the games' SDL resolves everything else). */
#include <stdbool.h>
#include <stdint.h>

#define NSO_EXPORT __attribute__((visibility("default")))

/* SDL2: 0 = success */
NSO_EXPORT int SDL_JoystickRumble(void *j, uint16_t lo, uint16_t hi, uint32_t ms)
{ (void)j; (void)lo; (void)hi; (void)ms; return 0; }
NSO_EXPORT int SDL_JoystickRumbleTriggers(void *j, uint16_t l, uint16_t r, uint32_t ms)
{ (void)j; (void)l; (void)r; (void)ms; return 0; }
NSO_EXPORT int SDL_GameControllerRumble(void *c, uint16_t lo, uint16_t hi, uint32_t ms)
{ (void)c; (void)lo; (void)hi; (void)ms; return 0; }
NSO_EXPORT int SDL_GameControllerRumbleTriggers(void *c, uint16_t l, uint16_t r, uint32_t ms)
{ (void)c; (void)l; (void)r; (void)ms; return 0; }
NSO_EXPORT int SDL_HapticRumblePlay(void *h, float strength, uint32_t ms)
{ (void)h; (void)strength; (void)ms; return 0; }
NSO_EXPORT int SDL_HapticRunEffect(void *h, int effect, uint32_t iterations)
{ (void)h; (void)effect; (void)iterations; return 0; }

/* SDL3: true = success */
NSO_EXPORT bool SDL_RumbleJoystick(void *j, uint16_t lo, uint16_t hi, uint32_t ms)
{ (void)j; (void)lo; (void)hi; (void)ms; return true; }
NSO_EXPORT bool SDL_RumbleJoystickTriggers(void *j, uint16_t l, uint16_t r, uint32_t ms)
{ (void)j; (void)l; (void)r; (void)ms; return true; }
NSO_EXPORT bool SDL_RumbleGamepad(void *g, uint16_t lo, uint16_t hi, uint32_t ms)
{ (void)g; (void)lo; (void)hi; (void)ms; return true; }
NSO_EXPORT bool SDL_RumbleGamepadTriggers(void *g, uint16_t l, uint16_t r, uint32_t ms)
{ (void)g; (void)l; (void)r; (void)ms; return true; }
NSO_EXPORT bool SDL_PlayHapticRumble(void *h, float strength, uint32_t ms)
{ (void)h; (void)strength; (void)ms; return true; }
NSO_EXPORT bool SDL_RunHapticEffect(void *h, int effect, uint32_t iterations)
{ (void)h; (void)effect; (void)iterations; return true; }
