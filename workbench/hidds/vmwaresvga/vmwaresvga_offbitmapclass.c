/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Offscreen bitmap class for VMWare hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <aros/symbolsets.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <hidd/gfx.h>
#include <oop/oop.h>

#include "vmwaresvga_intern.h"

#include LC_LIBDEFS_FILE

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddVMWareSVGAAttrBase;
static OOP_AttrBase HiddVMWareSVGABitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_PixFmt,           &HiddPixFmtAttrBase             },
    {IID_Hidd_Gfx,              &HiddGfxAttrBase                },
    /* Private bases */
    {IID_Hidd_VMWareSVGA,       &HiddVMWareSVGAAttrBase         },
    {IID_Hidd_VMWareSVGABitMap, &HiddVMWareSVGABitMapAttrBase   },
    {NULL,                      NULL                            }
};

#define DEBUGNAME "[VMWareSVGA:OffBitMap]"
#define MNAME_ROOT(x) VMWareSVGAOffBM__Root__ ## x
#define MNAME_BM(x) VMWareSVGAOffBM__Hidd_BitMap__ ## x

#include "vmwaresvga_bitmap_common.c"

/*
  include our debug overides after bitmap_common incase it sets its own values...
 */
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct BitmapData *data;
        LONG multi=1;
        IPTR width, height, depth;
        OOP_Object *bmfriend, *pf;

        data = OOP_INST_DATA(cl, o);

        /* clear all data  */
        memset(data, 0, sizeof(struct BitmapData));

        /* Get attr values */
        OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
        OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
        OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);

        /* Get the friend bitmap. This should be a displayable bitmap */
        OOP_GetAttr(o, aHidd_BitMap_Friend,	(IPTR *)&bmfriend);

        /* If you got a friend bitmap, copy its colormap */
        if (bmfriend)
        {
            struct BitmapData *src = OOP_INST_DATA(cl, bmfriend);
            CopyMem(&src->cmap, &data->cmap, 4*16);
        }
        ASSERT (width != 0 && height != 0 && depth != 0);
        width=(width+15) & ~15;
        data->width = width;
        data->height = height;
        data->bpp = depth;
        data->disp = 0;
        if (depth>16)
            multi = 4;
        else if (depth>8)
            multi = 2;
        data->bytesperpix = multi;

        data->VideoData = AllocVec(width*height*multi, MEMF_PUBLIC | MEMF_CLEAR);
        if (data->VideoData)
        {
            InitSemaphore(&data->bmsem);
            data->data = &XSD(cl)->data;
            if (XSD(cl)->activecallback)
                XSD(cl)->activecallback(XSD(cl)->callbackdata, o, TRUE);
        } /* if got data->VideoData */
        else
        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            o = NULL;
        }
    } /* if created object */

    D(bug(DEBUGNAME " %s: returning 0x%p\n", __func__, o));

    return o;
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug(DEBUGNAME " %s()\n", __func__);)

    FreeVec(data->VideoData);
    OOP_DoSuperMethod(cl, o, msg);
    ReturnVoid("VMWareSVGA.BitMap::Dispose");
}

/*** init_bmclass *********************************************************/

static int VMWareSVGAOffBM_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    ReturnInt("VMWareSVGAOffBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_offbitmapclass *********************************************************/

static int VMWareSVGAOffBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    ReturnInt("VMWareSVGAOffBM_Expunge", int, TRUE);
}

ADD2INITLIB(VMWareSVGAOffBM_Init, 0)
ADD2EXPUNGELIB(VMWareSVGAOffBM_Expunge, 0)
