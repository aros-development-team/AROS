/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: X11 hidd handling keypresses.
    Lang: English.
*/


#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include "x11.h"

#define DEBUG 1
#include <aros/debug.h>


long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd);

struct x11kbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static AttrBase HiddKbdAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

static struct _keytable
{
    KeySym keysym;
    WORD   hiddcode;
}
keytable[] =
{
    {XK_Return, 	0x44 },
    {XK_Right,		0x4e },
    {XK_Up,		0x4c },
    {XK_Left,		0x4f },
    {XK_Down,		0x4d },
    {XK_Help,		0x5f },
    {XK_KP_Enter,	0x43 },
    {XK_KP_Separator,	0x3c },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Decimal,	0x3c },
    {XK_KP_Add,		0x5e },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Multiply,	0x5d },
    {XK_KP_Divide,	0x5c },
    {XK_KP_0,		0x0f },
    {XK_KP_1,		0x1d },
    {XK_KP_2,		0x1e },
    {XK_KP_3,		0x1f },
    {XK_KP_4,		0x2d },
    {XK_KP_5,		0x2e },
    {XK_KP_6,		0x2f },
    {XK_KP_7,		0x3d },
    {XK_KP_8,		0x3e },
    {XK_KP_9,		0x3f },
    
    {XK_F1,		0x50 },
    {XK_F2,		0x51 },
    {XK_F3,		0x52 },
    {XK_F4,		0x53 },
    {XK_F5,		0x54 },
    {XK_F6,		0x55 },
    {XK_F7,		0x56 },
    {XK_F8,		0x57 },
    {XK_F9,		0x58 },
    {XK_F10,		0x59 },
    {XK_F11,		0x5f },		/* HELP */
    {XK_F12,		0x5f },		/* HELP */
    {XK_Home,		0x5f },		/* HELP */
    
    {XK_A,		0x20 },
    {XK_B,		0x35 },
    {XK_C,		0x33 },
    {XK_D,		0x22 },
    {XK_E,		0x12 },
    {XK_F,		0x23 },
    {XK_G,		0x24 },
    {XK_H,		0x25 },
    {XK_I,		0x17 },
    {XK_J,		0x26 },
    {XK_K,		0x27 },
    {XK_L,		0x28 },
    {XK_M,		0x37 },
    {XK_N,		0x36 },
    {XK_O,		0x18 },
    {XK_P,		0x19 },
    {XK_Q,		0x10 },
    {XK_R,		0x13 },
    {XK_S,		0x21 },
    {XK_T,		0x14 },
    {XK_U,		0x16 },
    {XK_V,		0x34 },
    {XK_W,		0x11 },
    {XK_X,		0x32 },
    {XK_Y,		0x15 },
    {XK_Z,		0x31 },
    
    {XK_a,		0x20 },
    {XK_b,		0x35 },
    {XK_c,		0x33 },
    {XK_d,		0x22 },
    {XK_e,		0x12 },
    {XK_f,		0x23 },
    {XK_g,		0x24 },
    {XK_h,		0x25 },
    {XK_i,		0x17 },
    {XK_j,		0x26 },
    {XK_k,		0x27 },
    {XK_l,		0x28 },
    {XK_m,		0x37 },
    {XK_n,		0x36 },
    {XK_o,		0x18 },
    {XK_p,		0x19 },
    {XK_q,		0x10 },
    {XK_r,		0x13 },
    {XK_s,		0x21 },
    {XK_t,		0x14 },
    {XK_u,		0x16 },
    {XK_v,		0x34 },
    {XK_w,		0x11 },
    {XK_x,		0x32 },
    {XK_y,		0x15 },
    {XK_z,		0x31 },

    {XK_1,		0x01 },
    {XK_2,		0x02 },
    {XK_3,		0x03 },
    {XK_4,		0x04 },    
    {XK_5,		0x05 },
    {XK_6,		0x06 },
    {XK_7,		0x07 },
    {XK_8,		0x08 },
    {XK_9,		0x09 },
    {XK_0,		0x0A },
    
    {XK_period,		0x39 },
    {XK_comma,		0x38 },
                        
    {XK_BackSpace,	0x41 },
    {XK_Delete,		0x46 },
    {XK_space,		0x40 },
    {XK_Shift_L,	0x60 },
    {XK_Shift_R,	0x61 },
    {XK_Alt_L,		0x64 }, /* doesn´t work?? */
    {XK_Alt_R,		0x65 }, /* doesn´t work?? */
    {XK_Meta_L,		0x64 }, /* LALT */
    {XK_Meta_R,		0x65 }, /* RALT */
    {XK_Control_L,	0x63 },
    {XK_Control_R,	0x63 }, /* doesn´t work?? */
    {XK_Multi_key,	0x63 }, /* right control */
    {XK_Super_L,	0x66 },	/* Left Win key = LCOMMAND */
    {XK_Super_R,	0x67 },	/* Right Win key = RCOMMAND */
    {XK_Escape,		0x45 },
    {XK_Tab,		0x42 },
    {0, -1 }
};

