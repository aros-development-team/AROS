/*
 *----------------------------------------------------------------------------
 *                   btbattery class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * Battery Service (GATT, 0x180F): keyboards, mice, controllers and headsets
 * report their charge through it. The class binds to the service of every
 * registered device that has one, reads the Battery Level (0x2A19) once the
 * device is connected, follows its notifications where the device sends
 * them and reads it again every few minutes where it does not.
 *
 * The level is shown to the system as a telemetry device: a subclass of
 * telemetry.hidd made here at run time, one object per connected device,
 * registered with the system's hardware root (where SysExplorer and other
 * telemetry clients find it) when the first level is known and removed when
 * the device disconnects. Each has one entry, the charge in percent.
 *
 * The class never connects a device itself: it learns from the stack's
 * device events when one came or went. It also waits a few seconds before
 * it asks a device that has just connected: a link serves one GATT request
 * at a time and refuses a second, and the classes that make the device
 * usable (bthid reading its report map) go first.
 */

#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include "debug.h"

#include "btbattery.h"

#include <hidd/hidd.h>
#include <hidd/system.h>
#include <hidd/telemetry.h>
#include <oop/oop.h>
#include <proto/oop.h>

static const STRPTR libname = MOD_NAME_STRING;

#define UtilityBase nh->nh_UtilityBase

/* attribute and method bases live in the class base */
#undef HiddAttrBase
#undef HiddTelemetryAB
#undef HWAttrBase
#undef HWBase
#undef HiddTelemetryBase
#define HiddAttrBase        (nh->nh_TelAB[0])
#define HiddTelemetryAB     (nh->nh_TelAB[1])
#define HWAttrBase          (nh->nh_TelAB[2])
#define HWBase              (nh->nh_TelMB[0])
#define HiddTelemetryBase   (nh->nh_TelMB[1])

struct BTBatDev
{
    struct BTBatBinding *bd_Binding;
};

/*
 * ***********************************************************************
 * * The telemetry device class (a subclass of CLID_Hidd_Telemetry)      *
 * ***********************************************************************
 */

/* /// "BTBat__Root__New()" */
static OOP_Object *BTBat__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct BTBatBase *nh = (struct BTBatBase *) cl->UserData;
    struct BTBatBinding *bb = (struct BTBatBinding *) GetTagData(aHidd_DriverData, 0, msg->attrList);

    if(!bb)
    {
        return(NULL);
    }
    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if(o)
    {
        struct BTBatDev *data = OOP_INST_DATA(cl, o);
        data->bd_Binding = bb;
    }
    return(o);
}
/* \\\ */

/* /// "BTBat__Root__Dispose()" */
static VOID BTBat__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}
/* \\\ */

/* /// "BTBat__Root__Get()" */
static VOID BTBat__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BTBatBase *nh = (struct BTBatBase *) cl->UserData;

    if((msg->attrID - HiddTelemetryAB) == aoHidd_Telemetry_EntryCount)
    {
        *msg->storage = 1;
        return;
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}
/* \\\ */

/* /// "BTBat__Hidd_Telemetry__GetEntryAttribs()" */
/* The one entry: the charge in percent. */
static BOOL BTBat__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
                                                   struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct BTBatBase *nh = (struct BTBatBase *) cl->UserData;
    struct BTBatDev *data = OOP_INST_DATA(cl, o);
    struct BTBatBinding *bb = data->bd_Binding;
    struct TagItem *tstate = msg->tags;
    struct TagItem *tag;
    LONG level = bb->bb_Level;

    if(msg->index != 0)
    {
        return(FALSE);
    }
    if(level < 0)
    {
        level = 0;
    }
    while((tag = NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case tHidd_Telemetry_EntryID:
                *(CONST_STRPTR *) tag->ti_Data = "Battery Level";
                break;
            case tHidd_Telemetry_EntryUnits:
                *(ULONG *) tag->ti_Data = vHW_TelemetryUnit_Percent;
                break;
            case tHidd_Telemetry_EntryMin:
                *(LONG *) tag->ti_Data = 0;
                break;
            case tHidd_Telemetry_EntryMax:
                *(LONG *) tag->ti_Data = 100;
                break;
            case tHidd_Telemetry_EntryValue:
                *(LONG *) tag->ti_Data = level;
                break;
            case tHidd_Telemetry_EntryReadOnly:
                *(BOOL *) tag->ti_Data = TRUE;
                break;
        }
    }
    return(TRUE);
}
/* \\\ */

