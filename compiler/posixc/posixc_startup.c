/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Additional startup code for posixc.library that is executed during
    init of a program that uses the library.
    This file is thus not part of posixc.library but is used by the
    startup section in posixc.conf
*/

#include <proto/stdc.h>
#include <proto/stdcio.h>
#include <proto/posixc.h>

static int __posixc_startup(void)
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();

    PosixCBase->StdCBase = __aros_getbase_StdCBase();
    PosixCBase->StdCIOBase = __aros_getbase_StdCIOBase();

    return PosixCBase->StdCBase != NULL
           && PosixCBase->StdCIOBase != NULL;
}

ADD2INIT(__posixc_startup, -50);
