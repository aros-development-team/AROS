/*
    (C) 1998 AROS - The Amiga Research OS
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
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


/*static AttrBase HiddGCAttrBase;*/

/*** HIDDGfx::NewGC() *********************************************************/

static Object *hiddgfx_newgc(Class *cl, Object *o, struct pHidd_Gfx_NewGC *msg)
{
    Object *gc = NULL;

    EnterFunc(bug("HIDDGfx::NewGC()\n"));

    gc = NewObject(NULL, CLID_Hidd_GC, msg->attrList);


    ReturnPtr("HIDDGfx::NewGC", Object *, gc);
}


/*** HIDDGfx::DisposeGC() ****************************************************/

static VOID hiddgfx_disposegc(Class *cl, Object *o, struct pHidd_Gfx_DisposeGC *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeGC()\n"));

    if(msg->gc) DisposeObject(msg->gc);

    ReturnVoid("HIDDGfx::DisposeGC");
}


/*** HIDDGfx::NewBitMap() ****************************************************/

static Object * hiddgfx_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    Object *bitMap = NULL;

    EnterFunc(bug("HIDDGfx::NewBitMap()\n"));

    bitMap = NewObject(NULL, CLID_Hidd_BitMap, msg->attrList);

    ReturnPtr("HIDDGfx::NewBitMap", Object *, bitMap);
}


/*** HIDDGfx::DisposeBitMap() ************************************************/

static VOID hiddgfx_disposebitmap(Class *cl, Object *o, struct pHidd_Gfx_DisposeBitMap *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeBitMap(bitmap=%p)\n", msg->bitMap));

    if(msg->bitMap) DisposeObject(msg->bitMap);

    ReturnVoid("HIDDGfx::DisposeBitMap");
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_GFXHIDD_METHODS 4

Class *init_gfxhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddgfx_newgc,         	moHidd_Gfx_NewGC},
        {(IPTR (*)())hiddgfx_disposegc,     	moHidd_Gfx_DisposeGC},
        {(IPTR (*)())hiddgfx_newbitmap,     	moHidd_Gfx_NewBitMap},
        {(IPTR (*)())hiddgfx_disposebitmap, 	moHidd_Gfx_DisposeBitMap},
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
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDGraphicsData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("    init_gfxhiddclass(csd=%p)\n", csd));

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

            csd->gcclass = init_gcclass(csd);
            D(bug("gcclass: %p\n", csd->gcclass));
            if(csd->gcclass)
            {
                D(bug("GCClass ok\n"));
		
	    	csd->planarbmclass = init_planarbmclass(csd);
	    	if (csd->planarbmclass)
		{
		    csd->chunkybmclass = init_chunkybmclass(csd);
		    if (csd->chunkybmclass)
		    {

                	ok = TRUE;
		    }
		}
            }
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

    ReturnPtr("init_gfxhiddclass", Class *, cl);
}


void free_gfxhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->gfxhiddclass);
	
	free_chunkybmclass(csd);
	free_planarbmclass(csd);
        free_gcclass(csd);
        free_bitmapclass(csd);
        if(csd->gfxhiddclass) DisposeObject((Object *) csd->gfxhiddclass);
        csd->gfxhiddclass = NULL;
    }

    ReturnVoid("free_gfxhiddclass");
}


