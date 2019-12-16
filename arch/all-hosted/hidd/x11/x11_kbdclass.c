/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd handling keypresses.
    Lang: English.
*/

#include "x11_debug.h"

#define __OOP_NOATTRBASES__

#include <proto/utility.h>
#include <devices/inputevent.h>

#include "x11_keytable.h"

#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"

/****************************************************************************************/

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd);

static OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd  , &HiddKbdAB    },
    { NULL  	    , NULL  	    }
};

/****************************************************************************************/
/* Include the required x11 keytable translations                                        */
/****************************************************************************************/

/* include the "generic" keyboard translation .. */
#include "x11_keytable_default.c"

/****************************************************************************************/

/* include the default keyboard translation .. */
#ifdef KEYTABLE_NAME
#  undef KEYTABLE_NAME
#endif
#define KEYTABLE_NAME builtin_keytable

#ifndef KEYTABLE_DEFAULT
#  define KEYTABLE_DEFAULT "x11_keytable-en_gb.c"
#endif
#include KEYTABLE_DEFAULT

/****************************************************************************************/
/* Kbd Hidd methods ...                                                                 */
/****************************************************************************************/

OOP_Object * X11Kbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL    	        has_kbd_hidd = FALSE;
    struct TagItem      *tag, *tstate;
    KbdIrqCallBack_t    callback = NULL;
    APTR    	        callbackdata = NULL;

    EnterFunc(bug("X11Kbd::New()\n"));

    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) { /* Cannot open twice */
	D(bug("[X11Kbd] Attempt to create second instance\n"));
    	ReturnPtr("X11Kbd::New", OOP_Object *, NULL); /* Should have some error code here */
    }

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
    	ReturnPtr("X11Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct x11kbd_data *data = OOP_INST_DATA(cl, o);
	
	data->kbd_callback = callback;
	data->callbackdata = callbackdata;
	data->prev_keycode = 0xFFFF;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    
    ReturnPtr("X11Kbd::New", OOP_Object *, o);
}

/****************************************************************************************/

VOID X11Kbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("[X11Kbd] Dispose()\n"));

    ObtainSemaphore( &XSD(cl)->sema);
    XSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore( &XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID X11Kbd__Hidd_Kbd_X11__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_X11_HandleEvent *msg)
{
    struct x11kbd_data  *data;    
    XKeyEvent 	    	*xk;
    long   	    	 keycode;

    EnterFunc(bug("x11kbd_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);
    xk = &(msg->event->xkey);

    keycode = xkey2hidd(xk, XSD(cl));
    if (keycode == -1)
    {
        ReturnVoid("X11Kbd::HandleEvent: Unknown key!");
    }

    if (msg->event->type == KeyRelease)
    {
	keycode |= IECODE_UP_PREFIX;    	
    }

    if (keycode != data->prev_keycode)
    {
        KbdIrqData_t keydata = keycode;
    	data->kbd_callback(data->callbackdata, keydata);
	data->prev_keycode = keycode;
    }

    ReturnVoid("X11Kbd::HandleEvent");
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

/****************************************************************************************/

WORD lookup_keytable(KeySym *ks, const struct _keytable *keytable)
{
    short t;
    WORD  result = -1;
    
    for (t = 0; keytable[t].hiddcode != -1; t++)
    {
	if (*ks == keytable[t].keysym)
	{
	    D(bug("xktac: found in key table\n"));
	    result = keytable[t].hiddcode;
	    break;
	}
    }
    
    return result;
}

/****************************************************************************************/

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd)
{
    char    buffer[10];
    KeySym  ks;
    D(int     count;)
    long    result;

    D(bug("xkey2hidd\n"));
   
    if (xsd->havetable)
    {
        result = -1;
	if ((xk->keycode >= 0) && (xk->keycode < 256))
	{
	    result = xsd->keycode2rawkey[xk->keycode];
	    if (result == 255) result = -1;
	}
	
	return result;
    }
    
    LOCK_X11
    xk->state = 0;
    D(count =) XCALL(XLookupString, xk, buffer, 10, &ks, NULL);
    UNLOCK_X11
    
    D(bug("xk2h: Code %d (0x%x). Event was decoded into %d chars: %d (0x%x)\n",xk->keycode, xk->keycode, count,ks,ks));
    
    result = lookup_keytable(&ks, keytable);
    if (result == -1) result = lookup_keytable(&ks, builtin_keytable);
    
    ReturnInt ("xk2h", long, result);
    
} /* XKeyToAmigaCode */


/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

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
