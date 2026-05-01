/*
 *----------------------------------------------------------------------------
 *                        lan78xx class for poseidon
 *----------------------------------------------------------------------------
 * Copyright (C) 2026, The AROS Development Team.
 *
 * Substantial portions of the chip-init, MII access, RX/TX framing and
 * link-watcher logic in this file are ports of OpenBSD's if_mue.c
 * (Copyright (c) 2018 Kevin Lo <kevlo@openbsd.org>), redistributed here
 * under the ISC licence:
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "debug.h"

#include "lan78xx.class.h"

static const STRPTR libname = MOD_NAME_STRING;

static const APTR DevFuncTable[] = {
    &AROS_SLIB_ENTRY(devOpen, dev, 1),
    &AROS_SLIB_ENTRY(devClose, dev, 2),
    &AROS_SLIB_ENTRY(devExpunge, dev, 3),
    &AROS_SLIB_ENTRY(devReserved, dev, 4),
    &AROS_SLIB_ENTRY(devBeginIO, dev, 5),
    &AROS_SLIB_ENTRY(devAbortIO, dev, 6),
    (APTR)-1,
};

static BOOL lan78xx_supported_device(IPTR vendid, IPTR prodid);
static UWORD lan78xx_flags_from_product(IPTR prodid);
static BOOL lan78xx_pick_interface(struct NepClassEth *ncp);
static void lan78xx_signal_ready(struct NepClassEth *ncp);
static void lan78xx_seed_mac(struct NepClassEth *ncp);
static BOOL lan78xx_mac_valid(const UBYTE *mac);
static void lan78xx_store_mac(struct NepClassEth *ncp, ULONG lo, ULONG hi);
static BOOL lan78xx_wait_reg_clear(struct NepClassEth *ncp, UWORD reg, ULONG mask);
static BOOL lan78xx_wait_reg_set(struct NepClassEth *ncp, UWORD reg, ULONG mask);
static LONG lan78xx_chip_init(struct NepClassEth *ncp);
static LONG lan78xx_eeprom_wait(struct NepClassEth *ncp);
static LONG lan78xx_eeprom_read(struct NepClassEth *ncp, UWORD offset, UWORD len, UBYTE *dest);
static void lan78xx_set_macaddr(struct NepClassEth *ncp);
static void lan78xx_handle_rx_buffer(struct NepClassEth *ncp, UBYTE *buf, ULONG total_len);
static LONG lan78xx_dataport_wait(struct NepClassEth *ncp);
static LONG lan78xx_dataport_write(struct NepClassEth *ncp, ULONG sel, ULONG addr, ULONG cnt, const ULONG *data);
static ULONG lan78xx_ether_crc32_be(const UBYTE *buf, int len);
static void lan78xx_link_poll(struct NepClassEth *ncp);

static inline void *callcopy(APTR routine, APTR to, APTR from, ULONG len)
{
    void *(*call)(APTR, APTR, ULONG) = (void *(*)(APTR, APTR, ULONG))routine;
    return (*call)(to, from, len);
}

#define callfilter(hook, obj, msg) CALLHOOKPKT((hook), (obj), (msg))

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepClassEth *ncp;

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define UtilityBase nh->nh_UtilityBase

    if (!UtilityBase) {
        return (FALSE);
    }

    NewList(&nh->nh_Units);

    nh->nh_DevBase = (struct NepEthDevBase *)MakeLibrary((APTR)DevFuncTable, NULL, (APTR)devInit,
                                                         sizeof(struct NepEthDevBase), NULL);
    if (!nh->nh_DevBase) {
        CloseLibrary(UtilityBase);
        return (FALSE);
    }

    ncp = &nh->nh_DummyNCP;
    ncp->ncp_ClsBase = nh;
    ncp->ncp_Interface = NULL;
    ncp->ncp_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC | MEMF_CLEAR);
    if (!ncp->ncp_CDC) {
        CloseLibrary(UtilityBase);
        return (FALSE);
    }

    nh->nh_DevBase->np_ClsBase = nh;
    Forbid();
    AddDevice((struct Device *)nh->nh_DevBase);
    nh->nh_DevBase->np_Library.lib_OpenCnt++;
    Permit();

    return (TRUE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    nLoadClassConfig(nh);
    return (TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    struct NepClassEth *ncp;

    if (nh->nh_DevBase->np_Library.lib_OpenCnt != 1) {
        return (FALSE);
    }

    CloseLibrary((struct Library *)UtilityBase);

    ncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
    while (ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ) {
        Remove((struct Node *)ncp);
        FreeVec(ncp->ncp_CDC);
        FreeVec(ncp);
        ncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
    }

    nh->nh_DevBase->np_Library.lib_OpenCnt--;
    RemDevice((struct Device *)nh->nh_DevBase);
    return (TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2OPENLIB(libOpen, 0)
ADD2EXPUNGELIB(libExpunge, 0)

struct NepClassEth *usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    IPTR prodid = 0;
    IPTR vendid = 0;

    ps = OpenLibrary("poseidon.library", 4);
    if (!ps) {
        return (NULL);
    }

    psdGetAttrs(PGA_DEVICE, pd, DA_VendorID, &vendid, DA_ProductID, &prodid, TAG_END);
    CloseLibrary(ps);

    if (!lan78xx_supported_device(vendid, prodid)) {
        return (NULL);
    }

    return (usbForceDeviceBinding(nh, pd));
}

struct NepClassEth *usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct NepClassEth *ncp;
    struct NepClassEth *tmpncp;
    struct ClsDevCfg *cfg;
    struct Task *tmptask;
    STRPTR devname = NULL;
    STRPTR devidstr = NULL;
    IPTR prodid = 0;
    IPTR vendid = 0;
    ULONG unitno;
    BOOL unitfound;
    UBYTE buf[64];

    ps = OpenLibrary("poseidon.library", 4);
    if (!ps) {
        return (NULL);
    }

    psdGetAttrs(PGA_DEVICE, pd, DA_ProductID, &prodid, DA_VendorID, &vendid, DA_ProductName, &devname, DA_IDString,
                &devidstr, TAG_END);

    if (!lan78xx_supported_device(vendid, prodid) || !devidstr) {
        CloseLibrary(ps);
        return (NULL);
    }

    Forbid();
    unitfound = FALSE;
    unitno = (ULONG)-1;
    ncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
    while (ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ) {
        if (!strcmp((const char *)ncp->ncp_DevIDString, devidstr)) {
            unitno = ncp->ncp_UnitNo;
            unitfound = TRUE;
            break;
        }
        ncp = (struct NepClassEth *)ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
    }

    if (!unitfound) {
        ncp = AllocVec(sizeof(struct NepClassEth), MEMF_PUBLIC | MEMF_CLEAR);
        if (!ncp) {
            Permit();
            CloseLibrary(ps);
            return (NULL);
        }

        cfg = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC | MEMF_CLEAR);
        if (!cfg) {
            Permit();
            FreeVec(ncp);
            CloseLibrary(ps);
            return (NULL);
        }

        ncp->ncp_CDC = cfg;
        ncp->ncp_UnitNo = (ULONG)-1;
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = (UBYTE)-1;
        NewList(&ncp->ncp_Unit.unit_MsgPort.mp_MsgList);
        NewList(&ncp->ncp_OrphanQueue);
        NewList(&ncp->ncp_WriteQueue);
        NewList(&ncp->ncp_BufManList);
        NewList(&ncp->ncp_EventList);
        NewList(&ncp->ncp_TrackList);
        NewList(&ncp->ncp_Multicasts);
        strncpy((char *)ncp->ncp_DevIDString, (const char *)devidstr, sizeof(ncp->ncp_DevIDString) - 1);
        AddTail(&nh->nh_Units, &ncp->ncp_Unit.unit_MsgPort.mp_Node);
    }

    ncp->ncp_ClsBase = nh;
    ncp->ncp_Device = pd;
    ncp->ncp_UnitProdID = (UWORD)prodid;
    ncp->ncp_UnitVendorID = (UWORD)vendid;

    nLoadBindingConfig(ncp);

    if (unitno == (ULONG)-1) {
        unitno = ncp->ncp_CDC->lc_DefaultUnit;
        tmpncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
        while (tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ) {
            if (tmpncp->ncp_UnitNo == unitno) {
                unitno++;
                tmpncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
            } else {
                tmpncp = (struct NepClassEth *)tmpncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
            }
        }
    }

    ncp->ncp_UnitNo = unitno;
    Permit();

    psdSafeRawDoFmt(buf, sizeof(buf), "lan78xx.class<%08lx>", ncp);
    ncp->ncp_ReadySignal = SIGB_SINGLE;
    ncp->ncp_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    tmptask = psdSpawnSubTask((STRPTR)buf, nEthTask, ncp);
    if (tmptask) {
        psdBorrowLocksWait(tmptask, 1L << ncp->ncp_ReadySignal);
        if (ncp->ncp_Task) {
            ncp->ncp_ReadySigTask = NULL;
            psdAddErrorMsg(RETURN_OK, (STRPTR)libname, "Bound '%s' on %s unit %ld",
                           devname ? devname : (STRPTR) "LAN78xx", nh->nh_DevBase->np_Library.lib_Node.ln_Name,
                           ncp->ncp_UnitNo);
            CloseLibrary(ps);
            return (ncp);
        }
    }

    ncp->ncp_ReadySigTask = NULL;
    CloseLibrary(ps);
    return (NULL);
}

void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp)
{
    struct Library *ps;
    STRPTR devname = NULL;

    (void)nh;

    ps = OpenLibrary("poseidon.library", 4);
    if (!ps) {
        return;
    }

    Forbid();
    ncp->ncp_ReadySignal = SIGB_SINGLE;
    ncp->ncp_ReadySigTask = FindTask(NULL);
    if (ncp->ncp_Task) {
        Signal(ncp->ncp_Task, SIGBREAKF_CTRL_C);
    }
    Permit();

    while (ncp->ncp_Task) {
        Wait(1L << ncp->ncp_ReadySignal);
    }

    psdGetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_ProductName, &devname, TAG_END);
    psdAddErrorMsg(RETURN_OK, (STRPTR)libname, "Released '%s'", devname ? devname : (STRPTR) "LAN78xx");
    CloseLibrary(ps);
}

AROS_LH3(LONG, usbGetAttrsA, AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1), LIBBASETYPEPTR, nh, 5, nep)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    switch (type) {
    case UGA_CLASS:
        if ((ti = FindTagItem(UCCA_Priority, tags))) {
            *((SIPTR *)ti->ti_Data) = 0;
            count++;
        }
        if ((ti = FindTagItem(UCCA_Description, tags))) {
            *((STRPTR *)ti->ti_Data) = "Clean-room LAN78xx USB ethernet class via usblan78xx.device";
            count++;
        }
        if ((ti = FindTagItem(UCCA_HasClassCfgGUI, tags))) {
            *((IPTR *)ti->ti_Data) = FALSE;
            count++;
        }
        if ((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags))) {
            *((IPTR *)ti->ti_Data) = FALSE;
            count++;
        }
        if ((ti = FindTagItem(UCCA_AfterDOSRestart, tags))) {
            *((IPTR *)ti->ti_Data) = FALSE;
            count++;
        }
        if ((ti = FindTagItem(UCCA_UsingDefaultCfg, tags))) {
            *((IPTR *)ti->ti_Data) = nh->nh_DummyNCP.ncp_UsingDefaultCfg;
            count++;
        }
        break;

    case UGA_BINDING:
        if ((ti = FindTagItem(UCBA_UsingDefaultCfg, tags))) {
            *((IPTR *)ti->ti_Data) = ((struct NepClassEth *)usbstruct)->ncp_UsingDefaultCfg;
            count++;
        }
        break;
    }

    return (count);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, usbSetAttrsA, AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1), LIBBASETYPEPTR, nh, 6, nep)
{
    AROS_LIBFUNC_INIT(void) type;
    (void)usbstruct;
    (void)tags;
    (void)nh;
    return (0);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(IPTR, usbDoMethodA, AROS_LHA(ULONG, methodid, D0), AROS_LHA(IPTR *, methoddata, A1), LIBBASETYPEPTR, nh, 7,
         nep)
{
    AROS_LIBFUNC_INIT

    struct NepClassEth *ncp;

    switch (methodid) {
    case UCM_AttemptDeviceBinding:
        return ((IPTR)usbAttemptDeviceBinding(nh, (struct PsdDevice *)methoddata[0]));

    case UCM_ForceDeviceBinding:
        return ((IPTR)usbForceDeviceBinding(nh, (struct PsdDevice *)methoddata[0]));

    case UCM_ReleaseDeviceBinding:
        usbReleaseDeviceBinding(nh, (struct NepClassEth *)methoddata[0]);
        return (TRUE);

    case UCM_ConfigChangedEvent:
        nLoadClassConfig(nh);
        Forbid();
        ncp = (struct NepClassEth *)nh->nh_Units.lh_Head;
        while (ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ) {
            nLoadBindingConfig(ncp);
            ncp = (struct NepClassEth *)ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Succ;
        }
        Permit();
        return (TRUE);

    case UCM_OpenCfgWindow:
    case UCM_OpenBindingCfgWindow:
        return (FALSE);
    }

    return (0);
    AROS_LIBFUNC_EXIT
}

BOOL nLoadClassConfig(struct NepEthBase *nh)
{
    struct NepClassEth *ncp = &nh->nh_DummyNCP;
    struct Library *ps;
    struct ClsDevCfg *cfg;
    struct PsdIFFContext *pic;

    if (ncp->ncp_GUITask) {
        return (FALSE);
    }

    ps = OpenLibrary("poseidon.library", 4);
    if (!ps) {
        return (FALSE);
    }

    cfg = ncp->ncp_CDC;
    cfg->lc_ChunkID = AROS_LONG2BE(MAKE_ID('L', '7', '8', '2'));
    cfg->lc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg) - 8);
    cfg->lc_DefaultUnit = 0;
    cfg->lc_MediaType = 0;
    ncp->ncp_UsingDefaultCfg = TRUE;

    pic = psdGetClsCfg(libname);
    if (pic) {
        cfg = psdGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->lc_ChunkID));
        if (cfg) {
            CopyMem(((UBYTE *)cfg) + 8, ((UBYTE *)ncp->ncp_CDC) + 8,
                    min(AROS_BE2LONG(cfg->lc_Length), AROS_BE2LONG(ncp->ncp_CDC->lc_Length)));
            psdFreeVec(cfg);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }

    CloseLibrary(ps);
    return (FALSE);
}

BOOL nLoadBindingConfig(struct NepClassEth *ncp)
{
    struct NepEthBase *nh = ncp->ncp_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cfg;
    struct PsdIFFContext *pic;

    if (ncp->ncp_GUITask) {
        return (FALSE);
    }

    *ncp->ncp_CDC = *nh->nh_DummyNCP.ncp_CDC;
    ncp->ncp_UsingDefaultCfg = TRUE;

    ps = OpenLibrary("poseidon.library", 4);
    if (!ps) {
        return (FALSE);
    }

    pic = psdGetUsbDevCfg(libname, ncp->ncp_DevIDString, NULL);
    if (pic) {
        cfg = psdGetCfgChunk(pic, AROS_LONG2BE(ncp->ncp_CDC->lc_ChunkID));
        if (cfg) {
            CopyMem(((UBYTE *)cfg) + 8, ((UBYTE *)ncp->ncp_CDC) + 8,
                    min(AROS_BE2LONG(cfg->lc_Length), AROS_BE2LONG(ncp->ncp_CDC->lc_Length)));
            psdFreeVec(cfg);
            ncp->ncp_UsingDefaultCfg = FALSE;
        }
    }

    CloseLibrary(ps);
    return (FALSE);
}

LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp)
{
    (void)nh;
    (void)ncp;
    return (FALSE);
}

#undef ps
#define ps ncp->ncp_Base

AROS_UFH0(void, nEthTask)
{
    struct NepClassEth *ncp;
    struct IOSana2Req *ioreq;
    struct PsdPipe *pp;
    struct MsgPort *timerPort = NULL;
    struct timerequest *timerReq = NULL;
    BOOL timerOpen = FALSE;
    BOOL timerPending = FALSE;
    ULONG sigmask;
    ULONG sigs;

    AROS_USERFUNC_INIT

    ncp = nAllocEth();
    if (ncp) {
        lan78xx_signal_ready(ncp);

        /* Hardware init only.  Don't bring the interface online here —
         * that would arm the bulk-IN pipe immediately and NAK-storm the
         * USB bus before AROSTCP has any reason to listen.  We go online
         * when the stack issues S2_CONFIGINTERFACE or S2_ONLINE. */
        nInitHardware(ncp);

        /* Set up a 1-second tick used by the link watcher.  The timer
         * runs independently of the USB pipe traffic; its signal bit
         * is folded into sigmask below. */
        if ((timerPort = CreateMsgPort())) {
            if ((timerReq = (struct timerequest *)CreateIORequest(timerPort, sizeof(struct timerequest)))) {
                if (!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timerReq, 0)) {
                    timerOpen = TRUE;
                    timerReq->tr_node.io_Command = TR_ADDREQUEST;
                    timerReq->tr_time.tv_secs = 1;
                    timerReq->tr_time.tv_micro = 0;
                    SendIO((struct IORequest *)timerReq);
                    timerPending = TRUE;
                }
            }
        }

        sigmask =
            (1L << ncp->ncp_Unit.unit_MsgPort.mp_SigBit) | (1L << ncp->ncp_TaskMsgPort->mp_SigBit) | SIGBREAKF_CTRL_C;
        if (timerOpen)
            sigmask |= (1L << timerPort->mp_SigBit);

        do {
            /* Keep the RX pipe armed whenever we're online. */
            if ((ncp->ncp_StateFlags & DDF_ONLINE) && ncp->ncp_LinkUp && ncp->ncp_ReadPending == NULL &&
                ncp->ncp_EPInPipe) {
                ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, LAN78XX_RX_BUFFER_SIZE);
                ncp->ncp_ReadBufNum ^= 1;
            }

            while ((pp = (struct PsdPipe *)GetMsg(ncp->ncp_TaskMsgPort))) {
                if (pp == ncp->ncp_EPOutPipe) {
                    ioreq = ncp->ncp_WritePending;
                    ncp->ncp_WritePending = NULL;
                    if (ioreq) {
                        LONG txerr = psdGetPipeError(pp);
                        if (txerr) {
                            KPRINTF(5, ("lan78xx: TX err=%ld\n", (LONG)txerr));
                            nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_TX);
                            ioreq->ios2_DataLength = 0;
                            ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
                            ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;
                        } else {
                            struct Sana2PacketTypeStats *stats = FindPacketTypeStats(ncp, ioreq->ios2_PacketType);
                            if (stats) {
                                stats->PacketsSent++;
                                stats->BytesSent += (ULONG)ioreq->ios2_DataLength;
                            }
                            ncp->ncp_DeviceStats.PacketsSent++;
                            ioreq->ios2_Req.io_Error = 0;
                        }
                        ReplyMsg((struct Message *)ioreq);
                    }
                } else if (pp == ncp->ncp_EPInPipe) {
                    UBYTE *pktptr = ncp->ncp_ReadPending;
                    LONG rxerr = psdGetPipeError(pp);
                    ULONG actual = psdGetPipeActual(pp);

                    /* Idle-rate gate: when the chip is returning empty
                     * bulk-IN bursts (no traffic), throttle the rearm to
                     * ~1 kHz so we don't monopolise the DWC2 scheduler
                     * and starve HID polling / SDHOST PIO. Single
                     * empties don't pay the cost — only sustained idle. */
                    if (rxerr == 0 && actual == 0) {
                        if (++ncp->ncp_RxIdleStreak > 1)
                            psdDelayMS(1);
                    } else {
                        ncp->ncp_RxIdleStreak = 0;
                    }

                    /* Rearm with the other buffer before processing this one
                     * so the chip keeps feeding us packets while we parse. */
                    if ((ncp->ncp_StateFlags & DDF_ONLINE) && ncp->ncp_LinkUp) {
                        ncp->ncp_ReadPending = ncp->ncp_ReadBuffer[ncp->ncp_ReadBufNum];
                        psdSendPipe(ncp->ncp_EPInPipe, ncp->ncp_ReadPending, LAN78XX_RX_BUFFER_SIZE);
                        ncp->ncp_ReadBufNum ^= 1;
                    } else {
                        ncp->ncp_ReadPending = NULL;
                    }

                    if (rxerr) {
                        KPRINTF(5, ("lan78xx: RX err=%ld actual=%lu\n", (LONG)rxerr, actual));
                        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_RX);
                        psdDelayMS(10);
                    } else if (pktptr && actual) {
                        lan78xx_handle_rx_buffer(ncp, pktptr, actual);
                    }
                }
            }

            /* Drain timer completions — each tick runs the link poll
             * and re-arms the timer for another second. */
            if (timerPending && CheckIO((struct IORequest *)timerReq)) {
                WaitIO((struct IORequest *)timerReq);
                timerPending = FALSE;

                lan78xx_link_poll(ncp);

                timerReq->tr_node.io_Command = TR_ADDREQUEST;
                timerReq->tr_time.tv_secs = 1;
                timerReq->tr_time.tv_micro = 0;
                SendIO((struct IORequest *)timerReq);
                timerPending = TRUE;
            }

            while ((ioreq = (struct IOSana2Req *)GetMsg(&ncp->ncp_Unit.unit_MsgPort))) {
                switch (ioreq->ios2_Req.io_Command) {
                case S2_CONFIGINTERFACE:
                    ncp->ncp_StateFlags |= DDF_CONFIGURED;
                    if (!(ncp->ncp_StateFlags & DDF_ONLINE))
                        nSetOnline(ncp);
                    ReplyMsg((struct Message *)ioreq);
                    break;

                case S2_ADDMULTICASTADDRESS:
                case S2_DELMULTICASTADDRESS:
                case S2_ADDMULTICASTADDRESSES:
                case S2_DELMULTICASTADDRESSES:
                    nUpdateRXMode(ncp);
                    ReplyMsg((struct Message *)ioreq);
                    break;

                case S2_ONLINE:
                    nSetOnline(ncp);
                    ReplyMsg((struct Message *)ioreq);
                    break;

                case S2_OFFLINE:
                    ncp->ncp_StateFlags &= ~DDF_ONLINE;
                    ncp->ncp_StateFlags |= DDF_OFFLINE;
                    ReplyMsg((struct Message *)ioreq);
                    break;

                default:
                    ioreq->ios2_Req.io_Error = IOERR_NOCMD;
                    ReplyMsg((struct Message *)ioreq);
                    break;
                }
            }

            Disable();
            while (!ncp->ncp_WritePending && ncp->ncp_WriteQueue.lh_Head->ln_Succ) {
                ioreq = (struct IOSana2Req *)RemHead(&ncp->ncp_WriteQueue);
                Enable();
                nWritePacket(ncp, ioreq);
                Disable();
            }
            Enable();

            sigs = Wait(sigmask);
        } while (!(sigs & SIGBREAKF_CTRL_C));

        Forbid();
        if ((ioreq = ncp->ncp_WritePending)) {
            psdAbortPipe(ncp->ncp_EPOutPipe);
            psdWaitPipe(ncp->ncp_EPOutPipe);
            ioreq->ios2_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)ioreq);
            ncp->ncp_WritePending = NULL;
        }
        if (ncp->ncp_ReadPending) {
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            ncp->ncp_ReadPending = NULL;
        }
        Permit();

        if (timerPending) {
            AbortIO((struct IORequest *)timerReq);
            WaitIO((struct IORequest *)timerReq);
            timerPending = FALSE;
        }
        if (timerOpen) {
            CloseDevice((struct IORequest *)timerReq);
            timerOpen = FALSE;
        }
        if (timerReq) {
            DeleteIORequest((struct IORequest *)timerReq);
            timerReq = NULL;
        }
        if (timerPort) {
            DeleteMsgPort(timerPort);
            timerPort = NULL;
        }

        nDoEvent(ncp, S2EVENT_OFFLINE);
        nFreeEth(ncp);
    }

    AROS_USERFUNC_EXIT
}

