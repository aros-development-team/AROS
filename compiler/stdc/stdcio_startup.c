/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Additional startup code for stdcio.library that is executed during
    init of a program that uses the library.
    This file is thus not part of stdcio.library but is used by the
    startup section in stdcio.conf
*/

#include <proto/stdc.h>
#include <proto/stdcio.h>
#include <libraries/stdcio.h>

static int __stdcio_startup(void)
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    StdCIOBase->StdCBase = __aros_getbase_StdCBase();

    return StdCIOBase->StdCBase != NULL;
}

ADD2INIT(__stdcio_startup, -50);
