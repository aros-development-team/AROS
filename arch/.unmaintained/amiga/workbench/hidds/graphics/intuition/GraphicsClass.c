/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GraphicsAmigaIntui hidd class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>
#include <hidd/graphics-amiga-intuition.h>


#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>


/*** HIDDGfx::NewBitMap() ****************************************************/

static Object * hiddgfx_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    Object *bitMap = NULL;

    EnterFunc(bug("HIDDGfxAmigaIntui::NewBitMap()\n"));

    bitMap = NewObject(NULL, CLID_Hidd_BitMapAmigaIntui, msg->attrList);

    ReturnPtr("HIDDGfxAmigaIntui::NewBitMap", Object *, bitMap);
}


/*** HIDDGfx::DisposeBitMap() ************************************************/

static VOID hiddgfx_disposebitmap(Class *cl, Object *o, struct pHidd_Gfx_DisposeBitMap *msg)
{
    EnterFunc(bug("HIDDGfxAmigaIntui::DisposeBitMap()\n"));

    DisposeObject(msg->bitMap);

    ReturnVoid("HIDDGfxAmigaIntui::DisposeBitMap");
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase


#define NUM_GFXHIDD_METHODS 2
#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

Class *init_gfxhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddgfx_newbitmap,     moHidd_Gfx_NewBitMap},
        {(IPTR (*)())hiddgfx_disposebitmap, moHidd_Gfx_DisposeBitMap},
        {NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
        {gfxhidd_descr, IID_Hidd_Gfx, NUM_GFXHIDD_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Hidd_Gfx},
        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_GfxAmigaIntui},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDGraphicsAmigaIntuiData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("init_gfxhiddclassAmigaIntui(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("GfxHiddClass ok\n"));
        cl->UserData = (APTR)csd;
        
        csd->bitmapclass = init_bitmapclass(csd);
        D(bug("bitmapclass: %p\n", csd->bitmapclass));

        if(csd->bitmapclass)
        {
            D(bug("BitMapClass ok\n"));

            ok = TRUE;
        }
    }

    if(ok == FALSE)
    {
        free_gfxhiddclass(csd);
        cl = NULL;
    }
    else
    {
        AddClass(cl);
    }

    ReturnPtr("init_gfxhiddclassAmigaIntui", Class *, cl);
}


void free_gfxhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxhiddclassAmigaIntui(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->gfxhiddclass);
        free_bitmapclass(csd);

        DisposeObject((Object *) csd->gfxhiddclass);
    }

    ReturnVoid("free_gfxhiddclassAmigaIntui");
}

