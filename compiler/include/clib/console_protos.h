#ifndef CLIB_CONSOLE_PROTOS_H
#define CLIB_CONSOLE_PROTOS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prototypes for console.device
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP2(struct InputEvent *, CDInputHandler,
    AROS_LPA(struct InputEvent *, events, A0),
    AROS_LPA(struct Library    *, consoleDevice, A1),
    struct Library *, ConsoleDevice, 7, Console)

AROS_LP4(LONG, RawKeyConvert,
    AROS_LPA(struct InputEvent *, events, A0),
    AROS_LPA(STRPTR             , buffer, A1),
    AROS_LPA(LONG               , length, D1),
    AROS_LPA(struct KeyMap     *, keyMap, A2),
    struct Library *, ConsoleDevice, 8, Console)


#endif /* CLIB_CONSOLE_PROTOS_H */
