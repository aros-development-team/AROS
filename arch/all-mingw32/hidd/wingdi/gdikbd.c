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
#include <devices/rawkeycodes.h>

#include <aros/symbolsets.h>

#define DEBUG 0
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

#define NOKEY -1    

#include "stdkeytable.h"
#include "e0keytable.h"

/****************************************************************************************/

static VOID KbdIntHandler(struct gdikbd_data *data, void *p);

static OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd  , &HiddKbdAB    },
    { NULL  	    , NULL  	    }
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
	
	KEYBOARDDATA->irq = KrnAllocIRQ();
	if (KEYBOARDDATA->irq != -1) {
	    data->interrupt = KrnAddIRQHandler(4, KbdIntHandler, data, NULL);
	    if (data->interrupt) {
	    	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
		data->callbackdata = callbackdata;
	
		ObtainSemaphore( &XSD(cl)->sema);
		XSD(cl)->kbdhidd = o;
		ReleaseSemaphore( &XSD(cl)->sema);
	    
	        ReturnPtr("GDIKbd::New", OOP_Object *, o);
	    }
	    KrnFreeIRQ(KEYBOARDDATA->irq);
	}
	OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	return NULL;
    }
}

/****************************************************************************************/

VOID GDIKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, struct pRoot_Dispose *msg)
{
    struct gdikbd_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("[GDIKbd] Dispose()\n"));

    KrnRemIRQHandler(data->interrupt);
    ObtainSemaphore( &XSD(cl)->sema);
    XSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore( &XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

static VOID KbdIntHandler(struct gdikbd_data *data, void *p)
{
    WORD *keytable;
    UBYTE numkeys;
    WORD keycode;
    UWORD eventcode = KEYBOARDDATA->EventCode;
    UWORD rawcode = KEYBOARDDATA->KeyCode;
    
    if (rawcode & 0x0100) {
        keytable = e0_keytable;
        numkeys = NUM_E0KEYS;
    } else {
        keytable = std_keytable;
        numkeys = NUM_STDKEYS;
    }
    rawcode &= 0x00FF;
    
    if (rawcode < numkeys) {
        keycode = keytable[rawcode];
        if (keycode != NOKEY) {
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
