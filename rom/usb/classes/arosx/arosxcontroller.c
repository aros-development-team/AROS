/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: controller.hidd devices for the XInput gamepad class.

    controller.hidd lives on disk. This class is ROM resident, so pads seen
    before DOS is available are not bound until then (arosxCtrlDeferBinding);
    the class scan that follows DOS startup binds them. When DOS is up and
    controller.hidd still cannot be opened the binding proceeds without a
    controller device, i.e. with the class' own gamepad structure only.
*/

#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include "debug.h"
#include "arosx.class.h"
#include "arosxcontroller.h"

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>

/* attribute and method bases live in the class base */
#undef HiddAttrBase
#undef HiddControllerAB
#undef HWAttrBase
#undef HWBase
#undef HiddControllerBase
#undef HWControllerBase
#define HiddAttrBase        (arosxb->CtrlAB[0])
#define HiddControllerAB    (arosxb->CtrlAB[1])
#define HWAttrBase          (arosxb->CtrlAB[2])
#define HWBase              (arosxb->CtrlMB[0])
#define HiddControllerBase  (arosxb->CtrlMB[1])
#define HWControllerBase    (arosxb->CtrlMB[2])

struct AROSXCtrlDev
{
    struct AROSXClassController *Binding;
};

/*****************************************************************************************
    the device class (a subclass of CLID_Hidd_Controller created by hand)
*****************************************************************************************/

static OOP_Object *XCtrl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct AROSXClassBase *arosxb = (struct AROSXClassBase *)cl->UserData;
    struct AROSXClassController *arosxc = (struct AROSXClassController *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    if (!arosxc)
        return NULL;
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct AROSXCtrlDev *data = OOP_INST_DATA(cl, o);
        data->Binding = arosxc;
    }
    return o;
}

static VOID XCtrl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

/* Hand an output request to the pad's task, which owns the pipes */
static void arosxCtrlSignalTask(struct AROSXClassController *arosxc)
{
    if (arosxc->Task && arosxc->TaskMsgPort)
        Signal(arosxc->Task, 1L << arosxc->TaskMsgPort->mp_SigBit);
}

static BOOL XCtrl__Hidd_Controller__SetRumble(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetRumble *msg)
{
    struct AROSXCtrlDev *data = OOP_INST_DATA(cl, o);
    struct AROSXClassController *arosxc = data->Binding;
    struct AROSXCtrl *ctrl;

    if (!msg->params || !arosxc)
        return FALSE;

    Forbid();
    ctrl = arosxc->Ctrl;
    if (ctrl)
    {
        ctrl->RumbleLow = msg->params->low;
        ctrl->RumbleHigh = msg->params->high;
        ctrl->RumblePending = TRUE;
        arosxCtrlSignalTask(arosxc);
    }
    Permit();

    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

static BOOL XCtrl__Hidd_Controller__SetPlayerIndex(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetPlayerIndex *msg)
{
    struct AROSXCtrlDev *data = OOP_INST_DATA(cl, o);
    struct AROSXClassController *arosxc = data->Binding;
    struct AROSXCtrl *ctrl;

    if (!arosxc)
        return FALSE;

    Forbid();
    ctrl = arosxc->Ctrl;
    if (ctrl)
    {
        ctrl->Player = msg->index;
        ctrl->LEDPending = TRUE;
        arosxCtrlSignalTask(arosxc);
    }
    Permit();

    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*****************************************************************************************
    class level setup
*****************************************************************************************/

BOOL arosxCtrlInit(struct AROSXClassBase *arosxb)
{
    BOOL ok = FALSE;

    ObtainSemaphore(&arosxb->CtrlLock);
    if (arosxb->CtrlClass)
    {
        ReleaseSemaphore(&arosxb->CtrlLock);
        return TRUE;
    }

    if (!arosxb->CtrlHiddBase)
        arosxb->CtrlHiddBase = OpenLibrary("controller.hidd", 0);
    if (arosxb->CtrlHiddBase)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,            (OOP_AttrBase *)&arosxb->CtrlAB[0] },
            { IID_Hidd_Controller, (OOP_AttrBase *)&arosxb->CtrlAB[1] },
            { IID_HW,              (OOP_AttrBase *)&arosxb->CtrlAB[2] },
            { NULL,                NULL                                 }
        };

        if (OOP_ObtainAttrBases(attrbases))
        {
            OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

            arosxb->CtrlMB[0] = OOP_GetMethodID(IID_HW, 0);
            arosxb->CtrlMB[1] = OOP_GetMethodID(IID_Hidd_Controller, 0);
            arosxb->CtrlMB[2] = OOP_GetMethodID(IID_HW_Controller, 0);
            arosxb->CtrlHW = OOP_NewObject(NULL, CLID_HW_Controller, NULL);

            if (MetaAttrBase && arosxb->CtrlHW)
            {
                struct OOP_MethodDescr root_descr[] =
                {
                    { (OOP_MethodFunc)XCtrl__Root__New,     moRoot_New     },
                    { (OOP_MethodFunc)XCtrl__Root__Dispose, moRoot_Dispose },
                    { NULL, 0 }
                };
                struct OOP_MethodDescr ctrl_descr[] =
                {
                    { (OOP_MethodFunc)XCtrl__Hidd_Controller__SetRumble,      moHidd_Controller_SetRumble      },
                    { (OOP_MethodFunc)XCtrl__Hidd_Controller__SetPlayerIndex, moHidd_Controller_SetPlayerIndex },
                    { NULL, 0 }
                };
                struct OOP_InterfaceDescr ifdescr[] =
                {
                    { root_descr, IID_Root,            2 },
                    { ctrl_descr, IID_Hidd_Controller, 2 },
                    { NULL, NULL, 0 }
                };
                struct TagItem tags[] =
                {
                    { aMeta_SuperID,        (IPTR)CLID_Hidd_Controller    },
                    { aMeta_InterfaceDescr, (IPTR)ifdescr                 },
                    { aMeta_InstSize,       sizeof(struct AROSXCtrlDev)   },
                    { TAG_DONE,             0                             }
                };
                OOP_Class *cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

                if (cl)
                {
                    cl->UserData = arosxb;
                    arosxb->CtrlClass = cl;
                    ok = TRUE;
                }
            }
            if (MetaAttrBase)
                OOP_ReleaseAttrBase(IID_Meta);
            if (!ok)
                OOP_ReleaseAttrBases(attrbases);
        }
        if (!ok)
        {
            CloseLibrary(arosxb->CtrlHiddBase);
            arosxb->CtrlHiddBase = NULL;
        }
    }
    ReleaseSemaphore(&arosxb->CtrlLock);
    return ok;
}