struct NepClassEth *nAllocEth(void)
{
    struct Task *thistask;
    struct NepClassEth *ncp;
    LONG sigbit;

    thistask = FindTask(NULL);
    ncp = thistask->tc_UserData;

    ncp->ncp_Base = OpenLibrary("poseidon.library", 4);
    if (!ncp->ncp_Base) {
        Alert(AG_OpenLib);
        return (NULL);
    }

    if (!lan78xx_pick_interface(ncp)) {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR)libname, "No LAN78xx bulk endpoint pair found");
        goto fail;
    }

    psdGetAttrs(PGA_ENDPOINT, ncp->ncp_EPOut, EA_MaxPktSize, &ncp->ncp_EPOutMaxPktSize, TAG_END);

    sigbit = AllocSignal(-1);
    if (sigbit < 0) {
        goto fail;
    }
    ncp->ncp_Unit.unit_MsgPort.mp_SigBit = (UBYTE)sigbit;

    ncp->ncp_Unit.unit_MsgPort.mp_SigTask = thistask;
    ncp->ncp_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

    ncp->ncp_ReadBuffer[0] = AllocVec(LAN78XX_RX_BUFFER_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    ncp->ncp_ReadBuffer[1] = AllocVec(LAN78XX_RX_BUFFER_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    ncp->ncp_WriteBuffer[0] = AllocVec(LAN78XX_TX_BUFFER_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    ncp->ncp_WriteBuffer[1] = AllocVec(LAN78XX_TX_BUFFER_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!ncp->ncp_ReadBuffer[0] || !ncp->ncp_ReadBuffer[1] || !ncp->ncp_WriteBuffer[0] || !ncp->ncp_WriteBuffer[1]) {
        goto fail;
    }

    ncp->ncp_TaskMsgPort = CreateMsgPort();
    if (!ncp->ncp_TaskMsgPort) {
        goto fail;
    }

    ncp->ncp_EP0Pipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, NULL);
    if (!ncp->ncp_EP0Pipe) {
        goto fail;
    }

    ncp->ncp_EPOutPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPOut);
    if (!ncp->ncp_EPOutPipe) {
        goto fail;
    }

    psdSetAttrs(PGA_PIPE, ncp->ncp_EPOutPipe, PPA_NoShortPackets, FALSE, PPA_NakTimeout, FALSE, PPA_NakTimeoutTime,
                5000, TAG_END);

    ncp->ncp_EPInPipe = psdAllocPipe(ncp->ncp_Device, ncp->ncp_TaskMsgPort, ncp->ncp_EPIn);
    if (!ncp->ncp_EPInPipe) {
        goto fail;
    }

    psdSetAttrs(PGA_PIPE, ncp->ncp_EPInPipe, PPA_NakTimeout, FALSE, PPA_NakTimeoutTime, 5000, PPA_AllowRuntPackets,
                TRUE, TAG_END);

    ncp->ncp_Task = thistask;
    return (ncp);

fail:
    nFreeEth(ncp);
    return (NULL);
}