/* /// "bTelInit()" */
/* Bindings are only made once the stack is up, i.e. after DOS, so
   telemetry.hidd can be loaded by now; without it the class still follows
   the levels, there is just nowhere to show them. */
static BOOL bTelInit(struct BTBatBase *nh)
{
    BOOL ok = FALSE;

    ObtainSemaphore(&nh->nh_TelLock);
    if(nh->nh_TelClass)
    {
        ReleaseSemaphore(&nh->nh_TelLock);
        return(TRUE);
    }
    if(!nh->nh_TelHiddBase)
    {
        /* in memory already, or to be loaded from where the drivers live */
        if(!(nh->nh_TelHiddBase = OpenLibrary("telemetry.hidd", 0)))
        {
            if(!(nh->nh_TelHiddBase = OpenLibrary("DRIVERS:telemetry.hidd", 0)))
            {
                nh->nh_TelHiddBase = OpenLibrary("DEVS:Drivers/telemetry.hidd", 0);
            }
        }
    }
    if(nh->nh_TelHiddBase)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,           (OOP_AttrBase *) &nh->nh_TelAB[0] },
            { IID_Hidd_Telemetry, (OOP_AttrBase *) &nh->nh_TelAB[1] },
            { IID_HW,             (OOP_AttrBase *) &nh->nh_TelAB[2] },
            { NULL,               NULL                              }
        };

        if(OOP_ObtainAttrBases(attrbases))
        {
            OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

            nh->nh_TelMB[0] = OOP_GetMethodID(IID_HW, 0);
            nh->nh_TelMB[1] = OOP_GetMethodID(IID_Hidd_Telemetry, 0);
            if(!nh->nh_TelRoot)
            {
                nh->nh_TelRoot = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
            }
            if(!nh->nh_TelRoot)
            {
                nh->nh_TelRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
            }
            if(MetaAttrBase && nh->nh_TelRoot)
            {
                struct OOP_MethodDescr root_descr[] =
                {
                    { (OOP_MethodFunc) BTBat__Root__New,     moRoot_New     },
                    { (OOP_MethodFunc) BTBat__Root__Dispose, moRoot_Dispose },
                    { (OOP_MethodFunc) BTBat__Root__Get,     moRoot_Get     },
                    { NULL, 0 }
                };
                struct OOP_MethodDescr tel_descr[] =
                {
                    { (OOP_MethodFunc) BTBat__Hidd_Telemetry__GetEntryAttribs, moHidd_Telemetry_GetEntryAttribs },
                    { NULL, 0 }
                };
                struct OOP_InterfaceDescr ifdescr[] =
                {
                    { root_descr, IID_Root,           3 },
                    { tel_descr,  IID_Hidd_Telemetry, 1 },
                    { NULL, NULL, 0 }
                };
                struct TagItem tags[] =
                {
                    { aMeta_SuperID,        (IPTR) CLID_Hidd_Telemetry      },
                    { aMeta_InterfaceDescr, (IPTR) ifdescr                  },
                    { aMeta_InstSize,       sizeof(struct BTBatDev)         },
                    { TAG_DONE,             0                               }
                };
                OOP_Class *cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);

                if(cl)
                {
                    cl->UserData = nh;
                    nh->nh_TelClass = cl;
                    ok = TRUE;
                }
            }
            if(MetaAttrBase)
            {
                OOP_ReleaseAttrBase(IID_Meta);
            }
            if(!ok)
            {
                OOP_ReleaseAttrBases(attrbases);
            }
        }
        if(!ok)
        {
            CloseLibrary(nh->nh_TelHiddBase);
            nh->nh_TelHiddBase = NULL;
        }
    }
    ReleaseSemaphore(&nh->nh_TelLock);
    return(ok);
}
/* \\\ */

/* /// "bTelExit()" */
static void bTelExit(struct BTBatBase *nh)
{
    ObtainSemaphore(&nh->nh_TelLock);
    if(nh->nh_TelClass)
    {
        struct OOP_ABDescr attrbases[] =
        {
            { IID_Hidd,           (OOP_AttrBase *) &nh->nh_TelAB[0] },
            { IID_Hidd_Telemetry, (OOP_AttrBase *) &nh->nh_TelAB[1] },
            { IID_HW,             (OOP_AttrBase *) &nh->nh_TelAB[2] },
            { NULL,               NULL                              }
        };

        OOP_DisposeObject((OOP_Object *) nh->nh_TelClass);
        nh->nh_TelClass = NULL;
        OOP_ReleaseAttrBases(attrbases);
    }
    if(nh->nh_TelHiddBase)
    {
        CloseLibrary(nh->nh_TelHiddBase);
        nh->nh_TelHiddBase = NULL;
    }
    ReleaseSemaphore(&nh->nh_TelLock);
}
/* \\\ */

