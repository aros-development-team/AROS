/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEFINES_CONSOLE_H
#define DEFINES_CONSOLE_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define CDInputHandler(events, consoleDevice) \
    AROS_LC2(struct InputEvent *, CDInputHandler, \
    AROS_LCA(struct InputEvent *, events, A0), \
    AROS_LCA(struct Library    *, consoleDevice, A1), \
    struct Library *, ConsoleDevice, 7, Console)

#define RawKeyConvert(events, buffer, length, keyMap) \
    AROS_LC4(LONG, RawKeyConvert, \
    AROS_LCA(struct InputEvent *, events, A0), \
    AROS_LCA(STRPTR             , buffer, A1), \
    AROS_LCA(LONG               , length, D1), \
    AROS_LCA(struct KeyMap     *, keyMap, A2), \
    struct Library *, ConsoleDevice, 8, Console)


#endif /* DEFINES_CONSOLE_H */
