/*
    (C) 1998 AROS - The Amiga Research OS
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

static AttrBase HiddMouseAB;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL,	NULL }
};



static Object *mouse_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;

ObtainSemaphore(&LSD(cl)->sema);    
    if (LSD(cl)->mousehidd)
    	has_mouse_hidd = TRUE;
ReleaseSemaphore(&LSD(cl)->sema);
    
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
ObtainSemaphore(&LSD(cl)->sema);	
	LSD(cl)->mousehidd = o;
ReleaseSemaphore(&LSD(cl)->sema);
    }
    return o;
}

static VOID mouse_dispose(Class *cl, Object *o, Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->mousehidd = NULL;
    ReleaseSemaphore(&LSD(cl)->sema);
    
    DoSuperMethod(cl, o, msg);
    
}


static VOID mouse_handleevent(Class *cl, Object *o, struct pHidd_LinuxMouse_HandleEvent *msg)
{

    struct mouse_data *data = INST_DATA(cl, o);
    
    data->mouse_callback(data->callbackdata, msg->mouseEvent);
    
    return;
}

/********************  init_mouseclass()  *********************************/

#undef LSD
#define LSD(cl) lsd

#define NUM_ROOT_METHODS 2
#define NUM_X11MOUSE_METHODS 1

Class *init_mouseclass (struct linux_staticdata *lsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(mouse_new),			moRoot_New},
    	{METHODDEF(mouse_dispose),		moRoot_Dispose},
	{NULL, 0UL}
    };
    struct MethodDescr mousehidd_descr[NUM_X11MOUSE_METHODS + 1] = 
    {
    	{METHODDEF(mouse_handleevent),	moHidd_LinuxMouse_HandleEvent},
	{NULL, 0UL}
    };
    struct InterfaceDescr ifdescr[] = {
    	{root_descr,	  IID_Root, 		NUM_ROOT_METHODS},
    	{mousehidd_descr, IID_Hidd_LinuxMouse, 	NUM_X11MOUSE_METHODS},
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


#undef LSD
#define LSD lsd

static int mousefd = 0;

#define MOUSE_DEVNAME "/dev/psaux"
static BOOL file_opened = FALSE;

BOOL init_mouse(struct linux_staticdata *lsd)
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
VOID cleanup_mouse(struct linux_staticdata *lsd)
{
    if (file_opened) {
	close(mousefd);	
    }
    return;
}

#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(OCLASS(o)))->UserData)

VOID HIDD_LinuxMouse_HandleEvent(Object *o, struct pHidd_Mouse_Event *mouseEvent)
{
    static MethodID mid = 0;
    struct pHidd_LinuxMouse_HandleEvent p;
    
    if (!mid)
	mid = GetMethodID(IID_Hidd_LinuxMouse, moHidd_LinuxMouse_HandleEvent);
	
    p.mID		= mid;
    p.mouseEvent	= mouseEvent;
    
    DoMethod(o, (Msg)&p);
}