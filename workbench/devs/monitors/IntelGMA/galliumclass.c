/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <hidd/gallium.h>
#include <proto/graphics.h>

#include "intelG45_intern.h"
#include "aros_winsys.h"
#include "gallium_intern.h"
#include "intelG45_regs.h"

#include "i915/i915_public.h"
#include "i915/i915_resource.h"
#include "util/u_memory.h"
#include "util/u_atomic.h"
#include "util/u_inlines.h"
#include "i915/i915_winsys.h"
#include "i915/i915_debug.h"

const struct OOP_InterfaceDescr Gallium_ifdescr[];
extern OOP_AttrBase MetaAttrBase;
OOP_AttrBase HiddGalliumAttrBase;
APTR i915MemPool;

extern ULONG allocated_mem;
extern struct g45staticdata sd;
#define sd ((struct g45staticdata*)&(sd))

struct Hidd915WinSys
{
    struct HIDDT_WinSys base;
    struct pipe_screen *pscreen;
};

static INLINE struct Hidd915WinSys *
Hidd915WinSys(struct pipe_winsys *ws)
{
    return (struct Hidd915WinSys *)ws;
}

static VOID
HIDD915DestroyWinSys(struct pipe_winsys *ws)
{
    struct Hidd915WinSys *hiddws = Hidd915WinSys(ws);
    FREE(hiddws);
}

static VOID
HIDD915FlushFrontBuffer( struct pipe_screen *screen,
                              struct pipe_resource *resource,
                              unsigned level, unsigned layer,
                              void *winsys_drawable_handle  )
{
    /* No Op */
}

BOOL InitGalliumClass()
{
    if( sd->force_gallium
        || sd->ProductID == 0x2582 // GMA 900
        || sd->ProductID == 0x2782
        || sd->ProductID == 0x2592
        || sd->ProductID == 0x2792
        || sd->ProductID == 0x2772 // GMA 950
        || sd->ProductID == 0x2776
        || sd->ProductID == 0x27A2
        || sd->ProductID == 0x27A6
        || sd->ProductID == 0x27AE
        || sd->ProductID == 0x2972 // GMA 3000
        || sd->ProductID == 0x2973
        || sd->ProductID == 0x2992
        || sd->ProductID == 0x2993
    ){
        CloseLibrary( OpenLibrary("gallium.library",0)); // ???
    
        if((HiddGalliumAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gallium)))
        {
    
            struct TagItem Gallium_tags[] =
            {
                {aMeta_SuperID   , (IPTR)CLID_Hidd_Gallium },
                {aMeta_InterfaceDescr, (IPTR)Gallium_ifdescr },
                {aMeta_InstSize  , sizeof(struct HIDDGalliumData)},
                {aMeta_ID        , (IPTR)"hidd.gallium.i915"},
                {TAG_DONE, 0}
            };
    
            sd->galliumclass = OOP_NewObject(NULL, CLID_HiddMeta, Gallium_tags);
            if (sd->galliumclass)
            {
                sd->galliumclass->UserData = sd;
                OOP_AddClass(sd->galliumclass);
                i915MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);
    
                init_aros_winsys();
                bug("i915 gallium init OK\n");
                return TRUE;
            }
    
            OOP_ReleaseAttrBase((STRPTR)HiddGalliumAttrBase);
        }
    }

    return FALSE;
}


