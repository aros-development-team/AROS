/*
 * vbthci.device -- unit task and simulated controller.
 */

#include <aros/debug.h>
#include <exec/exec.h>
#include <exec/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "vbthci_intern.h"

#define NewList(list) NEWLIST(list)

#define VBT_TICK_MS 100

/* the simulated neighbourhood */
static const struct VBTFakeDevice fakedevs[] = {
    /* addr,                                 atype, IsLE, Dual, CoD,      Appear, UUID,   name,                RSSI */
    /* the keyboard is one dual-mode device: reachable over BR/EDR (HIDP) and LE (HOGP) */
    { { 0x01, 0x00, 0x0f, 0xdc, 0x1b, 0x00 }, 0, FALSE, TRUE,  0x002540, 0x03c1, 0x1124, "Virtual Keyboard", -48 },
    { { 0x02, 0x00, 0x0f, 0xdc, 0x1b, 0x00 }, 0, FALSE, FALSE, 0x002580, 0,      0x1124, "Virtual Mouse", -55 },
    { { 0x03, 0x00, 0x0f, 0xdc, 0x1b, 0x00 }, 0, FALSE, FALSE, 0x5a020c, 0,      0x1101, "Virtual Phone", -70 },
    { { 0x01, 0x01, 0x0f, 0xdc, 0x1b, 0x00 }, 0, TRUE,  FALSE, 0,        0x03c2, 0x1812, "VirtLE Mouse", -60 },
    { { 0x02, 0x01, 0x0f, 0xdc, 0x1b, 0x00 }, 0, TRUE,  FALSE, 0,        0x0341, 0x180d, "VirtLE Heart Rate", -66 },
};
#define NUMFAKEDEVS (sizeof(fakedevs)/sizeof(fakedevs[0]))

ULONG vbt_NumFakeDevices(void)
{
    return NUMFAKEDEVS;
}

const struct VBTFakeDevice *vbt_FakeDevice(ULONG idx)
{
    return (idx < NUMFAKEDEVS) ? &fakedevs[idx] : NULL;
}

static const UBYTE localaddr[6] = { 0x01, 0xbb, 0xaa, 0xdc, 0x1b, 0x00 };

/* *** event delivery *** */

void vbt_SendEvent(struct VBTHCIUnit *unit, UBYTE code, const UBYTE *params, ULONG len)
{
    struct BTHCIEventMsg *bem;

    if(!unit->vu_EventPort || (len > 255)) {
        return;
    }
    bem = AllocVec(sizeof(struct BTHCIEventMsg), MEMF_PUBLIC|MEMF_CLEAR);
    if(!bem) {
        return;
    }
    bem->bem_Msg.mn_Node.ln_Type = NT_MESSAGE;
    bem->bem_Msg.mn_ReplyPort = unit->vu_EventReplyPort;
    bem->bem_Msg.mn_Length = sizeof(struct BTHCIEventMsg);
    bem->bem_Event.bhe_EventType = code;
    bem->bem_Event.bhe_PayloadLength = len;
    if(len) {
        CopyMem((APTR) params, bem->bem_Event.bhe_Payload, len);
    }
    unit->vu_EventsPending++;
    PutMsg(unit->vu_EventPort, &bem->bem_Msg);
}

void vbt_CommandComplete(struct VBTHCIUnit *unit, UWORD opcode, const UBYTE *rp, ULONG rplen)
{
    UBYTE buf[255];
    buf[0] = 1;
    buf[1] = opcode & 0xff;
    buf[2] = opcode >> 8;
    if(rplen > 252) {
        rplen = 252;
    }
    if(rplen) {
        CopyMem((APTR) rp, &buf[3], rplen);
    }
    vbt_SendEvent(unit, 0x0e, buf, 3 + rplen);
}

void vbt_CommandStatus(struct VBTHCIUnit *unit, UWORD opcode, UBYTE status)
{
    UBYTE buf[4];
    buf[0] = status;
    buf[1] = 1;
    buf[2] = opcode & 0xff;
    buf[3] = opcode >> 8;
    vbt_SendEvent(unit, 0x0f, buf, 4);
}

/* *** timed events *** */

void vbt_Schedule(struct VBTHCIUnit *unit, UWORD kind, UWORD index, UWORD arg, ULONG delayms)
{
    struct VBTTimedEvent *te = AllocVec(sizeof(struct VBTTimedEvent), MEMF_PUBLIC|MEMF_CLEAR);
    if(te) {
        te->te_Due = unit->vu_Tick + delayms;
        te->te_Kind = kind;
        te->te_Index = index;
        te->te_Arg = arg;
        AddTail((struct List *) &unit->vu_Timed, (struct Node *) te);
    }
}