void arosxCtrlExit(struct AROSXClassBase *arosxb)
{
    ObtainSemaphore(&arosxb->CtrlLock);
    if (arosxb->CtrlClass)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,            (OOP_AttrBase *)&arosxb->CtrlAB[0] },
            { IID_Hidd_Controller, (OOP_AttrBase *)&arosxb->CtrlAB[1] },
            { IID_HW,              (OOP_AttrBase *)&arosxb->CtrlAB[2] },
            { NULL,                NULL                                 }
        };

        OOP_DisposeObject((OOP_Object *)arosxb->CtrlClass);
        arosxb->CtrlClass = NULL;
        OOP_ReleaseAttrBases(attrbases);
    }
    if (arosxb->CtrlHiddBase)
    {
        CloseLibrary(arosxb->CtrlHiddBase);
        arosxb->CtrlHiddBase = NULL;
    }
    ReleaseSemaphore(&arosxb->CtrlLock);
}

/*
 * Every interface this class binds is a game controller, so before DOS is
 * available (controller.hidd out of reach) no binding is made at all.
 */
BOOL arosxCtrlDeferBinding(struct AROSXClassBase *arosxb)
{
    if (arosxCtrlInit(arosxb))
        return FALSE;
    return arosxb->DOSAvailable ? FALSE : TRUE;
}

/* DOS is up: ask every bound pad without a controller device to (re)try, in its own task */
void arosxCtrlDOSAvailable(struct AROSXClassBase *arosxb)
{
    struct AROSXClassController *pads[4];
    ULONG i;

    arosxb->DOSAvailable = TRUE;
    pads[0] = arosxb->arosxc_0;
    pads[1] = arosxb->arosxc_1;
    pads[2] = arosxb->arosxc_2;
    pads[3] = arosxb->arosxc_3;

    Forbid();
    for (i = 0; i < 4; i++)
    {
        struct AROSXClassController *arosxc = pads[i];

        if (arosxc && arosxc->status.connected && !arosxc->Ctrl && arosxc->Task && arosxc->TaskMsgPort)
        {
            arosxc->CtrlWanted = TRUE;
            Signal(arosxc->Task, 1L << arosxc->TaskMsgPort->mp_SigBit);
        }
    }
    Permit();
}

/*****************************************************************************************
    attach / detach (pad task context)
*****************************************************************************************/

