/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#define DEBUG 0
#include <aros/debug.h>


#include "x11.h"

/****************************************************************************************/

struct x11mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB  },
    { NULL  	    , NULL  	    }
};

/****************************************************************************************/

static ULONG xbutton2hidd(XButtonEvent *xb)
{
    ULONG button;
    
    switch (xb->button)
    {
	case Button1:
	    button = vHidd_Mouse_Button1;
	    break;
	
	case Button2:
	    button = vHidd_Mouse_Button3;
	    break;
	    
	case Button3:
	    button = vHidd_Mouse_Button2;
	    break;
	
    }
    
    return button;
}

/****************************************************************************************/

static OOP_Object * x11mouse_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    
    ObtainSemaphoreShared( &XSD(cl)->sema);
    
    if (XSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
	
    ReleaseSemaphore( &XSD(cl)->sema);
    
    if (has_mouse_hidd) /* Cannot open twice */
    	return NULL; /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct x11mouse_data *data = OOP_INST_DATA(cl, o);
	struct TagItem       *tag, *tstate;
	
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

/****************************************************************************************/

static VOID x11mouse_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_X11Mouse_HandleEvent *msg)
{

    struct x11mouse_data    	*data = OOP_INST_DATA(cl, o);    
    struct pHidd_Mouse_Event 	 e;
    
    XButtonEvent *xb = &(msg->event->xbutton);
    
    e.x = xb->x;
    e.y = xb->y;
   
    if (msg->event->type == ButtonRelease)
    {
    	switch(xb->button)
	{
	    case Button1:
	    case Button2:
	    case Button3:
    	    	e.button = xbutton2hidd(xb);
	    	e.type   = vHidd_Mouse_Release;
	    	data->mouse_callback(data->callbackdata, &e);
		break;
	}
    }
    else if (msg->event->type == ButtonPress)
    {
    	switch(xb->button)
	{
	    case Button1:
	    case Button2:
	    case Button3:	    	
    		e.button = xbutton2hidd(xb);
		e.type   = vHidd_Mouse_Press;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	    case Button4:
	    	e.type   = vHidd_Mouse_WheelMotion;
		e.button = vHidd_Mouse_NoButton;
		e.x      = 0;
		e.y      = -1;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	    case Button5:
	    	e.type   = vHidd_Mouse_WheelMotion;
		e.button = vHidd_Mouse_NoButton;
		e.x 	 = 0;
		e.y 	 = 1;
        	data->mouse_callback(data->callbackdata, &e);
		break;
		
	}
    }
    else if (msg->event->type == MotionNotify)
    {
    	e.button = vHidd_Mouse_NoButton;
	e.type = vHidd_Mouse_Motion;
	
        data->mouse_callback(data->callbackdata, &e);
    }
    
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS 1
#define NUM_X11MOUSE_METHODS 1

/****************************************************************************************/

OOP_Class *init_mouseclass (struct x11_staticdata *xsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{OOP_METHODDEF(x11mouse_new), moRoot_New},
	{NULL	    	    	    , 0UL   	}
    };
    
    struct OOP_MethodDescr mousehidd_descr[NUM_X11MOUSE_METHODS + 1] = 
    {
    	{OOP_METHODDEF(x11mouse_handleevent), moHidd_X11Mouse_HandleEvent   },
	{NULL	    	    	    	    , 0UL   	    	    	    }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{root_descr 	, IID_Root  	    , NUM_ROOT_METHODS	    },
    	{mousehidd_descr, IID_Hidd_X11Mouse , NUM_X11MOUSE_METHODS  },
	{NULL	    	, NULL	    	    , 0     	    	    }
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID     	, (IPTR)CLID_Hidd   	    	    	},
	{ aMeta_InterfaceDescr	, (IPTR)ifdescr     	    	    	},
	{ aMeta_InstSize    	, (IPTR)sizeof (struct x11mouse_data) 	},
	{ aMeta_ID  	    	, (IPTR)CLID_Hidd_X11Mouse  	    	},
	{ TAG_DONE  	    	, 0UL	    	    	    	    	}
    };

    EnterFunc(bug("init_mouseclass(xsd=%p)\n", xsd));
    
    if (MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->mouseclass = cl;
	    
	    if (OOP_ObtainAttrBases(attrbases))
	    {
		D(bug("MouseHiddClass ok\n"));
		
	    	OOP_AddClass(cl);
	    }
	    else
	    {
	    	free_mouseclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    }
    
    ReturnPtr("init_mouseclass", OOP_Class *, cl);
}

/****************************************************************************************/

VOID free_mouseclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_mouseclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->mouseclass);
	
        if(xsd->mouseclass) OOP_DisposeObject((OOP_Object *) xsd->mouseclass);
        xsd->mouseclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);

    }

    ReturnVoid("free_mouseclass");
}

/****************************************************************************************/



