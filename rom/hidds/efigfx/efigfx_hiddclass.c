/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Gfx class for the EFI framebuffer.
*/

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "efigfx_hidd.h"
#include "efigfx_support.h"

#include LC_LIBDEFS_FILE

AROS_INTH1(ResetHandler, struct HWData *, hwdata)
{
    AROS_INTFUNC_INIT

    ClearBuffer(hwdata);

    return FALSE;

    AROS_INTFUNC_EXIT
}

OOP_Object *EFIGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem msgNewTags[] =
    {
        { aHidd_Name            , (IPTR)"efigfx.hidd"                },
        { aHidd_HardwareName    , (IPTR)"EFI Framebuffer"            },
        { aHidd_ProducerName    , (IPTR)"UEFI"                       },
        {TAG_MORE, 0UL}
    };
    struct pRoot_New msgNew;

    EnterFunc(bug("EFIGfx::New()\n"));

    /* There is one firmware framebuffer; refuse a second driver object */
    if (XSD(cl)->efigfxhidd)
        return NULL;

    if ((msgNewTags[3].ti_Data = (IPTR)msg->attrList) == 0)
        msgNewTags[3].ti_Tag = TAG_DONE;

    msgNew.mID = msg->mID;
    msgNew.attrList = msgNewTags;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&msgNew);
    if (o)
    {
        struct EFIGfxHiddData *data = OOP_INST_DATA(cl, o);
        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd, (IPTR)o },
            { TAG_DONE,              0       }
        };

        D(bug("Got object from super\n"));

        XSD(cl)->efidisplay = OOP_NewObject(XSD(cl)->displayclass, NULL, displaytags);
        if (XSD(cl)->efidisplay)
        {
            D(bug("[EFIGfx:Driver] %s: display @ 0x%p\n", __func__, XSD(cl)->efidisplay));
            XSD(cl)->efigfxhidd = o;

            data->ResetInterrupt.is_Node.ln_Name = cl->ClassNode.ln_Name;
            data->ResetInterrupt.is_Code = (VOID_FUNC)ResetHandler;
            data->ResetInterrupt.is_Data = &XSD(cl)->data;
            AddResetCallback(&data->ResetInterrupt);
        }
        else
        {
            OOP_MethodID dispose_mid = XSD(cl)->mid_Dispose;
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
    }
    ReturnPtr("EFIGfx::New", OOP_Object *, o);
}

VOID EFIGfx__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct EFIGfxHiddData *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ResetInterrupt);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->efigfxhidd = NULL;
}

VOID EFIGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = TRUE;
                return;

            case aoHidd_Gfx_DisplayDefault:
                *msg->storage = (IPTR)XSD(cl)->efidisplay;
                return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
