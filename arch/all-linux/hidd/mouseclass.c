/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Linux hidd handling mouse events.
    Lang: English.
*/

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include "linux_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#define CLID_Hidd_LinuxMouse	"hidd.mouse.linux"
#define IID_Hidd_LinuxMouse	"hidd.mouse.linux"
struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

static AttrBase HiddMouseAB;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL,	NULL }
};


#if 0
static ULONG xbutton2hidd(XButtonEvent *xb)
{
    ULONG button;
    switch (xb->button)
    {
	case Button1:
	    button = vHidd_Mouse_Button1;
	    break;
/*	
	case Button2:
	    button = vHidd_Mouse_Button2;
	    break;
*/	    
	case Button3:
	    button = vHidd_Mouse_Button2;
	    break;
	
    }
    return button;
    
}

#endif

static Object *mouse_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
    
    if (LSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
    
    if (has_mouse_hidd) /* Cannot open twice */
    	return NULL; /* Should have some error code here */

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
	
	LSD(cl)->mousehidd = o;
	
    }
    return o;
}

#if 0
static VOID x11mouse_handleevent(Class *cl, Object *o, struct pHidd_X11Mouse_HandleEvent *msg)
{

    struct x11mouse_data *data = INST_DATA(cl, o);
    
    struct pHidd_Mouse_Event e;

    
    XButtonEvent *xb = &(msg->event->xbutton);
    

    e.x = xb->x;
    e.y = xb->y;
   
    
    if (msg->event->type == ButtonRelease)
    {
    	e.button = xbutton2hidd(xb);
	e.type =  vHidd_Mouse_Release;
	 
        data->mouse_callback(data->callbackdata, &e);
    }
    else if (msg->event->type == ButtonPress)
    {
    	e.button = xbutton2hidd(xb);
	e.type =  vHidd_Mouse_Press;
	
        data->mouse_callback(data->callbackdata, &e);
    }
    else if (msg->event->type == MotionNotify)
    {
    	e.button = vHidd_Mouse_NoButton;
	e.type = vHidd_Mouse_Motion;
	
        data->mouse_callback(data->callbackdata, &e);
    }
    
    return;
}
#endif

/********************  init_mouseclass()  *********************************/

#undef LSD
#define LSD(cl) lsd

#define NUM_ROOT_METHODS 1
#define NUM_X11MOUSE_METHODS 1

Class *init_mouseclass (struct linux_staticdata *lsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(mouse_new),		moRoot_New},
	{NULL, 0UL}
    };
#if 0
    struct MethodDescr mousehidd_descr[NUM_X11MOUSE_METHODS + 1] = 
    {
    	{METHODDEF(mouse_handleevent),	moHidd_X11Mouse_HandleEvent},
	{NULL, 0UL}
    };
#endif
    struct InterfaceDescr ifdescr[] = {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
#if 0	
    	{mousehidd_descr, IID_Hidd_X11Mouse, 	NUM_X11MOUSE_METHODS},
#endif
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] = {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct mouse_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_LinuxMouse },
	{TAG_DONE, 0UL}
    };

    if (MetaAttrBase) {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(NULL != cl) {
	    
	    if (ObtainAttrBases(attrbases)) {
	    
	    	cl->UserData = (APTR)lsd;
		lsd->mouseclass = cl;
		
	    	AddClass(cl);
	    } else {
	    	free_mouseclass(lsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    return cl;
}




/*************** free_mouseclass()  **********************************/
VOID free_mouseclass(struct linux_staticdata *lsd)
{

    if(NULL != lsd) {

	if (NULL != lsd->mouseclass) {
	    RemoveClass(lsd->mouseclass);
	
	    DisposeObject((Object *) lsd->mouseclass);
            lsd->mouseclass = NULL;
	}
	ReleaseAttrBases(attrbases);

    }
    return;
}



