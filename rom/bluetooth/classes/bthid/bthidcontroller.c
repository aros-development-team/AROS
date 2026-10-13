/*
 *----------------------------------------------------------------------------
 *         Game controller devices for the Bluetooth HID class
 *----------------------------------------------------------------------------
 *
 * One controller.hidd device is registered per bound HID service whose report
 * descriptor contains a Joystick, Game Pad or Multi-axis application
 * collection. The control table is derived from the parsed descriptor by
 * bthidctrl_map.c, input reports are decoded straight from the report buffer
 * and pushed as one raw report each, and the two rumble motors the class
 * already detects (LED page usages 0x45/0x46) are exposed as rumble outputs
 * driven through the class' output report path.
 *
 * Bluetooth bindings are only made once the stack is up, i.e. after DOS, so
 * controller.hidd is expected to be loadable at attach time; if it is not,
 * the binding simply runs without a controller device.
 */

#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include "debug.h"
#include "bthid.h"
#include "bthidcontroller.h"

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>

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

struct BtHidCtrlDev
{
    struct BTHidBinding *hcd_Binding;
};

/*****************************************************************************************
    the device class (a subclass of CLID_Hidd_Controller created by hand)
*****************************************************************************************/

static OOP_Object *BtHidCtrl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct BTHidBase *nh = (struct BTHidBase *)cl->UserData;
    struct BTHidBinding *nhb = (struct BTHidBinding *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    if (!nhb)
        return NULL;
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct BtHidCtrlDev *data = OOP_INST_DATA(cl, o);
        data->hcd_Binding = nhb;
    }
    return o;
}

static VOID BtHidCtrl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

/* Scale a 0..65535 request into a motor item's logical range */
static LONG bCtrlMotorValue(struct BtHidItem *nhi, UWORD level)
{
    LONG range = nhi->nhi_LogicalMax - nhi->nhi_LogicalMin;

    if (range <= 0)
        return level ? nhi->nhi_LogicalMax : nhi->nhi_LogicalMin;
    return nhi->nhi_LogicalMin + (LONG)(((QUAD)level * (QUAD)range) / 65535);
}

