/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <libcore/base.h>
#include <libcore/compiler.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

struct Library *aroscbase;
struct ExecBase *SysBase;

AROS_SET_LIBFUNC(InitData, LIBBASETYPE, LIBBASE)
{
    SysBase = LIBBASE->lh_SysBase;
    
    if (!(aroscbase = OpenLibrary("arosc.library",41)))
        return FALSE;

    return TRUE;
}

AROS_SET_LIBFUNC(CleanUp, LIBBASETYPE, LIBBASE)
{
    CloseLibrary(aroscbase);
    
    return TRUE;
}

ADD2INITLIB(InitData, 0);
ADD2EXPUNGELIB(CleanUp, 0);
