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
#include <hidd/serial.h>

#include <devices/inputevent.h>

#include "vga.h"

#define DEBUG 0
#include <aros/debug.h>

ULONG mouse_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum);

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
    
    Object * Ser;
    Object * Unit;
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

	if (OpenLibrary("serial.hidd",0))
	{
	    if ((data->Ser = NewObject(NULL, CLID_Hidd_Serial, NULL)))
	    {
		D(bug("Got serial object = %p", data->Ser));
		if ((data->Unit = HIDD_Serial_NewUnit(data->Ser, 0)))
		{
		    int i;
		    struct TagItem stags[] = {
			    { TAG_DATALENGTH,		7 },
			    { TAG_STOP_BITS,		1 },
			    { TAG_PARITY_OFF,	 	1 },
			    { TAG_DONE,			0 }};
		    struct TagItem t2[] = {
			    { TAG_SET_MCR,		0 },
			    { TAG_DONE,			0 }};	// DTR + RTS
		    
		    D(bug("Got Unit object = %p", data->Unit));
    		    HIDD_SerialUnit_SetBaudrate(data->Unit, 1200);
		    HIDD_SerialUnit_SetParameters(data->Unit, stags);
		    t2[0].ti_Data = 1;
		    HIDD_SerialUnit_SetParameters(data->Unit, t2);
		    i = 3000000;
    		    while (i) {i--;};
		    t2[0].ti_Data = 3;
		    HIDD_SerialUnit_SetParameters(data->Unit, t2);
		    i = 3000000;
		    while (i) {i--;};

		    HIDD_SerialUnit_Init(data->Unit, mouse_InterruptHandler, NULL);
		}
	    }
	}
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

    EnterFunc(bug("mouse_handleevent()\n"));

    data = INST_DATA(cl, o);

/* Nothing done yet */

    ReturnVoid("Mouse::HandleEvent");
}

#undef XSD
#define XSD(cl) xsd

static struct vga_staticdata *vsd;

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
	    
	    vsd = xsd;
	    
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

#undef OOPBase
#define OOPBase (vsd->oopbase)

ULONG mouse_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum)
{
    static UBYTE inbuf[3];
    static UBYTE cnt = 0;

    static MethodID mid = 0;
    static struct pHidd_Gfx_SetMouseXY p;
  
    if (!mid)
    {
	mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetMouseXY);
	p.mID = mid;
    }
    
    /* Get bytes untill there is anything to get */
    while (length)
    {
	while ((cnt < 3) && length)
	{
	    inbuf[cnt] = *data++;
	    length--;
	    cnt++;
	}
	if (cnt == 3)
	{
	    cnt = 0;
	    while (!(inbuf[0] & 0x40))
	    {
		inbuf[0] = inbuf[1];
	        inbuf[1] = inbuf[2];
		if (length)
		{
		    inbuf[2] = *data++;
		    length--;
	        }
		else return;
	    }
            p.dx = (char)(((inbuf[0] & 0x03) << 6) | (inbuf[1] & 0x3f));
	    p.dy = (char)(((inbuf[0] & 0x0c) << 4) | (inbuf[2] & 0x3f));

            DoMethod(vsd->vgahidd, (Msg) &p);
	}
    }
}