/* /// "bTelAttach()" */
/* The device is here and its level is known: show it. */
static void bTelAttach(struct BTBatBinding *bb)
{
    struct BTBatBase *nh = bb->bb_ClsBase;
    struct Library *BluetoothBase = bb->bb_Base;

    if(bb->bb_Object)
    {
        return;
    }
    if(!bTelInit(nh))
    {
        if(!bb->bb_Complained)
        {
            bb->bb_Complained = TRUE;
            btAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                           "%s: %ld%%, but telemetry.hidd is not available to show it.", bb->bb_Name, bb->bb_Level);
        }
        return;
    }
    {
        struct TagItem tags[] =
        {
            { aHidd_Name,         (IPTR) "btbattery.class" },
            { aHidd_HardwareName, (IPTR) bb->bb_Name       },
            { aHidd_DriverData,   (IPTR) bb                },
            { TAG_DONE,           0                        }
        };

        bb->bb_Object = HW_AddDriver((OOP_Object *) nh->nh_TelRoot, (OOP_Class *) nh->nh_TelClass, tags);
    }
    if(bb->bb_Object)
    {
        btAddErrorMsg(RETURN_OK, (STRPTR) libname, "%s: %ld%% (shown as a telemetry device).",
                       bb->bb_Name, bb->bb_Level);
    }
}
/* \\\ */

/* /// "bTelDetach()" */
static void bTelDetach(struct BTBatBinding *bb)
{
    struct BTBatBase *nh = bb->bb_ClsBase;

    if(bb->bb_Object)
    {
        HW_RemoveDriver((OOP_Object *) nh->nh_TelRoot, (OOP_Object *) bb->bb_Object);
        bb->bb_Object = NULL;
    }
}
/* \\\ */

/*
 * ***********************************************************************
 * * The class                                                           *
 * ***********************************************************************
 */

