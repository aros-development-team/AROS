/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

/* 
    AmigaVideo Compositor class.
    Manages the copperlist to display amiga gfx screen(s).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#define CMDDEBUGUNIMP(x)
#define CMDDEBUGPIXEL(x)
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "amigavideo_hidd.h"
#include "amigavideo_compositor.h"

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

/* PUBLIC METHODS */
OOP_Object *METHOD(AmigaVideoCompositor, Root, New)
{
    struct amigavideo_staticdata *csd = CSD(cl);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);
        struct Library *OOPBase = csd->cs_OOPBase;
        struct Library *UtilityBase = csd->cs_UtilityBase;
        BOOL dodispose = FALSE;

        NEWLIST(&compdata->bitmapstack);
        InitSemaphore(&compdata->semaphore);

        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositor_GfxHidd, 0, msg->attrList);
        if (compdata->gfx == NULL)
            dodispose = TRUE;
#if (0)
        else
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_CreateObject(compdata->gfx, CSD(cl)->basegc, NULL);
            if (compdata->gc == NULL)
                dodispose = TRUE;
        }
#endif

        if (dodispose)
        {
            /* Creation failed */
            OOP_MethodID disposemid;
            disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&disposemid);
            o = NULL;
        }
    }

    return o;
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapStackChanged)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);
    struct HIDD_ViewPortData * vpdata;
    struct StackBitMapNode * n = NULL;
    OOP_Object *bm = NULL;

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_WRITE

    if (!msg->data)
    {
        /* TODO: BLANK SCREEN */
        UNLOCK_COMPOSITOR
        return;
    }

    vpdata = msg->data;

    if (vpdata)
        bm = vpdata->Bitmap;

    D(bug("[AmigaVideo:Compositor] %s: vpdata @ 0x%p, bm @ 0x%p\n", __func__, vpdata, bm));

    if (bm) {
        struct Library *OOPBase = csd->cs_OOPBase;
        struct amigabm_data *bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);

        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
        IPTR modeid = vHidd_ModeID_Invalid;

        OOP_GetAttr(bm, aHidd_BitMap_ModeID , &modeid);
        csd->modeid = modeid;
        setmode(csd, bmdata);
        OOP_SetAttrs(bm, (struct TagItem *)tags);

        if (csd->acb)
             csd->acb(csd->acbdata, bm);

    } else {
        resetmode(csd);
    }

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapRectChanged)
{
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_READ

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, BitMapPositionChanged)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_WRITE

    struct amigabm_data *bmdata = OOP_INST_DATA(OOP_OCLASS(msg->bm), msg->bm);
    if (csd->disp == bmdata)
        setscroll(csd, bmdata);

    UNLOCK_COMPOSITOR
}

VOID METHOD(AmigaVideoCompositor, Hidd_Compositor, ValidateBitMapPositionChange)
{
    struct amigacomposit_data * compdata = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Compositor] %s()\n", __func__));

    LOCK_COMPOSITOR_READ

    UNLOCK_COMPOSITOR
}

