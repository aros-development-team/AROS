/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Gfx HIDD baseclass for AROS.
    Lang: English.
*/

#include "gfx_intern.h"
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
struct gfxhiddbase *AROS_SLIB_ENTRY(init,X11Gfx)();
struct gfxhiddbase *AROS_SLIB_ENTRY(open,X11Gfx)();
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

static const char name[]="gfx.hidd";

static const char version[]="$VER: gfx.hidd 0.1 (16.9.98)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct gfxhiddbase),
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

AROS_LH2(struct gfxhiddbase *, init,
    AROS_LHA(struct gfxhiddbase *, LIBBASE, D0),
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
#define OOPBase LIBBASE->oopbase


/* Some attrbases needed as global vars.
  These are write-once read-many */
  
  AttrBase HiddGfxAttrBase = 0;
  AttrBase HiddGCAttrBase = 0;


AROS_LH1(struct gfxhiddbase *, open,
    AROS_LHA(ULONG, version, D0),
    struct gfxhiddbase *, LIBBASE, 1, BASENAME)
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

    /* Initialze attrbase.
        NOTE : To be on the safe side we initialize both attrbases
	before the classes, so that we are 100% sure that they
	are initialized before they are used inside any of the class' methods */
	
    if (!HiddGCAttrBase)
    	HiddGCAttrBase = ObtainAttrBase(IID_Hidd_GC);
    if (!HiddGCAttrBase)
    	return (NULL);

    if (!HiddGfxAttrBase)
    	HiddGfxAttrBase = ObtainAttrBase(IID_Hidd_Gfx);
    if (!HiddGfxAttrBase)
    	return (NULL);

    /* Create HIDD classes */

    if (!LIBBASE->gcclass)
    	LIBBASE->gcclass = init_gcclass(LIBBASE);
    if (!LIBBASE->gcclass)
    	return (NULL);

    if (!LIBBASE->gfxclass)
    	LIBBASE->gfxclass = init_gfxclass(LIBBASE);
    if (!LIBBASE->gfxclass)
    	return (NULL);
	

    

    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct gfxhiddbase *, LIBBASE, 2, BASENAME)
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
	    
	if (LIBBASE->gcclass)
	    cleanup_gcclass(LIBBASE->gcclass, LIBBASE);
	    
	if (LIBBASE->gfxclass)
	    cleanup_gfxclass(LIBBASE->gfxclass, LIBBASE);
	    
	/* Release AttrBases */
	if (HiddGfxAttrBase)
	    ReleaseAttrBase(IID_Hidd_Gfx);
	
	if (HiddGCAttrBase)
	    ReleaseAttrBase(IID_Hidd_GC);
	    
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

AROS_LH0(BPTR, expunge, struct gfxhiddbase *, LIBBASE, 3, BASENAME)
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

AROS_LH0I(int, null, struct gfxhiddbase *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}



AROS_LH0(Class *, getclass, struct gfxhiddbase *, LIBBASE, 5, BASENAME)
{
     return LIBBASE->gfxclass;
}


#undef SysBase
#define SysBase (GfxHiddBase->sysbase)

#undef OOPBase
#define OOPBase (GfxHiddBase->oopbase)

#undef UtilityBase
#define UtilityBase (GfxHiddBase->utilitybase)

#define GfxHiddBase ((struct gfxhiddbase *)cl->UserData)





/********************************************************/



struct gfxhidd_data
{
    Class *bitmap;  /* bitmap class     */
    Class *gc;      /* graphics context */
};


