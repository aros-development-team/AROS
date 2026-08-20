/*
 *----------------------------------------------------------------------------
 *                    bthid class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * Binds to HID services of registered Bluetooth devices: HID over GATT
 * (service UUID 0x1812) on LE links now, classic HIDP (UUID 0x1124) once
 * the RFCOMM/HIDP interrupt channel handling is in place. A task per
 * binding reads the report map through the control channel, subscribes to
 * the input report endpoints and feeds the decoded events into
 * input.device, using the shared HID parser of libbtcore.
 */

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/exec.h>
#include <exec/errors.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>
#include <proto/input.h>

#include <string.h>

#include "bthid.h"

#include LC_LIBDEFS_FILE

#define NewList(list) NEWLIST(list)

static const STRPTR libname = MOD_NAME_STRING;

AROS_UFP0(void, bthidTask);

/* /// "libInit()" */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);
    if(!nh->nh_UtilityBase) {
        return FALSE;
    }
    NewList(&nh->nh_Bindings);
    return TRUE;
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    if(nh->nh_Bindings.lh_Head->ln_Succ) {
        return FALSE;
    }
    CloseLibrary(nh->nh_UtilityBase);
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
/* \\\ */

#define UtilityBase nh->nh_UtilityBase

/* /// "btcAttemptServiceBinding()" */
static struct BTHidBinding * btcAttemptServiceBinding(struct BTHidBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    IPTR uuid16 = 0;
    IPTR proto = 0;
    struct BtDevice *bd = NULL;
    struct BTHidBinding *nhb = NULL;
    BOOL classic = FALSE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1))) {
        return NULL;
    }
    btGetAttrs(BGA_SERVICE, bsv,
               BSVA_UUID16, &uuid16,
               BSVA_Protocol, &proto,
               BSVA_Device, &bd,
               TAG_END);
    if((uuid16 == 0x1812) && (proto == BSVP_ATT)) {
        classic = FALSE;         /* HID over GATT (HOGP) */
    } else if((uuid16 == 0x1124) && (proto == BSVP_L2CAP)) {
        classic = TRUE;          /* HID over L2CAP (HIDP) */
    } else {
        CloseLibrary(BluetoothBase);
        return NULL;
    }

    /* One HID binding per physical device. A dual-mode device offers an HID
       service on BOTH bearers (HIDP and HOGP) but it is one keyboard/mouse -
       binding both would double every keystroke, so refuse a second. */
    {
        struct BTHidBinding *ex;
        BOOL have = FALSE;
        Forbid();
        for(ex = (struct BTHidBinding *) nh->nh_Bindings.lh_Head;
            ex->nhb_Node.ln_Succ; ex = (struct BTHidBinding *) ex->nhb_Node.ln_Succ) {
            if(ex->nhb_Device == bd) {
                have = TRUE;
                break;
            }
        }
        Permit();
        if(have) {
            CloseLibrary(BluetoothBase);
            return NULL;
        }
    }

    if((nhb = btAllocVec(sizeof(struct BTHidBinding)))) {
        struct Task *tmptask;
        char buf[64];
        nhb->nhb_ClsBase = nh;
        nhb->nhb_Device = bd;
        nhb->nhb_Service = bsv;
        nhb->nhb_Classic = classic;
        btSafeRawDoFmt(buf, 64, "bthid.class<%08lx>", (IPTR) nhb);
        nhb->nhb_ReadySignal = SIGB_SINGLE;
        nhb->nhb_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = btSpawnSubTask(buf, (APTR) bthidTask, nhb))) {
            btBorrowLocksWait(tmptask, 1UL<<nhb->nhb_ReadySignal);
            if(nhb->nhb_Task) {
                STRPTR devname = NULL;
                nhb->nhb_ReadySigTask = NULL;
                Forbid();
                AddTail(&nh->nh_Bindings, &nhb->nhb_Node);
                Permit();
                btGetAttrs(BGA_DEVICE, bd, BDA_Name, &devname, TAG_END);
                btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "HID input from '%s' connected to input.device.",
                               devname ? devname : (STRPTR) "device");
                CloseLibrary(BluetoothBase);
                return nhb;
            }
        }
        nhb->nhb_ReadySigTask = NULL;
        btFreeVec(nhb);
    }
    CloseLibrary(BluetoothBase);
    return NULL;
}
/* \\\ */

