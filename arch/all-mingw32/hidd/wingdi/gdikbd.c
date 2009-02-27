/*
    Copyright  1995-2009, The AROS Development Team. All rights reserved.
    $Id: x11kbd.c 26918 2007-10-02 02:55:49Z rob $

    Desc: GDI hidd handling keypresses.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

/****************************************************************************************/

static UBYTE keycode2rawkey[256];

static VOID KbdIntHandler(struct gdikbd_data *data, void *p);
/* long xkey2hidd (XKeyEvent *xk, struct gdi_staticdata *xsd);*/

static OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd  , &HiddKbdAB    },
    { NULL  	    , NULL  	    }
};

/****************************************************************************************/

static WORD keytable[] = {
    0x41,	/* Backspace */
    0x42,	/* TAB */
    -1, -1,	/* Reserved */
    -1,		/* Clear */
    0x44,	/* Return */
    -1, -1,	/* Undefined */
    0x60,	/* Shift (mapped to left Shift) */
    0x63,	/* Control */
    0x64,	/* Alt (mapped to right Alt) */
    0x6E,	/* Pause */
    0x62,	/* Caps lock */
    -1,		/* IME Kana */
    -1,		/* Undefined */
    -1,		/* IME Junja */
    -1,		/* IME Final */
    -1,		/* IME Kanji */
    -1,		/* Undefined */
    0x45,	/* ESC */
    -1,		/* IME Convert */
    -1,		/* IME Nonconvert */
    -1,		/* IME Accept */
    -1,		/* IME Mode change */
    0x40,	/* Space */
    0x48,	/* Page up */
    0x49,	/* Page down */
    0x71,	/* End */
    0x70,	/* Home */
    0x4F,	/* Left */
    0x4C,	/* Up */
    0x4E,	/* Right */
    0x4D,	/* Down */
    -1,		/* Select */
    -1,		/* Print */
    -1,		/* Execute */
    -1,		/* Print screen */
    0x47,	/* Insert */
    0x46,	/* Delete */
    0x5F,	/* Help */
    0x0A,	/* 0 */
    0x01,   	/* 1 */
    0x02,	/* 2 */
    0x03,	/* 3 */
    0x04,	/* 4 */
    0x05,	/* 5 */
    0x06,	/* 6 */
    0x07,	/* 7 */
    0x08,	/* 8 */
    0x09,	/* 9 */
    -1, -1, -1, -1, -1, -1, -1, /* Undefined */
    0x20,	/* A */
    0x35,	/* B */
    0x33,	/* C */
    0x22,	/* D */
    0x12,	/* E */
    0x23,	/* F */
    0x24,	/* G */
    0x25,	/* H */
    0x17,	/* I */
    0x26,	/* J */
    0x27,	/* K */
    0x28,	/* L */
    0x37,	/* M */
    0x36,	/* N */
    0x18,	/* O */
    0x19,	/* P */
    0x10,	/* Q */
    0x13,	/* R */
    0x21,	/* S */
    0x14,	/* T */
    0x16,	/* U */
    0x34,	/* V */
    0x11,	/* W */
    0x32,	/* X */
    0x15,	/* Y */
    0x31,	/* Z */
    0x66,	/* Left Win (mapped to Left Amiga) */
    0x67,	/* Right Win (mapped to Right Amiga) */
    0x5F,	/* Menu (mapped to Help) */
    -1,		/* Reserved */
    -1,		/* Sleep */
    0x0f,	/* Numeric pad 0 */
    0x1d,	/* Numeric pad 1 */
    0x1e,	/* Numeric pad 2 */
    0x1f,	/* Numeric pad 3 */
    0x2d,	/* Numeric pad 4 */
    0x2e,	/* Numeric pad 5 */
    0x2f,	/* Numeric pad 6 */
    0x3d,	/* Numeric pad 7 */
    0x3e,	/* Numeric pad 8 */
    0x3f,    	/* Numeric pad 9 */
    0x5D,	/* Numeric pad multiply */
    0x5E,	/* Numeric pad add */
    0x3C,	/* Numeric pad separator (mapped to Decimal) */
    0x4A,	/* Numeric pad subtract */
    0x3C,	/* Numeric pad decimal */
    0x5C,	/* Muneric pad divide */
    0x50,	/* F1 */
    0x51,	/* F2 */
    0x52,	/* F3 */
    0x53,	/* F4 */
    0x54,	/* F5 */
    0x55,	/* F6 */
    0x56,	/* F7 */
    0x57,	/* F8 */
    0x58,	/* F9 */
    0x59,	/* F10 */
    0x4B,	/* F11 */
    0x6f,	/* F12 */
    -1,		/* F13 */
    -1,		/* F14 */
    -1,		/* F15 */
    -1,		/* F16 */
    -1,		/* F17 */
    -1,		/* F18 */
    -1,		/* F19 */
    -1,		/* F20 */
    -1,		/* F21 */
    -1,		/* F22 */
    -1,		/* F23 */
    -1,		/* F24 */
    -1, -1, -1, -1, -1, -1, -1, -1, /* Unassigned */
    0x5A,	/* Num Lock */
    -1,		/* Scroll Lock */
    -1, -1, -1, -1, -1, /* OEM Specific */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, /* Unassigned */
    0x60,	/* Left Shift */
    0x61,	/* Right Shift */
    0x63,	/* Left Control */
    0x63,	/* Right Control */
    0x64,	/* Left Alt */
    0x65,	/* Right Alt */
    -1,		/* Browser Back */
    -1,		/* Browser Forward */
    -1,		/* Browser Refresh */
    -1,		/* Browser Stop */
    -1,		/* Browser Search */
    -1,		/* Browser Favorites */
    -1,		/* Browser Home */
    -1,		/* Volume Mute */
    -1,		/* Volume Down */
    -1,		/* Volume Up */
    -1,		/* Media Next Track */
    -1,		/* Media Previous Track */
    -1,		/* Media Stop */
    -1,		/* Media Play/Pause */
    -1,		/* Launch Mail */
    -1,		/* Launch Media Select */
    -1,		/* Launch App1 */
    -1,		/* Launch App2 */
    -1, -1,	/* Reserved */
    0x29,	/* ; */
    0x0C,	/* = */
    0x38,	/* , */
    0x0B,	/* - */
    0x39,	/* . */
    0x3A,	/* / */
    0x00,	/* ~ */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* Reserved */
    -1, -1, -1, /* Unassigned */
    0x1A,	/* [ */
    0x2B,	/* \ */
    0x1B,	/* ] */
    0x2A	/* ' */
};

