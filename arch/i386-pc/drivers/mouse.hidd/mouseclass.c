/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

/*
    This is the native hidd maintaining PS2 mouse.

    Please keep code clean from all .bss and .data sections. .rodata may exist
    as it will be connected together with .text section during linking. In near
    future this driver will be compiled as elf executable (instead of object)
    with -fPIC flag.
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

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/* TODO: Remove all .data from file ! */

#ifdef HiddMouseAB
#undef HiddMouseAB
#endif
#define HiddMouseAB     (MSD(cl)->hiddMouseAB)

/* Prototypes */

int test_mouse_usb(OOP_Class *, OOP_Object *);
int test_mouse_ps2(OOP_Class *, OOP_Object *);
int test_mouse_serial(OOP_Class *, OOP_Object *);
void dispose_mouse_usb(OOP_Class *, OOP_Object *);
void dispose_mouse_ps2(OOP_Class *, OOP_Object *);
void dispose_mouse_serial(OOP_Class *, OOP_Object *);
void getps2State(OOP_Class *, OOP_Object *, struct pHidd_Mouse_Event *);

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

        /* Search for PS/2 mouse */
        if (!test_mouse_ps2(cl, o))
        {
            /* No mouse found. What we can do now is just Dispose() :( */
            OOP_MethodID disp_mid = msg->mID - moRoot_New + moRoot_Dispose;

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

    dispose_mouse_ps2(cl, o);
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
        case aoHidd_Mouse_State:
            getps2State(cl, o, (struct pHidd_Mouse_Event *)msg->storage);
            return;

        case aoHidd_Mouse_RelativeCoords:
            *msg->storage = TRUE;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/********************  init and expunge  *********************************/

static int PCMouse_InitAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd      , &LIBBASE->msd.hiddAttrBase },
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB  },
        { NULL          , NULL                       }
    };

    EnterFunc(bug("PCMouse_InitAttrs\n"));

    ReturnInt("PCMouse_InitAttr", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*************** free_kbdclass()  **********************************/
static int PCMouse_ExpungeAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd      , &LIBBASE->msd.hiddAttrBase },
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB  },
        { NULL          , NULL                       }
    };

    EnterFunc(bug("PCMouse_InitClass\n"));

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

ADD2INITLIB(PCMouse_InitAttrs, 0)
ADD2EXPUNGELIB(PCMouse_ExpungeAttrs, 0)
