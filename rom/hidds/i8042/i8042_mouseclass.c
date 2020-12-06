/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The PS/2 mouse driver class.
    Lang: English.
*/

/*
    Please keep code clean from all .bss and .data sections. .rodata may exist
    as it will be connected together with .text section during linking. In near
    future this driver will be compiled as elf executable (instead of object)
    with -fPIC flag.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <devices/inputevent.h>

#include <string.h>

#include "i8042_mouse.h"

/* defines for buttonstate */

/***** Mouse::New()  ***************************************/
OOP_Object * i8042Mouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *mouse;
    D(bug("[i8042:Mouse] %s()\n", __func__));

    if (XSD(cl)->mousehidd)
    {
        /* Cannot open twice */
        D(bug("[i8042:Mouse] %s: already instantiated!\n", __func__));
        return NULL;
    }

    mouse = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (mouse)
    {
        struct mouse_data   *data = OOP_INST_DATA(cl, mouse);
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
        NewCreateTask(TASKTAG_PC,           PS2Mouse_InitTask,
                         TASKTAG_NAME,      "i8042 Mouse init",
                         TASKTAG_STACKSIZE, 1024,
                         TASKTAG_PRI,       100,
                         TASKTAG_ARG1,      cl,
                         TASKTAG_ARG2,      mouse,
                         TASKTAG_USERDATA,  FindTask(NULL),
                         TAG_DONE);
        Wait(SIGF_SINGLE);
        if (!data->irq)
        {
            D(bug("[i8042:Kbd] %s: controller initialization failed\n", __func__));
            /*
             * No mouse found. What we can do now is just Dispose() :(
             * Note that we use OOP_DoSuperMethod() in order not to call
             * our own dispose_mouse_ps2().
             */
            OOP_MethodID disp_mid = msg->mID - moRoot_New + moRoot_Dispose;

            OOP_DoSuperMethod(cl, mouse, &disp_mid);
            mouse = NULL;
        }
        XSD(cl)->mousehidd = mouse;
    }

    return mouse;
}

VOID i8042Mouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);

    XSD(cl)->mousehidd = NULL;
    KrnRemIRQHandler(data->irq);
    OOP_DoSuperMethod(cl, o, msg);
}

/***** Mouse::Get()  ***************************************/

static const char *mice_str[] =
{
    "Generic PS/2 mouse",
    "IntelliMouse(tm)-compatible PS/2 mouse"
};

static const char *driver_name = "i8042.hidd";

VOID i8042Mouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Mouse_State:
            ps2mouse_getstate(cl, o, (struct pHidd_Mouse_Event *)msg->storage);
            return;

        case aoHidd_Mouse_RelativeCoords:
            *msg->storage = TRUE;
            return;
        }
    }
    else if (IS_IF_ATTR(msg->attrID, idx, HiddAttrBase, num_Hidd_Attrs))
    {
        /*
         * Since we have some knowledge of mouse type, we can
         * reflect this in hardware description.
         * A well-designed driver would first probe for hardware,
         * then create its objects. This code is very old, and it has
         * long story. Refactoring inner working of PS/2 driver can break
         * something and reveal some controller quirks, so we leave
         * everything as it is. First installing interrupt handler, then
         * mouse detection. It may be important.
         * Of course i could modify hiddclass to have setable attributes,
         * but this is not good and can be easily abused by bad code. So here
         * we do another thing, and just overload the respective attribute.
         */
        switch (idx)
        {
        case aoHidd_Name:
            *msg->storage = (IPTR)driver_name;
            return;

        case aoHidd_HardwareName:
            *msg->storage = (IPTR)mice_str[data->mouse_protocol];
            return;
        }
    }    

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
