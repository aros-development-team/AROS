/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "kbd.h"

/*****************************************************************************************

    NAME
        --background_kbdclass--

    LOCATION
        CLID_Hidd_Kbd

    NOTES
        Instances of this class are virtual devices being clients of the
        keyboard input subsystem. In order to receive keyboard events, you
        have to create an object of this class and supply a callback using
        aoHidd_Input_IrqHandler attribute. After this your callback will be
        called every time the event arrives until you dispose your object.

        Every client receives events from all keyboard devices merged into
        a single stream.

        Keyboard event handlers are specified by passing the aHidd_Input_IrqHandler attrib,
        The handler will be called every time a keyboard event occurs.  Handlers
        should be declared using 'C' calling conventions,
        e.g.:

        void KeyboardIRQ(APTR data, struct pHidd_Kbd_Event *event)

        Handler parameters are:
            data    - The handler will be called with this set to the value
                      defined using the aoHidd_Input_IrqHandlerData attribute.
            event - A pointer to a read-only event descriptor structure with the following
                      contents:
               flags - The input handlers flags
                      currently supported flags are -:
                      KBD_NOCAPSUP - The keyboard does not generate an UP event for Caps Lock.
               code - The raw key code as specified in devices/rawkeycodes.h.
                      A key 'release' event is indicated by OR'ing this value
                      with IECODE_UP_PREFIX (defined in devices/inputevent.h)

        The handler is called inside interrupts, so usual restrictions apply to it.

    SEE ALSO
        aoHidd_Input_IrqHandler, aoHidd_Input_IrqHandlerData

*****************************************************************************************/

OOP_Object *KBD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New kbdNewMsg;
    struct TagItem kbdTags[] =
    {
        {aHidd_Input_Subsystem, (IPTR)CSD(cl)->hwObj    },
        {TAG_MORE,              (IPTR)msg->attrList     },
        {TAG_DONE                                       }
    };
    kbdNewMsg.mID = msg->mID;
    kbdNewMsg.attrList = kbdTags;

    D(bug("[KbdHidd] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&kbdNewMsg);

    D(bug("[KbdHidd] %s: returning 0x%p\n", __func__, o));

    return o;
}

VOID KBD__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    D(bug("[KbdHidd] %s()\n", __func__));

    OOP_DoSuperMethod(cl, o, msg);
}