/* /// "Lib Stuff" */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    if(!(nh->nh_UtilityBase = OpenLibrary("utility.library", 39)))
    {
        return(FALSE);
    }
    InitSemaphore(&nh->nh_TelLock);
    return(TRUE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    /* the bindings were released before the class is closed */
    bTelExit(nh);
    CloseLibrary(nh->nh_UtilityBase);
    return(TRUE);
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
/* \\\ */

/* /// "bForceServiceBinding()" */
static struct BTBatBinding * bForceServiceBinding(struct BTBatBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    struct BTBatBinding *bb;
    struct BtDevice *bd = NULL;
    struct BtEndpoint *bep;
    STRPTR devname = NULL;
    IPTR handle = 0, props = 0;
    UBYTE buf[64];
    struct Task *tmptask;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        return(NULL);
    }
    btGetAttrs(BGA_SERVICE, bsv, BSVA_Device, &bd, TAG_END);
    bep = btFindEndpoint(bsv, NULL, BEA_UUID16, UUID_BATTERY_LEVEL, TAG_END);
    if(!bd || !bep || !(bb = AllocVec(sizeof(struct BTBatBinding), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        CloseLibrary(BluetoothBase);
        return(NULL);
    }
    btGetAttrs(BGA_DEVICE, bd, BDA_Name, &devname, TAG_END);
    btGetAttrs(BGA_ENDPOINT, bep, BEA_Handle, &handle, BEA_Properties, &props, TAG_END);
    bb->bb_ClsBase = nh;
    bb->bb_Device = bd;
    bb->bb_Service = bsv;
    bb->bb_Endpoint = bep;
    bb->bb_Handle = handle;
    bb->bb_CanNotify = (props & 0x30) ? TRUE : FALSE; /* notify | indicate */
    bb->bb_Level = -1;
    btSafeRawDoFmt((STRPTR) bb->bb_Name, sizeof(bb->bb_Name), "%s battery", devname ? devname : (STRPTR) "Bluetooth device");

    btSafeRawDoFmt(buf, 64, "btbattery.class<%08lx>", (IPTR) bb);
    bb->bb_ReadySignal = SIGB_SINGLE;
    bb->bb_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if((tmptask = btSpawnSubTask(buf, (APTR) bBatteryTask, bb)))
    {
        btBorrowLocksWait(tmptask, 1UL<<bb->bb_ReadySignal);
        if(bb->bb_Task)
        {
            bb->bb_ReadySigTask = NULL;
            CloseLibrary(BluetoothBase);
            return(bb);
        }
    }
    bb->bb_ReadySigTask = NULL;
    FreeVec(bb);
    CloseLibrary(BluetoothBase);
    return(NULL);
}
/* \\\ */

/* /// "bAttemptServiceBinding()" */
static struct BTBatBinding * bAttemptServiceBinding(struct BTBatBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    IPTR uuid16 = 0;
    IPTR proto = 0;

    if((BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        btGetAttrs(BGA_SERVICE, bsv, BSVA_UUID16, &uuid16, BSVA_Protocol, &proto, TAG_END);
        CloseLibrary(BluetoothBase);
    }
    if((proto == BSVP_ATT) && (uuid16 == UUID_BATTERY_SERVICE))
    {
        return(bForceServiceBinding(nh, bsv));
    }
    return(NULL);
}
/* \\\ */

/* /// "bReleaseServiceBinding()" */
static void bReleaseServiceBinding(struct BTBatBase *nh, struct BTBatBinding *bb)
{
    Forbid();
    bb->bb_ReadySignal = SIGB_SINGLE;
    bb->bb_ReadySigTask = FindTask(NULL);
    if(bb->bb_Task)
    {
        Signal(bb->bb_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(bb->bb_Task)
    {
        Wait(1UL<<bb->bb_ReadySignal);
    }
    bb->bb_ReadySigTask = NULL;
    FreeVec(bb);
}
/* \\\ */

/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, btbattery)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    LONG count = 0;

    switch(type)
    {
        case BCGA_CLASS:
            if((ti = FindTagItem(BCCA_Priority, tags)))
            {
                *((SIPTR *) ti->ti_Data) = 0;
                count++;
            }
            if((ti = FindTagItem(BCCA_Description, tags)))
            {
                *((STRPTR *) ti->ti_Data) = "Battery level of Bluetooth devices, as telemetry";
                count++;
            }
            if((ti = FindTagItem(BCCA_HasClassCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(BCCA_HasBindingCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(BCCA_AfterDOSRestart, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(BCCA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            break;

        case BCGA_BINDING:
        {
            struct BTBatBinding *bb = (struct BTBatBinding *) btstruct;
            if((ti = FindTagItem(BCBA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            if((ti = FindTagItem(BCBA_Device, tags)))
            {
                *((struct BtDevice **) ti->ti_Data) = bb->bb_Device;
                count++;
            }
            if((ti = FindTagItem(BCBA_Service, tags)))
            {
                *((struct BtService **) ti->ti_Data) = bb->bb_Service;
                count++;
            }
            if((ti = FindTagItem(BCBA_Task, tags)))
            {
                *((struct Task **) ti->ti_Data) = bb->bb_Task;
                count++;
            }
            break;
        }
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcSetAttrsA()" */
AROS_LH3(LONG, btcSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, btbattery)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcDoMethodA()" */
AROS_LH2(SIPTR, btcDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, btbattery)
{
    AROS_LIBFUNC_INIT

    switch(methodid)
    {
        case BCM_AttemptServiceBinding:
            return((SIPTR) bAttemptServiceBinding(nh, (struct BtService *) methoddata[0]));

        case BCM_ForceServiceBinding:
            return((SIPTR) bForceServiceBinding(nh, (struct BtService *) methoddata[0]));

        case BCM_ReleaseServiceBinding:
            bReleaseServiceBinding(nh, (struct BTBatBinding *) methoddata[0]);
            return(TRUE);

        case BCM_AttemptDeviceBinding:
        case BCM_ForceDeviceBinding:
            return(0); /* only service bindings */

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

#undef UtilityBase

/*
 * ***********************************************************************
 * * The binding task                                                    *
 * ***********************************************************************
 */

/* /// "bArmTimer()" */
static void bArmTimer(struct BTBatBinding *bb, ULONG secs)
{
    if(!bb->bb_TimerOpen)
    {
        return;
    }
    if(bb->bb_TimerPending)
    {
        AbortIO((struct IORequest *) bb->bb_TimerReq);
        WaitIO((struct IORequest *) bb->bb_TimerReq);
        SetSignal(0, 1UL<<bb->bb_TimerPort->mp_SigBit);
    }
    bb->bb_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
    bb->bb_TimerReq->tr_time.tv_secs = secs;
    bb->bb_TimerReq->tr_time.tv_micro = 0;
    SendIO((struct IORequest *) bb->bb_TimerReq);
    bb->bb_TimerPending = TRUE;
}
/* \\\ */

/* /// "bAskLevel()" */
/* Listen for notifications and read the level. Both simply fail while the
   device is away or the link is busy; the timer tries again. */
static void bAskLevel(struct BTBatBinding *bb)
{
    struct Library *BluetoothBase = bb->bb_Base;

    if(bb->bb_CanNotify && !bb->bb_NotifyPosted)
    {
        btSendChannel(bb->bb_NotifyCh, bb->bb_NotifyBuf, sizeof(bb->bb_NotifyBuf));
        bb->bb_NotifyPosted = TRUE;
    }
    if(!bb->bb_ReadBusy)
    {
        btSendChannel(bb->bb_ReadCh, bb->bb_ReadBuf, sizeof(bb->bb_ReadBuf));
        bb->bb_ReadBusy = TRUE;
    }
}
/* \\\ */

/* /// "bLevel()" */
static void bLevel(struct BTBatBinding *bb, UBYTE level)
{
    bb->bb_Level = (level > 100) ? 100 : level;
    bb->bb_Connected = TRUE;
    bTelAttach(bb);
}
/* \\\ */

/* /// "bBatteryTask()" */
AROS_UFH0(void, bBatteryTask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask = FindTask(NULL);
    struct BTBatBinding *bb = thistask->tc_UserData;
    struct Library *BluetoothBase;
    ULONG sigmask, sigs;
    BOOL running = TRUE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 45)))
    {
        Forbid();
        if(bb->bb_ReadySigTask)
        {
            Signal(bb->bb_ReadySigTask, 1UL<<bb->bb_ReadySignal);
        }
        return;
    }
    bb->bb_Base = BluetoothBase;

    do
    {
        if(!(bb->bb_TaskMsgPort = CreateMsgPort()) || !(bb->bb_EventPort = CreateMsgPort()) ||
           !(bb->bb_TimerPort = CreateMsgPort()))
        {
            break;
        }
        if(!(bb->bb_TimerReq = (struct timerequest *) CreateIORequest(bb->bb_TimerPort, sizeof(struct timerequest))))
        {
            break;
        }
        if(OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) bb->bb_TimerReq, 0))
        {
            break;
        }
        bb->bb_TimerOpen = TRUE;
        if(bb->bb_CanNotify)
        {
            if(!(bb->bb_NotifyCh = btAllocChannel(bb->bb_Device, bb->bb_TaskMsgPort, bb->bb_Endpoint)))
            {
                break;
            }
            btChannelSetup(bb->bb_NotifyCh, BTPR_READ, 0, 0);
        }
        if(!(bb->bb_ReadCh = btAllocChannel(bb->bb_Device, bb->bb_TaskMsgPort, NULL)))
        {
            break;
        }
        btChannelSetup(bb->bb_ReadCh, BTPR_GATTREAD, bb->bb_Handle, 0);
        bb->bb_EventHandler = btAddEventHandler(bb->bb_EventPort, BEHMF_DEVICECONNECTED|BEHMF_DEVICEDISCONNECTED);
        bb->bb_Task = thistask;
    } while(FALSE);

    Forbid();
    if(bb->bb_ReadySigTask)
    {
        Signal(bb->bb_ReadySigTask, 1UL<<bb->bb_ReadySignal);
    }
    Permit();

    if(bb->bb_Task)
    {
        /* a service is bound when its device has just been looked at, so it
           is most likely connected */
        bb->bb_Connected = TRUE;
        bArmTimer(bb, BTBAT_SETTLE_SECS);

        sigmask = (1UL<<bb->bb_TaskMsgPort->mp_SigBit) | (1UL<<bb->bb_EventPort->mp_SigBit) |
                  (1UL<<bb->bb_TimerPort->mp_SigBit) | SIGBREAKF_CTRL_C;
        while(running)
        {
            struct Message *msg;
            APTR ch;

            sigs = Wait(sigmask);
            while((ch = (APTR) GetMsg(bb->bb_TaskMsgPort)))
            {
                LONG err = btGetChannelError(ch);
                ULONG actual = btGetChannelActual(ch);

                if(ch == bb->bb_NotifyCh)
                {
                    bb->bb_NotifyPosted = FALSE;
                    if(!err && actual)
                    {
                        bLevel(bb, bb->bb_NotifyBuf[0]);
                        if(!(SetSignal(0, 0) & SIGBREAKF_CTRL_C))
                        {
                            btSendChannel(bb->bb_NotifyCh, bb->bb_NotifyBuf, sizeof(bb->bb_NotifyBuf));
                            bb->bb_NotifyPosted = TRUE;
                        }
                    }
                }
                else if(ch == bb->bb_ReadCh)
                {
                    bb->bb_ReadBusy = FALSE;
                    if(!err && actual)
                    {
                        bLevel(bb, bb->bb_ReadBuf[0]);
                    }
                }
            }
            while((msg = GetMsg(bb->bb_EventPort)))
            {
                IPTR ev = 0;
                APTR p1 = NULL;

                btGetAttrs(BGA_EVENTNOTE, msg, BENA_EventID, &ev, BENA_Param1, &p1, TAG_END);
                ReplyMsg(msg);
                if(p1 != (APTR) bb->bb_Device)
                {
                    continue;
                }
                if(ev == BEHMB_DEVICEDISCONNECTED)
                {
                    bb->bb_Connected = FALSE;
                    bb->bb_Level = -1;
                    bTelDetach(bb);
                }
                else if(ev == BEHMB_DEVICECONNECTED)
                {
                    bb->bb_Connected = TRUE;
                    bArmTimer(bb, BTBAT_SETTLE_SECS);
                }
            }
            if(sigs & (1UL<<bb->bb_TimerPort->mp_SigBit))
            {
                while(GetMsg(bb->bb_TimerPort))
                {
                    bb->bb_TimerPending = FALSE;
                }
                if(!bb->bb_TimerPending)
                {
                    if(bb->bb_Connected)
                    {
                        bAskLevel(bb);
                    }
                    bArmTimer(bb, (bb->bb_Level < 0) ? BTBAT_RETRY_SECS : BTBAT_REFRESH_SECS);
                }
            }
            if(sigs & SIGBREAKF_CTRL_C)
            {
                running = FALSE;
            }
        }
    }

    /* going down */
    bTelDetach(bb);
    if(bb->bb_EventHandler)
    {
        btRemEventHandler(bb->bb_EventHandler);
        bb->bb_EventHandler = NULL;
    }
    if(bb->bb_NotifyCh)
    {
        btAbortChannel(bb->bb_NotifyCh);
        btWaitChannel(bb->bb_NotifyCh);
        btFreeChannel(bb->bb_NotifyCh);
        bb->bb_NotifyCh = NULL;
    }
    if(bb->bb_ReadCh)
    {
        btAbortChannel(bb->bb_ReadCh);
        btWaitChannel(bb->bb_ReadCh);
        btFreeChannel(bb->bb_ReadCh);
        bb->bb_ReadCh = NULL;
    }
    if(bb->bb_TimerOpen)
    {
        if(bb->bb_TimerPending)
        {
            AbortIO((struct IORequest *) bb->bb_TimerReq);
            WaitIO((struct IORequest *) bb->bb_TimerReq);
        }
        CloseDevice((struct IORequest *) bb->bb_TimerReq);
        bb->bb_TimerOpen = FALSE;
    }
    if(bb->bb_TimerReq)
    {
        DeleteIORequest((struct IORequest *) bb->bb_TimerReq);
        bb->bb_TimerReq = NULL;
    }
    if(bb->bb_EventPort)
    {
        struct Message *msg;
        while((msg = GetMsg(bb->bb_EventPort)))
        {
            ReplyMsg(msg);
        }
        DeleteMsgPort(bb->bb_EventPort);
        bb->bb_EventPort = NULL;
    }
    if(bb->bb_TimerPort)
    {
        DeleteMsgPort(bb->bb_TimerPort);
        bb->bb_TimerPort = NULL;
    }
    if(bb->bb_TaskMsgPort)
    {
        DeleteMsgPort(bb->bb_TaskMsgPort);
        bb->bb_TaskMsgPort = NULL;
    }
    CloseLibrary(BluetoothBase);
    bb->bb_Base = NULL;
    Forbid();
    bb->bb_Task = NULL;
    if(bb->bb_ReadySigTask)
    {
        Signal(bb->bb_ReadySigTask, 1UL<<bb->bb_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */
