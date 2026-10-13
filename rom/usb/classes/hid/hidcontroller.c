/*
 *----------------------------------------------------------------------------
 *              Game controller devices for the HID class
 *----------------------------------------------------------------------------
 *
 * One controller.hidd device is registered per HID interface that contains a
 * Joystick, Game Pad or Multi-axis application collection. The control table
 * is derived from the parsed report descriptor by hidctrl_map.c. Input
 * reports are decoded straight from the report buffer and pushed as one raw
 * report each. The two rumble motors the class already detects (LED page
 * usages 0x45/0x46) are exposed as rumble outputs and driven through the
 * class' existing output report path.
 *
 * controller.hidd lives on disk. A ROM resident hid.class cannot reach it
 * before DOS is available, so interfaces with a controller collection are
 * not bound until then (nCtrlDeferBinding); the class scan after DOS startup
 * binds them. Interfaces bound while the hidd is reachable attach at once.
 */

#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include "debug.h"
#include "hid.class.h"
#include "hidcontroller.h"

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>

#define UtilityBase         (nh->nh_UtilityBase)

/* attribute and method bases live in the class base */
#undef HiddAttrBase
#undef HiddControllerAB
#undef HWAttrBase
#undef HWBase
#undef HiddControllerBase
#undef HWControllerBase
#define HiddAttrBase        (nh->nh_CtrlAB[0])
#define HiddControllerAB    (nh->nh_CtrlAB[1])
#define HWAttrBase          (nh->nh_CtrlAB[2])
#define HWBase              (nh->nh_CtrlMB[0])
#define HiddControllerBase  (nh->nh_CtrlMB[1])
#define HWControllerBase    (nh->nh_CtrlMB[2])

struct NepHidCtrlDev
{
    struct NepClassHid *hcd_Binding;
};

/*****************************************************************************************
    the device class (a subclass of CLID_Hidd_Controller created by hand)
*****************************************************************************************/

static OOP_Object *HidCtrl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NepHidBase *nh = (struct NepHidBase *)cl->UserData;
    struct NepClassHid *nch = (struct NepClassHid *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    if (!nch)
        return NULL;
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct NepHidCtrlDev *data = OOP_INST_DATA(cl, o);
        data->hcd_Binding = nch;
    }
    return o;
}

static VOID HidCtrl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

/* Scale a 0..65535 request into a motor item's logical range */
static LONG nCtrlMotorValue(struct NepHidItem *nhi, UWORD level)
{
    LONG range = nhi->nhi_LogicalMax - nhi->nhi_LogicalMin;

    if (range <= 0)
        return level ? nhi->nhi_LogicalMax : nhi->nhi_LogicalMin;
    return nhi->nhi_LogicalMin + (LONG)(((QUAD)level * (QUAD)range) / 65535);
}

