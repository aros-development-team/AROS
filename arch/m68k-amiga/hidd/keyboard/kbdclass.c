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
#include <proto/timer.h>

#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>

#include "kbd.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>


#undef HiddKbdAB
#define HiddKbdAB   (XSD(cl)->hiddKbdAB)

// CIA-A level 2 serial interrupt handler

static AROS_INTH1(keyboard_interrupt, struct kbd_data *, kbddata)
{ 
    AROS_INTFUNC_INIT

    volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
    struct Library *TimerBase = kbddata->TimerBase;
    struct EClockVal eclock1, eclock2;
    UBYTE keycode;

    if (kbddata->resetstate == 2) {
        // do nothing, we'll reset automatically in 10seconds
        return 0;
    }

    keycode = ciaa->ciasdr;

    ciaa->ciacra |= 0x40;
    ReadEClock(&eclock1);

    keycode = ~((keycode >> 1) | (keycode << 7));

    if (keycode == 0x78) { // reset warning
        kbddata->resetstate++;
        if (kbddata->resetstate == 2) {
            kbddata->kbd_callback(kbddata->callbackdata, keycode);
            // second reset warning, no handshake = starts 10s delay before forced reset
            return 0;
        }
        // first reset warning, handle it normally
    } else {
        kbddata->kbd_callback(kbddata->callbackdata, keycode);
    }
    /* "release" UAE mouse wheel up/down key codes */
    if (keycode == 0x7a || keycode == 0x7b)
	kbddata->kbd_callback(kbddata->callbackdata, 0x80 | keycode);

    // busy wait until handshake pulse has been long enough
    for (;;) {
    	ReadEClock(&eclock2);
    	if ((LONG)(eclock2.ev_lo - eclock1.ev_lo) >= 80)
    	    break;
    }

    ciaa->ciacra &= ~0x40; // end handshake

    return 0;

    AROS_INTFUNC_EXIT
}

OOP_Object * AmigaKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    BOOL has_kbd_hidd = FALSE;
    struct Library *UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    
    EnterFunc(bug("Kbd::New()\n"));

    if (!UtilityBase)
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    ObtainSemaphoreShared( &XSD(cl)->sema);

    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) { /* Cannot open twice */
        CloseLibrary(UtilityBase);
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */
    }

    tstate = msg->attrList;
    D(bug("Kbd: tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));
    
    while ((tag = NextTagItem(&tstate)))
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
    CloseLibrary(UtilityBase);

    if (NULL == callback)
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	struct Interrupt *inter = &XSD(cl)->kbint;
        struct kbd_data *data = OOP_INST_DATA(cl, o);
        
        data->kbd_callback   = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata   = callbackdata;
	
	XSD(cl)->timerio = (struct timerequest*)AllocMem(sizeof(struct timerequest), MEMF_CLEAR | MEMF_PUBLIC);
	if (OpenDevice("timer.device", UNIT_ECLOCK, (struct IORequest*)XSD(cl)->timerio, 0))
	    Alert(AT_DeadEnd | AG_OpenDev | AN_Unknown);
	XSD(cl)->TimerBase = data->TimerBase = (struct Library*)XSD(cl)->timerio->tr_node.io_Device;

	if (!(XSD(cl)->ciares = OpenResource("ciaa.resource")))
	    Alert(AT_DeadEnd | AG_OpenRes | AN_Unknown);

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
    if (XSD(cl)->TimerBase)
    	CloseDevice((struct IORequest*)XSD(cl)->timerio);
    ReleaseSemaphore(&XSD(cl)->sema);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

static int AmigaKbd_InitAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->ksd.cs_OOPBase;
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
    struct Library *OOPBase = LIBBASE->ksd.cs_OOPBase;
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