void nFreeEth(struct NepClassEth *ncp)
{
    struct IOSana2Req *ioreq;

    Forbid();
    ncp->ncp_Unit.unit_MsgPort.mp_SigTask = NULL;
    ncp->ncp_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
    if (ncp->ncp_Unit.unit_MsgPort.mp_SigBit != (UBYTE)-1) {
        FreeSignal((LONG)ncp->ncp_Unit.unit_MsgPort.mp_SigBit);
        ncp->ncp_Unit.unit_MsgPort.mp_SigBit = (UBYTE)-1;
    }
    while ((ioreq = (struct IOSana2Req *)GetMsg(&ncp->ncp_Unit.unit_MsgPort))) {
        ioreq->ios2_Req.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)ioreq);
    }
    Permit();

    if (ncp->ncp_EPInPipe) {
        psdFreePipe(ncp->ncp_EPInPipe);
        ncp->ncp_EPInPipe = NULL;
    }
    if (ncp->ncp_EPOutPipe) {
        psdFreePipe(ncp->ncp_EPOutPipe);
        ncp->ncp_EPOutPipe = NULL;
    }
    if (ncp->ncp_EP0Pipe) {
        psdFreePipe(ncp->ncp_EP0Pipe);
        ncp->ncp_EP0Pipe = NULL;
    }

    if (ncp->ncp_TaskMsgPort) {
        DeleteMsgPort(ncp->ncp_TaskMsgPort);
        ncp->ncp_TaskMsgPort = NULL;
    }

    if (ncp->ncp_ReadBuffer[0]) {
        FreeVec(ncp->ncp_ReadBuffer[0]);
        ncp->ncp_ReadBuffer[0] = NULL;
    }
    if (ncp->ncp_ReadBuffer[1]) {
        FreeVec(ncp->ncp_ReadBuffer[1]);
        ncp->ncp_ReadBuffer[1] = NULL;
    }
    if (ncp->ncp_WriteBuffer[0]) {
        FreeVec(ncp->ncp_WriteBuffer[0]);
        ncp->ncp_WriteBuffer[0] = NULL;
    }
    if (ncp->ncp_WriteBuffer[1]) {
        FreeVec(ncp->ncp_WriteBuffer[1]);
        ncp->ncp_WriteBuffer[1] = NULL;
    }

    if (ncp->ncp_Base) {
        CloseLibrary(ncp->ncp_Base);
        ncp->ncp_Base = NULL;
    }

    Forbid();
    ncp->ncp_Task = NULL;
    if (ncp->ncp_ReadySigTask) {
        Signal(ncp->ncp_ReadySigTask, 1L << ncp->ncp_ReadySignal);
    }
    Permit();
}