static BOOL HidCtrl__Hidd_Controller__SetRumble(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetRumble *msg)
{
    struct NepHidCtrlDev *data = OOP_INST_DATA(cl, o);
    struct NepClassHid *nch = data->hcd_Binding;
    BOOL sigit = FALSE;
    ULONG cnt;

    if (!msg->params || !nch)
        return FALSE;

    Forbid();
    for (cnt = 0; cnt < 2; cnt++)
    {
        struct NepHidItem *nhi = nch->nch_RumbleMotors[cnt];
        LONG value;

        if (!nhi)
            continue;
        value = nCtrlMotorValue(nhi, cnt ? msg->params->high : msg->params->low);
        if (nhi->nhi_OldValue != value)
        {
            nhi->nhi_OldValue = value;
            nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
            nch->nch_OutFeatTouched = TRUE;
            sigit = TRUE;
        }
    }
    if (sigit && nch->nch_Task && nch->nch_TaskMsgPort)
        Signal(nch->nch_Task, 1L << nch->nch_TaskMsgPort->mp_SigBit);
    Permit();

    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*****************************************************************************************
    class level setup
*****************************************************************************************/

BOOL nCtrlInit(struct NepHidBase *nh)
{
    BOOL ok = FALSE;

    ObtainSemaphore(&nh->nh_CtrlLock);
    if (nh->nh_CtrlClass)
    {
        ReleaseSemaphore(&nh->nh_CtrlLock);
        return TRUE;
    }

    if (!nh->nh_CtrlHiddBase)
        nh->nh_CtrlHiddBase = OpenLibrary("controller.hidd", 0);
    if (nh->nh_CtrlHiddBase)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,            (OOP_AttrBase *)&nh->nh_CtrlAB[0] },
            { IID_Hidd_Controller, (OOP_AttrBase *)&nh->nh_CtrlAB[1] },
            { IID_HW,              (OOP_AttrBase *)&nh->nh_CtrlAB[2] },
            { NULL,                NULL                                }
        };

        if (OOP_ObtainAttrBases(attrbases))
        {
            OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

            nh->nh_CtrlMB[0] = OOP_GetMethodID(IID_HW, 0);
            nh->nh_CtrlMB[1] = OOP_GetMethodID(IID_Hidd_Controller, 0);
            nh->nh_CtrlMB[2] = OOP_GetMethodID(IID_HW_Controller, 0);
            nh->nh_CtrlHW = OOP_NewObject(NULL, CLID_HW_Controller, NULL);

            if (MetaAttrBase && nh->nh_CtrlHW)
            {
                struct OOP_MethodDescr root_descr[] =
                {
                    { (OOP_MethodFunc)HidCtrl__Root__New,     moRoot_New     },
                    { (OOP_MethodFunc)HidCtrl__Root__Dispose, moRoot_Dispose },
                    { NULL, 0 }
                };
                struct OOP_MethodDescr ctrl_descr[] =
                {
                    { (OOP_MethodFunc)HidCtrl__Hidd_Controller__SetRumble, moHidd_Controller_SetRumble },
                    { NULL, 0 }
                };
                struct OOP_InterfaceDescr ifdescr[] =
                {
                    { root_descr, IID_Root,            2 },
                    { ctrl_descr, IID_Hidd_Controller, 1 },
                    { NULL, NULL, 0 }
                };
                struct TagItem tags[] =
                {
                    { aMeta_SuperID,        (IPTR)CLID_Hidd_Controller     },
                    { aMeta_InterfaceDescr, (IPTR)ifdescr                  },
                    { aMeta_InstSize,       sizeof(struct NepHidCtrlDev)   },
                    { TAG_DONE,             0                              }
                };
                OOP_Class *cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

                if (cl)
                {
                    cl->UserData = nh;
                    nh->nh_CtrlClass = cl;
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
            CloseLibrary(nh->nh_CtrlHiddBase);
            nh->nh_CtrlHiddBase = NULL;
        }
    }
    ReleaseSemaphore(&nh->nh_CtrlLock);
    return ok;
}

void nCtrlExit(struct NepHidBase *nh)
{
    ObtainSemaphore(&nh->nh_CtrlLock);
    if (nh->nh_CtrlClass)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,            (OOP_AttrBase *)&nh->nh_CtrlAB[0] },
            { IID_Hidd_Controller, (OOP_AttrBase *)&nh->nh_CtrlAB[1] },
            { IID_HW,              (OOP_AttrBase *)&nh->nh_CtrlAB[2] },
            { NULL,                NULL                                }
        };

        OOP_DisposeObject((OOP_Object *)nh->nh_CtrlClass);
        nh->nh_CtrlClass = NULL;
        OOP_ReleaseAttrBases(attrbases);
    }
    if (nh->nh_CtrlHiddBase)
    {
        CloseLibrary(nh->nh_CtrlHiddBase);
        nh->nh_CtrlHiddBase = NULL;
    }
    ReleaseSemaphore(&nh->nh_CtrlLock);
}

/*****************************************************************************************
    flattening the parsed reports for the mapper
*****************************************************************************************/

/* Usage of the application (top level) collection an item belongs to */
static ULONG nCtrlRootUsage(struct NepHidCollection *nhc)
{
    while (nhc && nhc->nhc_Parent)
        nhc = nhc->nhc_Parent;
    return nhc ? nhc->nhc_Usage : 0;
}