/****************************************************************************************/

OOP_Object * GDIKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL    	    has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    
    EnterFunc(bug("GDIKbd::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("GDIKbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
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
    	ReturnPtr("GDIKbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct gdikbd_data *data = OOP_INST_DATA(cl, o);
	
	data->interrupt = KrnAddIRQHandler(4, KbdIntHandler, data, NULL);
	if (!data->interrupt) {
    	    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    
    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            return NULL;
        }
	
	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    
    ReturnPtr("GDIKbd::New", OOP_Object *, o);
}

/****************************************************************************************/

static VOID KbdIntHandler(struct gdikbd_data *data, void *p)
{
    WORD keycode;
    UWORD eventcode = KEYBOARDDATA->EventCode;
    IPTR rawcode = KEYBOARDDATA->KeyCode - VK_BACK;
    
    if (rawcode < (sizeof(keytable) / sizeof(WORD))) {
        keycode = keytable[rawcode];
        if (keycode != -1) {
    	    if (eventcode == WM_KEYUP)
    	        keycode |= IECODE_UP_PREFIX;
    	    data->kbd_callback(data->callbackdata, keycode);
	}
    }
}

/****************************************************************************************/

static int kbd_init(LIBBASETYPEPTR LIBBASE) 
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int kbd_expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(kbd_init, 0);
ADD2EXPUNGELIB(kbd_expunge, 0);

/****************************************************************************************/