BOOL nInitHardware(struct NepClassEth *ncp)
{
    ULONG maclo = 0;
    ULONG machi = 0;
    UBYTE ind = 0;

    ncp->ncp_ChipFlags = lan78xx_flags_from_product((IPTR)ncp->ncp_UnitProdID);
    ncp->ncp_PhyNo = LAN78XX_PHY_INTERNAL;

    /* EEPROM programmed-indicator byte lives at offset 0. */
    if (lan78xx_eeprom_read(ncp, LAN78XX_EE_IND_OFFSET, 1, &ind) == 0) {
        ncp->ncp_EepromPresent = (ind == LAN78XX_EEPROM_INDICATOR) ? 1 : 0;
    } else {
        ncp->ncp_EepromPresent = 0;
    }

    if (lan78xx_chip_init(ncp) != 0) {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR)libname, "LAN78xx chip initialisation failed");
        return (FALSE);
    }

    /* Seed MAC — try EEPROM first, then the chip's own RX_ADDR registers,
     * then fall back to a synthetic locally-administered address.  The
     * chip holds a reasonable default in RX_ADDRL/H after reset on many
     * modules even when no SPI EEPROM is fitted. */
    if (ncp->ncp_EepromPresent) {
        UBYTE mac[ETHER_ADDR_SIZE];
        if (lan78xx_eeprom_read(ncp, LAN78XX_EE_MAC_OFFSET, ETHER_ADDR_SIZE, mac) == 0 && lan78xx_mac_valid(mac)) {
            CopyMem(mac, ncp->ncp_ROMAddress, ETHER_ADDR_SIZE);
            CopyMem(mac, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
        }
    }

    if (!lan78xx_mac_valid(ncp->ncp_ROMAddress)) {
        if (!lan78xx_read_reg(ncp, LAN78XX_REG_RX_ADDRL, &maclo) &&
            !lan78xx_read_reg(ncp, LAN78XX_REG_RX_ADDRH, &machi)) {
            lan78xx_store_mac(ncp, maclo, machi);
        }
    }

    lan78xx_seed_mac(ncp);

    /* Program the MAC back into the chip's address filter. */
    lan78xx_set_macaddr(ncp);

    KPRINTF(1, ("lan78xx: flags=%04lx prod=%04lx eeprom=%d "
                "mac=%02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
                (ULONG)ncp->ncp_ChipFlags, (ULONG)ncp->ncp_UnitProdID, (int)ncp->ncp_EepromPresent,
                ncp->ncp_MacAddress[0], ncp->ncp_MacAddress[1], ncp->ncp_MacAddress[2], ncp->ncp_MacAddress[3],
                ncp->ncp_MacAddress[4], ncp->ncp_MacAddress[5]));

    return (TRUE);
}

void nSetOnline(struct NepClassEth *ncp)
{
    ncp->ncp_StateFlags |= DDF_ONLINE;
    ncp->ncp_StateFlags &= ~DDF_OFFLINE;

    /* Refresh the receive filter so any promisc/multicast state set
     * before going online is reflected in the chip. */
    nUpdateRXMode(ncp);

    /* Link state is polled from the task loop — don't assert LinkUp
     * here.  The RX pipe will be armed on the first link-up transition
     * detected by lan78xx_link_poll(). */
    nDoEvent(ncp, S2EVENT_ONLINE);
}

/*
 * Rebuild the chip's receive filter so it matches the current set of
 * SANA-II multicast registrations plus the opener's promiscuous flag.
 *
 * Ported from OpenBSD mue_iff (if_mue.c) — same RFE_CTL layering and
 * same 16-word VHF hash table written via the indirect DP_* register
 * window.  SANA-II has no ALLMULTI concept, so we map:
 *
 *   opener with SANA2OPF_PROM set  →  accept all unicast + all multicast
 *   otherwise                      →  perfect-match my MAC + hashed multicasts
 *
 * Broadcast is always accepted.  Called with no lock held; the chip's
 * EP0 requests are serialised by our ethernet task, so it's safe to
 * read-modify-write register state here.
 */
void nUpdateRXMode(struct NepClassEth *ncp)
{
    ULONG hashtbl[LAN78XX_DP_SEL_VHF_HASH_LEN];
    ULONG rxfilt = 0;
    BOOL is_lan7500 = (ncp->ncp_ChipFlags & LAN78XX_FLAG_LAN7500) ? TRUE : FALSE;
    UWORD rfe_reg = is_lan7500 ? LAN78XX_REG_RFE_CTL : LAN78XX_REG_7800_RFE_CTL;
    BOOL promisc = (ncp->ncp_OpenFlags & SANA2OPF_PROM) ? TRUE : FALSE;

    if (lan78xx_read_reg(ncp, rfe_reg, &rxfilt))
        return;

    rxfilt &=
        ~(LAN78XX_RFE_CTL_PERFECT | LAN78XX_RFE_CTL_MCAST_HASH | LAN78XX_RFE_CTL_UNICAST | LAN78XX_RFE_CTL_MULTICAST);
    rxfilt |= LAN78XX_RFE_CTL_BROADCAST;

    memset(hashtbl, 0, sizeof(hashtbl));

    if (promisc) {
        rxfilt |= LAN78XX_RFE_CTL_UNICAST | LAN78XX_RFE_CTL_MULTICAST;
    } else {
        struct MulticastAddressRange *mar;

        rxfilt |= LAN78XX_RFE_CTL_PERFECT | LAN78XX_RFE_CTL_MCAST_HASH;

        /* Walk the SANA-II multicast list and fold each address into the
         * chip's 512-bit hash table.  For range entries we only hash the
         * lower bound — iterating across a full range could be millions
         * of addresses and we'd block the task loop; clients that need
         * exact range coverage should enable promiscuous mode. */
        Disable();
        mar = (struct MulticastAddressRange *)ncp->ncp_Multicasts.lh_Head;
        while (mar->mar_Node.ln_Succ) {
            ULONG crc = lan78xx_ether_crc32_be(mar->mar_LowerAddr, ETHER_ADDR_SIZE);
            ULONG h = crc >> 23; /* take the top 9 bits */
            hashtbl[h >> 5] |= 1UL << (h & 31);
            mar = (struct MulticastAddressRange *)mar->mar_Node.ln_Succ;
        }
        Enable();
    }

    lan78xx_dataport_write(ncp, LAN78XX_DP_SEL_VHF, LAN78XX_DP_SEL_VHF_VLAN_LEN, LAN78XX_DP_SEL_VHF_HASH_LEN, hashtbl);

    lan78xx_write_reg(ncp, rfe_reg, rxfilt);

    KPRINTF(5, ("lan78xx: RFE_CTL=%08lx promisc=%d\n", rxfilt, (int)promisc));
}

void nDoEvent(struct NepClassEth *ncp, ULONG events)
{
    struct IOSana2Req *worknode;
    struct IOSana2Req *nextnode;

    Disable();
    worknode = (struct IOSana2Req *)ncp->ncp_EventList.lh_Head;
    while ((nextnode = (struct IOSana2Req *)(((struct Node *)worknode)->ln_Succ))) {
        if (worknode->ios2_WireError & events) {
            Remove(&worknode->ios2_Req.io_Message.mn_Node);
            worknode->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
            ReplyMsg(&worknode->ios2_Req.io_Message);
        }
        worknode = nextnode;
    }
    Enable();
}

BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq)
{
    struct BufMan *bufman;
    struct EtherPacketHeader *eph;
    UBYTE *buf;
    UBYTE *copydest;
    ULONG *txcmd;
    ULONG packettype;
    ULONG writelen;
    ULONG frame_len;
    ULONG total_len;
    ULONG data_end;
    BOOL israw;

    packettype = ioreq->ios2_PacketType;
    writelen = ioreq->ios2_DataLength;
    bufman = (struct BufMan *)ioreq->ios2_BufferManagement;
    israw = (ioreq->ios2_Req.io_Flags & SANA2IOF_RAW) ? TRUE : FALSE;

    if (!bufman || !bufman->bm_CopyFromBuf) {
        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_TX);
        ioreq->ios2_DataLength = 0;
        ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        ioreq->ios2_WireError = S2WERR_NULL_POINTER;
        ReplyMsg((struct Message *)ioreq);
        return (FALSE);
    }

    frame_len = writelen;
    if (!israw)
        frame_len += sizeof(struct EtherPacketHeader);
    if (frame_len < ETHER_MIN_LEN)
        frame_len = ETHER_MIN_LEN;

    total_len = LAN78XX_TX_CMD_LEN + frame_len;
    if (total_len > LAN78XX_TX_BUFFER_SIZE) {
        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_BUFF);
        ioreq->ios2_DataLength = 0;
        ioreq->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;
        ioreq->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg((struct Message *)ioreq);
        return (FALSE);
    }

    buf = ncp->ncp_WriteBuffer[ncp->ncp_WriteBufNum];

    /* TX command header — the chip prepends this on every frame it sends.
     * tx_cmd_a holds the length and the FCS-generate bit; tx_cmd_b is
     * left zero (no VLAN insertion, no TSO). */
    txcmd = (ULONG *)buf;
    txcmd[0] = AROS_LONG2LE((frame_len & LAN78XX_TX_CMD_A_LEN_MASK) | LAN78XX_TX_CMD_A_FCS);
    txcmd[1] = AROS_LONG2LE(0);

    copydest = buf + LAN78XX_TX_CMD_LEN;

    /* SANA-II cooked mode — we prepend the ethernet L2 header using the
     * unit's MAC as source.  Raw mode delivers the client buffer as-is. */
    if (!israw) {
        UWORD cnt;
        eph = (struct EtherPacketHeader *)copydest;
        for (cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++) {
            eph->eph_Dest[cnt] = ioreq->ios2_DstAddr[cnt];
            eph->eph_Src[cnt] = ncp->ncp_MacAddress[cnt];
        }
        eph->eph_Type = AROS_WORD2BE(packettype);
        copydest += sizeof(struct EtherPacketHeader);
    }

    if (callcopy(bufman->bm_CopyFromBuf, copydest, ioreq->ios2_Data, writelen) == NULL) {
        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_BUFF);
        ioreq->ios2_DataLength = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
        ReplyMsg((struct Message *)ioreq);
        return (FALSE);
    }

    /* Zero-pad runts up to the ethernet minimum so the line sees a legal
     * frame.  FCS is generated by the MAC over this padded content. */
    data_end = LAN78XX_TX_CMD_LEN + (israw ? 0 : sizeof(struct EtherPacketHeader)) + writelen;
    if (data_end < total_len) {
        memset(buf + data_end, 0, total_len - data_end);
    }

    ncp->ncp_WritePending = ioreq;
    psdSendPipe(ncp->ncp_EPOutPipe, buf, total_len);
    ncp->ncp_WriteBufNum ^= 1;

    return (TRUE);
}

static UWORD nReadIOReq(struct NepClassEth *ncp, struct EtherPacketHeader *eph, UWORD datasize,
                        struct IOSana2Req *ioreq, UWORD flags)
{
    struct BufMan *bufman = (struct BufMan *)ioreq->ios2_BufferManagement;
    UBYTE *copyfrom;
    UWORD cnt;

    if (ioreq->ios2_Req.io_Flags & SANA2IOF_RAW) {
        copyfrom = (UBYTE *)eph;
        /* lan78xx_handle_rx_buffer() has already stripped the trailing
         * FCS, so the raw-mode length is simply header + payload. */
        datasize += sizeof(struct EtherPacketHeader);
    } else {
        copyfrom = (UBYTE *)(eph + 1);
    }

    ioreq->ios2_PacketType = AROS_BE2WORD(eph->eph_Type);
    for (cnt = 0; cnt < ETHER_ADDR_SIZE; cnt++) {
        ioreq->ios2_SrcAddr[cnt] = eph->eph_Src[cnt];
        ioreq->ios2_DstAddr[cnt] = eph->eph_Dest[cnt];
    }
    ioreq->ios2_DataLength = datasize;

    if ((flags & PACKETFILTER) && bufman && bufman->bm_PacketFilter &&
        !callfilter(bufman->bm_PacketFilter, ioreq, copyfrom)) {
        return (flags);
    }

    if (ioreq->ios2_DstAddr[0] & 1) {
        static const UBYTE bcast[ETHER_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        if (memcmp(bcast, ioreq->ios2_DstAddr, ETHER_ADDR_SIZE) == 0)
            ioreq->ios2_Req.io_Flags |= SANA2IOF_BCAST;
        else
            ioreq->ios2_Req.io_Flags |= SANA2IOF_MCAST;
    }

    if (callcopy(bufman->bm_CopyToBuf, ioreq->ios2_Data, copyfrom, ioreq->ios2_DataLength)) {
        flags &= ~DROPPED;
    } else {
        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_BUFF);
        ioreq->ios2_DataLength = 0;
        ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
    }

    Disable();
    Remove((struct Node *)ioreq);
    Enable();
    ReplyMsg((struct Message *)ioreq);
    return (flags);
}

BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen)
{
    struct EtherPacketHeader *eph;
    struct BufMan *bufman;
    struct IOSana2Req *worknode, *nextnode;
    struct Sana2PacketTypeStats *stats;
    UWORD flags;
    UWORD datasize;
    BOOL delivered = FALSE;

    if (pktlen < sizeof(struct EtherPacketHeader)) {
        ncp->ncp_DeviceStats.BadData++;
        return (FALSE);
    }

    ncp->ncp_DeviceStats.PacketsReceived++;
    eph = (struct EtherPacketHeader *)pktptr;
    stats = FindPacketTypeStats(ncp, (ULONG)AROS_BE2WORD(eph->eph_Type));
    flags = DROPPED | PACKETFILTER;
    datasize = pktlen - sizeof(struct EtherPacketHeader);

    if (pktlen > ETHER_MAX_LEN) {
        ncp->ncp_DeviceStats.BadData++;
        return (FALSE);
    }

    if (stats) {
        stats->PacketsReceived++;
        stats->BytesReceived += datasize;
    }

    Disable();
    bufman = (struct BufMan *)ncp->ncp_BufManList.lh_Head;
    while (((struct Node *)bufman)->ln_Succ) {
        worknode = (struct IOSana2Req *)bufman->bm_RXQueue.lh_Head;
        while ((nextnode = (struct IOSana2Req *)(((struct Node *)worknode)->ln_Succ))) {
            if (worknode->ios2_PacketType == AROS_BE2WORD(eph->eph_Type) ||
                (AROS_BE2WORD(eph->eph_Type) < ETHERPKT_SIZE && worknode->ios2_PacketType < ETHERPKT_SIZE)) {
                flags = nReadIOReq(ncp, eph, datasize, worknode, flags);
                break;
            }
            worknode = nextnode;
        }
        bufman = (struct BufMan *)(((struct Node *)bufman)->ln_Succ);
    }
    Enable();

    if (flags & DROPPED) {
        Disable();
        worknode = (struct IOSana2Req *)ncp->ncp_OrphanQueue.lh_Head;
        while ((nextnode = (struct IOSana2Req *)(((struct Node *)worknode)->ln_Succ))) {
            nReadIOReq(ncp, eph, datasize, worknode, 0);
            worknode = nextnode;
        }
        Enable();
    } else {
        delivered = TRUE;
    }

    if (!delivered) {
        ncp->ncp_DeviceStats.UnknownTypesReceived++;
        if (stats)
            stats->PacketsDropped++;
        nDoEvent(ncp, S2EVENT_ERROR | S2EVENT_RX);
    }

    return (delivered);
}

/*
 * Walk a single bulk-IN transfer and hand each encapsulated frame to
 * nReadPacket().  The chip prepends a 10-byte rx_cmd header to every
 * packet and may coalesce several frames into one transfer.  Packets
 * within a burst are padded to 4-byte alignment — see the OpenBSD
 * if_mue.c reference for the framing layout.
 */
static void lan78xx_handle_rx_buffer(struct NepClassEth *ncp, UBYTE *buf, ULONG total_len)
{
    ULONG offset = 0;

    while (offset + LAN78XX_RX_CMD_LEN <= total_len) {
        ULONG rx_cmd_a;
        ULONG frame_len;
        ULONG payload_len;
        ULONG aligned_next;

        rx_cmd_a = AROS_LE2LONG(*(ULONG *)(buf + offset));
        frame_len = rx_cmd_a & LAN78XX_RX_CMD_A_LEN_MASK;

        /* LAN7500 family reports the 2-byte IP-align padding inside
         * the length — strip it so the caller sees a plain ethernet
         * frame starting at byte 0. */
        if (ncp->ncp_ChipFlags & LAN78XX_FLAG_LAN7500) {
            if (frame_len >= 2)
                frame_len -= 2;
            else
                break;
        }

        if (frame_len == 0 || offset + LAN78XX_RX_CMD_LEN + frame_len > total_len) {
            /* Stream desync or runt — drop the rest of the buffer. */
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        if (rx_cmd_a & LAN78XX_RX_CMD_A_RED) {
            /* Receive error bit set — skip this frame but keep going. */
            ncp->ncp_DeviceStats.BadData++;
            nDoEvent(ncp, S2EVENT_HARDWARE | S2EVENT_RX);
        } else {
            /* The MAC delivers the FCS by default; the length field in
             * rx_cmd_a includes it.  Strip the 4 trailing CRC bytes
             * before handing the frame to SANA-II consumers. */
            payload_len = (frame_len > ETHER_CRC_LEN) ? (frame_len - ETHER_CRC_LEN) : 0;

            if (payload_len >= sizeof(struct EtherPacketHeader)) {
                nReadPacket(ncp, buf + offset + LAN78XX_RX_CMD_LEN, payload_len);
            } else {
                ncp->ncp_DeviceStats.BadData++;
            }
        }

        /* Advance to the next packet, 4-byte aligned. */
        aligned_next = offset + LAN78XX_RX_CMD_LEN + frame_len;
        aligned_next = (aligned_next + 3) & ~3UL;
        if (aligned_next <= offset)
            break;
        offset = aligned_next;
    }
}

void nGUITaskCleanup(struct NepClassEth *ncp) { (void)ncp; }

AROS_UFH0(void, nGUITask){AROS_USERFUNC_INIT AROS_USERFUNC_EXIT}

LONG lan78xx_read_reg(struct NepClassEth *ncp, UWORD reg, ULONG *val)
{
    LONG ioerr;
    ULONG data = 0;

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_IN | URTF_VENDOR | URTF_DEVICE, LAN78XX_USB_REQ_READ_REG, 0, (ULONG)reg);

    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &data, 4);
    if (!ioerr) {
        *val = AROS_LE2LONG(data);
    } else {
        *val = 0;
    }

    return (ioerr);
}

