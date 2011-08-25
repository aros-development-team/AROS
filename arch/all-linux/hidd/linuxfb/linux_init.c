/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LinuxFB hidd initialization code.
    Lang: English.
*/

#define DEBUG 0

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <utility/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#include "linux_intern.h"

/* 
 * Some attrbases needed as global vars.
 * These are write-once read-many.
 */
OOP_AttrBase HiddChunkyBMAttrBase;
OOP_AttrBase HiddBitMapAttrBase;  
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddLinuxFBAttrBase;

static const char *libc_symbols[] =
{
    "open",
    "close",
    "ioctl",
    "mmap",
    "munmap",
    NULL
};

static const struct OOP_ABDescr abd[] =
{
    { IID_Hidd_ChunkyBM , &HiddChunkyBMAttrBase },
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase	},
    { IID_Hidd_Sync 	, &HiddSyncAttrBase	},
    { IID_Hidd_Gfx  	, &HiddGfxAttrBase	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase	},
    { IID_Hidd_LinuxFB  , &HiddLinuxFBAttrBase  },
    { NULL  	    	, NULL      	    	}
};

#define HostLibBase LinuxFBBase->lsd.hostlibBase

static int Init_Hidd(LIBBASETYPEPTR LIBBASE)
{
    ULONG i;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    	return FALSE;

    LIBBASE->libcHandle = HostLib_Open("libc.so.6", NULL);
    if (!LIBBASE->libcHandle)
    	return FALSE;

    LIBBASE->lsd.SysIFace = (struct LibCInterface *)HostLib_GetInterface(LIBBASE->libcHandle, libc_symbols, &i);
    if ((!LIBBASE->lsd.SysIFace) || i)
	return FALSE;

    InitSemaphore(&LIBBASE->lsd.sema);

    return OOP_ObtainAttrBases(abd);
}

static int Expunge_Hidd(LIBBASETYPEPTR LIBBASE)
{
    if (!HostLibBase)
	return TRUE;

    OOP_ReleaseAttrBases(abd);

    if (LIBBASE->lsd.SysIFace)
	HostLib_DropInterface ((APTR *)LIBBASE->lsd.SysIFace);

    if (LIBBASE->libcHandle)
	HostLib_Close(LIBBASE->libcHandle, NULL);

    return TRUE;
}

ADD2INITLIB(Init_Hidd, 1)
ADD2EXPUNGELIB(Expunge_Hidd, 1)
