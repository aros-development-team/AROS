/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
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
#include <proto/oop.h>
#include <hidd/unixio.h>

#include LC_LIBDEFS_FILE

#include "linuxfb_intern.h"

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
OOP_AttrBase HiddLinuxFBBitmapAttrBase;

static const struct OOP_ABDescr abd[] =
{
    { IID_Hidd_ChunkyBM         , &HiddChunkyBMAttrBase         },
    { IID_Hidd_BitMap           , &HiddBitMapAttrBase           },
    { IID_Hidd_Sync             , &HiddSyncAttrBase             },
    { IID_Hidd_Gfx              , &HiddGfxAttrBase              },
    { IID_Hidd_PixFmt           , &HiddPixFmtAttrBase           },
    { IID_Hidd_LinuxFB          , &HiddLinuxFBAttrBase          },
    { IID_Hidd_LinuxFBBitmap    , &HiddLinuxFBBitmapAttrBase    },
    { NULL                      , NULL                          }
};

static int Init_Hidd(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->lsd.unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!LIBBASE->lsd.unixio)
        return FALSE;

    InitSemaphore(&LIBBASE->lsd.sema);

    return OOP_ObtainAttrBases(abd);
}

static int Expunge_Hidd(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(abd);

    if (LIBBASE->lsd.unixio)
        OOP_DisposeObject(LIBBASE->lsd.unixio);

    return TRUE;
}

ADD2INITLIB(Init_Hidd, 1)
ADD2EXPUNGELIB(Expunge_Hidd, 1)