/**************************
**  GfxHIDD::CreateGC()  **
**************************/
static Object *gfxhidd_creategc(Class *cl, Object *o, struct pHidd_Gfx_CreateGC *msg)
{
    Object *gc = NULL;
    struct TagItem tags[] =
    {
        {aHidd_GC_BitMap,	(IPTR)msg->bitMap},
	{TAG_DONE,		0}
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



/*******************************************
**  GC Class Methods
*******************************************/
/* OPTIMIZATION NOTE: Every gc should have a method object for the WritePixelDirect
   method which it stores in the instance data.

*/

struct gc_data
{
    APTR bitMap;     /* bitmap to which this gc is connected             */
    ULONG fg;        /* foreground color                                 */
    ULONG bg;        /* background color                                 */
    ULONG drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR  font;      /* current fonts                                    */
    ULONG colMask;   /* ColorMask prevents some color bits from changing */
    UWORD linePat;   /* LinePattern                                      */
    APTR  planeMask; /* Pointer to a shape bitMap                        */
    APTR  userData;  /* pointer to own data                              */
    /* WARNING: structure could be extented in the future                */

};
VOID hidd_GC_WritePixel(Object *gc, WORD x, WORD y);
VOID hidd_GC_WritePixelDirect(Object *gc, WORD x, WORD y, ULONG val );

#define IS_GC_ATTR(attr, idx) ( ( (idx) = (attr) - HiddGCAttrBase) < num_Hidd_GC_Attrs)

#define GC(o) ((struct gc_data *)o)

static Object *gc_new(Class *cl, Object *obj, struct pRoot_New *msg)
{
    Object *bitmap;	
    D(bug("GC_GCClass - OM_NEW:\n"));
    
    /* User MUST supply bitmap */
    
    bitmap = (APTR) GetTagData(aHidd_GC_BitMap, NULL, msg->attrList);
    if (!bitmap)
    	return NULL;

    obj  = (Object *) DoSuperMethod(cl, obj, (Msg)msg);
    if (obj)
    {
    	struct gc_data *data;
	struct TagItem *tag, *tstate;
    
 	data = INST_DATA(cl, obj);
	
	data->bitMap = bitmap;   /* bitmap to which this gc is connected    */
	
	/* fill in some defaults */
	data->fg	= 1;        /* foreground color                        */
        data->fg        = 1;        /* foreground color                        */
        data->bg        = 0;        /* background color                        */
        data->drMode    = HIDDV_GC_DrawMode_Copy;    /* drawmode               */
        data->font      = NULL;     /* current fonts                           */
        data->colMask   = -1;       /* ColorMask prevents some color bits from changing*/
       	data->linePat   = -1;       /* LinePattern                             */
        data->planeMask = NULL;     /* Pointer to a shape bitMap               */

	
	/* Override defaults with user suplied attrs */
	tstate = msg->attrList;
	while ( (tag = NextTagItem(&tstate)) )
	{
	    ULONG idx;
		
	    if (IS_GC_ATTR(tag->ti_Tag, idx))
	    {
		switch(idx)
		{
		case aoHidd_GC_BitMap     : /* was set with HIDD_Graphics_CreateGC() */ break;
		case aoHidd_GC_Foreground : data->fg        = tag->ti_Data; break;
		case aoHidd_GC_Background : data->bg        = tag->ti_Data; break;
		case aoHidd_GC_DrawMode   : data->drMode    = tag->ti_Data; break;
		case aoHidd_GC_Font       : data->font      = (APTR) tag->ti_Data; break;
		case aoHidd_GC_ColorMask  : data->colMask   = tag->ti_Data; break;
		case aoHidd_GC_LinePattern: data->linePat   = (UWORD) tag->ti_Data; break;
		case aoHidd_GC_PlaneMask  : data->planeMask = (APTR) tag->ti_Data; break;
		case aoHidd_GC_UserData   : data->userData  = (APTR) tag->ti_Data; break;

		default: D(bug("  unknown attribute %li\n", tag->ti_Data)); break;
		} /* switch tag */
	    } /* if (is GC attr) */
	}
    }
    return obj;
}

/***********************
**  GC::WritePixel()  **
***********************/
static VOID gc_writepixel(Class *cl, Object *o, struct pHidd_GC_WritePixel *msg)
{
    /* Write pixel using GC settings */
    hidd_GC_WritePixelDirect( o, msg->x, msg->y, GC(o)->fg );
}

/*********************
**  GC::DrawLine()  **
*********************/

static VOID gc_drawline(Class *cl, Object *o, struct pHidd_GC_DrawLine *msg)
{

    WORD dx, dy, incrE, incrNE, d, x, y, s1, s2, t, i;

    D(bug("HIDD_Graphics_DrawLine_Q\n"));

    /* Calculate slope */
    dx = abs(msg->x2 - msg->x1);
    dy = abs(msg->y2 - msg->y1);

    /* which direction? */
    if((msg->x2 - msg->x1) > 0) s1 = 1; else s1 = - 1;
    if((msg->y2 - msg->y1) > 0) s2 = 1; else s2 = - 1;

    /* change axes if dx < dy */
    if(dx < dy)
    {
        d = dx; dx = dy; dy = d; t = 0;
    }
    else
    {
       t = 1;
    }


    d  = 2 * dy - dx;        /* initial value of d */

    incrE  = 2 * dy;         /* Increment use for move to E  */
    incrNE = 2 * (dy - dx);  /* Increment use for move to NE */

    x = msg->x1; y = msg->y1;
    hidd_GC_WritePixelDirect(o, x, y, GC(o)->fg); /* The start pixel */

    for(i = 0; i <= dx; i++)
    {
        if(d <= 0)
        {
            if(t == 1)
            {
                x = x + s1;
            }
            else
            {
                y = y + s2;
            }

            d = d + incrE;
        }
        else
        {
            if(t == 1)
            {
                x = x + s1;
                y = y + s2;
            }
            else
            {
                x = x + s1;
                y = y + s2;
            }

            d = d + incrNE;
        }
        hidd_GC_WritePixelDirect(o, x, y, GC(o)->fg);
    }
    
}


/*************************** Classes *****************************/


#undef GfxHiddBase

#define NUM_ROOT_METHODS 1
#define NUM_GC_METHODS 2

#define NUM_GFXHIDD_METHODS 2

/**********************
**  init_gfxclass()  **
**********************/
static Class *init_gfxclass(struct gfxhiddbase *GfxHiddBase)
{
    Class *cl = NULL;

    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_creategc,	moHidd_Gfx_CreateGC},
    	{(IPTR (*)())gfxhidd_deletegc,	moHidd_Gfx_DeleteGC},
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
	{ aMeta_SuperID,		(IPTR)CLID_Hidd},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_ID,			(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InstSize,		(IPTR)sizeof (struct gfxhidd_data) },
	{TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("init_gfxhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
	D(bug("GfxHiddClass ok\n"));
	cl->UserData = (APTR)GfxHiddBase;
	
	HiddGfxAttrBase = GetAttrBase(IID_Hidd_Gfx);

	AddClass(cl);
    }
    ReturnPtr("init_gfxhiddclass", Class *, cl);
}

/*********************
**  init_gcclass()  **
*********************/
static Class *init_gcclass(struct gfxhiddbase *GfxHiddBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_new,	moRoot_New},
	{NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_GC_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_drawline,	moHidd_GC_DrawLine},
    	{(IPTR (*)())gc_writepixel,	moHidd_GC_WritePixel},
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
	{aMeta_ID,			(IPTR)CLID_Hidd_QuickGC},
	{aMeta_InstSize,		(IPTR)sizeof (struct gc_data) },
	{TAG_DONE, 0UL}
    };
    
    Class *cl;

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    if (cl)
    {
	D(bug("GC class ok\n"));
	cl->UserData = (APTR)GfxHiddBase;
	
	/* Get attrbase for the GC interface */
	HiddGCAttrBase = GetAttrBase(IID_Hidd_GC);

	AddClass(cl);
    }

    return cl;
}

