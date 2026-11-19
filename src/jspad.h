/* jspad.h - raw Linux joystick (/dev/input/js*) controls for the controller
 * configuration. Mednafen reads a pad through this same API, so a button or axis
 * recorded here by index is the one its bindings name (padmap.h). Open every js
 * device while a screen needs it; JsPad_Update drains the events. Kinds and
 * indexes are padmap.h's PAD_JS_*. Not Linux: no device, nothing recorded. */
#ifndef NSO_JSPAD_H
#define NSO_JSPAD_H

#include <stdbool.h>
#include <stddef.h>

bool JsPad_Open(void);                 /* true if at least one device opened */
void JsPad_Close(void);
void JsPad_Update(void);
bool JsPad_Down(int kind, int idx);    /* is that button / axis half held on any device */
/* The first press since the last call (a button down, or an axis moved far from its
 * rest), with Mednafen's 32-hex-digit id of its device. */
bool JsPad_TakePress(int *kind, int *idx, char *devId, size_t n);
/* Mednafen's id of the first js device, without keeping it open. */
bool JsPad_FirstId(char *out, size_t n);

#endif /* NSO_JSPAD_H */