LONG lan78xx_write_reg(struct NepClassEth *ncp, UWORD reg, ULONG val)
{
    LONG ioerr;
    ULONG data = AROS_LONG2LE(val);

    psdPipeSetup(ncp->ncp_EP0Pipe, URTF_OUT | URTF_VENDOR | URTF_DEVICE, LAN78XX_USB_REQ_WRITE_REG, 0, (ULONG)reg);

    ioerr = psdDoPipe(ncp->ncp_EP0Pipe, &data, 4);
    return (ioerr);
}

LONG lan78xx_phy_read(struct NepClassEth *ncp, UWORD reg, UWORD *val)
{
    ULONG acc;
    ULONG data = 0;

    if (!lan78xx_wait_reg_clear(ncp, LAN78XX_REG_MII_ACC, LAN78XX_MII_ACC_BUSY)) {
        *val = 0;
        return (-1);
    }

    acc = ((ULONG)LAN78XX_PHY_INTERNAL << LAN78XX_MII_ACC_PHY_ADDR_SHIFT) |
          ((ULONG)reg << LAN78XX_MII_ACC_REGADDR_SHIFT) | LAN78XX_MII_ACC_BUSY;

    if (lan78xx_write_reg(ncp, LAN78XX_REG_MII_ACC, acc)) {
        *val = 0;
        return (-1);
    }

    if (!lan78xx_wait_reg_clear(ncp, LAN78XX_REG_MII_ACC, LAN78XX_MII_ACC_BUSY)) {
        *val = 0;
        return (-1);
    }

    if (lan78xx_read_reg(ncp, LAN78XX_REG_MII_DATA, &data)) {
        *val = 0;
        return (-1);
    }

    *val = (UWORD)data;
    return (0);
}

LONG lan78xx_phy_write(struct NepClassEth *ncp, UWORD reg, UWORD val)
{
    ULONG acc;

    if (!lan78xx_wait_reg_clear(ncp, LAN78XX_REG_MII_ACC, LAN78XX_MII_ACC_BUSY)) {
        return (-1);
    }

    if (lan78xx_write_reg(ncp, LAN78XX_REG_MII_DATA, (ULONG)val)) {
        return (-1);
    }

    acc = ((ULONG)LAN78XX_PHY_INTERNAL << LAN78XX_MII_ACC_PHY_ADDR_SHIFT) |
          ((ULONG)reg << LAN78XX_MII_ACC_REGADDR_SHIFT) | LAN78XX_MII_ACC_WRITE | LAN78XX_MII_ACC_BUSY;

    if (lan78xx_write_reg(ncp, LAN78XX_REG_MII_ACC, acc)) {
        return (-1);
    }

    return (lan78xx_wait_reg_clear(ncp, LAN78XX_REG_MII_ACC, LAN78XX_MII_ACC_BUSY) ? 0 : -1);
}

static BOOL lan78xx_supported_device(IPTR vendid, IPTR prodid)
{
    if (vendid != LAN78XX_USB_VENDOR_ID) {
        return (FALSE);
    }

    return ((prodid == LAN78XX_USB_PRODUCT_ID_7500) || (prodid == LAN78XX_USB_PRODUCT_ID_7505) ||
            (prodid == LAN78XX_USB_PRODUCT_ID_7800) || (prodid == LAN78XX_USB_PRODUCT_ID_7801) ||
            (prodid == LAN78XX_USB_PRODUCT_ID_7850));
}

static UWORD lan78xx_flags_from_product(IPTR prodid)
{
    switch (prodid) {
    case LAN78XX_USB_PRODUCT_ID_7500:
    case LAN78XX_USB_PRODUCT_ID_7505:
        return (LAN78XX_FLAG_LAN7500);
    default:
        return (0);
    }
}

static BOOL lan78xx_pick_interface(struct NepClassEth *ncp)
{
    struct List *cfgs = NULL;
    struct PsdConfig *cfg;
    struct PsdConfig *cur_cfg = NULL;

    psdGetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_ConfigList, &cfgs, DA_Config, &cur_cfg, TAG_END);

    for (cfg = (struct PsdConfig *)(cfgs ? cfgs->lh_Head : NULL); cfg && cfg->pc_Node.ln_Succ;
         cfg = (struct PsdConfig *)cfg->pc_Node.ln_Succ) {
        struct List *ifs = NULL;
        struct PsdInterface *pif;

        psdGetAttrs(PGA_CONFIG, cfg, CA_InterfaceList, &ifs, TAG_END);

        for (pif = ifs ? (struct PsdInterface *)ifs->lh_Head : NULL; pif && pif->pif_Node.ln_Succ;
             pif = (struct PsdInterface *)pif->pif_Node.ln_Succ) {
            struct PsdInterface *altpif;
            struct List *altlist = NULL;
            struct PsdEndpoint *in;
            struct PsdEndpoint *out;

            in = psdFindEndpoint(pif, NULL, EA_IsIn, TRUE, EA_TransferType, USEAF_BULK, TAG_END);
            out = psdFindEndpoint(pif, NULL, EA_IsIn, FALSE, EA_TransferType, USEAF_BULK, TAG_END);
            if (in && out) {
                if (cur_cfg != cfg) {
                    psdSetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_Config, cfg, TAG_END);
                }
                ncp->ncp_Config = cfg;
                ncp->ncp_Interface = pif;
                ncp->ncp_EPIn = in;
                ncp->ncp_EPOut = out;
                return (TRUE);
            }

            psdGetAttrs(PGA_INTERFACE, pif, IFA_AlternateIfList, &altlist, TAG_END);
            for (altpif = altlist ? (struct PsdInterface *)altlist->lh_Head : NULL; altpif && altpif->pif_Node.ln_Succ;
                 altpif = (struct PsdInterface *)altpif->pif_Node.ln_Succ) {
                IPTR altnum = 0;

                in = psdFindEndpoint(altpif, NULL, EA_IsIn, TRUE, EA_TransferType, USEAF_BULK, TAG_END);
                out = psdFindEndpoint(altpif, NULL, EA_IsIn, FALSE, EA_TransferType, USEAF_BULK, TAG_END);
                if (!(in && out)) {
                    continue;
                }

                psdGetAttrs(PGA_INTERFACE, altpif, IFA_AlternateNum, &altnum, TAG_END);
                if (cur_cfg != cfg) {
                    psdSetAttrs(PGA_DEVICE, ncp->ncp_Device, DA_Config, cfg, TAG_END);
                }
                psdSetAttrs(PGA_INTERFACE, altpif, IFA_AlternateNum, altnum, TAG_END);
                ncp->ncp_Config = cfg;
                ncp->ncp_Interface = altpif;
                ncp->ncp_EPIn = in;
                ncp->ncp_EPOut = out;
                return (TRUE);
            }
        }
    }

    return (FALSE);
}

static void lan78xx_signal_ready(struct NepClassEth *ncp)
{
    Forbid();
    if (ncp->ncp_ReadySigTask) {
        Signal(ncp->ncp_ReadySigTask, 1L << ncp->ncp_ReadySignal);
    }
    Permit();
}