void vbt_CancelKind(struct VBTHCIUnit *unit, UWORD kind)
{
    struct VBTTimedEvent *te, *next;
    te = (struct VBTTimedEvent *) unit->vu_Timed.mlh_Head;
    while((next = (struct VBTTimedEvent *) te->te_Node.mln_Succ)) {
        if(te->te_Kind == kind) {
            Remove((struct Node *) te);
            FreeVec(te);
        }
        te = next;
    }
}

static void vbt_EmitInquiryResult(struct VBTHCIUnit *unit, UWORD idx)
{
    const struct VBTFakeDevice *fd = &fakedevs[idx];
    UBYTE buf[255];
    ULONG namelen = strlen(fd->fd_Name);

    memset(buf, 0, sizeof(buf));
    buf[0] = 1;                       /* num responses */
    CopyMem((APTR) fd->fd_Addr, &buf[1], 6);
    buf[7] = 1;                       /* page scan repetition mode */
    buf[8] = 0;                       /* reserved */
    buf[9] = fd->fd_CoD & 0xff;
    buf[10] = (fd->fd_CoD >> 8) & 0xff;
    buf[11] = (fd->fd_CoD >> 16) & 0xff;
    buf[12] = 0;                      /* clock offset */
    buf[13] = 0;
    buf[14] = (UBYTE) fd->fd_RSSI;
    /* EIR: complete local name + 16 bit uuid list */
    buf[15] = namelen + 1;
    buf[16] = 0x09;
    CopyMem((APTR) fd->fd_Name, &buf[17], namelen);
    buf[17 + namelen] = 3;
    buf[18 + namelen] = 0x03;
    buf[19 + namelen] = fd->fd_ServiceUUID & 0xff;
    buf[20 + namelen] = fd->fd_ServiceUUID >> 8;
    vbt_SendEvent(unit, 0x2f, buf, 255);
}

static void vbt_EmitAdvReport(struct VBTHCIUnit *unit, UWORD idx)
{
    const struct VBTFakeDevice *fd = &fakedevs[idx];
    UBYTE buf[64];
    UBYTE *ad;
    ULONG namelen = strlen(fd->fd_Name);
    ULONG adlen;

    buf[0] = 0x02;                    /* subevent: advertising report */
    buf[1] = 1;                       /* num reports */
    buf[2] = 0x00;                    /* ADV_IND */
    buf[3] = fd->fd_AddrType;
    CopyMem((APTR) fd->fd_Addr, &buf[4], 6);
    ad = &buf[11];
    ad[0] = 2; ad[1] = 0x01; ad[2] = 0x06;                       /* flags */
    ad[3] = 3; ad[4] = 0x03; ad[5] = fd->fd_ServiceUUID & 0xff; ad[6] = fd->fd_ServiceUUID >> 8;
    ad[7] = 3; ad[8] = 0x19; ad[9] = fd->fd_Appearance & 0xff; ad[10] = fd->fd_Appearance >> 8;
    if(namelen > 18) {
        namelen = 18;
    }
    ad[11] = namelen + 1; ad[12] = 0x09;
    CopyMem((APTR) fd->fd_Name, &ad[13], namelen);
    adlen = 13 + namelen;
    buf[10] = adlen;
    buf[11 + adlen] = (UBYTE) fd->fd_RSSI;
    vbt_SendEvent(unit, 0x3e, buf, 12 + adlen);
}

static void vbt_EmitRemoteName(struct VBTHCIUnit *unit, UWORD idx, const UBYTE *addr)
{
    UBYTE buf[255];
    memset(buf, 0, sizeof(buf));
    if(idx < NUMFAKEDEVS) {
        buf[0] = 0;
        CopyMem((APTR) fakedevs[idx].fd_Addr, &buf[1], 6);
        strncpy((char *) &buf[7], fakedevs[idx].fd_Name, 247);
    } else {
        buf[0] = 0x04; /* page timeout */
        CopyMem((APTR) addr, &buf[1], 6);
    }
    vbt_SendEvent(unit, 0x07, buf, 255);
}

