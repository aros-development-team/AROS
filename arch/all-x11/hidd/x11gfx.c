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

/* Library info */
#define BASENAME X11Gfx

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct x11gfxbase *AROS_SLIB_ENTRY(init,X11Gfx)();
struct x11gfxbase *AROS_SLIB_ENTRY(open,X11Gfx)();
BPTR AROS_SLIB_ENTRY(close,X11Gfx)();
BPTR AROS_SLIB_ENTRY(expunge,X11Gfx)();
int AROS_SLIB_ENTRY(null,X11Gfx)();
Class *AROS_SLIB_ENTRY(getclass,X11Gfx)();
static const char end;

int entry(void)
{
    /* If the handler was executed by accident return error code. */
    return -1;
}

const struct Resident X11Gfx_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&X11Gfx_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    0,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="x11gfx.hidd";

static const char version[]="$VER: x11gfx.hidd 0.1 (16.9.98)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct x11gfxbase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,X11Gfx)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,X11Gfx),
    &AROS_SLIB_ENTRY(close,X11Gfx),
    &AROS_SLIB_ENTRY(expunge,X11Gfx),
    &AROS_SLIB_ENTRY(null,X11Gfx),
    &AROS_SLIB_ENTRY(getclass,X11Gfx),
    (void *)-1
};

AROS_LH2(struct x11gfxbase *, init,
    AROS_LHA(struct x11gfxbase *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    LIBBASE->sysbase = sysBase;

    LIBBASE->seglist = segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

/* Use This from now on */
#define SysBase LIBBASE->sysbase

/* Predeclaration */
struct IClass *InitMutualExcludeClass(struct x11gfxbase *);

AROS_LH1(struct x11gfxbase *, open,
    AROS_LHA(ULONG, version, D0),
    struct x11gfxbase *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */


    /* Keep compiler happy */
    version=0;

    if (!LIBBASE->oopbase)
    	LIBBASE->oopbase = OpenLibrary(AROSOOP_NAME, 37);
    if (!LIBBASE->oopbase)
	return (NULL);


    if (!LIBBASE->utilitybase)
	LIBBASE->utilitybase = OpenLibrary("utility.library", 0);
    if (!LIBBASE->utilitybase)
	return(NULL);


    /* Create HIDD classes */
    if (!LIBBASE->gfxclass)
    	LIBBASE->gfxclass = init_gfxclass(LIBBASE);
    if (!LIBBASE->gfxclass)
    	return (NULL);
    
    if (!LIBBASE->gcclass)
    	LIBBASE->gcclass = init_gcclass(LIBBASE);
    if (!LIBBASE->gcclass)
    	return (NULL);
    	
    if (!LIBBASE->bitmapclass)
    	LIBBASE->bitmapclass = init_bitmapclass(LIBBASE);
    if (!LIBBASE->bitmapclass)
    	return (NULL);
    

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct x11gfxbase *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->library.lib_OpenCnt)
    {
	if (LIBBASE->bitmapclass)
	    cleanup_class(LIBBASE->bitmapclass, LIBBASE);
	    
	if (LIBBASE->gcclass)
	    cleanup_class(LIBBASE->gcclass, LIBBASE);
	    
	if (LIBBASE->gfxclass)
	    cleanup_class(LIBBASE->gfxclass, LIBBASE);
	
	if (LIBBASE->utilitybase)
	{
	    CloseLibrary(LIBBASE->utilitybase);
	    LIBBASE->utilitybase = NULL;
	}
	
	if (LIBBASE->oopbase)
	{
	    CloseLibrary(LIBBASE->oopbase);
	    LIBBASE->oopbase = NULL;
	}

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct x11gfxbase *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct x11gfxbase *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}



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
        case  vHIDD_Gfx_NewGC_Quick:
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
