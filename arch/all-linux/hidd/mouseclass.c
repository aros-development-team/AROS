/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd handling mouse events.
    Lang: English.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

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

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL,	NULL }
};


static OOP_Object *mouse_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;

    ObtainSemaphore(&LSD(cl)->sema);    
    if (LSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
    ReleaseSemaphore(&LSD(cl)->sema);
    
    if (has_mouse_hidd) /* Cannot open twice */
    	return NULL; /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct mouse_data *data = OOP_INST_DATA(cl, o);
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
    	ObtainSemaphore(&LSD(cl)->sema);	
	LSD(cl)->mousehidd = o;
    	ReleaseSemaphore(&LSD(cl)->sema);
    }
    return o;
}

static VOID mouse_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->mousehidd = NULL;
    ReleaseSemaphore(&LSD(cl)->sema);
    
    OOP_DoSuperMethod(cl, o, msg);
    
}


static VOID mouse_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Mouse_RelativeCoords:
		*msg->storage = TRUE;
		return;
    	}
    }
    
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


static VOID mouse_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_LinuxMouse_HandleEvent *msg)
{

    struct mouse_data *data = OOP_INST_DATA(cl, o);
    
    data->mouse_callback(data->callbackdata, msg->mouseEvent);
    
    return;
}

/********************  init_mouseclass()  *********************************/

#undef LSD
#define LSD(cl) lsd

#define NUM_ROOT_METHODS 3
#define NUM_X11MOUSE_METHODS 1

OOP_Class *init_linuxmouseclass (struct linux_staticdata *lsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{OOP_METHODDEF(mouse_new),			moRoot_New},
    	{OOP_METHODDEF(mouse_dispose),		moRoot_Dispose},
    	{OOP_METHODDEF(mouse_get),			moRoot_Get},
	{NULL, 0UL}
    };
    struct OOP_MethodDescr mousehidd_descr[NUM_X11MOUSE_METHODS + 1] = 
    {
    	{OOP_METHODDEF(mouse_handleevent),	moHidd_LinuxMouse_HandleEvent},
	{NULL, 0UL}
    };
    struct OOP_InterfaceDescr ifdescr[] = {
    	{root_descr,	  IID_Root, 		NUM_ROOT_METHODS},
    	{mousehidd_descr, IID_Hidd_LinuxMouse, 	NUM_X11MOUSE_METHODS},
	{NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] = {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct mouse_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_LinuxMouse },
	{TAG_DONE, 0UL}
    };

    if (MetaAttrBase) {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if(NULL != cl) {
	    
	    if (OOP_ObtainAttrBases(attrbases)) {
	    
	    	cl->UserData = (APTR)lsd;
		lsd->mouseclass = cl;
		
	    	OOP_AddClass(cl);
	    } else {
	    	free_linuxmouseclass(lsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    }
    return cl;
}




/*************** free_mouseclass()  **********************************/
VOID free_linuxmouseclass(struct linux_staticdata *lsd)
{

    if(NULL != lsd) {

	if (NULL != lsd->mouseclass) {
	    OOP_RemoveClass(lsd->mouseclass);
	
	    OOP_DisposeObject((OOP_Object *) lsd->mouseclass);
            lsd->mouseclass = NULL;
	}
	OOP_ReleaseAttrBases(attrbases);

    }
    return;
}


#undef LSD
#define LSD lsd

static int mousefd = 0;

#define MOUSE_DEVNAME "/dev/psaux"
static BOOL file_opened = FALSE;

BOOL init_linuxmouse(struct linux_staticdata *lsd)
{
    mousefd = open(MOUSE_DEVNAME, O_RDONLY);
    if (-1 == mousefd) {
	kprintf("!!! init_mous(): COULD NOT OPEND MOUSE DEVICE %s: %s\n"
	    , MOUSE_DEVNAME, strerror(errno));
    } else {
	file_opened = TRUE;
	lsd->mousefd = mousefd;
	
	return TRUE;
    }
    return FALSE;
}
VOID cleanup_linuxmouse(struct linux_staticdata *lsd)
{
    if (file_opened) {
	close(mousefd);	
    }
    return;
}

#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(OOP_OCLASS(OOP_OCLASS(o)))->UserData)

VOID HIDD_LinuxMouse_HandleEvent(OOP_Object *o, struct pHidd_Mouse_Event *mouseEvent)
{
    static OOP_MethodID mid = 0;
    struct pHidd_LinuxMouse_HandleEvent p;
    
    if (!mid)
	mid = OOP_GetMethodID(IID_Hidd_LinuxMouse, moHidd_LinuxMouse_HandleEvent);
	
    p.mID		= mid;
    p.mouseEvent	= mouseEvent;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
}