static void vbt_ProcessTimed(struct VBTHCIUnit *unit)
{
    struct VBTTimedEvent *te, *next;
    te = (struct VBTTimedEvent *) unit->vu_Timed.mlh_Head;
    while((next = (struct VBTTimedEvent *) te->te_Node.mln_Succ)) {
        if((LONG) (unit->vu_Tick - te->te_Due) >= 0) {
            Remove((struct Node *) te);
            switch(te->te_Kind) {
            case VBTE_INQUIRY_RESULT:
                if(unit->vu_Inquiring) {
                    vbt_EmitInquiryResult(unit, te->te_Index);
                }
                break;
            case VBTE_INQUIRY_COMPLETE:
                if(unit->vu_Inquiring) {
                    UBYTE st = 0;
                    unit->vu_Inquiring = FALSE;
                    vbt_SendEvent(unit, 0x01, &st, 1);
                }
                break;
            case VBTE_LE_ADV_REPORT:
                if(unit->vu_LEScanning) {
                    vbt_EmitAdvReport(unit, te->te_Index);
                    /* repeat while scanning */
                    vbt_Schedule(unit, VBTE_LE_ADV_REPORT, te->te_Index, 0, 1500 + te->te_Index * 300);
                }
                break;
            case VBTE_REMOTE_NAME:
                vbt_EmitRemoteName(unit, te->te_Index, te->te_Addr);
                break;
            default:
                vbtp_Timed(unit, te);
                break;
            }
            FreeVec(te);
        }
        te = next;
    }
}

/* *** command handling *** */

