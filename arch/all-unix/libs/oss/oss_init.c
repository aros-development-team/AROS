/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
#include <hidd/unixio.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/alib.h>

#include "oss_intern.h"

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

struct Library *OOPBase;
struct ExecBase *SysBase;

OOP_Object *unixio;
int audio_fd;

AROS_SET_LIBFUNC(InitData, LIBBASETYPE, LIBBASE)
{
    SysBase = LIBBASE->lh.lh_SysBase;
 
    if (!(OOPBase = OpenLibrary("oop.library", 0)))
        return FALSE;

    unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!unixio) return FALSE;
    
    return TRUE;
}

AROS_SET_LIBFUNC(OpenLib, LIBBASETYPE, LIBBASE)
{
    /* Allow only one opener */

    return ((struct Library *)LIBBASE)->lib_OpenCnt ? FALSE : TRUE;
}

AROS_SET_LIBFUNC(CleanUp, LIBBASETYPE, LIBBASE)
{
    if (unixio) OOP_DisposeObject(unixio);
    
    CloseLibrary(OOPBase);
    
    return TRUE;
}

ADD2INITLIB(InitData, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2EXPUNGELIB(CleanUp, 0);
