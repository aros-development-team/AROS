#ifndef CLIB_CONSOLE_PROTOS_H
#define CLIB_CONSOLE_PROTOS_H

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
#define CDInputHandler(events, consoleDevice) \
    AROS_LC2(struct InputEvent *, CDInputHandler, \
    AROS_LCA(struct InputEvent *, events, A0), \
    AROS_LCA(struct Library    *, consoleDevice, A1), \
    struct Library *, ConsoleDevice, 7, Console)

AROS_LP4(LONG, RawKeyConvert,
    AROS_LPA(struct InputEvent *, events, A0),
    AROS_LPA(STRPTR             , buffer, A1),
    AROS_LPA(long               , length, D1),
    AROS_LPA(struct KeyMap     *, keyMap, A2),
    struct Library *, ConsoleDevice, 8, Console)
#define RawKeyConvert(events, buffer, length, keyMap) \
    AROS_LC4(LONG, RawKeyConvert, \
    AROS_LCA(struct InputEvent *, events, A0), \
    AROS_LCA(STRPTR             , buffer, A1), \
    AROS_LCA(long               , length, D1), \
    AROS_LCA(struct KeyMap     *, keyMap, A2), \
    struct Library *, ConsoleDevice, 8, Console)


#endif /* CLIB_CONSOLE_PROTOS_H */