/* Count, or fill (when items != NULL), the flat item array of all real reports */
static ULONG nCtrlCollectItems(struct NepClassHid *nch, struct HidCtrlItem *items, ULONG max)
{
    struct NepHidReport *nhr;
    ULONG n = 0;

    for (nhr = (struct NepHidReport *)nch->nch_HidReports.lh_Head; nhr->nhr_Node.ln_Succ;
         nhr = (struct NepHidReport *)nhr->nhr_Node.ln_Succ)
    {
        struct NepHidCollection *nhc;

        if (nhr->nhr_ReportID == 0xffff)   /* the synthetic "[Extra]" report */
            continue;

        for (nhc = (struct NepHidCollection *)nhr->nhr_Collections.lh_Head; nhc->nhc_Node.ln_Succ;
             nhc = (struct NepHidCollection *)nhc->nhc_Node.ln_Succ)
        {
            struct NepHidItem *nhi;
            ULONG root = nCtrlRootUsage(nhc);

            for (nhi = (struct NepHidItem *)nhc->nhc_Items.lh_Head; nhi->nhi_Node.ln_Succ;
                 nhi = (struct NepHidItem *)nhi->nhi_Node.ln_Succ)
            {
                if (nhi->nhi_Type != REPORT_MAIN_INPUT)
                    continue;
                if (nhi->nhi_Usage == HID_PARAM_UNDEF)
                    continue;
                if (items)
                {
                    if (n >= max)
                        return n;
                    items[n].hci_Usage = nhi->nhi_Usage;
                    items[n].hci_Flags = nhi->nhi_Flags;
                    items[n].hci_Type = nhi->nhi_Type;
                    items[n].hci_Offset = nhi->nhi_Offset;
                    items[n].hci_Size = nhi->nhi_Size;
                    items[n].hci_ReportID = (UWORD)nhr->nhr_ReportID;
                    items[n].hci_Min = nhi->nhi_LogicalMin;
                    items[n].hci_Max = nhi->nhi_LogicalMax;
                    items[n].hci_IsSigned = nhi->nhi_IsSigned;
                    items[n].hci_RootUsage = root;
                    items[n].hci_Ref = nhi;
                }
                n++;
            }
        }
    }
    return n;
}

/* Does this interface expose a game controller collection? */
static BOOL nCtrlHasControllerColl(struct NepClassHid *nch)
{
    struct NepHidReport *nhr;

    for (nhr = (struct NepHidReport *)nch->nch_HidReports.lh_Head; nhr->nhr_Node.ln_Succ;
         nhr = (struct NepHidReport *)nhr->nhr_Node.ln_Succ)
    {
        struct NepHidCollection *nhc;

        if (nhr->nhr_ReportID == 0xffff)
            continue;
        for (nhc = (struct NepHidCollection *)nhr->nhr_Collections.lh_Head; nhc->nhc_Node.ln_Succ;
             nhc = (struct NepHidCollection *)nhc->nhc_Node.ln_Succ)
        {
            if (!nhc->nhc_Parent &&
                (nhc->nhc_Usage == HIDCTRL_COLL_JOYSTICK || nhc->nhc_Usage == HIDCTRL_COLL_GAMEPAD ||
                 nhc->nhc_Usage == HIDCTRL_COLL_MULTIAXIS))
                return TRUE;
        }
    }
    return FALSE;
}

/*
 * Decide whether an interface that carries a game controller collection may
 * be bound now. Before DOS is available the disk based controller.hidd is out
 * of reach, so such interfaces stay unbound until the class scan that follows
 * DOS startup. Once DOS is up and controller.hidd still cannot be opened (not
 * installed), the binding proceeds without a controller device, i.e. with the
 * legacy behaviour.
 */
BOOL nCtrlDeferBinding(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;

    if (!nCtrlHasControllerColl(nch))
        return FALSE;
    if (nCtrlInit(nh))
        return FALSE;
    return nh->nh_DOSAvailable ? FALSE : TRUE;
}