static void vbt_HandleCommand(struct VBTHCIUnit *unit, const UBYTE *data, ULONG len)
{
    UWORD opcode;
    UBYTE plen;
    const UBYTE *p;
    UBYTE rp[255];
    ULONG n;

    if(len < 3) {
        return;
    }
    opcode = data[0] | (data[1] << 8);
    plen = data[2];
    p = &data[3];
    if(len < 3 + (ULONG) plen) {
        plen = len - 3;
    }
    memset(rp, 0, sizeof(rp));

    switch(opcode) {
    case 0x0c03: /* reset */
        vbtp_Reset(unit);
        unit->vu_Inquiring = FALSE;
        unit->vu_LEScanning = FALSE;
        vbt_CancelKind(unit, VBTE_INQUIRY_RESULT);
        vbt_CancelKind(unit, VBTE_INQUIRY_COMPLETE);
        vbt_CancelKind(unit, VBTE_LE_ADV_REPORT);
        vbt_CommandComplete(unit, opcode, rp, 1);
        break;

    case 0x1001: /* read local version */
        rp[0] = 0;
        rp[1] = 8;      /* HCI 4.2 */
        rp[2] = 0x00; rp[3] = 0x01;
        rp[4] = 8;      /* LMP 4.2 */
        rp[5] = 0x3f; rp[6] = 0x00; /* manufacturer: Bluetooth SIG */
        rp[7] = 0x01; rp[8] = 0x00;
        vbt_CommandComplete(unit, opcode, rp, 9);
        break;

    case 0x1003: { /* read local supported features */
        static const UBYTE feat[8] = { 0xbf, 0xfe, 0x8f, 0xfe, 0xdb, 0xff, 0x5b, 0x87 };
        rp[0] = 0;
        CopyMem((APTR) feat, &rp[1], 8);
        vbt_CommandComplete(unit, opcode, rp, 9);
        break;
    }

    case 0x1005: /* read buffer size */
        rp[0] = 0;
        rp[1] = 0xfd; rp[2] = 0x03; /* 1021 */
        rp[3] = 64;
        rp[4] = 8; rp[5] = 0;
        rp[6] = 8; rp[7] = 0;
        vbt_CommandComplete(unit, opcode, rp, 8);
        break;

    case 0x1009: /* read bd_addr */
        rp[0] = 0;
        CopyMem((APTR) localaddr, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        break;

    case 0x2002: /* le read buffer size */
        rp[0] = 0;
        rp[1] = 0xfb; rp[2] = 0x00; /* 251 */
        rp[3] = 8;
        vbt_CommandComplete(unit, opcode, rp, 4);
        break;

    case 0x0c14: /* read local name */
        rp[0] = 0;
        if(!unit->vu_LocalName[0]) {
            strcpy((char *) unit->vu_LocalName, "AROS virtual radio");
        }
        CopyMem(unit->vu_LocalName, &rp[1], 248);
        vbt_CommandComplete(unit, opcode, rp, 249);
        break;

    case 0x0c13: /* write local name */
        memset(unit->vu_LocalName, 0, 248);
        for(n = 0; (n < plen) && (n < 247); n++) {
            unit->vu_LocalName[n] = p[n];
        }
        vbt_CommandComplete(unit, opcode, rp, 1);
        break;

    case 0x0c1a: /* write scan enable */
        unit->vu_ScanEnable = plen ? p[0] : 0;
        vbt_CommandComplete(unit, opcode, rp, 1);
        break;

    case 0x0c24: /* write class of device */
        if(plen >= 3) {
            unit->vu_CoD = p[0] | (p[1] << 8) | (p[2] << 16);
        }
        vbt_CommandComplete(unit, opcode, rp, 1);
        break;

    case 0x0401: { /* inquiry */
        ULONG dur = (plen >= 4) ? p[3] : 8;
        ULONG i, cnt = 0;
        if(unit->vu_Inquiring) {
            vbt_CommandStatus(unit, opcode, 0x0c); /* command disallowed */
            break;
        }
        vbt_CommandStatus(unit, opcode, 0);
        unit->vu_Inquiring = TRUE;
        for(i = 0; i < NUMFAKEDEVS; i++) {
            if(VBT_HASCLASSIC(&fakedevs[i])) {
                vbt_Schedule(unit, VBTE_INQUIRY_RESULT, i, 0, 400 + cnt * 350);
                cnt++;
            }
        }
        if(dur * 1280 < 3000) {
            dur = 3;
        } else {
            dur = dur * 1280 / 1000;
            if(dur > 8) {
                dur = 8;
            }
        }
        vbt_Schedule(unit, VBTE_INQUIRY_COMPLETE, 0, 0, dur * 1000);
        break;
    }

    case 0x0402: /* inquiry cancel */
        unit->vu_Inquiring = FALSE;
        vbt_CancelKind(unit, VBTE_INQUIRY_RESULT);
        vbt_CancelKind(unit, VBTE_INQUIRY_COMPLETE);
        vbt_CommandComplete(unit, opcode, rp, 1);
        break;

    case 0x0419: { /* remote name request */
        ULONG i;
        UWORD idx = 0xffff;
        if(plen < 6) {
            vbt_CommandStatus(unit, opcode, 0x12);
            break;
        }
        for(i = 0; i < NUMFAKEDEVS; i++) {
            if(VBT_HASCLASSIC(&fakedevs[i]) && !memcmp(fakedevs[i].fd_Addr, p, 6)) {
                idx = i;
            }
        }
        vbt_CommandStatus(unit, opcode, 0);
        if(idx == 0xffff) {
            /* unknown device: page timeout after a while */
            struct VBTTimedEvent *te = AllocVec(sizeof(struct VBTTimedEvent), MEMF_PUBLIC|MEMF_CLEAR);
            if(te) {
                te->te_Due = unit->vu_Tick + 2000;
                te->te_Kind = VBTE_REMOTE_NAME;
                te->te_Index = 0xffff;
                CopyMem((APTR) p, te->te_Addr, 6);
                AddTail((struct List *) &unit->vu_Timed, (struct Node *) te);
            }
        } else {
            vbt_Schedule(unit, VBTE_REMOTE_NAME, idx, 0, 250);
        }
        break;
    }

    case 0x200c: { /* le set scan enable */
        BOOL enable = plen ? (p[0] ? TRUE : FALSE) : FALSE;
        vbt_CommandComplete(unit, opcode, rp, 1);
        if(enable && !unit->vu_LEScanning) {
            ULONG i, cnt = 0;
            unit->vu_LEScanning = TRUE;
            for(i = 0; i < NUMFAKEDEVS; i++) {
                if(VBT_HASLE(&fakedevs[i])) {
                    vbt_Schedule(unit, VBTE_LE_ADV_REPORT, i, 0, 300 + cnt * 400);
                    cnt++;
                }
            }
        } else if(!enable) {
            unit->vu_LEScanning = FALSE;
            vbt_CancelKind(unit, VBTE_LE_ADV_REPORT);
        }
        break;
    }

    default:
        if(!vbtp_HandleCommand(unit, opcode, p, plen)) {
            /* accept everything else */
            vbt_CommandComplete(unit, opcode, rp, 1);
        }
        break;
    }
}

/* *** requests *** */

LONG vbthci_QueueRequest(struct VBTHCIUnit *unit, struct IOBTHCIReq *ioreq)
{
    switch(ioreq->iobt_Req.io_Command) {
    case BTCMD_QUERYDEVICE: {
        struct TagItem *tag = (struct TagItem *) ioreq->iobt_Data;
        ULONG count = 0;
        while(tag && (tag->ti_Tag != TAG_END)) {
            switch(tag->ti_Tag) {
            case BTA_Author:
                *((STRPTR *) tag->ti_Data) = "The AROS Development Team";
                count++;
                break;
            case BTA_ProductName:
                *((STRPTR *) tag->ti_Data) = "Virtual Bluetooth radio";
                count++;
                break;
            case BTA_Description:
                *((STRPTR *) tag->ti_Data) = "Simulated dual-mode HCI controller for testing";
                count++;
                break;
            case BTA_Copyright:
                *((STRPTR *) tag->ti_Data) = "©2026 The AROS Development Team";
                count++;
                break;
            case BTA_Version:
                *((IPTR *) tag->ti_Data) = 1;
                count++;
                break;
            case BTA_Revision:
                *((IPTR *) tag->ti_Data) = 0;
                count++;
                break;
            case BTA_DriverVersion:
                *((IPTR *) tag->ti_Data) = 0x0100;
                count++;
                break;
            }
            tag++;
        }
        ioreq->iobt_Actual = count;
        return 0;
    }
    default:
        PutMsg(&unit->vu_Unit.unit_MsgPort, &ioreq->iobt_Req.io_Message);
        return -1;
    }
}

static void vbt_AbortReads(struct VBTHCIUnit *unit)
{
    struct IOBTHCIReq *ioreq;
    Forbid();
    while((ioreq = (struct IOBTHCIReq *) RemHead((struct List *) &unit->vu_ReadQueue))) {
        ioreq->iobt_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&ioreq->iobt_Req.io_Message);
    }
    Permit();
}

