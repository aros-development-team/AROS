/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: X11 hidd handling mouse events.
    Lang: English.
*/

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <X11/Xlib.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include "x11.h"

#define DEBUG 0
#include <aros/debug.h>

struct x11mouse_data
{
    VOID (*mouse_callback)(APTR, ULONG, ULONG);
    APTR callbackdata;
};

static AttrBase HiddMouseAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL,	NULL }
};


static ULONG xbutton2hidd(XButtonEvent *xb)
{
    ULONG button;
    switch (xb->button)
    {
	case Button1:
	    button = 0;
	    break;
/* !!! For now we don't care about the middle mousebutton

	
	case Button2:
	    button = 1;
	    break;
*/
	    
	case Button3:
	    button = 1;
	    break;
	
    }
    return button;
    
}

static Object * x11mouse_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    ObtainSemaphoreShared( &XSD(cl)->sema);
    
    if (XSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
	
    ReleaseSemaphore( &XSD(cl)->sema);
    
    if (has_mouse_hidd) /* Cannot open twice */
    	return NULL; /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct x11mouse_data *data = INST_DATA(cl, o);
	struct TagItem *tag, *tstate;
	
	tstate = msg->attrList;
	while ((tag = NextTagItem(&tstate)))
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

static VOID x11mouse_handleevent(Class *cl, Object *o, struct pHidd_X11Mouse_HandleEvent *msg)
{

    struct x11mouse_data *data = INST_DATA(cl, o);
    
    XButtonEvent *xb = &(msg->event->xbutton);
    
    
    if (msg->event->type == ButtonRelease)
    {
        data->mouse_callback(data->callbackdata
		, xbutton2hidd(xb)
		, vHidd_Mouse_Release);
		

    }
    else if (msg->event->type == ButtonPress)
    {
        data->mouse_callback(data->callbackdata
			, xbutton2hidd(xb)
			, vHidd_Mouse_Press);
    }
    else if (msg->event->type == MotionNotify)
    {
#warning TODO
/*    
        data->mousecallback(data->
*/    }
    
    return;
}


/********************  init_mouseclass()  *********************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS 1
#define NUM_X11MOUSE_METHODS 1

Class *init_mouseclass (struct x11_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(x11mouse_new),		moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr mousehidd_descr[NUM_X11MOUSE_METHODS + 1] = 
    {
    	{METHODDEF(x11mouse_handleevent),	moHidd_X11Mouse_HandleEvent},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{mousehidd_descr, IID_Hidd_X11Mouse, 	NUM_X11MOUSE_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct x11mouse_data) },
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
	    
	    if (obtainattrbases(attrbases, OOPBase))
	    {
		D(bug("MousHiddClass ok\n"));
		
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
    return cl;
}




/*************** free_mouseclass()  **********************************/
VOID free_mouseclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_mouseclass(xsd=%p)\n", xsd));

    if(xsd)
    {

        RemoveClass(xsd->mouseclass);
	
        if(xsd->mouseclass) DisposeObject((Object *) xsd->mouseclass);
        xsd->mouseclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);

    }

    ReturnVoid("free_mouseclass");
}