static BOOL BtHidCtrl__Hidd_Controller__SetRumble(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetRumble *msg)
{
    struct BtHidCtrlDev *data = OOP_INST_DATA(cl, o);
    struct BTHidBinding *nhb = data->hcd_Binding;
    BOOL sigit = FALSE;
    ULONG cnt;

    if (!msg->params || !nhb)
        return FALSE;

    Forbid();
    for (cnt = 0; cnt < 2; cnt++)
    {
        struct BtHidItem *nhi = nhb->nhb_RumbleMotors[cnt];
        LONG value;

        if (!nhi)
            continue;
        value = bCtrlMotorValue(nhi, cnt ? msg->params->high : msg->params->low);
        if (nhi->nhi_OldValue != value)
        {
            nhi->nhi_OldValue = value;
            nhi->nhi_Collection->nhc_Report->nhr_OutTouched = TRUE;
            nhb->nhb_OutFeatTouched = TRUE;
            sigit = TRUE;
        }
    }
    if (sigit && nhb->nhb_Task && nhb->nhb_TaskMsgPort)
        Signal(nhb->nhb_Task, 1L << nhb->nhb_TaskMsgPort->mp_SigBit);
    Permit();

    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*****************************************************************************************
    class level setup
*****************************************************************************************/

BOOL bCtrlInit(struct BTHidBase *nh)
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
                    { (OOP_MethodFunc)BtHidCtrl__Root__New,     moRoot_New     },
                    { (OOP_MethodFunc)BtHidCtrl__Root__Dispose, moRoot_Dispose },
                    { NULL, 0 }
                };
                struct OOP_MethodDescr ctrl_descr[] =
                {
                    { (OOP_MethodFunc)BtHidCtrl__Hidd_Controller__SetRumble, moHidd_Controller_SetRumble },
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
                    { aMeta_InstSize,       sizeof(struct BtHidCtrlDev)    },
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

void bCtrlExit(struct BTHidBase *nh)
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
static ULONG bCtrlRootUsage(struct BtHidCollection *nhc)
{
    while (nhc && nhc->nhc_Parent)
        nhc = nhc->nhc_Parent;
    return nhc ? nhc->nhc_Usage : 0;
}

/* Count, or fill (when items != NULL), the flat item array of all real reports */
static ULONG bCtrlCollectItems(struct BTHidBinding *nhb, struct BtHidCtrlItem *items, ULONG max)
{
    struct BtHidReport *nhr;
    ULONG n = 0;

    for (nhr = (struct BtHidReport *)nhb->nhb_HidReports.lh_Head; nhr->nhr_Node.ln_Succ;
         nhr = (struct BtHidReport *)nhr->nhr_Node.ln_Succ)
    {
        struct BtHidCollection *nhc;

        if (nhr->nhr_ReportID == 0xffff)   /* the synthetic "[Extra]" report */
            continue;

        for (nhc = (struct BtHidCollection *)nhr->nhr_Collections.lh_Head; nhc->nhc_Node.ln_Succ;
             nhc = (struct BtHidCollection *)nhc->nhc_Node.ln_Succ)
        {
            struct BtHidItem *nhi;
            ULONG root = bCtrlRootUsage(nhc);

            for (nhi = (struct BtHidItem *)nhc->nhc_Items.lh_Head; nhi->nhi_Node.ln_Succ;
                 nhi = (struct BtHidItem *)nhi->nhi_Node.ln_Succ)
            {
                if (nhi->nhi_Type != REPORT_MAIN_INPUT)
                    continue;
                if (nhi->nhi_Usage == HID_PARAM_UNDEF)
                    continue;
                if (items)
                {
                    if (n >= max)
                        return n;
                    items[n].bhci_Usage = nhi->nhi_Usage;
                    items[n].bhci_Flags = nhi->nhi_Flags;
                    items[n].bhci_Type = nhi->nhi_Type;
                    items[n].bhci_Offset = nhi->nhi_Offset;
                    items[n].bhci_Size = nhi->nhi_Size;
                    items[n].bhci_ReportID = (UWORD)nhr->nhr_ReportID;
                    items[n].bhci_Min = nhi->nhi_LogicalMin;
                    items[n].bhci_Max = nhi->nhi_LogicalMax;
                    items[n].bhci_IsSigned = nhi->nhi_IsSigned;
                    items[n].bhci_RootUsage = root;
                    items[n].bhci_Ref = nhi;
                }
                n++;
            }
        }
    }
    return n;
}

/* Does this binding expose a game controller collection? */
static BOOL bCtrlHasControllerColl(struct BTHidBinding *nhb)
{
    struct BtHidReport *nhr;

    for (nhr = (struct BtHidReport *)nhb->nhb_HidReports.lh_Head; nhr->nhr_Node.ln_Succ;
         nhr = (struct BtHidReport *)nhr->nhr_Node.ln_Succ)
    {
        struct BtHidCollection *nhc;

        if (nhr->nhr_ReportID == 0xffff)
            continue;
        for (nhc = (struct BtHidCollection *)nhr->nhr_Collections.lh_Head; nhc->nhc_Node.ln_Succ;
             nhc = (struct BtHidCollection *)nhc->nhc_Node.ln_Succ)
        {
            if (!nhc->nhc_Parent &&
                (nhc->nhc_Usage == BTHIDCTRL_COLL_JOYSTICK || nhc->nhc_Usage == BTHIDCTRL_COLL_GAMEPAD ||
                 nhc->nhc_Usage == BTHIDCTRL_COLL_MULTIAXIS))
                return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************************
    attach / detach (binding task context)
*****************************************************************************************/

static void bCtrlFree(struct BTHidBinding *nhb, struct BtHidCtrl *ctrl)
{
    struct Library *BluetoothBase = nhb->nhb_Base;

    if (ctrl->hc_Table)   btFreeVec(ctrl->hc_Table);
    if (ctrl->hc_Items)   btFreeVec(ctrl->hc_Items);
    if (ctrl->hc_Outputs) btFreeVec(ctrl->hc_Outputs);
    if (ctrl->hc_Raw)     btFreeVec(ctrl->hc_Raw);
    btFreeVec(ctrl);
}

void bCtrlAttach(struct BTHidBinding *nhb)
{
    struct BTHidBase *nh = nhb->nhb_ClsBase;
    struct Library *BluetoothBase = nhb->nhb_Base;
    struct BtHidCtrl *ctrl;
    struct Hidd_Controller_OutputDesc *od;
    ULONG nitems;
    UBYTE type;

    if (nhb->nhb_Ctrl || !bCtrlHasControllerColl(nhb))
        return;
    if (!bCtrlInit(nh))
    {
        btAddErrorMsg(RETURN_WARN, (STRPTR)"bthid.class",
                      "controller.hidd not available, game controller '%s' runs without it.", nhb->nhb_DevIDString);
        return;
    }

    nitems = bCtrlCollectItems(nhb, NULL, 0);
    if (!nitems)
        return;
    if (nitems > BTHIDCTRL_MAX_ITEMS)
        nitems = BTHIDCTRL_MAX_ITEMS;

    ctrl = btAllocVec(sizeof(struct BtHidCtrl));
    if (!ctrl)
        return;
    ctrl->hc_Items   = btAllocVec(nitems * sizeof(struct BtHidCtrlItem));
    ctrl->hc_Table   = btAllocVec(sizeof(struct BtHidCtrlTable));
    ctrl->hc_Outputs = btAllocVec(4 * sizeof(struct Hidd_Controller_OutputDesc));
    ctrl->hc_Raw     = btAllocVec(sizeof(struct pHidd_Controller_RawReport));
    if (!ctrl->hc_Items || !ctrl->hc_Table || !ctrl->hc_Outputs || !ctrl->hc_Raw)
    {
        bCtrlFree(nhb, ctrl);
        return;
    }
    ctrl->hc_ItemCount = bCtrlCollectItems(nhb, ctrl->hc_Items, nitems);

    if (!bthidctrl_Build(ctrl->hc_Items, ctrl->hc_ItemCount, ctrl->hc_Table))
    {
        bCtrlFree(nhb, ctrl);
        return;
    }

    /* outputs: the rumble motors the class already knows about */
    od = ctrl->hc_Outputs;
    if (nhb->nhb_RumbleMotors[0])
    {
        od->kind = vHidd_Controller_Out_RumbleLow;
        od->index = 0;
        od->locality = vHidd_Controller_Loc_Left;
        od->name = (CONST_STRPTR)"Rumble motor 1";
        od++;
    }
    if (nhb->nhb_RumbleMotors[1])
    {
        od->kind = vHidd_Controller_Out_RumbleHigh;
        od->index = 0;
        od->locality = vHidd_Controller_Loc_Right;
        od->name = (CONST_STRPTR)"Rumble motor 2";
        od++;
    }
    od->kind = vHidd_Controller_Out_End;

    switch (ctrl->hc_Table->bhct_RootUsage)
    {
    case BTHIDCTRL_COLL_JOYSTICK: type = vHidd_Controller_Type_Joystick; break;
    case BTHIDCTRL_COLL_GAMEPAD:  type = vHidd_Controller_Type_Gamepad;  break;
    default:                      type = vHidd_Controller_Type_Other;    break;
    }

    {
        IPTR vendid = 0, prodid = 0, version = 0, name = 0, address = 0;
        struct TagItem tags[] =
        {
            { aHidd_Name,                    (IPTR)"bthid.class"                     },
            { aHidd_HardwareName,            0                                       },
            { aHidd_DriverData,              (IPTR)nhb                               },
            { aHidd_Controller_Serial,       0                                       },
            { aHidd_Controller_VendorID,     0                                       },
            { aHidd_Controller_ProductID,    0                                       },
            { aHidd_Controller_Version,      0                                       },
            { aHidd_Controller_Bus,          vHidd_Controller_Bus_Bluetooth          },
            { aHidd_Controller_Family,       vHidd_Controller_Family_HID             },
            { aHidd_Controller_Type,         type                                    },
            { aHidd_Controller_Connection,   vHidd_Controller_Conn_Wireless          },
            { aHidd_Controller_Path,         (IPTR)nhb->nhb_SvcIDString              },
            { aHidd_Controller_ControlTable, (IPTR)ctrl->hc_Table->bhct_Controls     },
            { aHidd_Controller_OutputTable,  (IPTR)ctrl->hc_Outputs                  },
            { TAG_DONE,                      0                                       }
        };

        btGetAttrs(BGA_DEVICE, nhb->nhb_Device,
                   BDA_VendorID, &vendid,
                   BDA_ProductID, &prodid,
                   BDA_ProductVersion, &version,
                   BDA_Name, &name,
                   BDA_AddressString, &address,
                   TAG_END);
        tags[1].ti_Data = name ? name : (IPTR)"Bluetooth Game Controller";
        tags[3].ti_Data = address;
        tags[4].ti_Data = vendid;
        tags[5].ti_Data = prodid;
        tags[6].ti_Data = version;

        ctrl->hc_Device = HW_AddDriver((OOP_Object *)nh->nh_CtrlHW, (OOP_Class *)nh->nh_CtrlClass, tags);
    }

    if (!ctrl->hc_Device)
    {
        KPRINTF(10, ("controller registration failed\n"));
        bCtrlFree(nhb, ctrl);
        return;
    }

    nhb->nhb_Ctrl = ctrl;
    btAddErrorMsg(RETURN_OK, (STRPTR)"bthid.class",
                  "Registered game controller (%ld buttons, %ld axes, %ld hats).",
                  (LONG)ctrl->hc_Table->bhct_Buttons, (LONG)ctrl->hc_Table->bhct_Axes, (LONG)ctrl->hc_Table->bhct_Hats);
}

void bCtrlDetach(struct BTHidBinding *nhb)
{
    struct BTHidBase *nh = nhb->nhb_ClsBase;
    struct BtHidCtrl *ctrl = nhb->nhb_Ctrl;

    if (!ctrl)
        return;
    nhb->nhb_Ctrl = NULL;
    if (ctrl->hc_Device)
        HW_RemoveDriver((OOP_Object *)nh->nh_CtrlHW, (OOP_Object *)ctrl->hc_Device);
    bCtrlFree(nhb, ctrl);
}

/*****************************************************************************************
    input reports
*****************************************************************************************/

void bCtrlHandleReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf, ULONG buflen)
{
    struct BTHidBase *nh = nhb->nhb_ClsBase;
    struct BtHidCtrl *ctrl = nhb->nhb_Ctrl;

    if (!ctrl || !ctrl->hc_Device)
        return;
    if (bthidctrl_Decode(ctrl->hc_Table, ctrl->hc_Items, (UWORD)nhr->nhr_ReportID, buf, buflen,
                         (struct pHidd_Controller_RawReport *)ctrl->hc_Raw))
        HIDD_Controller_PushReport((OOP_Object *)ctrl->hc_Device, (struct pHidd_Controller_RawReport *)ctrl->hc_Raw);
}
