/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include "x11gfx_intern.h"
#include <stddef.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>




AROS_LH0(Class *, getclass, struct x11gfxbase *, LIBBASE, 5, BASENAME)
{
     return LIBBASE->gfxclass;
}


#undef SysBase
#define SysBase (X11GfxBase->sysbase)

#undef OOPBase
#define OOPBase (X11GfxBase->oopbase)

#undef UtilityBase
#define UtilityBase (X11GfxBase->utilitybase)

#define X11GfxBase ((struct x11gfxbase *)cl->UserData)


/* Some attrbases needed as global vars.
  These are write-once read-many */
  
  AttrBase HiddBitMapAttrBase;
  AttrBase HiddGCAttrBase;



/* Private instance data for Gfx hidd class */
struct gfx_data
{
    ULONG dummy;
};

/*********************
**  GfxHidd::New()  **
*********************/
static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    {
    	if (o)
	{
	    /* Do GfxHidd initalization here */
	
	}
    }
    return o;
}

/**************************
**  GfxHidd::CreateGC()  **
**************************/
static Object *gfxhidd_newgc(Class *cl, Object *o, struct pHidd_Gfx_NewGC *msg)
{

    Object *gc = NULL;
    
    switch (msg->gcType)
    {
        case  vHIDD_Gfx_GCType_Quick:
	    gc = NewObject( X11GfxBase->gcclass, NULL, msg->attrList);
	    break;
	    
	default:
	    /* Do not support the type, maybe superclass does ? */
	    gc = (Object *)DoSuperMethod(cl, o, (Msg)msg);
	    break;
	
    }
    return gc;
}


/*****************************/

/* Private instance data for GC class */
struct gc_data
{
    ULONG dummy;
};

/****************
**  GC::New()  **
****************/
static Object *gc_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
        
    }
    return o;

}


/********************************************************/

/* Private instance data for GC class */
struct bitmap_data
{
    LONG width, height;
    UBYTE depth;
};

#define IS_BITMAP_ATTR(attr, idx) ((idx = attr - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)


/********************
**  BitMap::New()  **
********************/
static Object *bitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct bitmap_data *data;
    struct TagItem *tag, *tstate;
    
    o  = (Object *) DoSuperMethod(cl, o, (Msg)msg);

    if(!o)
    	return NULL;
	
    data = INST_DATA(cl, o);
    
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
	ULONG idx;
	
	if (IS_BITMAP_ATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
		case aoHidd_BitMap_Width:
		    data->width = tag->ti_Data;
		    break;
		case aoHidd_BitMap_Height:
		    data->height = tag->ti_Data;
		    break;
		case aoHidd_BitMap_Depth:
		    data->depth = tag->ti_Data;
		    break;
		    
		default:
		   D(bug("  unknown attribute %li\n", tag->ti_Data));
		   break;
	    } /* switch tagidx */
	    
	} /* if (bitmap tag) */
	
    } /* while tags  */
    

    return o;

}

/********************************************************/

#undef X11GfxBase

/**********************
**  init_gfxclass()  **
**********************/

#define NUM_ROOT_METHODS 1
#define NUM_GFXHIDD_METHODS 1

Class *init_gfxclass (struct x11gfxbase *X11GfxBase)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,	moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newgc,	moHidd_Gfx_NewGC},
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 	NUM_ROOT_METHODS},
    	{gfxhidd_descr, IID_Hidd_Gfx, 	NUM_GFXHIDD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct gfx_data) },
	{TAG_DONE, 0UL}
    };

    D(bug("GfxHiddClass init\n"));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    if(cl)
    {
	D(bug("GfxHiddClass ok\n"));
	cl->UserData = (APTR)X11GfxBase;

	AddClass(cl);
    }
    return cl;
}


/*********************
**  init_gcclass()  **
*********************/

#undef NUM_ROOT_METHODS
#define NUM_ROOT_METHODS 1

#define NUM_GC_METHODS 0

Class *init_gcclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_new,	moRoot_New},
	{NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_GC_METHODS + 1] = 
    {
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr,	IID_Root,		NUM_ROOT_METHODS},
    	{gc_descr, 	IID_Hidd_GC,		NUM_GC_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{aMeta_SuperID,			(IPTR)CLID_Root},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_InstSize,		(IPTR)sizeof (struct gc_data) },
	{TAG_DONE, 0UL}
    };
    
    Class *cl;

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    if (cl)
    {
	D(bug("GC class ok\n"));
	cl->UserData = (APTR)X11GfxBase;
	
	/* Get attrbase for the GC interface */
	HiddGCAttrBase = GetAttrBase(IID_Hidd_GC);

	AddClass(cl);
    }

    return cl;
}

/*************************
**  init_bitmapclass()  **
*************************/
#define NUM_BITMAP_METHODS 0

#undef NUM_ROOT_METHODS
#define NUM_ROOT_METHODS 1

Class *init_bitmapclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())bitmap_new,	moRoot_New},
	{NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_BITMAP_METHODS + 1] = 
    {
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr,	IID_Root,		NUM_ROOT_METHODS},
    	{gc_descr, 	IID_Hidd_BitMap,	NUM_BITMAP_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{aMeta_SuperID,			(IPTR)CLID_Root},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_InstSize,		(IPTR)sizeof (struct bitmap_data) },
	{TAG_DONE, 0UL}
    };
    
    Class *cl;

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    if (cl)
    {
	D(bug("GC class ok\n"));
	cl->UserData = (APTR)X11GfxBase;
	
	/* Get attrbase for the GC interface */
	HiddBitMapAttrBase = GetAttrBase(IID_Hidd_GC);

	AddClass(cl);
    }

    return cl;
}

/**********************
**  cleanup_class()  **
**********************/
VOID cleanup_class(Class *class, struct x11gfxbase *X11GfxBase)
{
    RemoveClass(class);
    
    DisposeObject((Object *)class);
}

const char X11Gfx_end = 0;
