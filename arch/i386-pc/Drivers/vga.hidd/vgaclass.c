/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Class for VGA and compatible cards.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/system.h>
#include <aros/machine.h>
#include <aros/asmcall.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>

#include "vga.h"
#include "vgaclass.h"

#define DEBUG 0
#include <aros/debug.h>

/* Some attrbases needed as global vars.
  These are write-once read-many */

static AttrBase HiddBitMapAttrBase = 0;  
static AttrBase HiddVGAAB = 0;

static struct abdescr attrbases[] =
{
    { IID_Hidd_BitMap, &HiddBitMapAttrBase },
    { IID_Hidd_VGAgfx, &HiddVGAAB },
    { NULL, NULL }
};

struct vga_data
{
    int	i;	//dummy!!!!!!!!!
};

/* Default graphics modes */

struct vgaModeDesc
    vgaDefMode[NUM_MODES]={
		{"640x480x4 @ 60Hz",	// h: 31.5 kHz v: 60Hz
		640,480,4,0,
		0,
		640,664,760,800,0,
		480,491,493,525},
		{"768x576x4 @ 50Hz",	// h: 32.5 kHz v: 54Hz
		768,576,4,1,
		0,
		768,795,805,872,0,
		576,577,579,600},
		{"800x600x4 @ 50Hz",	// h: 31.5 kHz v: 52Hz
		800,600,4,1,
		0,
		800,826,838,900,0,
		600,601,603,617}
		};

/*********************
**  GfxHidd::New()  **
*********************/

static Object *gfx_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VGAGfx::New()\n"));

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
    
	MethodID dispose_mid;
//	struct vga_data *data = INST_DATA(cl, o);
	
	D(bug("Got object from super\n"));

	if(1)	/* Always exit */
	{	
	    ReturnPtr("VGAGfx::New", Object *, o);
	}
	
	D(bug("Disposing obj\n"));
	
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
    }
    ReturnPtr("VGAGfx::New", Object *, NULL);
}

static VOID gfx_dispose(Class *cl, Object *o, Msg msg)
{
    DoSuperMethod(cl, o, (Msg)msg);
    return;
}

static VOID gfx_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    DoSuperMethod(cl, o, (Msg)msg);
    return;
}

/********** GfxHidd::NewBitMap()  ****************************/
static Object *gfxhidd_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{

    BOOL displayable;
    Object *bm;
    
    struct vga_data *data;
    
    EnterFunc(bug("VGAGfx::NewBitMap()\n"));
    
    data = INST_DATA(cl, o);
    
    /* Displayeable bitmap ? */
    
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable)
    {
    	bm = NewObject(XSD(cl)->onbmclass, NULL, msg->attrList);
    }
    else
    {
	bm = NewObject(XSD(cl)->offbmclass, NULL, msg->attrList);
    }
    
    ReturnPtr("VGAGfx::NewBitMap", Object *, bm);
}

#undef XSD
#define XSD(cl) xsd

/********************  init_vgaclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_VGA_METHODS 1

Class *init_vgaclass (struct vga_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{(IPTR (*)())gfx_new,		moRoot_New},
    	{(IPTR (*)())gfx_dispose,	moRoot_Dispose},
    	{(IPTR (*)())gfx_get,		moRoot_Get},
	{NULL, 0UL}
    };
    
    struct MethodDescr vgahidd_descr[NUM_VGA_METHODS + 1] = 
    {
    	{(IPTR (*)())gfxhidd_newbitmap,	moHidd_Gfx_NewBitMap},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{vgahidd_descr, IID_Hidd_Gfx,	 	NUM_VGA_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd_Gfx},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct vga_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_VGAgfx },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("VgaHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->vgaclass = cl;
	    
	    if (obtainattrbases(attrbases, OOPBase))
	    {
		D(bug("VgaHiddClass ok\n"));
		
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_vgaclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_vgaclass", Class *, cl);
}

/*************** free_vgaclass()  **********************************/
VOID free_vgaclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_vgaclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        RemoveClass(xsd->vgaclass);
	
        if(xsd->vgaclass) DisposeObject((Object *) xsd->vgaclass);
        xsd->vgaclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);
    }
    ReturnVoid("free_vgaclass");
}