static void lan78xx_seed_mac(struct NepClassEth *ncp)
{
    UBYTE fallback[ETHER_ADDR_SIZE] = {0x02, 0x42, 0x78, 0x00, 0x00, 0x00};

    fallback[3] = (UBYTE)(ncp->ncp_UnitVendorID & 0xff);
    fallback[4] = (UBYTE)(ncp->ncp_UnitProdID & 0xff);
    fallback[5] = (UBYTE)(ncp->ncp_UnitNo & 0xff);

    if (lan78xx_mac_valid(ncp->ncp_ROMAddress)) {
        if (!lan78xx_mac_valid(ncp->ncp_MacAddress)) {
            CopyMem(ncp->ncp_ROMAddress, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
        }
        return;
    }

    CopyMem(fallback, ncp->ncp_ROMAddress, ETHER_ADDR_SIZE);
    CopyMem(fallback, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
}

static BOOL lan78xx_mac_valid(const UBYTE *mac)
{
    int idx;
    BOOL all_zero = TRUE;

    if (!mac) {
        return (FALSE);
    }

    for (idx = 0; idx < ETHER_ADDR_SIZE; idx++) {
        if (mac[idx] != 0) {
            all_zero = FALSE;
            break;
        }
    }

    if (all_zero || (mac[0] & 0x01)) {
        return (FALSE);
    }

    return (TRUE);
}

static void lan78xx_store_mac(struct NepClassEth *ncp, ULONG lo, ULONG hi)
{
    ncp->ncp_ROMAddress[0] = (UBYTE)(lo & 0xff);
    ncp->ncp_ROMAddress[1] = (UBYTE)((lo >> 8) & 0xff);
    ncp->ncp_ROMAddress[2] = (UBYTE)((lo >> 16) & 0xff);
    ncp->ncp_ROMAddress[3] = (UBYTE)((lo >> 24) & 0xff);
    ncp->ncp_ROMAddress[4] = (UBYTE)(hi & 0xff);
    ncp->ncp_ROMAddress[5] = (UBYTE)((hi >> 8) & 0xff);
    CopyMem(ncp->ncp_ROMAddress, ncp->ncp_MacAddress, ETHER_ADDR_SIZE);
}

static BOOL lan78xx_wait_reg_clear(struct NepClassEth *ncp, UWORD reg, ULONG mask)
{
    ULONG val = 0;
    int timeout = 100;

    while (timeout-- > 0) {
        if (lan78xx_read_reg(ncp, reg, &val)) {
            return (FALSE);
        }
        if (!(val & mask)) {
            return (TRUE);
        }
        psdDelayMS(1);
    }

    return (FALSE);
}

static BOOL lan78xx_wait_reg_set(struct NepClassEth *ncp, UWORD reg, ULONG mask)
{
    ULONG val = 0;
    int timeout = 1000;

    while (timeout-- > 0) {
        if (lan78xx_read_reg(ncp, reg, &val)) {
            return (FALSE);
        }
        if (val & mask) {
            return (TRUE);
        }
        psdDelayMS(1);
    }

    return (FALSE);
}

static LONG lan78xx_eeprom_wait(struct NepClassEth *ncp)
{
    ULONG val = 0;
    int ntries;

    for (ntries = 0; ntries < 100; ntries++) {
        if (lan78xx_read_reg(ncp, LAN78XX_REG_E2P_CMD, &val))
            return (-1);
        if (!(val & LAN78XX_E2P_CMD_BUSY) || (val & LAN78XX_E2P_CMD_TIMEOUT))
            return ((val & LAN78XX_E2P_CMD_TIMEOUT) ? -1 : 0);
        psdDelayMS(1);
    }
    return (-1);
}

static LONG lan78xx_eeprom_read(struct NepClassEth *ncp, UWORD offset, UWORD len, UBYTE *dest)
{
    ULONG hw_saved = 0;
    BOOL led_muxed = (ncp->ncp_UnitProdID == LAN78XX_USB_PRODUCT_ID_7800);
    UWORD i;
    LONG err = 0;

    /* On LAN7800 the EEPROM pins are muxed with the LED drivers — clear
     * the LED enables for the duration of the access, then restore. */
    if (led_muxed) {
        if (lan78xx_read_reg(ncp, LAN78XX_REG_HW_CFG, &hw_saved))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, hw_saved & ~(LAN78XX_HW_CFG_LED0_EN | LAN78XX_HW_CFG_LED1_EN));
    }

    for (i = 0; i < len; i++) {
        ULONG val;

        if (lan78xx_eeprom_wait(ncp) != 0) {
            err = -1;
            break;
        }

        lan78xx_write_reg(ncp, LAN78XX_REG_E2P_CMD,
                          LAN78XX_E2P_CMD_READ | LAN78XX_E2P_CMD_BUSY |
                              (((ULONG)(offset + i)) & LAN78XX_E2P_CMD_ADDR_MASK));

        if (lan78xx_eeprom_wait(ncp) != 0) {
            err = -1;
            break;
        }

        if (lan78xx_read_reg(ncp, LAN78XX_REG_E2P_DATA, &val)) {
            err = -1;
            break;
        }
        dest[i] = (UBYTE)(val & 0xff);
    }

    if (led_muxed) {
        lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, hw_saved);
    }

    return (err);
}

static void lan78xx_set_macaddr(struct NepClassEth *ncp)
{
    const UBYTE *mac = ncp->ncp_MacAddress;
    UWORD reg = (ncp->ncp_ChipFlags & LAN78XX_FLAG_LAN7500) ? LAN78XX_REG_ADDR_FILTX : LAN78XX_REG_7800_ADDR_FILTX;
    ULONG lo, hi;

    lo = ((ULONG)mac[3] << 24) | ((ULONG)mac[2] << 16) | ((ULONG)mac[1] << 8) | ((ULONG)mac[0]);
    hi = ((ULONG)mac[5] << 8) | ((ULONG)mac[4]);

    lan78xx_write_reg(ncp, LAN78XX_REG_RX_ADDRL, lo);
    lan78xx_write_reg(ncp, reg + 4, lo);
    lan78xx_write_reg(ncp, LAN78XX_REG_RX_ADDRH, hi);
    lan78xx_write_reg(ncp, reg, hi | LAN78XX_ADDR_FILTX_VALID);
}

/*
 * Bring the LAN75xx/LAN78xx out of reset and into a known, receive-ready
 * state.  Mirrors OpenBSD mue_chip_init():
 *
 *   1. wait for PMT_CTL.READY on the LAN7500 family
 *   2. assert a lite soft-reset via HW_CFG.LRST, wait for self-clear
 *   3. program the BIR (bulk-in-respond-with-NAK) bit — on LAN7800
 *      that moved from HW_CFG to USB_CFG0
 *   4. program BURST_CAP + BULK_IN_DELAY (framing coalescer)
 *   5. enable HW_CFG.BCE|MEF / USB_CFG0.BCE (burst + multi-ethernet)
 *   6. set undocumented FIFO end markers on LAN7500 family only
 *   7. clear INT_STS, FCT_FLOW, FLOW
 *   8. trigger a PHY reset via PMT_CTL.PHY_RST, wait for it to complete
 *   9. enable MAC auto-speed/duplex if no EEPROM present
 *  10. enable MAC_TX + FCT_TX_CTL
 *  11. program MAC_RX max-frame size and enable MAC_RX + FCT_RX_CTL
 *
 * The magic 0x28 / 0x17 / bulk-delay constants come verbatim from the
 * chip reference in if_mue.c — they are facts about the silicon.
 */
