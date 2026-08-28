/*
 *----------------------------------------------------------------------------
 *                   btserial class for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * Serial Port Profile: binds to RFCOMM services (0x1101 Serial Port and
 * friends) of registered Bluetooth devices and exposes each as a unit of
 * btserial.device with the serial.device command set - the counterpart of
 * Poseidon's USB serial classes (cdcacm etc.). The RFCOMM multiplexer lives
 * in bluetooth.library; this class only reads and writes its endpoint
 * channels, buffering input in a ring so SDCMD_QUERY and partial CMD_READs
 * behave like a real serial port.
 */

#include "debug.h"

#include "btserial.h"

static const STRPTR libname = MOD_NAME_STRING;

/* /// "Lib Stuff" */
static
const APTR DevFuncTable[] =
{
    &AROS_SLIB_ENTRY(devOpen, dev, 1),
    &AROS_SLIB_ENTRY(devClose, dev, 2),
    &AROS_SLIB_ENTRY(devExpunge, dev, 3),
    &AROS_SLIB_ENTRY(devReserved, dev, 4),
    &AROS_SLIB_ENTRY(devBeginIO, dev, 5),
    &AROS_SLIB_ENTRY(devAbortIO, dev, 6),
    (APTR) -1,
};

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR nh)
{
    struct BTSerialBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define UtilityBase nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Units);

        if((nh->nh_DevBase = (struct BTSerDevBase *) MakeLibrary((APTR) DevFuncTable, NULL, (APTR) devInit,
           sizeof(struct BTSerDevBase), NULL)))
        {
            struct Library *BluetoothBase;
            nh->nh_DevBase->nsd_ClsBase = nh;
            Forbid();
            AddDevice((struct Device *) nh->nh_DevBase);
            nh->nh_DevBase->nsd_Library.lib_OpenCnt++;
            Permit();
            /* be a serial port for peers too: they connect to channel 1 and
               show up as incoming SPP services, bound like the outgoing ones */
            if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
            {
                nh->nh_Record = btAddServiceRecord(BSRA_UUID16, 0x1101, BSRA_Protocol, BSVP_RFCOMM,
                                                   BSRA_RFCOMMChannel, 1, BSRA_Name, (IPTR) "Serial Port",
                                                   TAG_END);
                CloseLibrary(BluetoothBase);
            }
            ret = nh;
        } else {
            KPRINTF(20, ("failed to create btserial.device\n"));
        }
        if(!ret)
        {
            CloseLibrary(UtilityBase);
        }
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }
    return(ret ? TRUE : FALSE);
}

static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR nh)
{
    struct BTSerialUnit *nsu;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_DevBase->nsd_Library.lib_OpenCnt == 1)
    {
        struct Library *BluetoothBase;
        if(nh->nh_Record && (BluetoothBase = OpenLibrary("bluetooth.library", 1)))
        {
            btRemServiceRecord(nh->nh_Record);
            nh->nh_Record = NULL;
            CloseLibrary(BluetoothBase);
        }
        CloseLibrary((struct Library *) UtilityBase);

        nsu = (struct BTSerialUnit *) nh->nh_Units.lh_Head;
        while(nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Succ)
        {
            Remove((struct Node *) nsu);
            FreeVec(nsu);
            nsu = (struct BTSerialUnit *) nh->nh_Units.lh_Head;
        }

        nh->nh_DevBase->nsd_Library.lib_OpenCnt--;
        RemDevice((struct Device *) nh->nh_DevBase);

        KPRINTF(5, ("libExpunge: Unloading done! btserial.class expunged!\n\n"));
    } else {
        KPRINTF(5, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        return(FALSE);
    }
    return(TRUE);
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
/* \\\ */

/* /// "bAttemptServiceBinding()" */
struct BTSerialUnit * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTSerialBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    IPTR uuid16 = 0;
    IPTR proto = 0;
    BOOL isser = FALSE;

    KPRINTF(1, ("bAttemptServiceBinding(%08lx)\n", bsv));
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        btGetAttrs(BGA_SERVICE, bsv,
                   BSVA_UUID16, &uuid16,
                   BSVA_Protocol, &proto,
                   TAG_END);
        CloseLibrary(BluetoothBase);
        /* 0x1101 Serial Port; 0x1103 Dialup Networking is a modem behind
           the same profile and works the same way */
        isser = (proto == BSVP_RFCOMM) && ((uuid16 == 0x1101) || (uuid16 == 0x1103));
    }
    return(isser ? GM_UNIQUENAME(bForceServiceBinding)(nh, bsv) : NULL);
}
/* \\\ */

