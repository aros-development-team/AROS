#ifndef CLIB_CONSOLE_PROTOS_H
#define CLIB_CONSOLE_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
__AROS_LP2(struct InputEvent *, CDInputHandler,
    __AROS_LPA(struct InputEvent *, events, A0),
    __AROS_LPA(struct Library    *, consoleDevice, A1),
    struct Library *, ConsoleDevice, 7, Console)
#define CDInputHandler(events, consoleDevice) \
    __AROS_LC2(struct InputEvent *, CDInputHandler, \
    __AROS_LCA(struct InputEvent *, events, A0), \
    __AROS_LCA(struct Library    *, consoleDevice, A1), \
    struct Library *, ConsoleDevice, 7, Console)

__AROS_LP4(LONG, RawKeyConvert,
    __AROS_LPA(struct InputEvent *, events, A0),
    __AROS_LPA(STRPTR             , buffer, A1),
    __AROS_LPA(long               , length, D1),
    __AROS_LPA(struct KeyMap     *, keyMap, A2),
    struct Library *, ConsoleDevice, 8, Console)
#define RawKeyConvert(events, buffer, length, keyMap) \
    __AROS_LC4(LONG, RawKeyConvert, \
    __AROS_LCA(struct InputEvent *, events, A0), \
    __AROS_LCA(STRPTR             , buffer, A1), \
    __AROS_LCA(long               , length, D1), \
    __AROS_LCA(struct KeyMap     *, keyMap, A2), \
    struct Library *, ConsoleDevice, 8, Console)


#endif /* CLIB_CONSOLE_PROTOS_H */
