/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <hidd/unixio.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/alib.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

OOP_Object *unixio = NULL;
int audio_fd;

static int InitData(LIBBASETYPEPTR LIBBASE)
{
    unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!unixio) return FALSE;
    
    return TRUE;
}

static int OpenLib(LIBBASETYPEPTR LIBBASE)
{
    /* Allow only one opener */

    return ((struct Library *)LIBBASE)->lib_OpenCnt ? FALSE : TRUE;
}

static int CleanUp(LIBBASETYPEPTR LIBBASE)
{
    if (unixio)
    {
	OOP_DisposeObject(unixio);
	unixio = NULL;
    }
    return TRUE;
}

ADD2INITLIB(InitData, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2EXPUNGELIB(CleanUp, 0);