static LONG lan78xx_chip_init(struct NepClassEth *ncp)
{
    ULONG val;
    BOOL is_lan7500 = (ncp->ncp_ChipFlags & LAN78XX_FLAG_LAN7500) ? TRUE : FALSE;
    UWORD fct_flow_reg = is_lan7500 ? LAN78XX_REG_FCT_FLOW : LAN78XX_REG_7800_FCT_FLOW;
    UWORD fct_tx_reg = is_lan7500 ? LAN78XX_REG_FCT_TX_CTL : LAN78XX_REG_7800_FCT_TX_CTL;
    UWORD fct_rx_reg = is_lan7500 ? LAN78XX_REG_FCT_RX_CTL : LAN78XX_REG_7800_FCT_RX_CTL;
    UWORD burst_cap_reg = is_lan7500 ? LAN78XX_REG_BURST_CAP : LAN78XX_REG_7800_BURST_CAP;
    UWORD bulk_in_reg = is_lan7500 ? LAN78XX_REG_BULK_IN_DELAY : LAN78XX_REG_7800_BULK_IN_DELAY;

    if (is_lan7500) {
        if (!lan78xx_wait_reg_set(ncp, LAN78XX_REG_PMT_CTL, LAN78XX_PMT_CTL_READY)) {
            KPRINTF(20, ("lan78xx: device never reported READY\n"));
            return (-1);
        }
    }

    /* Lite soft-reset. */
    if (lan78xx_read_reg(ncp, LAN78XX_REG_HW_CFG, &val))
        return (-1);
    lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, val | LAN78XX_HW_CFG_LRST);

    if (!lan78xx_wait_reg_clear(ncp, LAN78XX_REG_HW_CFG, LAN78XX_HW_CFG_LRST)) {
        KPRINTF(20, ("lan78xx: LRST never cleared\n"));
        return (-1);
    }

    /*
     * Do NOT set BIR.
     *
     * Leaving BIR clear is what makes the chip drive the bulk-IN
     * endpoint in the "NAK-when-empty / DATA-when-ready" mode that DWC2
     * expects. With BIR=1 the chip stops responding to IN tokens
     * entirely on this silicon revision (no NAK, no ZLP — total
     * silence), and DWC2's watchdog has to force-fail every read after
     * 3 s, which surfaces as `act=0/18944 err=6 last_intr=0000` to the
     * SANA-II layer.
     *
     * The "NAK until we're ready" hand-off this would otherwise enable
     * is unnecessary: a normal LRST already keeps the bulk-IN silent
     * until the chip's RX path is enabled below (FCT_RX_CTL + MAC_RX).
     */

    /* Burst cap + bulk-in delay.  Pi3 is HS — use the high-speed sizing. */
    if (is_lan7500) {
        lan78xx_write_reg(ncp, burst_cap_reg, LAN78XX_BURST_MIN_BUFSZ);
        lan78xx_write_reg(ncp, bulk_in_reg, LAN78XX_DEFAULT_BULKIN_DELAY);

        if (lan78xx_read_reg(ncp, LAN78XX_REG_HW_CFG, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, val | LAN78XX_HW_CFG_BCE | LAN78XX_HW_CFG_MEF);

        /* Undocumented LAN7500 FIFO end markers. */
        lan78xx_write_reg(ncp, LAN78XX_REG_FCT_RX_FIFO_END, 0x27);
        lan78xx_write_reg(ncp, LAN78XX_REG_FCT_TX_FIFO_END, 0x17);
    } else {
        lan78xx_write_reg(ncp, burst_cap_reg, LAN78XX_7800_BURST_MAX_BUFSZ);
        lan78xx_write_reg(ncp, bulk_in_reg, LAN78XX_7800_DEFAULT_BULKIN_DELAY);

        if (lan78xx_read_reg(ncp, LAN78XX_REG_HW_CFG, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, val | LAN78XX_HW_CFG_MEF);

        if (lan78xx_read_reg(ncp, LAN78XX_REG_USB_CFG0, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_USB_CFG0, val | LAN78XX_USB_CFG0_BCE);
    }

    lan78xx_write_reg(ncp, LAN78XX_REG_INT_STS, 0xFFFFFFFFUL);
    {
        /* HS flow watermarks: ON=8704, OFF=1024 (units of 512 bytes), enable bit
         * 0x8000. */
        ULONG fct_flow = ((8704 / 512) & 0x7F) | (((1024 / 512) & 0x7F) << 16) | 0x00008000;
        lan78xx_write_reg(ncp, fct_flow_reg, fct_flow);
    }
    lan78xx_write_reg(ncp, LAN78XX_REG_FLOW, 0);

    /* PHY reset. */
    if (lan78xx_read_reg(ncp, LAN78XX_REG_PMT_CTL, &val))
        return (-1);
    lan78xx_write_reg(ncp, LAN78XX_REG_PMT_CTL, val | LAN78XX_PMT_CTL_PHY_RST);

    {
        int ntries;
        for (ntries = 0; ntries < 100; ntries++) {
            if (lan78xx_read_reg(ncp, LAN78XX_REG_PMT_CTL, &val))
                return (-1);
            if (!(val & LAN78XX_PMT_CTL_PHY_RST) && (val & LAN78XX_PMT_CTL_READY))
                break;
            psdDelayMS(10);
        }
        if (ntries == 100) {
            KPRINTF(20, ("lan78xx: PHY reset timed out (PMT_CTL=%08lx)\n", val));
            return (-1);
        }
    }

    /* LAN7801 uses RGMII — its MAC_CR is configured differently. */
    if (ncp->ncp_UnitProdID == LAN78XX_USB_PRODUCT_ID_7801) {
        if (lan78xx_read_reg(ncp, LAN78XX_REG_MAC_CR, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_MAC_CR, val & ~LAN78XX_MAC_CR_GMII_EN);
    }

    /* Without an EEPROM the chip can't have been pre-programmed with
     * speed/duplex defaults — tell the MAC to auto-sense from the PHY. */
    if (is_lan7500 || !ncp->ncp_EepromPresent) {
        if (lan78xx_read_reg(ncp, LAN78XX_REG_MAC_CR, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_MAC_CR, val | LAN78XX_MAC_CR_AUTO_SPEED | LAN78XX_MAC_CR_AUTO_DUPLEX);
    }

    /* Enable transmit path. */
    if (lan78xx_read_reg(ncp, LAN78XX_REG_MAC_TX, &val))
        return (-1);
    lan78xx_write_reg(ncp, LAN78XX_REG_MAC_TX, val | LAN78XX_MAC_TX_TXEN);

    if (lan78xx_read_reg(ncp, fct_tx_reg, &val))
        return (-1);
    lan78xx_write_reg(ncp, fct_tx_reg, val | LAN78XX_FCT_TX_CTL_EN);

    /* Max receive frame size — disable first, program, re-enable. */
    if (lan78xx_read_reg(ncp, LAN78XX_REG_MAC_RX, &val))
        return (-1);
    lan78xx_write_reg(ncp, LAN78XX_REG_MAC_RX, val & ~LAN78XX_MAC_RX_RXEN);

    if (lan78xx_read_reg(ncp, LAN78XX_REG_MAC_RX, &val))
        return (-1);
    val &= ~LAN78XX_MAC_RX_MAX_SIZE_MASK;
    val |= LAN78XX_MAC_RX_MAX_LEN(ETHER_MAX_VLAN_LEN);
    lan78xx_write_reg(ncp, LAN78XX_REG_MAC_RX, val | LAN78XX_MAC_RX_RXEN);

    if (lan78xx_read_reg(ncp, fct_rx_reg, &val))
        return (-1);
    lan78xx_write_reg(ncp, fct_rx_reg, val | LAN78XX_FCT_RX_CTL_EN);

    /* Accept broadcasts + our own unicast.  Multicast hash programming
     * is deferred to step (4) of the overall port plan. */
    {
        UWORD rfe_reg = is_lan7500 ? LAN78XX_REG_RFE_CTL : LAN78XX_REG_7800_RFE_CTL;
        if (lan78xx_read_reg(ncp, rfe_reg, &val))
            return (-1);
        val |= LAN78XX_RFE_CTL_BROADCAST | LAN78XX_RFE_CTL_UNICAST;
        lan78xx_write_reg(ncp, rfe_reg, val);
    }

    /* Enable LEDs on LAN7800 boards without an EEPROM so the link/act
     * indicators work at all. */
    if (ncp->ncp_UnitProdID == LAN78XX_USB_PRODUCT_ID_7800 && !ncp->ncp_EepromPresent) {
        if (lan78xx_read_reg(ncp, LAN78XX_REG_HW_CFG, &val))
            return (-1);
        lan78xx_write_reg(ncp, LAN78XX_REG_HW_CFG, val | LAN78XX_HW_CFG_LED0_EN | LAN78XX_HW_CFG_LED1_EN);
    }

    return (0);
}

/*
 * Indirect-register window used to program the 512-bit multicast hash
 * table (VHF) and the VLAN filter.  The chip serialises writes through
 * a tiny DP_* state machine: select the target, then for each word
 * push ADDR + DATA + CMD and wait for DP_SEL.DPRDY.
 */
static LONG lan78xx_dataport_wait(struct NepClassEth *ncp)
{
    return (lan78xx_wait_reg_set(ncp, LAN78XX_REG_DP_SEL, LAN78XX_DP_SEL_DPRDY) ? 0 : -1);
}

static LONG lan78xx_dataport_write(struct NepClassEth *ncp, ULONG sel, ULONG addr, ULONG cnt, const ULONG *data)
{
    ULONG val;
    ULONG i;

    if (lan78xx_dataport_wait(ncp) != 0)
        return (-1);

    if (lan78xx_read_reg(ncp, LAN78XX_REG_DP_SEL, &val))
        return (-1);
    lan78xx_write_reg(ncp, LAN78XX_REG_DP_SEL, (val & ~LAN78XX_DP_SEL_RSEL_MASK) | sel);

    for (i = 0; i < cnt; i++) {
        lan78xx_write_reg(ncp, LAN78XX_REG_DP_ADDR, addr + i);
        lan78xx_write_reg(ncp, LAN78XX_REG_DP_DATA, data[i]);
        lan78xx_write_reg(ncp, LAN78XX_REG_DP_CMD, LAN78XX_DP_CMD_WRITE);
        if (lan78xx_dataport_wait(ncp) != 0)
            return (-1);
    }

    return (0);
}

/*
 * Big-endian Ethernet CRC-32 — bytes are consumed MSB-first and the CRC
 * register shifts left.  Polynomial 0x04C11DB7.  This matches what the
 * LAN78xx hash-filter hardware computes on incoming frames.
 */
static ULONG lan78xx_ether_crc32_be(const UBYTE *buf, int len)
{
    ULONG crc = 0xFFFFFFFFUL;
    int i, j;

    for (i = 0; i < len; i++) {
        crc ^= ((ULONG)buf[i]) << 24;
        for (j = 0; j < 8; j++) {
            if (crc & 0x80000000UL)
                crc = (crc << 1) ^ 0x04C11DB7UL;
            else
                crc <<= 1;
        }
    }
    return (crc);
}

/*
 * Periodic PHY poll.  Mirrors the function of OpenBSD's mue_tick_task:
 * read BMSR, detect link state changes, emit SANA-II events.
 *
 * BMSR.LSTATUS latches-low on read (it stays low until the bit has been
 * read once after a drop), so we read it twice — the second read
 * reflects the current state.  Arming/disarming the RX pipe is the job
 * of the main task loop, which already gates on ncp_LinkUp; we just
 * flip the flag here.
 */
static void lan78xx_link_poll(struct NepClassEth *ncp)
{
    UWORD bmsr = 0;
    BOOL up;

    if (lan78xx_phy_read(ncp, LAN78XX_MII_BMSR, &bmsr))
        return;
    if (lan78xx_phy_read(ncp, LAN78XX_MII_BMSR, &bmsr))
        return;

    up = (bmsr & LAN78XX_BMSR_LSTATUS) ? TRUE : FALSE;

    if (up && !ncp->ncp_LinkUp) {
        ncp->ncp_LinkUp = 1;
        KPRINTF(1, ("lan78xx: link UP (BMSR=%04lx)\n", (ULONG)bmsr));
        nDoEvent(ncp, S2EVENT_CONNECT);
    } else if (!up && ncp->ncp_LinkUp) {
        ncp->ncp_LinkUp = 0;
        KPRINTF(1, ("lan78xx: link DOWN (BMSR=%04lx)\n", (ULONG)bmsr));

        /* Cancel any in-flight RX; the main loop will stop arming new
         * transfers while LinkUp is 0. */
        if (ncp->ncp_ReadPending) {
            psdAbortPipe(ncp->ncp_EPInPipe);
            psdWaitPipe(ncp->ncp_EPInPipe);
            ncp->ncp_ReadPending = NULL;
        }

        nDoEvent(ncp, S2EVENT_DISCONNECT);
    }
}
