/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
    struct TagItem tags[] = {
	{0       , (IPTR)"oss.library"    },
	{0       , (IPTR)AROS_ARCHITECTURE},
	{TAG_DONE, 0                      }
    };

    LIBBASE->UnixIOAttrBase = OOP_ObtainAttrBase(IID_Hidd_UnixIO);
    if (!LIBBASE->UnixIOAttrBase)
	return FALSE;

    /* Initialise tag IDs only after getting UnixIOAttrBase */
    tags[0].ti_Tag = aHidd_UnixIO_Opener;
    tags[1].ti_Tag = aHidd_UnixIO_Architecture;

    unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, tags);
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
    if (LIBBASE->UnixIOAttrBase)
	OOP_ReleaseAttrBase(IID_Hidd_UnixIO);

    /* UnixIO is a singletone, there's no need to dispose it */

    return TRUE;
}

ADD2INITLIB(InitData, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2EXPUNGELIB(CleanUp, 0);
ADD2LIBS("unixio.hidd", 0, static struct Library *, unixioBase);