static void vbt_HandleRequest(struct VBTHCIUnit *unit, struct IOBTHCIReq *ioreq)
{
    switch(ioreq->iobt_Req.io_Command) {
    case BTCMD_ADDMSGPORT:
        unit->vu_EventPort = (struct MsgPort *) ioreq->iobt_Data;
        break;
    case BTCMD_REMMSGPORT:
        if(unit->vu_EventPort == (struct MsgPort *) ioreq->iobt_Data) {
            unit->vu_EventPort = NULL;
        }
        break;
    case BTCMD_WRITEHCI:
        vbt_HandleCommand(unit, ioreq->iobt_Data, ioreq->iobt_Length);
        ioreq->iobt_Actual = ioreq->iobt_Length;
        break;
    case BTCMD_READACL:
        Forbid();
        AddTail((struct List *) &unit->vu_ReadQueue, &ioreq->iobt_Req.io_Message.mn_Node);
        Permit();
        vbtp_DeliverReads(unit);
        return; /* replied later */
    case BTCMD_WRITEACL:
        vbtp_HandleACL(unit, ioreq->iobt_Data, ioreq->iobt_Length);
        ioreq->iobt_Actual = ioreq->iobt_Length;
        /* Credit the packet back with a Number Of Completed Packets event.
         * Without it the host's ACL flow-control credits deplete and never
         * recover, stalling further ACL traffic once the initial buffer count
         * is used up (e.g. midway through GATT service enumeration). */
        if(ioreq->iobt_Length >= 2) {
            const UBYTE *ad = (const UBYTE *) ioreq->iobt_Data;
            UBYTE ncp[5];
            UWORD h = (ad[0] | (ad[1] << 8)) & 0x0fff;
            ncp[0] = 1;                 /* number of handles */
            ncp[1] = h & 0xff;
            ncp[2] = (h >> 8) & 0x0f;
            ncp[3] = 1; ncp[4] = 0;     /* one completed packet on this handle */
            vbt_SendEvent(unit, 0x13, ncp, 5);
        }
        break;
    case CMD_RESET:
    case CMD_FLUSH:
        vbt_AbortReads(unit);
        break;
    default:
        ioreq->iobt_Req.io_Error = IOERR_NOCMD;
        break;
    }
    ReplyMsg(&ioreq->iobt_Req.io_Message);
}

/* *** the unit task *** */

