/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

/****************************************************************************************/

#define AROS_ALMOST_COMPATIBLE
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <aros/system.h>
#include <aros/symbolsets.h>

#include <hardware/custom.h>
#include <hardware/cia.h>
#include <proto/cia.h>

#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include "kbd.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>


#undef HiddKbdAB
#define HiddKbdAB   (XSD(cl)->hiddKbdAB)

// CIA-A level 2 serial interrupt handler

static AROS_UFH4(ULONG, keyboard_interrupt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct kbd_data *kbddata = (struct kbd_data*)data;
    volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	
    UBYTE keycode = ciaa->ciasdr;

    ciaa->ciacra = 0x48;
    ciaa->ciatalo = 100;
    ciaa->ciatahi = 0;

    keycode = ~((keycode >> 1) | (keycode << 7));
    kbddata->kbd_callback(kbddata->callbackdata, keycode);
    /* "release" UAE mouse wheel up/down key codes */
    if (keycode == 0x7a || keycode == 0x7b)
	kbddata->kbd_callback(kbddata->callbackdata, 0x80 | keycode);

    // timer still not finished? busy wait
    while (ciaa->ciacra & 1);

    ciaa->ciacra = 0x08; // back to input mode, end handshake
	
    return 0;
	
    AROS_USERFUNC_EXIT
}

OOP_Object * AmigaKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    BOOL has_kbd_hidd = FALSE;
    
    EnterFunc(bug("Kbd::New()\n"));

    ObtainSemaphoreShared( &XSD(cl)->sema);

    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("Kbd: tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        ULONG idx;
	
        D(bug("Kbd: Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
	    
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
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	struct Interrupt *inter = &XSD(cl)->kbint;
        struct kbd_data *data = OOP_INST_DATA(cl, o);
        
        data->kbd_callback   = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata   = callbackdata;
	
	if (!(XSD(cl)->ciares = OpenResource("ciaa.resource")))
	    Alert(AT_DeadEnd | AG_OpenRes | AN_Unknown);
	
	ciaa->ciacra = 0x08; // oneshot

	inter = &XSD(cl)->kbint;
	inter->is_Node.ln_Pri = 0;
	inter->is_Node.ln_Type = NT_INTERRUPT;
	inter->is_Node.ln_Name = "kbr";
	inter->is_Code = (APTR)keyboard_interrupt;
	inter->is_Data = data;
	
	if (AddICRVector(XSD(cl)->ciares, 3, inter))
	    Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);	
    }
 
    ReturnPtr("Kbd::New", OOP_Object *, o);
}

VOID AmigaKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&XSD(cl)->sema);
    if (XSD(cl)->ciares)
    	RemICRVector(XSD(cl)->ciares, 3, &XSD(cl)->kbint);
    ReleaseSemaphore(&XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID AmigaKbd__Hidd_Kbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct kbd_data * data;

    EnterFunc(bug("kbd_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);
    
    ReturnVoid("Kbd::HandleEvent");
}


/****************************************************************************************/

static int AmigaKbd_InitAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd	, &LIBBASE->ksd.hiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    
    ReturnInt("AmigaKbd_InitAttrs", ULONG, OOP_ObtainAttrBases(attrbases));
}

/****************************************************************************************/

static int AmigaKbd_ExpungeAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd	, &LIBBASE->ksd.hiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    
    EnterFunc(bug("AmigaKbd_ExpungeAttrs\n"));

    OOP_ReleaseAttrBases(attrbases);
    
    ReturnInt("AmigaKbd_ExpungeAttrs", int, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(AmigaKbd_InitAttrs, 0)
ADD2EXPUNGELIB(AmigaKbd_ExpungeAttrs, 0)