/* /// "btcReleaseServiceBinding()" */
static void btcReleaseServiceBinding(struct BTHidBase *nh, struct BTHidBinding *nhb)
{
    struct Library *BluetoothBase;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1))) {
        return;
    }
    Forbid();
    nhb->nhb_ReadySignal = SIGB_SINGLE;
    nhb->nhb_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if(nhb->nhb_Task) {
        Signal(nhb->nhb_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(nhb->nhb_Task) {
        Wait(1UL<<nhb->nhb_ReadySignal);
    }
    Forbid();
    Remove(&nhb->nhb_Node);
    Permit();
    btFreeVec(nhb);
    CloseLibrary(BluetoothBase);
}
/* \\\ */

/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, bthid)
{
    AROS_LIBFUNC_INIT
    struct TagItem *ti;
    LONG count = 0;

    switch(type) {
    case BCGA_CLASS:
        if((ti = FindTagItem(BCCA_Priority, tags))) {
            *((SIPTR *) ti->ti_Data) = 0;
            count++;
        }
        if((ti = FindTagItem(BCCA_Description, tags))) {
            *((STRPTR *) ti->ti_Data) = "HID keyboards and mice via input.device";
            count++;
        }
        if((ti = FindTagItem(BCCA_HasClassCfgGUI, tags))) {
            *((IPTR *) ti->ti_Data) = FALSE;
            count++;
        }
        if((ti = FindTagItem(BCCA_HasBindingCfgGUI, tags))) {
            *((IPTR *) ti->ti_Data) = FALSE;
            count++;
        }
        if((ti = FindTagItem(BCCA_AfterDOSRestart, tags))) {
            *((IPTR *) ti->ti_Data) = FALSE;
            count++;
        }
        if((ti = FindTagItem(BCCA_UsingDefaultCfg, tags))) {
            *((IPTR *) ti->ti_Data) = TRUE;
            count++;
        }
        break;

    case BCGA_BINDING: {
        struct BTHidBinding *nhb = (struct BTHidBinding *) btstruct;
        if((ti = FindTagItem(BCBA_UsingDefaultCfg, tags))) {
            *((IPTR *) ti->ti_Data) = TRUE;
            count++;
        }
        if((ti = FindTagItem(BCBA_Device, tags))) {
            *((struct BtDevice **) ti->ti_Data) = nhb->nhb_Device;
            count++;
        }
        if((ti = FindTagItem(BCBA_Service, tags))) {
            *((struct BtService **) ti->ti_Data) = nhb->nhb_Service;
            count++;
        }
        if((ti = FindTagItem(BCBA_Task, tags))) {
            *((struct Task **) ti->ti_Data) = nhb->nhb_Task;
            count++;
        }
        break;
    }
    }
    return count;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcSetAttrsA()" */
AROS_LH3(LONG, btcSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, bthid)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btcDoMethodA()" */
AROS_LH2(SIPTR, btcDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, bthid)
{
    AROS_LIBFUNC_INIT

    switch(methodid) {
    case BCM_AttemptServiceBinding:
    case BCM_ForceServiceBinding:
        return (SIPTR) btcAttemptServiceBinding(nh, (struct BtService *) methoddata[0]);

    case BCM_ReleaseServiceBinding:
        btcReleaseServiceBinding(nh, (struct BTHidBinding *) methoddata[0]);
        return TRUE;

    case BCM_AttemptDeviceBinding:
    case BCM_ForceDeviceBinding:
        return 0; /* only service bindings */

    case BCM_DeviceDisconnected:
        /* the binding stays; its read channels fail and are re-issued when
           the device reconnects (BCHA_AutoConnect) */
        return TRUE;

    default:
        break;
    }
    return 0;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/*
 * ***********************************************************************
 * * The binding task                                                    *
 * ***********************************************************************
 */

#undef UtilityBase

struct BTHidTaskCtx
{
    struct BTHidBinding *ctx_Binding;
    struct Library *ctx_BluetoothBase;
};

/* /// "bthidEmitInput()" */
static bt_status_t bthidEmitInput(void *context, const struct bt_aros_input_event *event)
{
    struct BTHidBinding *nhb = context;
    struct InputEvent iev;

    if(!nhb->nhb_InputOpen) {
        return BT_ERR_IO;
    }
    memset(&iev, 0, sizeof(iev));
    switch(event->event_class) {
    case BT_AROS_IECLASS_RAWKEY:
        iev.ie_Class = IECLASS_RAWKEY;
        iev.ie_Code = event->code;
        iev.ie_Qualifier = event->qualifier;
        break;
    case BT_AROS_IECLASS_RAWMOUSE:
        iev.ie_Class = IECLASS_RAWMOUSE;
        iev.ie_Code = event->code;
        iev.ie_Qualifier = event->qualifier | IEQUALIFIER_RELATIVEMOUSE;
        iev.ie_X = event->x;
        iev.ie_Y = event->y;
        break;
    default:
        return BT_OK;
    }
    nhb->nhb_InputIO->io_Command = IND_WRITEEVENT;
    nhb->nhb_InputIO->io_Data = &iev;
    nhb->nhb_InputIO->io_Length = sizeof(iev);
    DoIO((struct IORequest *) nhb->nhb_InputIO);
    return BT_OK;
}
/* \\\ */

/* /// "bthidReadReportMap()" */
static BOOL bthidReadReportMap(struct BTHidBinding *nhb, struct Library *BluetoothBase)
{
    APTR ch;
    UBYTE mapbuf[512];
    const UBYTE *map = mapbuf;
    LONG err = -1;
    ULONG actual = 0;
    struct BtEndpoint *bep = NULL;
    IPTR handle = 0;

    if(nhb->nhb_Classic) {
        /* classic HIDP: the report descriptor came from SDP (HIDDescriptorList),
           stored on the service - no channel read needed. */
        IPTR desc = 0, desclen = 0;
        btGetAttrs(BGA_SERVICE, nhb->nhb_Service,
                   BSVA_HIDDescriptor, &desc,
                   BSVA_HIDDescriptorLen, &desclen,
                   TAG_END);
        if(!desc || !desclen) {
            return FALSE;
        }
        map = (const UBYTE *) desc;
        actual = (ULONG) desclen;
        err = 0;
    } else {
        /* HID over GATT: read the Report Map characteristic (0x2a4b). */
        bep = btFindEndpoint(nhb->nhb_Service, NULL, BEA_UUID16, 0x2a4b, TAG_END);
        if(!bep) {
            return FALSE;
        }
        if((ch = btAllocChannel(nhb->nhb_Device, nhb->nhb_ChannelPort, NULL))) {
            btGetAttrs(BGA_ENDPOINT, bep, BEA_Handle, &handle, TAG_END);
            btSetAttrs(BGA_CHANNEL, ch, BCHA_AutoConnect, TRUE, TAG_END);
            btChannelSetup(ch, BTPR_GATTREAD, handle, 0);
            err = btDoChannel(ch, mapbuf, sizeof(mapbuf));
            actual = btGetChannelActual(ch);
            btFreeChannel(ch);
        }
    }
    if(err) {
        return FALSE;
    }
    if(bt_hid_report_parse(map, actual, &nhb->nhb_Descriptor) != BT_OK) {
        return FALSE;
    }
    bt_hid_input_init(&nhb->nhb_Input, &nhb->nhb_Descriptor);
    bt_aros_input_bridge_init(&nhb->nhb_Bridge, bthidEmitInput, nhb);
    nhb->nhb_HaveDescriptor = TRUE;
    return TRUE;
}
/* \\\ */

/* /// "bthidTask()" */
AROS_UFH0(void, bthidTask)
{
    AROS_USERFUNC_INIT
    struct Task *thistask = FindTask(NULL);
    struct BTHidBinding *nhb = thistask->tc_UserData;
    struct Library *BluetoothBase;
    struct BtEndpoint *reportep = NULL;
    APTR readch[2] = { NULL, NULL };
    UBYTE readbuf[2][64];
    UWORD numch = 0;
    UWORD n;
    ULONG sigmask;
    ULONG sigs;
    BOOL running = TRUE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1))) {
        Forbid();
        if(nhb->nhb_ReadySigTask) {
            Signal(nhb->nhb_ReadySigTask, 1UL<<nhb->nhb_ReadySignal);
        }
        return;
    }
    nhb->nhb_Base = BluetoothBase;

    do {
        if(!(nhb->nhb_ChannelPort = CreateMsgPort())) {
            break;
        }
        if(!(nhb->nhb_InputPort = CreateMsgPort())) {
            break;
        }
        nhb->nhb_InputIO = (struct IOStdReq *) CreateIORequest(nhb->nhb_InputPort, sizeof(struct IOStdReq));
        if(!nhb->nhb_InputIO) {
            break;
        }
        if(OpenDevice("input.device", 0, (struct IORequest *) nhb->nhb_InputIO, 0)) {
            break;
        }
        nhb->nhb_InputOpen = TRUE;

        if(!bthidReadReportMap(nhb, BluetoothBase)) {
            btAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Could not read the HID report map.");
            break;
        }

        if(nhb->nhb_Classic) {
            /* HIDP: read input reports from the interrupt channel (PSM 0x13).
               Each SDU is prefixed with the HIDP header byte (0xa1 = DATA,
               Input report), stripped before parsing. Opening the channel
               (BCHA_AutoConnect) brings the L2CAP link up and makes the peer
               start streaming. */
            reportep = btFindEndpoint(nhb->nhb_Service, NULL, BEA_PSM, 0x0013, TAG_END);
            if(reportep && (readch[0] = btAllocChannel(nhb->nhb_Device, nhb->nhb_ChannelPort, reportep))) {
                btSetAttrs(BGA_CHANNEL, readch[0], BCHA_AutoConnect, TRUE, TAG_END);
                btChannelSetup(readch[0], BTPR_READ, 0, 0);
                btSendChannel(readch[0], readbuf[0], sizeof(readbuf[0]));
                numch = 1;
            }
        } else {
            /* HOGP: subscribe to every readable Report characteristic (0x2a4d) */
            while((reportep = btFindEndpoint(nhb->nhb_Service, reportep, BEA_UUID16, 0x2a4d, BEA_CanRead, TRUE, TAG_END))) {
                if(numch >= 2) {
                    break;
                }
                if((readch[numch] = btAllocChannel(nhb->nhb_Device, nhb->nhb_ChannelPort, reportep))) {
                    btSetAttrs(BGA_CHANNEL, readch[numch], BCHA_AutoConnect, TRUE, TAG_END);
                    btChannelSetup(readch[numch], BTPR_READ, 0, 0);
                    btSendChannel(readch[numch], readbuf[numch], sizeof(readbuf[numch]));
                    numch++;
                }
            }
        }
        if(!numch) {
            btAddErrorMsg(RETURN_WARN, (STRPTR) libname, "No input report endpoint found.");
            break;
        }

        nhb->nhb_Task = thistask;
    } while(FALSE);

    Forbid();
    if(nhb->nhb_ReadySigTask) {
        Signal(nhb->nhb_ReadySigTask, 1UL<<nhb->nhb_ReadySignal);
    }
    Permit();

    if(nhb->nhb_Task) {
        sigmask = (1UL<<nhb->nhb_ChannelPort->mp_SigBit) | SIGBREAKF_CTRL_C;
        while(running) {
            APTR ch;
            sigs = Wait(sigmask);
            while((ch = (APTR) GetMsg(nhb->nhb_ChannelPort))) {
                for(n = 0; n < numch; n++) {
                    if(ch == readch[n]) {
                        LONG err = btGetChannelError(ch);
                        if(!err) {
                            ULONG actual = btGetChannelActual(ch);
                            const UBYTE *rep = readbuf[n];
                            if(nhb->nhb_Classic && (actual >= 1)) {
                                /* drop the HIDP header byte (0xa1 = DATA/Input) */
                                rep++;
                                actual--;
                            }
                            bt_hid_input_process(&nhb->nhb_Input, rep, actual,
                                                 bt_aros_input_bridge_handle, &nhb->nhb_Bridge);
                        } else if((err == IOERR_ABORTED) || (err == BTIOERR_NOTCONNECTED)) {
                            /* device gone: wait for reconnect via auto connect */
                            btDelayMS(1000);
                        } else {
                            btDelayMS(250);
                        }
                        if(!(SetSignal(0, 0) & SIGBREAKF_CTRL_C)) {
                            btSendChannel(ch, readbuf[n], sizeof(readbuf[n]));
                        }
                        break;
                    }
                }
            }
            if(sigs & SIGBREAKF_CTRL_C) {
                running = FALSE;
            }
        }
    }

    /* cleanup */
    for(n = 0; n < numch; n++) {
        if(readch[n]) {
            btAbortChannel(readch[n]);
            btWaitChannel(readch[n]);
            btFreeChannel(readch[n]);
        }
    }
    if(nhb->nhb_InputOpen) {
        CloseDevice((struct IORequest *) nhb->nhb_InputIO);
    }
    if(nhb->nhb_InputIO) {
        DeleteIORequest((struct IORequest *) nhb->nhb_InputIO);
    }
    if(nhb->nhb_InputPort) {
        DeleteMsgPort(nhb->nhb_InputPort);
    }
    if(nhb->nhb_ChannelPort) {
        DeleteMsgPort(nhb->nhb_ChannelPort);
    }
    CloseLibrary(BluetoothBase);
    Forbid();
    nhb->nhb_Task = NULL;
    if(nhb->nhb_ReadySigTask) {
        Signal(nhb->nhb_ReadySigTask, 1UL<<nhb->nhb_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */
