/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/


#include <stddef.h>
#include <stdio.h>

#include <X11/Xlib.h>

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

#include "x11gfx_intern.h"

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>


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

static int MyErrorHandler (Display * display, XErrorEvent * errevent)
{
    char buffer[256];

    XGetErrorText (display, errevent->error_code, buffer, sizeof (buffer));
    fprintf (stderr
	, "XError %d (Major=%d, Minor=%d)\n%s\n"
	, errevent->error_code
	, errevent->request_code
	, errevent->minor_code
	, buffer
    );
    fflush (stderr);

    return 0;
}

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    return 0;
}

static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{

    EnterFunc(bug("X11Gfx::New()\n"));

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    {
    	if (o)
	{
	    /* Do GfxHidd initalization here */
	    XSetErrorHandler (MyErrorHandler);
	    XSetIOErrorHandler (MySysErrorHandler);
	
	}
    }
    ReturnPtr("X11Gfx::New", Object *, o);
}

/********** GfxHidd::NewBitMap()  ****************************/
static Object *gfxhidd_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    EnterFunc(bug("X11Gfx::NewBitMap()\n"));
    
    ReturnPtr("X11Gfx::NewBitMap", Object *, NewObject(X11GfxBase->bitmapclass, NULL, msg->attrList));
}

/*
static VOID gfxhidd_disposebitmap(Class *cl, Object *o, struct pHidd_Gfx_DisposeBitMap *msg)
{
    EnterFunc(bug("X11Gfx::DisposeBitMap()\n"));
    
    DisposeObject(msg->bitMap);
    ReturnVoid("X11Gfx::DisposeBitMap");
}
*/
/**********  GfxHidd::CreateGC()  ****************************/
/*
static Object *gfxhidd_newgc(Class *cl, Object *o, struct pHidd_Gfx_NewGC *msg)
{

    Object *gc = NULL;
    
    switch (msg->gcType)
    {
        case  vHIDD_Gfx_GCType_Quick:
	    gc = NewObject( X11GfxBase->gcclass, NULL, msg->attrList);
	    break;
	    
	default:
	    gc = (Object *)DoSuperMethod(cl, o, (Msg)msg);
	    break;
	
    }
    return gc;
}

*/
#undef X11GfxBase

/********************  init_gfxclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_GFXHIDD_METHODS 2

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
    	{(IPTR (*)())gfxhidd_newbitmap,	moHidd_Gfx_NewBitMap},
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 	NUM_ROOT_METHODS},
    	{gfxhidd_descr, IID_Hidd_Gfx, 	NUM_GFXHIDD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct gfx_data) },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("GfxHiddClass init\n"));
    
    if (MetaAttrBase)
    {

    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    	if(cl)
    	{
	    D(bug("GfxHiddClass ok\n"));
	    cl->UserData = (APTR)X11GfxBase;

	    AddClass(cl);
	}
	
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    return cl;
}




/*************** free_gfxclass()  **********************************/
VOID free_gfxclass(struct x11gfxbase *X11GfxBase)
{
    EnterFunc(bug("free_gfxclass(X11GfxBase=%p)\n", X11GfxBase));

    if(X11GfxBase)
    {

        RemoveClass(X11GfxBase->gfxclass);
	
        if(X11GfxBase->gfxclass) DisposeObject((Object *) X11GfxBase->gfxclass);
        X11GfxBase->gfxclass = NULL;

    }

    ReturnVoid("free_gfxclass");
}