/************************
**  cleanup_gcclass()  **
************************/
static VOID cleanup_gcclass(Class *gcclass, struct gfxhiddbase *GfxHiddBase)
{
    RemoveClass(gcclass);
    DisposeObject((Object *)gcclass);
}


/************************
**  cleanup_gcclass()  **
************************/
static VOID cleanup_gfxclass(Class *gfxclass, struct gfxhiddbase *GfxHiddBase)
{
    RemoveClass(gfxclass);
    DisposeObject((Object *)gfxclass);
}

/************
**  Stubs  **
************/
#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(OCLASS(gc)))->UserData)
VOID hidd_GC_WritePixel(Object *gc, WORD x, WORD y)
{
    static MethodID mid = NULL;
    struct pHidd_GC_WritePixel p;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_GC, moHidd_GC_WritePixel);
	
    p.mID = mid;
    p.x = x;
    p.y = y;
    
    DoMethod(gc, (Msg)&p);
}

VOID hidd_GC_WritePixelDirect(Object *gc, WORD x, WORD y, ULONG val)
{
    static MethodID mid = NULL;
    struct pHidd_GC_WritePixelDirect p;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_GC, moHidd_GC_WritePixelDirect);
	
    p.mID = mid;
    p.x = x;
    p.y = y;
    p.val = val;
    
    DoMethod(gc, (Msg)&p);
}

static const char GfxHidd_end = 0;