/* /// "bForceServiceBinding()" */
struct BTSerialUnit * GM_UNIQUENAME(bForceServiceBinding)(struct BTSerialBase *nh, struct BtService *bsv)
{
    struct Library *BluetoothBase;
    struct BTSerialUnit *nsu;
    struct BtDevice *bd = NULL;
    struct BtEndpoint *bep;
    UBYTE *addr = NULL;
    IPTR channel = 0;
    STRPTR devname = NULL;
    ULONG unitno;
    BOOL unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("bForceServiceBinding(%08lx)\n", bsv));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return(NULL);
    }
    btGetAttrs(BGA_SERVICE, bsv, BSVA_Device, &bd, TAG_END);
    btGetAttrs(BGA_DEVICE, bd, BDA_Name, &devname, BDA_Address, &addr, TAG_END);
    bep = btFindEndpoint(bsv, NULL, BEA_Type, BEPT_RFCOMM, TAG_END);
    if(!bep)
    {
        CloseLibrary(BluetoothBase);
        return(NULL);
    }
    btGetAttrs(BGA_ENDPOINT, bep, BEA_RFCOMMChannel, &channel, TAG_END);

    Forbid();
    /* Find next free unit number */
    unitno = 0;
    nsu = (struct BTSerialUnit *) nh->nh_Units.lh_Head;
    while(nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Succ)
    {
        if(nsu->nsu_UnitNo == unitno)
        {
            unitno++;
            nsu = (struct BTSerialUnit *) nh->nh_Units.lh_Head;
        } else {
            nsu = (struct BTSerialUnit *) nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
    }
    /* the same remote serial port keeps its unit number across rebinds */
    unitfound = FALSE;
    nsu = (struct BTSerialUnit *) nh->nh_Units.lh_Head;
    while(nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Succ)
    {
        if(addr && !memcmp(nsu->nsu_UnitAddr, addr, 6) && (nsu->nsu_UnitChannel == channel))
        {
            if(nsu->nsu_Task)
            {
                /* already bound (a second record for the same port):
                   one unit, one task */
                Permit();
                KPRINTF(10, ("unit %ld already bound\n", nsu->nsu_UnitNo));
                CloseLibrary(BluetoothBase);
                return(NULL);
            }
            unitno = nsu->nsu_UnitNo;
            unitfound = TRUE;
            break;
        }
        nsu = (struct BTSerialUnit *) nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Succ;
    }
    if(!unitfound)
    {
        /* as units are freed in the expunge vector, the memory is outside
           the scope of bluetooth.library */
        if(!(nsu = AllocVec(sizeof(struct BTSerialUnit), MEMF_PUBLIC|MEMF_CLEAR)))
        {
            Permit();
            CloseLibrary(BluetoothBase);
            return(NULL);
        }
        /* IORequests may be queued even if the task is gone. */
        NewList(&nsu->nsu_Unit.unit_MsgPort.mp_MsgList);
        NewList(&nsu->nsu_ReadQueue);
        NewList(&nsu->nsu_WriteQueue);
        AddTail(&nh->nh_Units, &nsu->nsu_Unit.unit_MsgPort.mp_Node);
        nsu->nsu_DenyRequests = TRUE;
    }
    nsu->nsu_UnitNo = unitno;
    nsu->nsu_Device = bd;
    nsu->nsu_Service = bsv;
    nsu->nsu_Endpoint = bep;
    if(addr)
    {
        CopyMem(addr, nsu->nsu_UnitAddr, 6);
    }
    nsu->nsu_UnitChannel = channel;
    Permit();

    btSafeRawDoFmt(buf, 64, "btserial.class<%08lx>", (IPTR) nsu);
    nsu->nsu_ReadySignal = SIGB_SINGLE;
    nsu->nsu_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if((tmptask = btSpawnSubTask(buf, (APTR) GM_UNIQUENAME(bSerialTask), nsu)))
    {
        btBorrowLocksWait(tmptask, 1UL<<nsu->nsu_ReadySignal);
        if(nsu->nsu_Task)
        {
            nsu->nsu_ReadySigTask = NULL;
            btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Serial port on '%s' (channel %ld) at btserial.device unit %ld.",
                           devname ? devname : (STRPTR) "device", channel, nsu->nsu_UnitNo);
            CloseLibrary(BluetoothBase);
            return(nsu);
        }
    }
    nsu->nsu_ReadySigTask = NULL;
    /* the unit stays in the list for a later rebind */
    CloseLibrary(BluetoothBase);
    return(NULL);
}
/* \\\ */