void arosxCtrlAttach(struct AROSXClassController *arosxc)
{
    struct AROSXClassBase *arosxb = arosxc->arosxb;
    struct Library *ps = arosxc->Base;
    struct AROSXCtrl *ctrl;
    IPTR vendid = 0, prodid = 0, version = 0, manufacturer = 0, product = 0, serial = 0, ifidstr = 0;

    if (arosxc->Ctrl)
        return;
    if (!arosxCtrlInit(arosxb))
    {
        mybug(10, ("controller.hidd not available\n"));
        return;
    }

    ctrl = psdAllocVec(sizeof(struct AROSXCtrl));
    if (!ctrl)
        return;
    ctrl->Player = -1;

    psdGetAttrs(PGA_DEVICE, arosxc->Device,
                DA_VendorID, &vendid,
                DA_ProductID, &prodid,
                DA_Version, &version,
                DA_Manufacturer, &manufacturer,
                DA_ProductName, &product,
                DA_SerialNumber, &serial,
                TAG_END);
    psdGetAttrs(PGA_INTERFACE, arosxc->Interface, IFA_IDString, &ifidstr, TAG_END);

    {
        struct TagItem tags[] =
        {
            { aHidd_Name,                    (IPTR)"arosx.class"                            },
            { aHidd_HardwareName,            product ? product : (IPTR)"XInput Gamepad"     },
            { aHidd_DriverData,              (IPTR)arosxc                                   },
            { aHidd_Controller_Manufacturer, manufacturer                                   },
            { aHidd_Controller_Serial,       serial                                         },
            { aHidd_Controller_VendorID,     vendid                                         },
            { aHidd_Controller_ProductID,    prodid                                         },
            { aHidd_Controller_Version,      version                                        },
            { aHidd_Controller_Bus,          vHidd_Controller_Bus_USB                       },
            { aHidd_Controller_Family,       vHidd_Controller_Family_Proprietary            },
            { aHidd_Controller_Type,         vHidd_Controller_Type_Gamepad                  },
            { aHidd_Controller_Connection,   arosxc->status.wireless ? vHidd_Controller_Conn_Wireless
                                                                     : vHidd_Controller_Conn_Wired },
            { aHidd_Controller_Path,         ifidstr                                        },
            { aHidd_Controller_ControlTable, (IPTR)arosxctrl_Controls                       },
            { aHidd_Controller_OutputTable,  (IPTR)arosxctrl_Outputs                        },
            { aHidd_Controller_BindingTable, (IPTR)arosxctrl_Bindings                       },
            { TAG_DONE,                      0                                              }
        };

        ctrl->Device = HW_AddDriver((OOP_Object *)arosxb->CtrlHW, (OOP_Class *)arosxb->CtrlClass, tags);
    }

    if (!ctrl->Device)
    {
        mybug(10, ("controller registration failed\n"));
        psdFreeVec(ctrl);
        return;
    }

    Forbid();
    arosxc->Ctrl = ctrl;
    Permit();
    psdAddErrorMsg(RETURN_OK, (STRPTR)libname, "Registered game controller '%s' (%ld buttons, %ld axes).",
                   arosxc->name, (LONG)XINPUT_BUTTON_COUNT, (LONG)XINPUT_AXIS_COUNT);
}

void arosxCtrlDetach(struct AROSXClassController *arosxc)
{
    struct AROSXClassBase *arosxb = arosxc->arosxb;
    struct Library *ps = arosxc->Base;
    struct AROSXCtrl *ctrl;

    Forbid();
    ctrl = arosxc->Ctrl;
    arosxc->Ctrl = NULL;
    Permit();
    if (!ctrl)
        return;
    if (ctrl->Device)
        HW_RemoveDriver((OOP_Object *)arosxb->CtrlHW, (OOP_Object *)ctrl->Device);
    psdFreeVec(ctrl);
}

/*****************************************************************************************
    input and output (pad task context)
*****************************************************************************************/

void arosxCtrlHandleReport(struct AROSXClassController *arosxc, UBYTE *buf, ULONG len)
{
    struct AROSXClassBase *arosxb = arosxc->arosxb;
    struct AROSXCtrl *ctrl = arosxc->Ctrl;

    if (!ctrl || !ctrl->Device)
        return;
    if (arosxctrl_Decode(buf, len, &ctrl->Raw))
        HIDD_Controller_PushReport((OOP_Object *)ctrl->Device, &ctrl->Raw);
}

void arosxCtrlFlushOutput(struct AROSXClassController *arosxc)
{
    struct Library *ps = arosxc->Base;
    struct AROSXCtrl *ctrl = arosxc->Ctrl;
    UWORD low = 0, high = 0;
    WORD player = -1;
    BOOL rumble, led;

    if (!ctrl || !arosxc->EPOutPipe)
        return;

    Forbid();
    rumble = ctrl->RumblePending;
    led = ctrl->LEDPending;
    ctrl->RumblePending = FALSE;
    ctrl->LEDPending = FALSE;
    low = ctrl->RumbleLow;
    high = ctrl->RumbleHigh;
    player = ctrl->Player;
    Permit();

    if (rumble)
    {
        ULONG n = arosxctrl_BuildRumble(arosxc->EPOutBuf, low, high);
        LONG ioerr = psdDoPipe(arosxc->EPOutPipe, arosxc->EPOutBuf, n);
        if (ioerr)
            mybug(1, ("Rumble command failed %ld\n", ioerr));
    }
    if (led)
    {
        ULONG n = arosxctrl_BuildLED(arosxc->EPOutBuf, player);
        LONG ioerr = psdDoPipe(arosxc->EPOutPipe, arosxc->EPOutBuf, n);
        if (ioerr)
            mybug(1, ("LED command failed %ld\n", ioerr));
    }
}
