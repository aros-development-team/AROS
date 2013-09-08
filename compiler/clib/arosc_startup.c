/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <libraries/stdc.h>
#include <libraries/stdcio.h>
#include <libraries/posixc.h>

#include "__arosc_privdata.h"

static int __arosc_init(void)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    aroscbase->StdCBase = __aros_getbase_StdCBase();
    aroscbase->StdCIOBase = __aros_getbase_StdCIOBase();
    aroscbase->PosixCBase = __aros_getbase_PosixCBase();

    return aroscbase->StdCBase != NULL
           && aroscbase->StdCIOBase != NULL
           && aroscbase->PosixCBase != NULL;
}

ADD2INIT(__arosc_init, -50);
