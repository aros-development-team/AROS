/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_CONSOLE_H
#define _INLINE_CONSOLE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef CONSOLE_BASE_NAME
#define CONSOLE_BASE_NAME ConsoleDevice
#endif

#define CDInputHandler(events, consoleDevice) \
	LP2(0x2a, struct InputEvent *, CDInputHandler, struct InputEvent *, events, a0, struct Library *, consoleDevice, a1, \
	, CONSOLE_BASE_NAME)

#define RawKeyConvert(events, buffer, length, keyMap) \
	LP4(0x30, LONG, RawKeyConvert, struct InputEvent *, events, a0, STRPTR, buffer, a1, long, length, d1, struct KeyMap *, keyMap, a2, \
	, CONSOLE_BASE_NAME)

#endif /* _INLINE_CONSOLE_H */
