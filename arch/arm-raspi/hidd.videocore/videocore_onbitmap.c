/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for VideoCore hidd.
    Lang: English.
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

#include "videocore_bitmap.h"
#include "videocore_class.h"

#include LC_LIBDEFS_FILE

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVideoCoreAttrBase;
static OOP_AttrBase HiddVideoCoreBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,              &HiddBitMapAttrBase             },
    { IID_Hidd_PixFmt,              &HiddPixFmtAttrBase             },
    { IID_Hidd_Gfx,                 &HiddGfxAttrBase                },
    { IID_Hidd_Sync,                &HiddSyncAttrBase               },
    /* Private bases */
    { IID_Hidd_VideoCore,          &HiddVideoCoreAttrBase         },
    { IID_Hidd_VideoCoreBitMap,    &HiddVideoCoreBitMapAttrBase   },
    { NULL,                         NULL                            }
};

#define MNAME_ROOT(x) VideoCoreOnBM__Root__ ## x
#define MNAME_BM(x) VideoCoreOnBM__Hidd_BitMap__ ## x

#define OnBitmap 1
#include "videocore_bitmap_common.c"

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VideoCore.BitMap::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct BitmapData *data;
        LONG multi=1;
        OOP_Object *pf;
        IPTR width, height, depth;
        HIDDT_ModeID modeid;

        /* clear all data  */
        data = OOP_INST_DATA(cl, o);
        memset(data, 0, sizeof(struct BitmapData));

        /* Get attr values */
        OOP_GetAttr(o, aHidd_BitMap_Width, &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        
        ASSERT (width != 0 && height != 0 && depth != 0);
        /* 
            We must only create depths that are supported by the friend drawable
            Currently we only support the default depth
        */

        width=(width+15) & ~15;
        data->width = width;
        data->height = height;
        data->bpp = depth;
        data->disp = -1;
        if (depth>16)
            multi = 4;
        else if (depth>8)
            multi = 2;
        data->bytesperpix = multi;
        data->data = &XSD(cl)->data;
        data->mouse = &XSD(cl)->mouse;
#warning "TODO: Allocate using GPU memory.. "
        data->VideoData = data->data->vrambase;
        /* We should be able to get modeID from the bitmap */
        OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
        if (modeid != vHidd_ModeID_Invalid)
        {
            /*
                Because of not defined BitMap_Show method show 
                bitmap immediately
            */
            setModeVideoCore(&XSD(cl)->data, width, height);
            XSD(cl)->visible = data;	/* Set created object as visible */
            ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
        }
        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
        }
        o = NULL;
    } /* if created object */
    ReturnPtr("VideoCore.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("VideoCore.BitMap::Dispose()\n")); 
    OOP_DoSuperMethod(cl, o, msg);
    ReturnVoid("VideoCore.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

static int VideoCoreOnBM_Init(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("VideoCoreOnBM_Init\n"));

    ReturnInt("VideoCoreOnBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_bitmapclass *********************************************************/

static int VideoCoreOnBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("VideoCoreOnBM_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);

    ReturnInt("VideoCoreOnBM_Expunge", int, TRUE);
}

ADD2INITLIB(VideoCoreOnBM_Init, 0)
ADD2EXPUNGELIB(VideoCoreOnBM_Expunge, 0)