/* /// "bReleaseServiceBinding()" */
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTSerialBase *nh, struct BTSerialUnit *nsu)
{
    struct Library *BluetoothBase;
    STRPTR devname = NULL;

    KPRINTF(1, ("bReleaseServiceBinding(%08lx)\n", nsu));
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        return;
    }
    Forbid();
    nsu->nsu_ReadySignal = SIGB_SINGLE;
    nsu->nsu_ReadySigTask = FindTask(NULL);
    if(nsu->nsu_Task)
    {
        Signal(nsu->nsu_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(nsu->nsu_Task)
    {
        Wait(1UL<<nsu->nsu_ReadySignal);
    }
    nsu->nsu_ReadySigTask = NULL;
    btGetAttrs(BGA_DEVICE, nsu->nsu_Device, BDA_Name, &devname, TAG_END);
    btAddErrorMsg(RETURN_OK, (STRPTR) libname,
                   "Serial port on '%s' released (unit %ld stays reserved).",
                   devname ? devname : (STRPTR) "device", nsu->nsu_UnitNo);
    CloseLibrary(BluetoothBase);
}
/* \\\ */

#undef UtilityBase
#define UtilityBase nh->nh_UtilityBase

/* /// "btcGetAttrsA()" */
AROS_LH3(LONG, btcGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, btstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, btserial)
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
                *((STRPTR *) ti->ti_Data) = "Serial ports (SPP) via btserial.device";
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
            struct BTSerialUnit *nsu = (struct BTSerialUnit *) btstruct;
            if((ti = FindTagItem(BCBA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            if((ti = FindTagItem(BCBA_Device, tags)))
            {
                *((struct BtDevice **) ti->ti_Data) = nsu->nsu_Device;
                count++;
            }
            if((ti = FindTagItem(BCBA_Service, tags)))
            {
                *((struct BtService **) ti->ti_Data) = nsu->nsu_Service;
                count++;
            }
            if((ti = FindTagItem(BCBA_Task, tags)))
            {
                *((struct Task **) ti->ti_Data) = nsu->nsu_Task;
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
         LIBBASETYPEPTR, nh, 6, btserial)
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
         LIBBASETYPEPTR, nh, 7, btserial)
{
    AROS_LIBFUNC_INIT

    switch(methodid)
    {
        case BCM_AttemptServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bAttemptServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ForceServiceBinding:
            return((SIPTR) GM_UNIQUENAME(bForceServiceBinding)(nh, (struct BtService *) methoddata[0]));

        case BCM_ReleaseServiceBinding:
            GM_UNIQUENAME(bReleaseServiceBinding)(nh, (struct BTSerialUnit *) methoddata[0]);
            return(TRUE);

        case BCM_AttemptDeviceBinding:
        case BCM_ForceDeviceBinding:
            return(0); /* only service bindings */

        case BCM_DeviceDisconnected:
            /* the unit stays; its channels fail and are re-issued when the
               device reconnects (BCHA_AutoConnect) */
            return(TRUE);

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
 * * The unit task                                                       *
 * ***********************************************************************
 */

#define RINGBYTES(nsu) ((nsu->nsu_RingHead - nsu->nsu_RingTail + BTSER_RINGSIZE) % BTSER_RINGSIZE)
#define RINGFREE(nsu)  (BTSER_RINGSIZE - 1 - RINGBYTES(nsu))

/* /// "bRingPut()" */
static void bRingPut(struct BTSerialUnit *nsu, const UBYTE *data, ULONG len)
{
    while(len--)
    {
        ULONG next = (nsu->nsu_RingHead + 1) % BTSER_RINGSIZE;
        if(next == nsu->nsu_RingTail)
        {
            break;   /* overrun: oldest data wins, rest is dropped */
        }
        nsu->nsu_Ring[nsu->nsu_RingHead] = *data++;
        nsu->nsu_RingHead = next;
    }
}
/* \\\ */

/* /// "bRingGet()" */
static ULONG bRingGet(struct BTSerialUnit *nsu, UBYTE *data, ULONG len)
{
    ULONG done = 0;
    while(len-- && (nsu->nsu_RingTail != nsu->nsu_RingHead))
    {
        *data++ = nsu->nsu_Ring[nsu->nsu_RingTail];
        nsu->nsu_RingTail = (nsu->nsu_RingTail + 1) % BTSER_RINGSIZE;
        done++;
    }
    return(done);
}
/* \\\ */

/* /// "bServeReads()" */
static void bServeReads(struct BTSerialUnit *nsu)
{
    struct IOExtSer *ioreq;

    Forbid();
    ioreq = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
    while(ioreq->IOSer.io_Message.mn_Node.ln_Succ && RINGBYTES(nsu))
    {
        ioreq->IOSer.io_Actual += bRingGet(nsu, ((UBYTE *) ioreq->IOSer.io_Data) + ioreq->IOSer.io_Actual,
                                           ioreq->IOSer.io_Length - ioreq->IOSer.io_Actual);
        if(ioreq->IOSer.io_Actual >= ioreq->IOSer.io_Length)
        {
            Remove((struct Node *) ioreq);
            Permit();
            ReplyMsg((struct Message *) ioreq);
            Forbid();
            ioreq = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
        } else {
            break;   /* out of buffered data, stays queued */
        }
    }
    Permit();
}
/* \\\ */

/* /// "bServeWrites()" */
static void bServeWrites(struct BTSerialUnit *nsu)
{
    struct Library *BluetoothBase = nsu->nsu_Base;
    struct IOExtSer *ioreq;
    IPTR mtu = 0;
    ULONG chunk;

    if(nsu->nsu_WriteBusy)
    {
        return;
    }
    if(!(ioreq = nsu->nsu_WritePending))
    {
        Forbid();
        ioreq = (struct IOExtSer *) RemHead(&nsu->nsu_WriteQueue);
        Permit();
        if(!ioreq)
        {
            return;
        }
        nsu->nsu_WritePending = ioreq;
        nsu->nsu_WriteOffset = 0;
    }
    btGetAttrs(BGA_ENDPOINT, nsu->nsu_Endpoint, BEA_MaxPktSize, &mtu, TAG_END);
    if(!mtu)
    {
        mtu = BTSER_CHUNK;
    }
    chunk = ioreq->IOSer.io_Length - nsu->nsu_WriteOffset;
    if(chunk > mtu)
    {
        chunk = mtu;
    }
    btSendChannel(nsu->nsu_WriteCh, ((UBYTE *) ioreq->IOSer.io_Data) + nsu->nsu_WriteOffset, chunk);
    nsu->nsu_WriteBusy = TRUE;
}
/* \\\ */

/* /// "bSerialTask()" */
AROS_UFH0(void, GM_UNIQUENAME(bSerialTask))
{
    AROS_USERFUNC_INIT

    struct Task *thistask = FindTask(NULL);
    struct BTSerialUnit *nsu = thistask->tc_UserData;
    struct Library *BluetoothBase;
    ULONG sigmask, sigs;
    struct IOExtSer *ioreq;
    struct IOExtSer *ioreq2;
    BOOL running = TRUE;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        Forbid();
        if(nsu->nsu_ReadySigTask)
        {
            Signal(nsu->nsu_ReadySigTask, 1UL<<nsu->nsu_ReadySignal);
        }
        return;
    }
    nsu->nsu_Base = BluetoothBase;

    do
    {
        if(!(nsu->nsu_TaskMsgPort = CreateMsgPort()))
        {
            break;
        }
        nsu->nsu_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
        if(nsu->nsu_Unit.unit_MsgPort.mp_SigBit == (UBYTE) -1)
        {
            break;
        }
        nsu->nsu_Unit.unit_MsgPort.mp_SigTask = thistask;
        nsu->nsu_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        nsu->nsu_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

        if(!(nsu->nsu_ReadCh = btAllocChannel(nsu->nsu_Device, nsu->nsu_TaskMsgPort, nsu->nsu_Endpoint)))
        {
            break;
        }
        if(!(nsu->nsu_WriteCh = btAllocChannel(nsu->nsu_Device, nsu->nsu_TaskMsgPort, nsu->nsu_Endpoint)))
        {
            break;
        }
        btSetAttrs(BGA_CHANNEL, nsu->nsu_ReadCh, BCHA_AutoConnect, TRUE, TAG_END);
        btSetAttrs(BGA_CHANNEL, nsu->nsu_WriteCh, BCHA_AutoConnect, TRUE, TAG_END);
        btChannelSetup(nsu->nsu_ReadCh, BTPR_READ, 0, 0);
        btChannelSetup(nsu->nsu_WriteCh, BTPR_WRITE, 0, 0);
        btSendChannel(nsu->nsu_ReadCh, nsu->nsu_ReadBuf, sizeof(nsu->nsu_ReadBuf));
        nsu->nsu_ReadPosted = TRUE;
        nsu->nsu_RingHead = nsu->nsu_RingTail = 0;
        nsu->nsu_Task = thistask;
    } while(FALSE);

    Forbid();
    if(nsu->nsu_ReadySigTask)
    {
        Signal(nsu->nsu_ReadySigTask, 1UL<<nsu->nsu_ReadySignal);
    }
    Permit();

    if(nsu->nsu_Task)
    {
        sigmask = (1UL<<nsu->nsu_Unit.unit_MsgPort.mp_SigBit) |
                  (1UL<<nsu->nsu_TaskMsgPort->mp_SigBit) | SIGBREAKF_CTRL_C;
        while(running)
        {
            APTR ch;
            sigs = Wait(sigmask);
            while((ch = (APTR) GetMsg(nsu->nsu_TaskMsgPort)))
            {
                if(ch == nsu->nsu_ReadCh)
                {
                    LONG err = btGetChannelError(ch);
                    nsu->nsu_ReadPosted = FALSE;
                    if(!err)
                    {
                        bRingPut(nsu, nsu->nsu_ReadBuf, btGetChannelActual(ch));
                    }
                    else if((err == IOERR_ABORTED) || (err == BTIOERR_NOTCONNECTED) || (err == BTIOERR_DISCONNECTED))
                    {
                        btDelayMS(1000);   /* link down; auto connect brings it back */
                    } else {
                        btDelayMS(250);
                    }
                    if(!(SetSignal(0, 0) & SIGBREAKF_CTRL_C))
                    {
                        btSendChannel(nsu->nsu_ReadCh, nsu->nsu_ReadBuf, sizeof(nsu->nsu_ReadBuf));
                        nsu->nsu_ReadPosted = TRUE;
                    }
                }
                else if(ch == nsu->nsu_WriteCh)
                {
                    LONG err = btGetChannelError(ch);
                    nsu->nsu_WriteBusy = FALSE;
                    if((ioreq = nsu->nsu_WritePending))
                    {
                        if(err)
                        {
                            ioreq->IOSer.io_Error = SerErr_LineErr;
                            ioreq->IOSer.io_Actual = nsu->nsu_WriteOffset;
                            nsu->nsu_WritePending = NULL;
                            ReplyMsg((struct Message *) ioreq);
                        } else {
                            nsu->nsu_WriteOffset += btGetChannelActual(ch);
                            if(nsu->nsu_WriteAbort)
                            {
                                nsu->nsu_WriteAbort = FALSE;
                                ioreq->IOSer.io_Error = IOERR_ABORTED;
                                ioreq->IOSer.io_Actual = nsu->nsu_WriteOffset;
                                nsu->nsu_WritePending = NULL;
                                ReplyMsg((struct Message *) ioreq);
                            }
                            else if(nsu->nsu_WriteOffset >= ioreq->IOSer.io_Length)
                            {
                                ioreq->IOSer.io_Actual = nsu->nsu_WriteOffset;
                                nsu->nsu_WritePending = NULL;
                                ReplyMsg((struct Message *) ioreq);
                            }
                        }
                    }
                }
            }
            while((ioreq = (struct IOExtSer *) GetMsg(&nsu->nsu_Unit.unit_MsgPort)))
            {
                switch(ioreq->IOSer.io_Command)
                {
                    case CMD_CLEAR:
                        nsu->nsu_RingHead = nsu->nsu_RingTail = 0;
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case CMD_RESET:
                    case CMD_FLUSH:
                        Forbid();
                        ioreq2 = (struct IOExtSer *) nsu->nsu_WriteQueue.lh_Head;
                        while(ioreq2->IOSer.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOSer.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtSer *) nsu->nsu_WriteQueue.lh_Head;
                        }
                        ioreq2 = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
                        while(ioreq2->IOSer.io_Message.mn_Node.ln_Succ)
                        {
                            Remove((struct Node *) ioreq2);
                            ioreq2->IOSer.io_Error = IOERR_ABORTED;
                            ReplyMsg((struct Message *) ioreq2);
                            ioreq2 = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
                        }
                        Permit();
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case SDCMD_QUERY:
                        ioreq->io_Status = 0;   /* carrier present, all clear */
                        ioreq->IOSer.io_Actual = RINGBYTES(nsu);
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case SDCMD_SETPARAMS:
                        /* baud rate and friends have no meaning on RFCOMM */
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    case SDCMD_BREAK:
                        ReplyMsg((struct Message *) ioreq);
                        break;

                    default:
                        ioreq->IOSer.io_Error = IOERR_NOCMD;
                        ReplyMsg((struct Message *) ioreq);
                        break;
                }
            }
            if(!nsu->nsu_DevSuspend)
            {
                bServeReads(nsu);
                bServeWrites(nsu);
            }
            if(sigs & SIGBREAKF_CTRL_C)
            {
                running = FALSE;
            }
        }
    }

    /* going down: deny and flush everything */
    nsu->nsu_DenyRequests = TRUE;
    if(nsu->nsu_ReadCh)
    {
        btAbortChannel(nsu->nsu_ReadCh);
        btWaitChannel(nsu->nsu_ReadCh);
        btFreeChannel(nsu->nsu_ReadCh);
        nsu->nsu_ReadCh = NULL;
    }
    if(nsu->nsu_WriteCh)
    {
        btAbortChannel(nsu->nsu_WriteCh);
        btWaitChannel(nsu->nsu_WriteCh);
        btFreeChannel(nsu->nsu_WriteCh);
        nsu->nsu_WriteCh = NULL;
    }
    Forbid();
    if((ioreq = nsu->nsu_WritePending))
    {
        nsu->nsu_WritePending = NULL;
        ioreq->IOSer.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    ioreq = (struct IOExtSer *) nsu->nsu_WriteQueue.lh_Head;
    while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
    {
        Remove((struct Node *) ioreq);
        ioreq->IOSer.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
        ioreq = (struct IOExtSer *) nsu->nsu_WriteQueue.lh_Head;
    }
    ioreq = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
    while(ioreq->IOSer.io_Message.mn_Node.ln_Succ)
    {
        Remove((struct Node *) ioreq);
        ioreq->IOSer.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
        ioreq = (struct IOExtSer *) nsu->nsu_ReadQueue.lh_Head;
    }
    /* Disable the message port, messages may still be queued */
    nsu->nsu_Unit.unit_MsgPort.mp_SigTask = NULL;
    nsu->nsu_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    if(nsu->nsu_Unit.unit_MsgPort.mp_SigBit != (UBYTE) -1)
    {
        FreeSignal((LONG) nsu->nsu_Unit.unit_MsgPort.mp_SigBit);
        nsu->nsu_Unit.unit_MsgPort.mp_SigBit = (UBYTE) -1;
    }
    while((ioreq = (struct IOExtSer *) GetMsg(&nsu->nsu_Unit.unit_MsgPort)))
    {
        ioreq->IOSer.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *) ioreq);
    }
    Permit();
    if(nsu->nsu_TaskMsgPort)
    {
        DeleteMsgPort(nsu->nsu_TaskMsgPort);
        nsu->nsu_TaskMsgPort = NULL;
    }
    CloseLibrary(BluetoothBase);
    nsu->nsu_Base = NULL;
    Forbid();
    nsu->nsu_Task = NULL;
    if(nsu->nsu_ReadySigTask)
    {
        Signal(nsu->nsu_ReadySigTask, 1UL<<nsu->nsu_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */
