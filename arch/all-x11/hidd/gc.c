/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: GC class for the X11Gfx HIDD
    Lang: English.
*/

#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>


#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

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


/* Attrbases */
static AttrBase HiddGCAttrBase = 0;

/* Private instance data for GC class */
struct gc_data
{
    GC 		gc;
    Window 	xwindow;
    Display	*display;
    int		screen;
    
};

/****************  GC::New()  *******************************/
static Object *gc_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("X11Gfx.GC::New()\n"));
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    D(bug("Returned from super: %p\n", o));
    if (o)
    {
         Object *bitmap;
	 struct bitmap_data *bm_data;
	 struct gc_data *data;
	 XGCValues gcval;
	 BOOL ok = TRUE;
	 
	 data = (struct gc_data *)INST_DATA(cl, o);
	 D(bug("data=%p\n", data));
	 
	 memset(data, 0, sizeof (struct gc_data));
	 
	 D(bug("Data set\n"));

	 
         /* Get the bitmap attriute from the superclass */

	 GetAttr(o, aHidd_GC_BitMap, (IPTR *)&bitmap);
	 D(bug("Bitmap: %p\n", bitmap));
	 	 
	 /* Get direct access to the instance data (ugly, should
	    go through private methods/attrs but we don't hav'em yet
	 */
	 bm_data = (struct bitmap_data *)INST_DATA(X11GfxBase->bitmapclass, bitmap);

	 D(bug("bm_data: %p\n", bm_data));
	 
	 /* For the sake of speed we duplicate some info from the bitmap object */
	 data->xwindow = bm_data->xwindow;
	 data->display = bm_data->sysdisplay;
	 data->screen  = bm_data->sysscreen;
	 
	 /* Create X11 GC */
	 
	 gcval.plane_mask = BlackPixel(data->display, data->screen); /* bm_data->sysplanemask; */
	 gcval.graphics_exposures = True;
	 
	 D(bug("Creating X11 GC\n"));
	 data->gc = XCreateGC( data->display
	 	, DefaultRootWindow( data->display )
		, GCPlaneMask | GCGraphicsExposures
		, &gcval
	 );
	 D(bug("GC: %p\n", data->gc));
	 if (!data->gc)
	     ok = FALSE;
	 
	 if (!ok)
	 {
	      MethodID dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	      CoerceMethod(cl, o, (Msg)&dispose_mid);
	      o = NULL;
	 }
	 
    } /* if (Object created by superclass) */
    
    ReturnPtr("X11Gfx.GC::New", Object *, o);

}

/**********  GC::Dispose() **********************************/

static VOID gc_dispose(Class *cl, Object *o, Msg msg)
{
    struct gc_data *data = INST_DATA(cl, o);
    struct bitmap_data *bm_data;
    Object *bitmap;
     
    EnterFunc(bug("X11Gfx.GC::Dispose()\n"));
    GetAttr(o, aHidd_GC_BitMap, (IPTR *)&bitmap);
    bm_data = INST_DATA(X11GfxBase->bitmapclass, bitmap);
     
    if (data->gc)
    {
         XFreeGC(bm_data->sysdisplay, data->gc);
	 data->gc = NULL;
    }
    DoSuperMethod(cl, o, msg);
    
   ReturnVoid("X11Gfx.GC::Dispose");
}


/*********  GC::WritePixelDirect()  ***************************/

static VOID gc_writepixeldirect(Class *cl, Object *o, struct pHidd_GC_WritePixelDirect *msg)
{
     struct gc_data *data = INST_DATA(cl, o);
     UWORD i;
     
     EnterFunc(bug("X11Gfx.GC::WritePixelDirect()\n"));

     for (i = 0; i < 100; i ++) {
     
     XSetForeground(data->display, data->gc, BlackPixel(data->display, data->screen));
     XDrawPoint(data->display, data->xwindow, data->gc, i, i); /* msg->x, msg->y); */
     }
     ReturnVoid("X11Gfx.GC::WritePixelDirect");
   
}


/****************  init_gcclass()  **************************/
#undef X11GfxBase



#define NUM_ROOT_METHODS	2
#define NUM_GC_METHODS 		1


Class *init_gcclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_new,		moRoot_New},
    	{(IPTR (*)())gc_dispose,	moRoot_Dispose},
	{NULL, 0UL}
    };

    struct MethodDescr gc_descr[NUM_GC_METHODS + 1] = 
    {
    	{(IPTR (*)())gc_writepixeldirect,	moHidd_GC_WritePixelDirect},
	{NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr,	IID_Root,		NUM_ROOT_METHODS},
    	{gc_descr, 	IID_Hidd_GC,		NUM_GC_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase   = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{aMeta_SuperID,			(IPTR)CLID_Hidd_GCQuick},
	{aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{aMeta_InstSize,		(IPTR)sizeof (struct gc_data) },
	{TAG_DONE, 0UL}
    };
    
    Class *cl;

    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    
    	if (cl)
    	{
	    /* Get attrbase for the GC interface */
	    HiddGCAttrBase = ObtainAttrBase(IID_Hidd_GC);

	    if (HiddGCAttrBase)
	    {
	    	D(bug("GC class ok\n"));
	    	cl->UserData = (APTR)X11GfxBase;
		AddClass(cl);
	    }
	    else
	    {
	    	free_gcclass(X11GfxBase);
		
		cl = NULL;
	    }
	

	} /* if (cl) */

	/* We don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    return cl;
}


/*************** free_gclass() *******************************/

VOID free_gcclass(struct x11gfxbase *X11GfxBase)
{
    EnterFunc(bug("free_gcclass(X11GfxBase=%p)\n", X11GfxBase));

    if(X11GfxBase)
    {
        RemoveClass(X11GfxBase->gcclass);
	
        if(X11GfxBase->gcclass)
	    DisposeObject((Object *) X11GfxBase->gcclass);
	
        X11GfxBase->gcclass = NULL;
	
        if(HiddGCAttrBase) 
	    ReleaseAttrBase(IID_Hidd_GC);
	HiddGCAttrBase = 0;
	
    }

    ReturnVoid("free_gcclass");
}
