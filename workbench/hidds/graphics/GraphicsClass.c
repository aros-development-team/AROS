/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>


#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>


struct GfxHiddData
{
    Class *bitmap;  /* bitmap class     */
    Class *gc;      /* graphics context */
};

static AttrBase HiddGCAttrBase;

/**************************
**  GfxHIDD::CreateGC()  **
**************************/
static Object *gfxhidd_creategc(Class *cl, Object *o, struct pHidd_Gfx_CreateGC *msg)
{
    Object *gc = NULL;
    struct TagItem tags[] =
    {
        {aHidd_GC_BitMap,       (IPTR)msg->bitMap},
        {TAG_DONE,              0}
    };
    
    
    switch (msg->gcType)
    {
        case  GCTYPE_QUICK:
            /* The Quick GC must come from a subclass     */
            gc = NULL;
            break;
            
        case GCTYPE_CLIPPING:
            gc = NewObject(NULL, CLID_Hidd_ClipGC, tags);
            break;
            
        
    }
    return gc;
}

/**************************
**  GfxHIDD::DeleteGC()  **
**************************/
static VOID gfxhidd_deletegc(Class *cl, Object *o, struct pHidd_Gfx_DeleteGC *msg)
{
    DisposeObject(msg->gc);
    
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
        {(IPTR (*)())gfxhidd_creategc,  moHidd_Gfx_CreateGC},
        {(IPTR (*)())gfxhidd_deletegc,  moHidd_Gfx_DeleteGC},
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
     /*   { aMeta_SuperID,                (IPTR)CLID_Hidd},*/
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_Gfx},
        { aMeta_InstSize,               (IPTR)sizeof (struct GfxHiddData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("    init_gfxhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("      GfxHiddClass ok\n"));
        cl->UserData = (APTR)csd;
        
        HiddGCAttrBase = GetAttrBase(IID_Hidd_GC);

        csd->bitmapclass = init_bitmapclass(csd);
        D(bug("bitmapclass: %p\n", csd->bitmapclass));

        if(csd->bitmapclass)
        {
            ok = TRUE;
        }
#if 0
/*        csd->gcclass = init_gcclass(csd);*/
        D(bug("      GCClass: %p\n", csd->gcclass));
        if(csd->gcclass)
        {
            D(bug("      Got GCClass\n"));
            ok = TRUE;
        }
#endif

        if(ok == FALSE)
        {
            free_gfxhiddclass(csd);
            cl = NULL;
        }
        else
        {
            AddClass(cl);
        }
    }

    ReturnPtr("init_gfxhiddclass", Class *, cl);
}


void free_gfxhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->gfxhiddclass);
        free_bitmapclass(csd);

     /*   free_gcclass(csd); */

        DisposeObject((Object *) csd->gfxhiddclass);
    }

    ReturnVoid("free_gfxhiddclass");
}

