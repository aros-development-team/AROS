/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <devices/inputevent.h>

#include "vga.h"

#define DEBUG 1
#include <aros/debug.h>

static AttrBase HiddMouseAB;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL, NULL }
};

struct mouse_data
{
    VOID (*mouse_callback)(APTR, UWORD);
    APTR callbackdata;
};

/***** Mouse::New()  ***************************************/
static Object * mouse_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("Mouse::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_mouse_hidd) /* Cannot open twice */
    	ReturnPtr("Mouse::New", Object *, NULL); /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct mouse_data *data = INST_DATA(cl, o);
	struct TagItem *tag, *tstate;
	
	tstate = msg->attrList;
	while ((tag = NextTagItem((const struct TagItem **)&tstate)))
	{
	    ULONG idx;
	    
	    if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
	    {
	    	switch (idx)
		{
		    case aoHidd_Mouse_IrqHandler:
		    	data->mouse_callback = (VOID (*)())tag->ti_Data;
			break;
			
		    case aoHidd_Mouse_IrqHandlerData:
		    	data->callbackdata = (APTR)tag->ti_Data;
			break;
		}
	    }
	    
	} /* while (tags to process) */
	
	/* Install the mouse hidd */


	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->mousehidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    return o;
}

/***** Mouse::HandleEvent()  ***************************************/

static VOID mouse_handleevent(Class *cl, Object *o, struct pHidd_Mouse_HandleEvent *msg)
{
    struct mouse_data * data;

    EnterFunc(bug("kbd_handleevent()\n"));

    data = INST_DATA(cl, o);

/* Nothing done yet */

    ReturnVoid("Kbd::HandleEvent");
}

#undef XSD
#define XSD(cl) xsd

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_MOUSE_METHODS 1

Class *init_mouseclass (struct vga_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(mouse_new),		moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr mousehidd_descr[NUM_MOUSE_METHODS + 1] = 
    {
    	{METHODDEF(mouse_handleevent),	moHidd_Mouse_HandleEvent},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{mousehidd_descr, IID_Hidd_HwMouse, 	NUM_MOUSE_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct mouse_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_HwMouse },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("MouseHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->mouseclass = cl;
	    
	    if (ObtainAttrBases(attrbases))
	    {
		D(bug("MouseHiddClass ok\n"));
		
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_mouseclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_mouseclass", Class *, cl);
}

/*************** free_kbdclass()  **********************************/
VOID free_mouseclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_mouseclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        RemoveClass(xsd->mouseclass);
	
        if(xsd->mouseclass) DisposeObject((Object *) xsd->mouseclass);
        xsd->mouseclass = NULL;
	
	ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_mouseclass");
}