/* PUBLIC METHODS */
OOP_Object *METHOD(i915Gallium, Root, New)
{
    D(bug("[i915gallium] New\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if(o)
    {

    }

    return o;
}


VOID METHOD(i915Gallium, Root, Get)
{
    ULONG idx;
    D(bug("[i915gallium] get\n"));
    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_GalliumInterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(i915Gallium, Hidd_Gallium, CreatePipeScreen)
{

    bug("[i915gallium] CreatePipeScreen currently allocated_mem %d\n",allocated_mem);

    struct Hidd915WinSys *hiddws;
    struct aros_winsys *aws;
    struct pipe_winsys *ws;

    aws = winsys_create();

    if (!aws) {
        return NULL;
    }

    hiddws = CALLOC_STRUCT(Hidd915WinSys);
    if (!hiddws) {
        return NULL;
    }

    ws = &hiddws->base.base;
    ws->destroy = HIDD915DestroyWinSys;

    hiddws->pscreen = i915_screen_create( &aws->base );
    if (!hiddws->pscreen) {
        ws->destroy(ws);
        return NULL;
    }

    hiddws->pscreen->flush_frontbuffer = HIDD915FlushFrontBuffer;
    hiddws->pscreen->winsys = (struct pipe_winsys*)hiddws;
    /* Preserve pointer to HIDD driver */
    hiddws->base.driver = o;

    return hiddws->pscreen;
}


VOID METHOD(i915Gallium, Hidd_Gallium, DisplayResource)
{
   //  bug("[i915gallium] DisplayResource\n");

#ifndef GALLIUM_SIMULATION
    OOP_Object *bm = HIDD_BM_OBJ(msg->bitmap);
    GMABitMap_t *bm_dst;

   // if (!((IPTR)bm == (IPTR)sd.BMClass) ) return; // Check if bitmap is really intelGFX bitmap

    bm_dst = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    struct i915_texture *tex = i915_texture(msg->resource);

    IF_BAD_MAGIC(tex->buffer) return;

    LOCK_BITMAP_BM(bm_dst)

    uint32_t br00, br13, br22, br23, br09, br11, br26, br12;
    int mode = 3;

    br00 = (2 << 29) | (0x53 << 22) | (6);
    if (bm_dst->bpp == 4)
    br00 |= 3 << 20;

    br13 = bm_dst->pitch | ROP_table[mode].rop;
    if (bm_dst->bpp == 4)
    br13 |= 3 << 24;
    else if (bm_dst->bpp == 2)
    br13 |= 1 << 24;

    br22 = msg->dstx | (msg->dsty << 16);
    br23 = (msg->dstx + msg->width) | (msg->dsty + msg->height) << 16;
    br09 = bm_dst->framebuffer;

    br11 = tex->stride;
    br26 = msg->srcx | (msg->srcy << 16);
    br12 = (uint32_t)tex->buffer->map - (uint32_t)sd->Card.Framebuffer;

    while(buffer_is_busy(0,tex->buffer)){};
    
    LOCK_HW
        START_RING(8);
            OUT_RING(br00);
            OUT_RING(br13);
            OUT_RING(br22);
            OUT_RING(br23);
            OUT_RING(br09);
            OUT_RING(br26);
            OUT_RING(br11);
            OUT_RING(br12);
        ADVANCE_RING();
        //DO_FLUSH();
    UNLOCK_HW
    UNLOCK_BITMAP_BM(bm_dst)
#endif
    destroy_unused_buffers();
}


static const struct OOP_MethodDescr Gallium_Root_descr[] =
{
    {(OOP_MethodFunc)i915Gallium__Root__New, moRoot_New},
    {(OOP_MethodFunc)i915Gallium__Root__Get, moRoot_Get},
    {NULL, 0}
};

static const struct OOP_MethodDescr Gallium_Hidd_Gallium_descr[] =
{
    {(OOP_MethodFunc)i915Gallium__Hidd_Gallium__CreatePipeScreen, moHidd_Gallium_CreatePipeScreen},
    {(OOP_MethodFunc)i915Gallium__Hidd_Gallium__DisplayResource, moHidd_Gallium_DisplayResource},
    {NULL, 0}
};

const struct OOP_InterfaceDescr Gallium_ifdescr[] =
{
    {(struct OOP_MethodDescr*)Gallium_Root_descr, IID_Root, 2},
    {(struct OOP_MethodDescr*)Gallium_Hidd_Gallium_descr, IID_Hidd_Gallium, 2},
    {NULL, NULL,0}
};