/* DOS is up: ask every bound interface without a controller to (re)try, in its own task */
void nCtrlDOSAvailable(struct NepHidBase *nh)
{
    struct NepClassHid *nch;

    nh->nh_DOSAvailable = TRUE;
    Forbid();
    nch = (struct NepClassHid *)nh->nh_Interfaces.lh_Head;
    while (nch->nch_Node.ln_Succ)
    {
        if (!nch->nch_Ctrl && nch->nch_Task && nch->nch_TaskMsgPort)
        {
            nch->nch_CtrlWanted = TRUE;
            Signal(nch->nch_Task, 1L << nch->nch_TaskMsgPort->mp_SigBit);
        }
        nch = (struct NepClassHid *)nch->nch_Node.ln_Succ;
    }
    Permit();
}

/*****************************************************************************************
    attach / detach (hid task context)
*****************************************************************************************/

static void nCtrlFree(struct NepClassHid *nch, struct NepHidCtrl *ctrl)
{
    struct Library *ps = nch->nch_Base;

    if (ctrl->hc_Table)   psdFreeVec(ctrl->hc_Table);
    if (ctrl->hc_Items)   psdFreeVec(ctrl->hc_Items);
    if (ctrl->hc_Outputs) psdFreeVec(ctrl->hc_Outputs);
    if (ctrl->hc_Raw)     psdFreeVec(ctrl->hc_Raw);
    psdFreeVec(ctrl);
}

