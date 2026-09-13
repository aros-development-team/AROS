/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: __dso_handle for modules.

    g++ registers static destructors with __cxa_atexit(fn, obj, &__dso_handle).
    Programs get the symbol from the cxx-startup.o start file; modules are
    linked with -nostartfiles, so they pick up this archive member from
    libautoinit instead (it is only pulled in when something references it,
    so a program never sees two definitions).
*/

void *__dso_handle = 0;
