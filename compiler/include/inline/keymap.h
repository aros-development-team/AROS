#ifndef _INLINE_KEYMAP_H
#define _INLINE_KEYMAP_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef KEYMAP_BASE_NAME
#define KEYMAP_BASE_NAME KeymapBase
#endif

#define AskKeyMapDefault() \
	LP0(0x24, struct KeyMap *, AskKeyMapDefault, \
	, KEYMAP_BASE_NAME)

#define MapANSI(string, count, buffer, length, keyMap) \
	LP5(0x30, LONG, MapANSI, STRPTR, string, a0, long, count, d0, STRPTR, buffer, a1, long, length, d1, struct KeyMap *, keyMap, a2, \
	, KEYMAP_BASE_NAME)

#define MapRawKey(event, buffer, length, keyMap) \
	LP4(0x2a, WORD, MapRawKey, struct InputEvent *, event, a0, STRPTR, buffer, a1, long, length, d1, struct KeyMap *, keyMap, a2, \
	, KEYMAP_BASE_NAME)

#define SetKeyMapDefault(keyMap) \
	LP1NR(0x1e, SetKeyMapDefault, struct KeyMap *, keyMap, a0, \
	, KEYMAP_BASE_NAME)

#endif /* _INLINE_KEYMAP_H */