void vbthci_UnitTask(void)
{
    struct Task *thistask = FindTask(NULL);
    struct VBTHCIUnit *unit = thistask->tc_UserData;
    struct IOBTHCIReq *ioreq;
    struct BTHCIEventMsg *bem;
    struct VBTTimedEvent *te;
    ULONG sigmask;
    ULONG sigs;
    BOOL ok = FALSE;

    NewList((struct List *) &unit->vu_ACLToHost);
    unit->vu_NextHandle = 0x0040;
    unit->vu_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    unit->vu_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
    unit->vu_Unit.unit_MsgPort.mp_SigTask = thistask;
    unit->vu_Unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);

    if((unit->vu_EventReplyPort = CreateMsgPort())) {
        if((unit->vu_TimerPort = CreateMsgPort())) {
            unit->vu_TimerReq = (struct timerequest *) CreateIORequest(unit->vu_TimerPort, sizeof(struct timerequest));
            if(unit->vu_TimerReq) {
                if(!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *) unit->vu_TimerReq, 0)) {
                    ok = TRUE;
                }
            }
        }
    }
    if(ok) {
        unit->vu_Task = thistask;
    }
    Forbid();
    if(unit->vu_ReadySigTask) {
        Signal(unit->vu_ReadySigTask, 1L<<unit->vu_ReadySignal);
    }
    Permit();

    if(ok) {
        sigmask = (1UL<<unit->vu_Unit.unit_MsgPort.mp_SigBit) |
                  (1UL<<unit->vu_EventReplyPort->mp_SigBit) |
                  (1UL<<unit->vu_TimerPort->mp_SigBit) | SIGBREAKF_CTRL_C;
        unit->vu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
        unit->vu_TimerReq->tr_time.tv_secs = 0;
        unit->vu_TimerReq->tr_time.tv_micro = VBT_TICK_MS * 1000;
        SendIO((struct IORequest *) unit->vu_TimerReq);
        unit->vu_TimerPending = TRUE;
        do {
            while((ioreq = (struct IOBTHCIReq *) GetMsg(&unit->vu_Unit.unit_MsgPort))) {
                vbt_HandleRequest(unit, ioreq);
            }
            while((bem = (struct BTHCIEventMsg *) GetMsg(unit->vu_EventReplyPort))) {
                unit->vu_EventsPending--;
                FreeVec(bem);
            }
            if(GetMsg(unit->vu_TimerPort)) {
                unit->vu_TimerPending = FALSE;
                unit->vu_Tick += VBT_TICK_MS;
                vbt_ProcessTimed(unit);
                unit->vu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                unit->vu_TimerReq->tr_time.tv_secs = 0;
                unit->vu_TimerReq->tr_time.tv_micro = VBT_TICK_MS * 1000;
                SendIO((struct IORequest *) unit->vu_TimerReq);
                unit->vu_TimerPending = TRUE;
            }
            sigs = Wait(sigmask);
        } while(!(sigs & SIGBREAKF_CTRL_C));

        unit->vu_Shutdown = TRUE;
        if(unit->vu_TimerPending) {
            AbortIO((struct IORequest *) unit->vu_TimerReq);
            WaitIO((struct IORequest *) unit->vu_TimerReq);
        }
        vbt_AbortReads(unit);
        while((ioreq = (struct IOBTHCIReq *) GetMsg(&unit->vu_Unit.unit_MsgPort))) {
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&ioreq->iobt_Req.io_Message);
        }
        /* wait for outstanding event messages to come back */
        {
            ULONG cnt = 0;
            while(unit->vu_EventsPending && (cnt++ < 50)) {
                while((bem = (struct BTHCIEventMsg *) GetMsg(unit->vu_EventReplyPort))) {
                    unit->vu_EventsPending--;
                    FreeVec(bem);
                }
                if(unit->vu_EventsPending) {
                    Delay(5);
                }
            }
        }
        while((te = (struct VBTTimedEvent *) RemHead((struct List *) &unit->vu_Timed))) {
            FreeVec(te);
        }
        {
            struct VBTACLData *ad;
            while((ad = (struct VBTACLData *) RemHead((struct List *) &unit->vu_ACLToHost))) {
                FreeVec(ad);
            }
        }
    }
    if(unit->vu_TimerReq) {
        if(ok) {
            CloseDevice((struct IORequest *) unit->vu_TimerReq);
        }
        DeleteIORequest((struct IORequest *) unit->vu_TimerReq);
    }
    if(unit->vu_TimerPort) {
        DeleteMsgPort(unit->vu_TimerPort);
    }
    if(unit->vu_EventReplyPort) {
        DeleteMsgPort(unit->vu_EventReplyPort);
    }
    FreeSignal(unit->vu_Unit.unit_MsgPort.mp_SigBit);
    Forbid();
    unit->vu_Task = NULL;
    if(unit->vu_ReadySigTask) {
        Signal(unit->vu_ReadySigTask, 1L<<unit->vu_ReadySignal);
    }
}