/***** X11Kbd::New()  ***************************************/
static Object * x11kbd_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("X11Kbd::New()\n"));
    
    ObtainSemaphoreShared( &XSD(cl)->sema);
    
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;
	
    ReleaseSemaphore( &XSD(cl)->sema);
    
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("X11Kbd::New", Object *, NULL); /* Should have some error code here */

    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    tstate = msg->attrList;
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
    	ReturnPtr("X11Kbd::New", Object *, NULL); /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct x11kbd_data *data = INST_DATA(cl, o);
	data->kbd_callback = (VOID (*)())callback;
	data->callbackdata = callbackdata;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    ReturnPtr("X11Kbd::New", Object *, o);
}

/***** X11Kbd::HandleEvent()  ***************************************/

static VOID x11kbd_handleevent(Class *cl, Object *o, struct pHidd_X11Kbd_HandleEvent *msg)
{
    struct x11kbd_data * data;
    
    XKeyEvent *xk;

    EnterFunc(bug("x11kbd_handleevent()\n"));
    xk = &(msg->event->xkey);
    data = INST_DATA(cl, o);
    if (msg->event->type == KeyPress)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, XSD(cl)));
		
    }
    else if (msg->event->type == KeyRelease)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, XSD(cl)) | IECODE_UP_PREFIX);
    }

    
    ReturnVoid("X11Kbd::HandleEvent");
}



#undef XSD
#define XSD(cl) xsd

#if 0
long StateToQualifier (unsigned long state)
{
    long result;

    result = 0;

    if (state & ShiftMask)
	result |= SHIFT;

    if (state & ControlMask)
	result |= CTRL;

    if (state & LockMask)
	result |= CAPS;

    if (state & Mod2Mask) /* Right Alt */
	result |= LALT;

    if (state & 0x2000) /* Mode switch */
	result |= RALT;

    if (state & Mod1Mask) /* Left Alt */
	result |= AMIGAKEYS;

    if (state & Button1Mask)
	result |= IEQUALIFIER_LEFTBUTTON;

    if (state & Button2Mask)
	result |= IEQUALIFIER_RBUTTON;

    if (state & Button3Mask)
	result |= IEQUALIFIER_MIDBUTTON;

    return (result);
} /* StateToQualifier */

#endif

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd)
{
    char buffer[10];
    KeySym ks;
    int count;
    long result;
    short t;

 
    D(bug("xkey2hidd\n"));
   
LX11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
UX11
    D(bug("xk2h: Code %d (0x%x). Event was decoded into %d chars: %d (0x%x)\n",xk->keycode, xk->keycode, count,ks,ks));
    
    for (t=0; keytable[t].hiddcode != -1; t++)
    {
	if (ks == keytable[t].keysym)
	{
	    D(bug("xktac: found in key table\n"));
	    result = keytable[t].hiddcode;
	    
	    ReturnInt ("xk2h", long, result);
	}
    }
    
    D(bug("xk2h: Passing X keycode\n", xk->keycode & 0xffff));

    result = xk->keycode & 0xffff;

    ReturnInt ("xk2h", long, result);
} /* XKeyToAmigaCode */


/********************  init_kbdclass()  *********************************/


#define NUM_ROOT_METHODS 1
#define NUM_X11KBD_METHODS 1

Class *init_kbdclass (struct x11_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(x11kbd_new),		moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr kbdhidd_descr[NUM_X11KBD_METHODS + 1] = 
    {
    	{METHODDEF(x11kbd_handleevent),	moHidd_X11Kbd_HandleEvent},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{kbdhidd_descr, IID_Hidd_X11Kbd, 	NUM_X11KBD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct x11kbd_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_X11Kbd },
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
VOID free_kbdclass(struct x11_staticdata *xsd)
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