void nCtrlAttach(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct Library *ps = nch->nch_Base;
    struct NepHidCtrl *ctrl;
    struct Hidd_Controller_OutputDesc *od;
    ULONG nitems;
    UBYTE type;

    if (nch->nch_Ctrl || !nCtrlHasControllerColl(nch))
        return;
    if (!nCtrlInit(nh))
    {
        KPRINTF(10, ("controller.hidd not available\n"));
        return;
    }

    nitems = nCtrlCollectItems(nch, NULL, 0);
    if (!nitems)
        return;
    if (nitems > HIDCTRL_MAX_ITEMS)
        nitems = HIDCTRL_MAX_ITEMS;

    ctrl = psdAllocVec(sizeof(struct NepHidCtrl));
    if (!ctrl)
        return;
    ctrl->hc_Items   = psdAllocVec(nitems * sizeof(struct HidCtrlItem));
    ctrl->hc_Table   = psdAllocVec(sizeof(struct HidCtrlTable));
    ctrl->hc_Outputs = psdAllocVec(4 * sizeof(struct Hidd_Controller_OutputDesc));
    ctrl->hc_Raw     = psdAllocVec(sizeof(struct pHidd_Controller_RawReport));
    if (!ctrl->hc_Items || !ctrl->hc_Table || !ctrl->hc_Outputs || !ctrl->hc_Raw)
    {
        nCtrlFree(nch, ctrl);
        return;
    }
    ctrl->hc_ItemCount = nCtrlCollectItems(nch, ctrl->hc_Items, nitems);

    if (!hidctrl_Build(ctrl->hc_Items, ctrl->hc_ItemCount, ctrl->hc_Table))
    {
        nCtrlFree(nch, ctrl);
        return;
    }

    /* outputs: the rumble motors the class already knows about */
    od = ctrl->hc_Outputs;
    if (nch->nch_RumbleMotors[0])
    {
        od->kind = vHidd_Controller_Out_RumbleLow;
        od->index = 0;
        od->locality = vHidd_Controller_Loc_Left;
        od->name = (CONST_STRPTR)"Rumble motor 1";
        od++;
    }
    if (nch->nch_RumbleMotors[1])
    {
        od->kind = vHidd_Controller_Out_RumbleHigh;
        od->index = 0;
        od->locality = vHidd_Controller_Loc_Right;
        od->name = (CONST_STRPTR)"Rumble motor 2";
        od++;
    }
    od->kind = vHidd_Controller_Out_End;

    switch (ctrl->hc_Table->hct_RootUsage)
    {
    case HIDCTRL_COLL_JOYSTICK: type = vHidd_Controller_Type_Joystick; break;
    case HIDCTRL_COLL_GAMEPAD:  type = vHidd_Controller_Type_Gamepad;  break;
    default:                    type = vHidd_Controller_Type_Other;    break;
    }

    {
        IPTR vendid = 0, prodid = 0, version = 0, manufacturer = 0, product = 0, serial = 0;
        struct TagItem tags[] =
        {
            { aHidd_Name,                    (IPTR)"hid.class"                    },
            { aHidd_HardwareName,            0                                    },
            { aHidd_DriverData,              (IPTR)nch                            },
            { aHidd_Controller_Manufacturer, 0                                    },
            { aHidd_Controller_Serial,       0                                    },
            { aHidd_Controller_VendorID,     0                                    },
            { aHidd_Controller_ProductID,    0                                    },
            { aHidd_Controller_Version,      0                                    },
            { aHidd_Controller_Bus,          vHidd_Controller_Bus_USB             },
            { aHidd_Controller_Family,       vHidd_Controller_Family_HID          },
            { aHidd_Controller_Type,         type                                 },
            { aHidd_Controller_Connection,   vHidd_Controller_Conn_Wired          },
            { aHidd_Controller_Path,         (IPTR)nch->nch_IfIDString            },
            { aHidd_Controller_ControlTable, (IPTR)ctrl->hc_Table->hct_Controls   },
            { aHidd_Controller_OutputTable,  (IPTR)ctrl->hc_Outputs               },
            { TAG_DONE,                      0                                    }
        };

        psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                    DA_VendorID, &vendid,
                    DA_ProductID, &prodid,
                    DA_Version, &version,
                    DA_Manufacturer, &manufacturer,
                    DA_ProductName, &product,
                    DA_SerialNumber, &serial,
                    TAG_END);
        tags[1].ti_Data = product ? product : (IPTR)"USB Game Controller";
        tags[3].ti_Data = manufacturer;
        tags[4].ti_Data = serial;
        tags[5].ti_Data = vendid;
        tags[6].ti_Data = prodid;
        tags[7].ti_Data = version;

        ctrl->hc_Device = HW_AddDriver((OOP_Object *)nh->nh_CtrlHW, (OOP_Class *)nh->nh_CtrlClass, tags);
    }

    if (!ctrl->hc_Device)
    {
        KPRINTF(10, ("controller registration failed\n"));
        nCtrlFree(nch, ctrl);
        return;
    }

    nch->nch_Ctrl = ctrl;
    psdAddErrorMsg(RETURN_OK, (STRPTR)"hid.class",
                   "Registered game controller (%ld buttons, %ld axes, %ld hats).",
                   (LONG)ctrl->hc_Table->hct_Buttons, (LONG)ctrl->hc_Table->hct_Axes, (LONG)ctrl->hc_Table->hct_Hats);
}

void nCtrlDetach(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct NepHidCtrl *ctrl = nch->nch_Ctrl;

    if (!ctrl)
        return;
    nch->nch_Ctrl = NULL;
    if (ctrl->hc_Device)
        HW_RemoveDriver((OOP_Object *)nh->nh_CtrlHW, (OOP_Object *)ctrl->hc_Device);
    nCtrlFree(nch, ctrl);
}

/*****************************************************************************************
    input reports
*****************************************************************************************/

void nCtrlHandleReport(struct NepClassHid *nch, struct NepHidReport *nhr, UBYTE *buf, ULONG buflen)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct NepHidCtrl *ctrl = nch->nch_Ctrl;

    if (!ctrl || !ctrl->hc_Device)
        return;
    if (hidctrl_Decode(ctrl->hc_Table, ctrl->hc_Items, (UWORD)nhr->nhr_ReportID, buf, buflen,
                       (struct pHidd_Controller_RawReport *)ctrl->hc_Raw))
        HIDD_Controller_PushReport((OOP_Object *)ctrl->hc_Device, (struct pHidd_Controller_RawReport *)ctrl->hc_Raw);
}
