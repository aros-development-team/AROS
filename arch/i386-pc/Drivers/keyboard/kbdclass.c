/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include "kbd.h"
#include "keys.h"

#define DEBUG 1
#include <aros/debug.h>

static AttrBase HiddKbdAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

struct kbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static struct _keytable
{
    ULONG  keysym;
    ULONG  hiddcode;
}
keytable[] =
{
    {K_Enter,	 	0x44 },
    {K_Right,		0x4e },
    {K_Up,		0x4c },
    {K_Left,		0x4f },
    {K_Down,		0x4d },
    {K_F12,		0x5f },	/* Pause key might be better */
    {K_KP_Enter,	0x43 },
    {K_KP_Decimal,	0x3c },
    {K_KP_Sub,		0x4a },
    {K_KP_Add,		0x5e },
    {K_KP_Multiply,	0x5d },
    {K_KP_Divide,	0x5c },
    {K_KP_0,		0x0f },
    {K_KP_1,		0x1d },
    {K_KP_2,		0x1e },
    {K_KP_3,		0x1f },
    {K_KP_4,		0x2d },
    {K_KP_5,		0x2e },
    {K_KP_6,		0x2f },
    {K_KP_7,		0x3d },
    {K_KP_8,		0x3e },
    {K_KP_9,		0x3f },
    
    {K_F1,		0x50 },
    {K_F2,		0x51 },
    {K_F3,		0x52 },
    {K_F4,		0x53 },
    {K_F5,		0x54 },
    {K_F6,		0x55 },
    {K_F7,		0x56 },
    {K_F8,		0x57 },
    {K_F9,		0x58 },
    {K_F10,		0x59 },
    {K_F11,		0x5f },		/* HELP */
    {K_F12,		0x5f },		/* HELP */
    {K_Home,		0x5f },		/* HELP */
    
    {K_A,		0x20 },
    {K_B,		0x35 },
    {K_C,		0x33 },
    {K_D,		0x22 },
    {K_E,		0x12 },
    {K_F,		0x23 },
    {K_G,		0x24 },
    {K_H,		0x25 },
    {K_I,		0x17 },
    {K_J,		0x26 },
    {K_K,		0x27 },
    {K_L,		0x28 },
    {K_M,		0x37 },
    {K_N,		0x36 },
    {K_O,		0x18 },
    {K_P,		0x19 },
    {K_Q,		0x10 },
    {K_R,		0x13 },
    {K_S,		0x21 },
    {K_T,		0x14 },
    {K_U,		0x16 },
    {K_V,		0x34 },
    {K_W,		0x11 },
    {K_X,		0x32 },
    {K_Y,		0x15 },
    {K_Z,		0x31 },
    
    {K_1,		0x01 },
    {K_2,		0x02 },
    {K_3,		0x03 },
    {K_4,		0x04 },    
    {K_5,		0x05 },
    {K_6,		0x06 },
    {K_7,		0x07 },
    {K_8,		0x08 },
    {K_9,		0x09 },
    {K_0,		0x0A },
    
    {K_Period,		0x39 },
    {K_Comma,		0x38 },
                        
    {K_Backspace,	0x41 },
    {K_Del,		0x46 },
    {K_Space,		0x40 },
    {K_LShift,		0x60 },
    {K_RShift,		0x61 },
    {K_LAlt,		0x64 },
    {K_RAlt,		0x65 },
    {K_LCtrl,		0x63 },
    {K_RCtrl,		0x63 },
    {K_LMeta,		0x66 },	/* Left Win key = LAmi */
    {K_RMeta,		0x67 },	/* Right Win key = RAmi */
    {K_Escape,		0x45 },
    {K_Tab,		0x42 },
    /* New stuff */
    {K_CapsLock,	0x62 },
    {K_LBracket,	0x1a },
    {K_RBracket,	0x1b },
    {K_Semicolon,	0x29 },
    {K_Slash,		0x3a },
    {K_BackSlash,	0x0d },
    {K_Quote,		0x2a },
    {K_BackQuote,	0x00 },
    {K_Minus,		0x0b },
    {K_Equal,		0x0c },

    {K_ResetRequest,	0x78 },
    {0, -1 }
};

/***** Kbd::New()  ***************************************/
static Object * kbd_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("Kbd::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("Kbd::New", Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    while ((tag = NextTagItem(&tstate)))
    {
	ULONG idx;
	
	D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
	    
	if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Kbd hidd tag\n"));
	    switch (idx)
	    {
		case aoHidd_Kbd_IrqHandler:
		    callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
		    break;
			
		case aoHidd_Kbd_IrqHandlerData:
		    callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", (APTR)tag->ti_Data));
		    break;
	    }
	}
	    
    } /* while (tags to process) */
    if (NULL == callback)
    	ReturnPtr("Kbd::New", Object *, NULL); /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct kbd_data *data = INST_DATA(cl, o);
	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    ReturnPtr("Kbd::New", Object *, o);
}

/***** X11Kbd::HandleEvent()  ***************************************/

static VOID kbd_handleevent(Class *cl, Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct kbd_data * data;
    
    EnterFunc(bug("kbd_handleevent()\n"));

    data = INST_DATA(cl, o);
    data->kbd_callback(data->callbackdata
		,0);
    ReturnVoid("Kbd::HandleEvent");
}

#undef XSD
#define XSD(cl) xsd

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_KBD_METHODS 1

Class *init_kbdclass (struct kbd_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(kbd_new),		moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr kbdhidd_descr[NUM_KBD_METHODS + 1] = 
    {
    	{METHODDEF(kbd_handleevent),	moHidd_Kbd_HandleEvent},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{kbdhidd_descr, IID_Hidd_Kbd,	 	NUM_KBD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct kbd_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_Kbd },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("KbdHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->kbdclass = cl;
	    
	    if (obtainattrbases(attrbases, OOPBase))
	    {
		D(bug("KbdHiddClass ok\n"));
		
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_kbdclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_kbdclass", Class *, cl);
}

/*************** free_kbdclass()  **********************************/
VOID free_kbdclass(struct kbd_staticdata *xsd)
{
    EnterFunc(bug("free_kbdclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        RemoveClass(xsd->kbdclass);
	
        if(xsd->kbdclass) DisposeObject((Object *) xsd->kbdclass);
        xsd->kbdclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);
    }
    ReturnVoid("free_kbdclass");
}
