/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Separate class for offscreen bitmaps.
    Lang: English.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/graphics.h>

#include <exec/memory.h>

#include "x11gfx_intern.h"

#define DEBUG 0
#include <aros/debug.h>
/* This class handles offscreen (ie. not displayable)
    bitmaps/images for the X11 hidd (using XImages)
    Since this image is held in the X client, manipulating it
    is MUCH faster than manipulating drawables on the server side
*/
struct osbm_data
{
    XImage *ximage;
    char *imdata;
    Display *display;
    int	screen;
    ULONG *hidd2x11cmap;
    
};

#undef X11GfxBase
#define X11GfxBase ((struct x11gfxbase *)cl->UserData)

#undef OOPBase
#define OOPBase (X11GfxBase->oopbase)

#undef SysBase
#define SysBase (X11GfxBase->sysbase)

#undef UtilityBase
#define UtilityBase (X11GfxBase->utilitybase)

static AttrBase HiddBitMapAttrBase = 0;
static AttrBase HiddX11GfxAB = 0;

static struct abdescr attrbases[] = 
{
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase },
    { IID_Hidd_X11Gfx,	&HiddX11GfxAB	},
    { NULL, NULL }
    
};


static Object *bitmap_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
    EnterFunc(bug("X11Gfx.OffBitMap::New()\n"));

#warning Design problem: Superclass will now also allocate a bitmap, which will never be used
    o = (Object *)DoSuperMethod(cl, o, (Msg) msg);
    if (o)
    {
    	struct osbm_data *data;
	
        ULONG width, height, depth, numwords;
        data = INST_DATA(cl, o);

	/* clear all data  */
        memset(data, 0, sizeof( *data ));
	
	/* Get sysdisplay and sysscreen. (X11Gfx hidd gives it to us) */
	data->display = (Display *)GetTagData(aHidd_X11Gfx_SysDisplay, (IPTR)NULL, msg->attrList);
	data->screen  = GetTagData(aHidd_X11Gfx_SysScreen, 0, msg->attrList);
	data->hidd2x11cmap = (ULONG *)GetTagData(aHidd_X11Gfx_Hidd2X11CMap, (IPTR)NULL, msg->attrList);
	
	/* Get attr values */
	GetAttr(o, aHidd_BitMap_Width,		&width);
	GetAttr(o, aHidd_BitMap_Height, 	&height);
	GetAttr(o, aHidd_BitMap_Depth,		&depth);


	/* 16-bit align width */
	numwords = ((width - 1) >> 4) + 1;
	/* Allocate memory for image data */
	
	data->imdata = AllocVec( (numwords / 2) * depth * height, MEMF_ANY);
	if (!data->imdata)
	    ok = FALSE;
	else
	{
	    data->ximage = XCreateImage( data->display
		, DefaultVisual(data->display, data->screen)
		, depth
		, ZPixmap
		, 0	/* Offset	*/
		, NULL	/* Data		*/
		, width
		, height
		, 16
		, 0
	     );
	     if (!data->ximage)
	     	ok = FALSE;
	     
	     
	}     
	

    	if (!ok)
    	{
    
            MethodID disp_mid = GetMethodID(IID_Root, moRoot_Dispose);
    	    CoerceMethod(cl, o, (Msg) &disp_mid);
	
	    o = NULL;
    	}


    } /* if (object allocated by superclass) */
    
    ReturnPtr("X11Gfx.BitMap::New()", Object *, o);
}


/*********** Bitmap::Dispose() ***************************************/

static VOID bitmap_dispose(Class *cl, Object *o, Msg msg)
{
    struct osbm_data * data = INST_DATA(cl, o);
    
    if (data->ximage)
    	XFree(data->ximage);
	
    if (data->imdata)
    	FreeVec(data->imdata);
	
    DoSuperMethod(cl, o, msg);
}
/********* BitMap::PutPixel()  ***************************************/

static VOID bitmap_putpixel(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct osbm_data *data = INST_DATA(cl, o);
    
    XPutPixel(data->ximage, msg->x, msg->y, data->hidd2x11cmap[msg->val]);
    
}
static ULONG bitmap_getpixel(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct osbm_data *data = INST_DATA(cl, o);
    return map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(data->ximage, msg->x, msg->y));
}
#define NUM_ROOT_METHODS   2
#define NUM_BITMAP_METHODS 3


#undef X11GfxBase

Class *init_osbitmapclass(struct x11gfxbase *X11GfxBase)
{
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())bitmap_new    , moRoot_New    },
        {(IPTR (*)())bitmap_dispose, moRoot_Dispose},
        {NULL, 0UL}
    };

    struct MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
    	{(IPTR (*)())bitmap_putpixel,		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())bitmap_getpixel,		moHidd_BitMap_GetPixel},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof (struct osbm_data)},
        {TAG_DONE, 0UL}
    };
    
    Class *cl = NULL;

    EnterFunc(bug("init_osbitmapclass(X11GfxBase=%p)\n", X11GfxBase));

    if(MetaAttrBase)
    {
        cl = NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            X11GfxBase->osbitmapclass = cl;
            cl->UserData     = (APTR) X11GfxBase;
           
            /* Get attrbase for the BitMap interface */
   	   
	    /* Get semiprivate attrbase */
	    if (obtainattrbases(attrbases, OOPBase))
            {
                AddClass(cl);
            }
            else
            {
                free_osbitmapclass(X11GfxBase);
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_osbitmapclass", Class *,  cl);
}


/*** free_osbitmapclass *********************************************************/

void free_osbitmapclass(struct x11gfxbase *X11GfxBase)
{
    EnterFunc(bug("free_osbitmapclass(X11GfxBase=%p)\n", X11GfxBase));

    if(X11GfxBase)
    {
        RemoveClass(X11GfxBase->osbitmapclass);
        if(X11GfxBase->osbitmapclass) DisposeObject((Object *) X11GfxBase->osbitmapclass);
        X11GfxBase->osbitmapclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);
    }

    ReturnVoid("free_osbitmapclass");
}
