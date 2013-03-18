/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

/*
    This is the universal serial mouse driver.
    The code is very old, and is actually one big TODO. Here is a short list
    of things to change:
    1. Drop serial HIDDs and use device API instead. This will allow to do
       interesting things, like handling serial mice on USB-Serial adapters.
       Useful at least for testing.
    2. Rewrite detection routine. Device detection should be done in startup.c,
       and one instance should be created for every mouse detected. Yes, we can
       handle several mice at once. Additionally it will provide correct
       hardware name.
    3. Replace busyloop PIT-based mouse_usleep() with timer.device-based delay.
       Will make the driver working on any arch.

    Please keep code clean from all .bss and .data sections. .rodata may exist
    as it will be connected together with .text section during linking.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <devices/inputevent.h>
#include <string.h>
#include <aros/symbolsets.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

/* Prototypes */

int test_mouse_serial(OOP_Class *, OOP_Object *);
void dispose_mouse_serial(OOP_Class *, OOP_Object *);

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

/***** Mouse::New()  ***************************************/
OOP_Object * PCMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("_Mouse::New()\n"));

    if (MSD(cl)->mousehidd) /* Cannot open twice */
        ReturnPtr("_Mouse::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct mouse_data   *data = OOP_INST_DATA(cl, o);
        struct TagItem      *tag, *tstate;

        tstate = msg->attrList;

        /* Search for all mouse attrs */

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
            {
                switch (idx)
                {
                    case aoHidd_Mouse_IrqHandler:
                        data->mouse_callback = (APTR)tag->ti_Data;
                        break;

                    case aoHidd_Mouse_IrqHandlerData:
                        data->callbackdata = (APTR)tag->ti_Data;
                        break;
                }
            }

        } /* while (tags to process) */

        /* Search for mouse installed. Check every COM port in the system */
        if (!test_mouse_serial(cl, o))
        {
            /* No mouse found. What we can do now is just Dispose() :( */
            OOP_MethodID disp_mid = msg->mID - moRoot_New + moRoot_Dispose;

            /* Do not call own Dispose */
            OOP_DoSuperMethod(cl, o, &disp_mid);
            o = NULL;
        }

        MSD(cl)->mousehidd = o;
    }

    return o;
}

VOID PCMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    MSD(cl)->mousehidd = NULL;

    dispose_mouse_serial(cl, o);

    OOP_DoSuperMethod(cl, o, msg);
}

/***** Mouse::Get()  ***************************************/
VOID PCMouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG              idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Mouse_IrqHandler:
                *msg->storage = (IPTR)data->mouse_callback;
                return;

            case aoHidd_Mouse_IrqHandlerData:
                *msg->storage = (IPTR)data->callbackdata;
                return;

            case aoHidd_Mouse_State:
                *msg->storage = 0; /* FIXME: Implement this */
                return;

            case aoHidd_Mouse_RelativeCoords:
                *msg->storage = TRUE;
                return;
        }

    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/********************  init and expunge  *********************************/

#undef OOPBase
#define OOPBase (LIBBASE->msd.oopBase)

static int PCMouse_InitAttrs(struct mousebase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd      , &LIBBASE->msd.hiddAttrBase },
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB  },
        { NULL          , NULL                       }
    };

    EnterFunc(bug("PCMouse_InitAttrs\n"));

    LIBBASE->msd.utilityBase = OpenLibrary("utility.library", 36);
    if (!LIBBASE->msd.utilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;
    
    LIBBASE->msd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);
    return TRUE;
}

/*************** free_kbdclass()  **********************************/
static int PCMouse_ExpungeAttrs(struct mousebase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd      , &LIBBASE->msd.hiddAttrBase },
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB  },
        { NULL          , NULL                       }
    };

    EnterFunc(bug("PCMouse_InitClass\n"));

    OOP_ReleaseAttrBases(attrbases);
    
    if (LIBBASE->msd.utilityBase)
        CloseLibrary(LIBBASE->msd.utilityBase);

    return TRUE;
}

ADD2INITLIB(PCMouse_InitAttrs, 0)
ADD2EXPUNGELIB(PCMouse_ExpungeAttrs, 0)
