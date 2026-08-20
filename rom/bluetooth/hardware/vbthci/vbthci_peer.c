/*
 * vbthci.device -- simulated remote devices (ACL level).
 *
 * Implements just enough of the peer side to exercise the stack end to end:
 * connection establishment (classic and LE), an L2CAP signaling responder
 * and initiator-side acceptance, an SDP server with a HID record for the
 * classic devices, a GATT server (HID over GATT) for the LE mouse, SSP
 * pairing (numeric comparison), and periodic HID input reports.
 */

#include <aros/debug.h>
#include <exec/exec.h>
#include <exec/errors.h>
#include <proto/exec.h>

#include <string.h>

#include "vbthci_intern.h"
#include "vbthci_devclass.h"

#define NewList(list) NEWLIST(list)

/* *** ACL to host *** */

/* /// "vbtp_QueueACL()" */
static void vbtp_QueueACL(struct VBTHCIUnit *unit, UWORD handle, const UBYTE *l2cap, ULONG len)
{
    struct VBTACLData *ad = AllocVec(sizeof(struct VBTACLData) + 4 + len, MEMF_PUBLIC|MEMF_CLEAR);
    if(!ad) {
        return;
    }
    ad->ad_Length = 4 + len;
    ad->ad_Data[0] = handle & 0xff;
    ad->ad_Data[1] = ((handle >> 8) & 0x0f) | 0x20; /* first packet */
    ad->ad_Data[2] = len & 0xff;
    ad->ad_Data[3] = len >> 8;
    CopyMem((APTR) l2cap, &ad->ad_Data[4], len);
    AddTail((struct List *) &unit->vu_ACLToHost, (struct Node *) ad);
    vbtp_DeliverReads(unit);
}
/* \\\ */

/* /// "vbtp_SendPDU()" */
static void vbtp_SendPDU(struct VBTHCIUnit *unit, struct VBTLink *ln, UWORD cid, const UBYTE *payload, ULONG len)
{
    UBYTE buf[600];
    if(len + 4 > sizeof(buf)) {
        return;
    }
    buf[0] = len & 0xff;
    buf[1] = len >> 8;
    buf[2] = cid & 0xff;
    buf[3] = cid >> 8;
    CopyMem((APTR) payload, &buf[4], len);
    vbtp_QueueACL(unit, ln->ln_Handle, buf, len + 4);
}
/* \\\ */

/* /// "vbtp_DeliverReads()" */
void vbtp_DeliverReads(struct VBTHCIUnit *unit)
{
    struct IOBTHCIReq *ioreq;
    struct VBTACLData *ad;

    while(unit->vu_ACLToHost.mlh_Head->mln_Succ) {
        Forbid();
        ioreq = (struct IOBTHCIReq *) RemHead((struct List *) &unit->vu_ReadQueue);
        Permit();
        if(!ioreq) {
            return;
        }
        ad = (struct VBTACLData *) RemHead((struct List *) &unit->vu_ACLToHost);
        if(ad->ad_Length > ioreq->iobt_Length) {
            ioreq->iobt_Req.io_Error = BTIOERR_OVERFLOW;
            ioreq->iobt_Actual = 0;
        } else {
            CopyMem(ad->ad_Data, ioreq->iobt_Data, ad->ad_Length);
            ioreq->iobt_Actual = ad->ad_Length;
        }
        FreeVec(ad);
        ReplyMsg(&ioreq->iobt_Req.io_Message);
    }
}
/* \\\ */

/* *** links *** */

/* /// "vbtp_FindLink()" */
static struct VBTLink * vbtp_FindLink(struct VBTHCIUnit *unit, UWORD handle)
{
    UWORD n;
    for(n = 0; n < VBT_MAXLINKS; n++) {
        if(unit->vu_Links[n].ln_Used && (unit->vu_Links[n].ln_Handle == handle)) {
            return(&unit->vu_Links[n]);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "vbtp_LinkIndex()" */
static UWORD vbtp_LinkIndex(struct VBTHCIUnit *unit, struct VBTLink *ln)
{
    return(ln - unit->vu_Links);
}
/* \\\ */

/* /// "vbtp_Reset()" */
void vbtp_Reset(struct VBTHCIUnit *unit)
{
    struct VBTACLData *ad;
    memset(unit->vu_Links, 0, sizeof(unit->vu_Links));
    unit->vu_NextHandle = 0x0040;
    while((ad = (struct VBTACLData *) RemHead((struct List *) &unit->vu_ACLToHost))) {
        FreeVec(ad);
    }
    vbt_CancelKind(unit, VBTE_CONN_COMPLETE);
    vbt_CancelKind(unit, VBTE_DISCONN_COMPLETE);
    vbt_CancelKind(unit, VBTE_HID_REPORT);
    vbt_CancelKind(unit, VBTE_PAIR_STEP);
    vbt_CancelKind(unit, VBTE_LE_NOTIFY);
}
/* \\\ */

/* *** SDP server *** */

/* /// "vbtp_BuildHIDRecord()" */
/* One HID service record: attributes 0x0000 (handle), 0x0001 (HID class),
   0x0004 (L2CAP PSM 0x11 + HIDP), 0x0009 (HID profile), 0x000d (PSM 0x13),
   0x0100 (name), 0x0206 (HIDDescriptorList - the report descriptor). Encoded
   by hand; the report descriptor is the device's own (mouse/keyboard). */
static ULONG vbtp_BuildHIDRecord(UBYTE *buf, ULONG handle, const struct VBTFakeDevice *fd)
{
    UBYTE *p = buf;
    CONST_STRPTR name = fd ? fd->fd_Name : (CONST_STRPTR) "HID device";
    ULONG namelen = strlen(name);
    ULONG desclen;
    const UBYTE *desc = vbtp_HidReportDesc(fd, &desclen);
    UBYTE *seqlen;

    /* attribute 0x0000: record handle */
    *p++ = 0x09; *p++ = 0x00; *p++ = 0x00;
    *p++ = 0x0a; *p++ = handle >> 24; *p++ = handle >> 16; *p++ = handle >> 8; *p++ = handle;
    /* attribute 0x0001: service class id list { HID } */
    *p++ = 0x09; *p++ = 0x00; *p++ = 0x01;
    *p++ = 0x35; *p++ = 0x03; *p++ = 0x19; *p++ = 0x11; *p++ = 0x24;
    /* attribute 0x0004: protocol descriptor list { { L2CAP, 0x0011 }, { HIDP } } */
    *p++ = 0x09; *p++ = 0x00; *p++ = 0x04;
    *p++ = 0x35; *p++ = 0x0d;
    *p++ = 0x35; *p++ = 0x06; *p++ = 0x19; *p++ = 0x01; *p++ = 0x00; *p++ = 0x09; *p++ = 0x00; *p++ = 0x11;
    *p++ = 0x35; *p++ = 0x03; *p++ = 0x19; *p++ = 0x00; *p++ = 0x11;
    /* attribute 0x0009: profile descriptor list { { HID, 0x0101 } } */
    *p++ = 0x09; *p++ = 0x00; *p++ = 0x09;
    *p++ = 0x35; *p++ = 0x08;
    *p++ = 0x35; *p++ = 0x06; *p++ = 0x19; *p++ = 0x11; *p++ = 0x24; *p++ = 0x09; *p++ = 0x01; *p++ = 0x01;
    /* attribute 0x000d: additional protocols { { { L2CAP, 0x0013 }, { HIDP } } } */
    *p++ = 0x09; *p++ = 0x00; *p++ = 0x0d;
    *p++ = 0x35; *p++ = 0x0f;
    *p++ = 0x35; *p++ = 0x0d;
    *p++ = 0x35; *p++ = 0x06; *p++ = 0x19; *p++ = 0x01; *p++ = 0x00; *p++ = 0x09; *p++ = 0x00; *p++ = 0x13;
    *p++ = 0x35; *p++ = 0x03; *p++ = 0x19; *p++ = 0x00; *p++ = 0x11;
    /* attribute 0x0100: service name */
    *p++ = 0x09; *p++ = 0x01; *p++ = 0x00;
    *p++ = 0x25; *p++ = namelen;
    CopyMem((APTR) name, p, namelen);
    p += namelen;
    /* attribute 0x0206: HIDDescriptorList = { { UINT8 0x22 (Report), <descriptor> } } */
    *p++ = 0x09; *p++ = 0x02; *p++ = 0x06;
    *p++ = 0x35; *p++ = (UBYTE)(6 + desclen);   /* list sequence: one record */
    *p++ = 0x35; *p++ = (UBYTE)(4 + desclen);   /* record sequence */
    *p++ = 0x08; *p++ = 0x22;                   /* UINT8 descriptor type = Report */
    *p++ = 0x25; *p++ = (UBYTE) desclen;        /* text string (descriptor bytes) */
    CopyMem((APTR) desc, p, desclen);
    p += desclen;
    (void) seqlen;
    return(p - buf);
}
/* \\\ */

/* /// "vbtp_SDPRequest()" */
static void vbtp_SDPRequest(struct VBTHCIUnit *unit, struct VBTLink *ln, struct VBTChan *ch,
                            const UBYTE *req, ULONG len)
{
    UBYTE rsp[560];
    UBYTE attrs[520];
    UWORD tid;
    ULONG attrlen;

    if(len < 5) {
        return;
    }
    tid = (req[1] << 8) | req[2];
    switch(req[0]) {
    case 0x02: { /* service search request -> one handle */
        rsp[0] = 0x03;
        rsp[1] = tid >> 8;
        rsp[2] = tid & 0xff;
        rsp[3] = 0; rsp[4] = 9;  /* param len */
        rsp[5] = 0; rsp[6] = 1;  /* total count */
        rsp[7] = 0; rsp[8] = 1;  /* current count */
        rsp[9] = 0x00; rsp[10] = 0x01; rsp[11] = 0x00; rsp[12] = 0x00; /* handle 0x00010000 */
        rsp[13] = 0;             /* no continuation */
        vbtp_SendPDU(unit, ln, ch->lc_RemoteCID, rsp, 14);
        break;
    }
    case 0x04: { /* service attribute request */
        const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
        ULONG plen;
        attrlen = vbtp_BuildHIDRecord(attrs, 0x00010000, fd);
        rsp[0] = 0x05;
        rsp[1] = tid >> 8;
        rsp[2] = tid & 0xff;
        plen = 2 + 2 + attrlen + 1; /* bytecount + seq hdr + attrs + cont */
        rsp[3] = plen >> 8; rsp[4] = plen & 0xff;
        rsp[5] = (attrlen + 2) >> 8; rsp[6] = (attrlen + 2) & 0xff; /* attribute list byte count */
        rsp[7] = 0x35; rsp[8] = attrlen; /* outer sequence */
        CopyMem(attrs, &rsp[9], attrlen);
        rsp[9 + attrlen] = 0; /* no continuation */
        vbtp_SendPDU(unit, ln, ch->lc_RemoteCID, rsp, 10 + attrlen);
        break;
    }
    default: {
        /* error response */
        rsp[0] = 0x01;
        rsp[1] = tid >> 8;
        rsp[2] = tid & 0xff;
        rsp[3] = 0; rsp[4] = 2;
        rsp[5] = 0; rsp[6] = 4; /* invalid PDU size / request */
        vbtp_SendPDU(unit, ln, ch->lc_RemoteCID, rsp, 7);
        break;
    }
    }
}
/* \\\ */

/* *** GATT server (LE HID mouse) *** */

/*
 * handle layout:
 * 0x0001 primary service GAP (0x1800)
 * 0x0002  char decl, 0x0003 device name value
 * 0x0010 primary service HID (0x1812)
 * 0x0011  char decl report map, 0x0012 report map value
 * 0x0013  char decl report (input), 0x0014 report value, 0x0015 CCCD, 0x0016 report reference
 * 0x0017  char decl protocol mode, 0x0018 protocol mode value
 */

/* /// "vbtp_ATTError()" */
static void vbtp_ATTError(struct VBTHCIUnit *unit, struct VBTLink *ln, UBYTE reqop, UWORD handle, UBYTE code)
{
    UBYTE rsp[5];
    rsp[0] = 0x01;
    rsp[1] = reqop;
    rsp[2] = handle & 0xff;
    rsp[3] = handle >> 8;
    rsp[4] = code;
    vbtp_SendPDU(unit, ln, 0x0004, rsp, 5);
}
/* \\\ */

/* /// "vbtp_ATTRequest()" */
static void vbtp_ATTRequest(struct VBTHCIUnit *unit, struct VBTLink *ln, const UBYTE *req, ULONG len)
{
    UBYTE rsp[64];
    const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
    CONST_STRPTR name = fd ? fd->fd_Name : (CONST_STRPTR) "LE device";

    if(len < 1) {
        return;
    }
    switch(req[0]) {
    case 0x02: /* exchange MTU request */
        if(len < 3) {
            return;
        }
        ln->ln_MTU = 64;
        rsp[0] = 0x03;
        rsp[1] = 64; rsp[2] = 0;
        vbtp_SendPDU(unit, ln, 0x0004, rsp, 3);
        break;

    case 0x10: { /* read by group type (services) */
        UWORD start, end;
        if(len < 7) {
            return;
        }
        start = req[1] | (req[2] << 8);
        end = req[3] | (req[4] << 8);
        (void) end;
        if(start <= 0x0001) {
            rsp[0] = 0x11;
            rsp[1] = 6; /* entry length */
            rsp[2] = 0x01; rsp[3] = 0x00; /* start */
            rsp[4] = 0x0f; rsp[5] = 0x00; /* end */
            rsp[6] = 0x00; rsp[7] = 0x18; /* GAP */
            rsp[8] = 0x10; rsp[9] = 0x00;
            rsp[10] = 0x18; rsp[11] = 0x00;
            rsp[12] = 0x12; rsp[13] = 0x18; /* HID */
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 14);
        } else {
            vbtp_ATTError(unit, ln, 0x10, start, 0x0a); /* attribute not found */
        }
        break;
    }

    case 0x08: { /* read by type (characteristics, report reference...) */
        UWORD start, end, type;
        if(len < 7) {
            return;
        }
        start = req[1] | (req[2] << 8);
        end = req[3] | (req[4] << 8);
        type = req[5] | (req[6] << 8);
        if(type == 0x2803) { /* characteristic declarations */
            if((start <= 0x0002) && (end >= 0x0002)) {
                rsp[0] = 0x09;
                rsp[1] = 7;
                rsp[2] = 0x02; rsp[3] = 0x00;
                rsp[4] = 0x02;               /* read */
                rsp[5] = 0x03; rsp[6] = 0x00;
                rsp[7] = 0x00; rsp[8] = 0x2a; /* device name */
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 9);
            } else if((start <= 0x0011) && (end >= 0x0011)) {
                rsp[0] = 0x09;
                rsp[1] = 7;
                rsp[2] = 0x11; rsp[3] = 0x00;
                rsp[4] = 0x02;               /* read */
                rsp[5] = 0x12; rsp[6] = 0x00;
                rsp[7] = 0x4b; rsp[8] = 0x2a; /* report map */
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 9);
            } else if((start <= 0x0013) && (end >= 0x0013)) {
                rsp[0] = 0x09;
                rsp[1] = 7;
                rsp[2] = 0x13; rsp[3] = 0x00;
                rsp[4] = 0x12;               /* read | notify */
                rsp[5] = 0x14; rsp[6] = 0x00;
                rsp[7] = 0x4d; rsp[8] = 0x2a; /* report */
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 9);
            } else if((start <= 0x0017) && (end >= 0x0017)) {
                rsp[0] = 0x09;
                rsp[1] = 7;
                rsp[2] = 0x17; rsp[3] = 0x00;
                rsp[4] = 0x06;               /* read/write without response */
                rsp[5] = 0x18; rsp[6] = 0x00;
                rsp[7] = 0x4e; rsp[8] = 0x2a; /* protocol mode */
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 9);
            } else {
                vbtp_ATTError(unit, ln, 0x08, start, 0x0a);
            }
        } else if(type == 0x2908) { /* report reference */
            if((start <= 0x0016) && (end >= 0x0016)) {
                rsp[0] = 0x09;
                rsp[1] = 4;
                rsp[2] = 0x16; rsp[3] = 0x00;
                rsp[4] = 0x00; /* report id 0 */
                rsp[5] = 0x01; /* input */
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 6);
            } else {
                vbtp_ATTError(unit, ln, 0x08, start, 0x0a);
            }
        } else {
            vbtp_ATTError(unit, ln, 0x08, start, 0x0a);
        }
        break;
    }

    case 0x04: { /* find information (descriptors) */
        UWORD start, end;
        if(len < 5) {
            return;
        }
        start = req[1] | (req[2] << 8);
        end = req[3] | (req[4] << 8);
        if((start <= 0x0015) && (end >= 0x0015)) {
            rsp[0] = 0x05;
            rsp[1] = 0x01; /* 16 bit uuids */
            rsp[2] = 0x15; rsp[3] = 0x00;
            rsp[4] = 0x02; rsp[5] = 0x29; /* CCCD */
            rsp[6] = 0x16; rsp[7] = 0x00;
            rsp[8] = 0x08; rsp[9] = 0x29; /* report reference */
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 10);
        } else {
            vbtp_ATTError(unit, ln, 0x04, start, 0x0a);
        }
        break;
    }

    case 0x0a: { /* read request */
        UWORD handle;
        if(len < 3) {
            return;
        }
        handle = req[1] | (req[2] << 8);
        switch(handle) {
        case 0x0003: { /* device name */
            ULONG n = strlen(name);
            if(n > 22) {
                n = 22;
            }
            rsp[0] = 0x0b;
            CopyMem((APTR) name, &rsp[1], n);
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 1 + n);
            break;
        }
        case 0x0012: { /* report map */
            ULONG total;
            const UBYTE *map = vbtp_HidReportDesc(fd, &total);
            ULONG n = total;
            ULONG mtu = ln->ln_MTU ? ln->ln_MTU : 23;
            if(n > mtu - 1) {
                n = mtu - 1;
            }
            rsp[0] = 0x0b;
            CopyMem((APTR) map, &rsp[1], n);
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 1 + n);
            break;
        }
        case 0x0014: { /* report value: return an empty report of the right size */
            ULONG rlen = vbtp_HidInputReport(fd, 1, &rsp[1]); /* odd step == idle/release */
            rsp[0] = 0x0b;
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 1 + rlen);
            break;
        }
        case 0x0015: /* CCCD */
            rsp[0] = 0x0b;
            rsp[1] = ln->ln_Notify ? 1 : 0;
            rsp[2] = 0;
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 3);
            break;
        case 0x0018: /* protocol mode */
            rsp[0] = 0x0b;
            rsp[1] = 0x01; /* report protocol */
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 2);
            break;
        default:
            vbtp_ATTError(unit, ln, 0x0a, handle, 0x0a);
            break;
        }
        break;
    }

    case 0x0c: { /* read blob request (report map continuation) */
        UWORD handle, offset;
        if(len < 5) {
            return;
        }
        handle = req[1] | (req[2] << 8);
        offset = req[3] | (req[4] << 8);
        if(handle == 0x0012) {
            ULONG total;
            const UBYTE *map = vbtp_HidReportDesc(fd, &total);
            ULONG mtu = ln->ln_MTU ? ln->ln_MTU : 23;
            if(offset > total) {
                vbtp_ATTError(unit, ln, 0x0c, handle, 0x07); /* invalid offset */
            } else {
                ULONG n = total - offset;
                if(n > mtu - 1) {
                    n = mtu - 1;
                }
                rsp[0] = 0x0d;
                CopyMem((APTR) &map[offset], &rsp[1], n);
                vbtp_SendPDU(unit, ln, 0x0004, rsp, 1 + n);
            }
        } else {
            vbtp_ATTError(unit, ln, 0x0c, handle, 0x0a);
        }
        break;
    }

    case 0x12: { /* write request */
        UWORD handle;
        if(len < 3) {
            return;
        }
        handle = req[1] | (req[2] << 8);
        if(handle == 0x0015) { /* CCCD */
            ln->ln_Notify = (len >= 4) && (req[3] & 1);
            rsp[0] = 0x13;
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 1);
            if(ln->ln_Notify) {
                vbt_Schedule(unit, VBTE_LE_NOTIFY, vbtp_LinkIndex(unit, ln), 0, 500);
            }
        } else if((handle == 0x0014) || (handle == 0x0018)) {
            rsp[0] = 0x13;
            vbtp_SendPDU(unit, ln, 0x0004, rsp, 1);
        } else {
            vbtp_ATTError(unit, ln, 0x12, handle, 0x03); /* write not permitted */
        }
        break;
    }

    case 0x52: /* write command: accept silently */
        break;

    default:
        vbtp_ATTError(unit, ln, req[0], 0, 0x06); /* request not supported */
        break;
    }
}
/* \\\ */

/* *** L2CAP signaling (peer side) *** */

/* /// "vbtp_FindChanByLocal()" */
static struct VBTChan * vbtp_FindChanByLocal(struct VBTLink *ln, UWORD cid)
{
    UWORD n;
    for(n = 0; n < VBT_MAXCHANS; n++) {
        if(ln->ln_Chans[n].lc_State && (ln->ln_Chans[n].lc_LocalCID == cid)) {
            return(&ln->ln_Chans[n]);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "vbtp_ChannelOpened()" */
/* An L2CAP channel just reached the open state. If it is the HIDP interrupt
   channel (PSM 0x13), start streaming HID input reports on it. */
static void vbtp_ChannelOpened(struct VBTHCIUnit *unit, struct VBTLink *ln, struct VBTChan *ch)
{
    if(ch->lc_PSM == 0x0013) {
        vbt_Schedule(unit, VBTE_HID_REPORT, vbtp_LinkIndex(unit, ln), 0, 500);
    }
}
/* \\\ */

/* /// "vbtp_Signaling()" */
static void vbtp_Signaling(struct VBTHCIUnit *unit, struct VBTLink *ln, const UBYTE *sig, ULONG len)
{
    UBYTE rsp[32];
    UBYTE code, id;
    UWORD dlen;

    while(len >= 4) {
        code = sig[0];
        id = sig[1];
        dlen = sig[2] | (sig[3] << 8);
        if(len < 4 + (ULONG) dlen) {
            return;
        }
        switch(code) {
        case 0x02: { /* connection request from the host */
            UWORD psm = sig[4] | (sig[5] << 8);
            UWORD scid = sig[6] | (sig[7] << 8);
            struct VBTChan *ch = NULL;
            UWORD n;
            for(n = 0; n < VBT_MAXCHANS; n++) {
                if(!ln->ln_Chans[n].lc_State) {
                    ch = &ln->ln_Chans[n];
                    break;
                }
            }
            rsp[0] = 0x03;
            rsp[1] = id;
            rsp[2] = 8; rsp[3] = 0;
            if(ch && ((psm == 0x0001) || (psm == 0x0011) || (psm == 0x0013))) {
                ch->lc_State = 1;
                ch->lc_PSM = psm;
                ch->lc_RemoteCID = scid;
                ch->lc_LocalCID = 0x0070 + ln->ln_NextCID++;
                rsp[4] = ch->lc_LocalCID & 0xff; rsp[5] = ch->lc_LocalCID >> 8;
                rsp[6] = scid & 0xff; rsp[7] = scid >> 8;
                rsp[8] = 0; rsp[9] = 0; /* success */
                rsp[10] = 0; rsp[11] = 0;
                vbtp_SendPDU(unit, ln, 0x0001, rsp, 12);
                /* our configure request */
                rsp[0] = 0x04;
                rsp[1] = 0x80 + ln->ln_NextCID;
                rsp[2] = 4; rsp[3] = 0;
                rsp[4] = scid & 0xff; rsp[5] = scid >> 8;
                rsp[6] = 0; rsp[7] = 0;
                vbtp_SendPDU(unit, ln, 0x0001, rsp, 8);
            } else {
                rsp[4] = 0; rsp[5] = 0;
                rsp[6] = scid & 0xff; rsp[7] = scid >> 8;
                rsp[8] = ch ? 0x02 : 0x04; rsp[9] = 0; /* refused */
                rsp[10] = 0; rsp[11] = 0;
                vbtp_SendPDU(unit, ln, 0x0001, rsp, 12);
            }
            break;
        }
        case 0x04: { /* configure request */
            UWORD dcid = sig[4] | (sig[5] << 8);
            struct VBTChan *ch = vbtp_FindChanByLocal(ln, dcid);
            rsp[0] = 0x05;
            rsp[1] = id;
            rsp[2] = 6; rsp[3] = 0;
            rsp[4] = ch ? (ch->lc_RemoteCID & 0xff) : 0;
            rsp[5] = ch ? (ch->lc_RemoteCID >> 8) : 0;
            rsp[6] = 0; rsp[7] = 0; /* flags */
            rsp[8] = 0; rsp[9] = 0; /* success */
            vbtp_SendPDU(unit, ln, 0x0001, rsp, 10);
            if(ch) {
                ch->lc_ConfIn = TRUE;
                if(ch->lc_ConfOut) {
                    ch->lc_State = 2;
                    vbtp_ChannelOpened(unit, ln, ch);
                }
            }
            break;
        }
        case 0x05: { /* configure response (to our request) */
            UWORD scid = sig[4] | (sig[5] << 8);
            struct VBTChan *ch = NULL;
            UWORD n;
            for(n = 0; n < VBT_MAXCHANS; n++) {
                if(ln->ln_Chans[n].lc_State && (ln->ln_Chans[n].lc_RemoteCID == scid)) {
                    ch = &ln->ln_Chans[n];
                    break;
                }
            }
            if(!ch) {
                /* match by any configuring channel */
                for(n = 0; n < VBT_MAXCHANS; n++) {
                    if(ln->ln_Chans[n].lc_State == 1) {
                        ch = &ln->ln_Chans[n];
                        break;
                    }
                }
            }
            if(ch) {
                ch->lc_ConfOut = TRUE;
                if(ch->lc_ConfIn) {
                    ch->lc_State = 2;
                    vbtp_ChannelOpened(unit, ln, ch);
                }
            }
            break;
        }
        case 0x06: { /* disconnection request */
            UWORD dcid = sig[4] | (sig[5] << 8);
            UWORD scid = sig[6] | (sig[7] << 8);
            struct VBTChan *ch = vbtp_FindChanByLocal(ln, dcid);
            if(ch) {
                memset(ch, 0, sizeof(*ch));
            }
            rsp[0] = 0x07;
            rsp[1] = id;
            rsp[2] = 4; rsp[3] = 0;
            rsp[4] = dcid & 0xff; rsp[5] = dcid >> 8;
            rsp[6] = scid & 0xff; rsp[7] = scid >> 8;
            vbtp_SendPDU(unit, ln, 0x0001, rsp, 8);
            break;
        }
        default:
            break;
        }
        sig += 4 + dlen;
        len -= 4 + dlen;
    }
}
/* \\\ */

/* *** inbound ACL *** */

/* /// "vbtp_ProcessPDU()" */
static void vbtp_ProcessPDU(struct VBTHCIUnit *unit, struct VBTLink *ln, const UBYTE *pdu, ULONG len)
{
    UWORD plen, cid;
    if(len < 4) {
        return;
    }
    plen = pdu[0] | (pdu[1] << 8);
    cid = pdu[2] | (pdu[3] << 8);
    if(len < 4 + (ULONG) plen) {
        return;
    }
    if(cid == 0x0001) {
        vbtp_Signaling(unit, ln, &pdu[4], plen);
    } else if(cid == 0x0004) {
        vbtp_ATTRequest(unit, ln, &pdu[4], plen);
    } else {
        struct VBTChan *ch = vbtp_FindChanByLocal(ln, cid);
        if(ch && (ch->lc_PSM == 0x0001)) {
            vbtp_SDPRequest(unit, ln, ch, &pdu[4], plen);
        } else if(ch && (ch->lc_PSM == 0x0011) && (plen >= 1)) {
            /* HIDP control channel: acknowledge each transaction (SET_PROTOCOL,
               SET_REPORT, ...) with HANDSHAKE(successful). GET_* is not used by
               our host, which reads input reports on the interrupt channel. */
            UBYTE hs = 0x00; /* transaction type HANDSHAKE (0x0), result successful (0x0) */
            vbtp_SendPDU(unit, ln, ch->lc_RemoteCID, &hs, 1);
        }
        /* HIDP interrupt channel: input only, host output accepted silently */
    }
}
/* \\\ */

/* /// "vbtp_HandleACL()" */
void vbtp_HandleACL(struct VBTHCIUnit *unit, const UBYTE *data, ULONG len)
{
    UWORD handle;
    UBYTE pb;
    UWORD dlen;
    struct VBTLink *ln;

    if(len < 4) {
        return;
    }
    handle = (data[0] | (data[1] << 8)) & 0x0fff;
    pb = (data[1] >> 4) & 0x03;
    dlen = data[2] | (data[3] << 8);
    if(len < 4 + (ULONG) dlen) {
        dlen = len - 4;
    }
    ln = vbtp_FindLink(unit, handle);
    if(!ln) {
        return;
    }
    if(pb == 0x02 || pb == 0x00) { /* first fragment */
        ln->ln_RxLen = 0;
        if(dlen >= 2) {
            ln->ln_RxExpected = (data[4] | (data[5] << 8)) + 4;
        } else {
            ln->ln_RxExpected = 0; /* header split: unsupported, drop */
            return;
        }
    }
    if(ln->ln_RxLen + dlen > VBT_RXBUFSIZE) {
        ln->ln_RxLen = 0;
        return;
    }
    CopyMem((APTR) &data[4], &ln->ln_RxBuf[ln->ln_RxLen], dlen);
    ln->ln_RxLen += dlen;
    if(ln->ln_RxExpected && (ln->ln_RxLen >= ln->ln_RxExpected)) {
        vbtp_ProcessPDU(unit, ln, ln->ln_RxBuf, ln->ln_RxLen);
        ln->ln_RxLen = 0;
        ln->ln_RxExpected = 0;
    }
}
/* \\\ */

/* *** connection commands *** */

/* /// "vbtp_HandleCommand()" */
BOOL vbtp_HandleCommand(struct VBTHCIUnit *unit, UWORD opcode, const UBYTE *p, UBYTE plen)
{
    switch(opcode) {
    case 0x0405: { /* create connection */
        ULONG i;
        UWORD idx = 0xffff;
        UWORD n;
        if(plen < 6) {
            vbt_CommandStatus(unit, opcode, 0x12);
            return(TRUE);
        }
        for(i = 0; i < vbt_NumFakeDevices(); i++) {
            const struct VBTFakeDevice *fd = vbt_FakeDevice(i);
            if(VBT_HASCLASSIC(fd) && !memcmp(fd->fd_Addr, p, 6)) {
                idx = i;
            }
        }
        vbt_CommandStatus(unit, opcode, 0);
        if(idx == 0xffff) {
            /* unknown: connection complete with page timeout */
            vbt_Schedule(unit, VBTE_CONN_COMPLETE, 0xff00, 0, 2000);
        } else {
            /* find a free link */
            for(n = 0; n < VBT_MAXLINKS; n++) {
                if(!unit->vu_Links[n].ln_Used) {
                    break;
                }
            }
            if(n == VBT_MAXLINKS) {
                vbt_Schedule(unit, VBTE_CONN_COMPLETE, 0xff09, 0, 100); /* conn limit */
            } else {
                struct VBTLink *ln = &unit->vu_Links[n];
                memset(ln, 0, sizeof(*ln));
                ln->ln_Used = TRUE;
                ln->ln_Handle = unit->vu_NextHandle++;
                ln->ln_DevIdx = idx;
                ln->ln_LE = FALSE;
                vbt_Schedule(unit, VBTE_CONN_COMPLETE, n, 0, 300);
            }
        }
        return(TRUE);
    }

    case 0x200d: { /* le create connection */
        ULONG i;
        UWORD idx = 0xffff;
        UWORD n;
        if(plen < 25) {
            vbt_CommandStatus(unit, opcode, 0x12);
            return(TRUE);
        }
        for(i = 0; i < vbt_NumFakeDevices(); i++) {
            const struct VBTFakeDevice *fd = vbt_FakeDevice(i);
            if(VBT_HASLE(fd) && !memcmp(fd->fd_Addr, &p[6], 6)) {
                idx = i;
            }
        }
        vbt_CommandStatus(unit, opcode, 0);
        if(idx == 0xffff) {
            vbt_Schedule(unit, VBTE_CONN_COMPLETE, 0xfe3e, 0, 3000); /* le conn failed */
        } else {
            for(n = 0; n < VBT_MAXLINKS; n++) {
                if(!unit->vu_Links[n].ln_Used) {
                    break;
                }
            }
            if(n == VBT_MAXLINKS) {
                vbt_Schedule(unit, VBTE_CONN_COMPLETE, 0xfe09, 0, 100);
            } else {
                struct VBTLink *ln = &unit->vu_Links[n];
                memset(ln, 0, sizeof(*ln));
                ln->ln_Used = TRUE;
                ln->ln_Handle = unit->vu_NextHandle++;
                ln->ln_DevIdx = idx;
                ln->ln_LE = TRUE;
                vbt_Schedule(unit, VBTE_CONN_COMPLETE, 0x8000 | n, 0, 250);
            }
        }
        return(TRUE);
    }

    case 0x200e: /* le create connection cancel */
        vbt_CommandComplete(unit, opcode, (const UBYTE *) "\0", 1);
        return(TRUE);

    case 0x0406: { /* disconnect */
        UWORD handle;
        struct VBTLink *ln;
        if(plen < 3) {
            vbt_CommandStatus(unit, opcode, 0x12);
            return(TRUE);
        }
        handle = (p[0] | (p[1] << 8)) & 0x0fff;
        ln = vbtp_FindLink(unit, handle);
        vbt_CommandStatus(unit, opcode, ln ? 0x00 : 0x02);
        if(ln) {
            vbt_Schedule(unit, VBTE_DISCONN_COMPLETE, vbtp_LinkIndex(unit, ln), 0x16, 100);
        }
        return(TRUE);
    }

    case 0x0411: { /* auth requested */
        UWORD handle;
        struct VBTLink *ln;
        if(plen < 2) {
            vbt_CommandStatus(unit, opcode, 0x12);
            return(TRUE);
        }
        handle = (p[0] | (p[1] << 8)) & 0x0fff;
        ln = vbtp_FindLink(unit, handle);
        vbt_CommandStatus(unit, opcode, ln ? 0x00 : 0x02);
        if(ln) {
            if(unit->vu_HasLinkKey[ln->ln_DevIdx]) {
                /* already bonded: link key request -> auth complete */
                ln->ln_PairStep = 10;
            } else {
                ln->ln_PairStep = 1;
            }
            vbt_Schedule(unit, VBTE_PAIR_STEP, vbtp_LinkIndex(unit, ln), 0, 100);
        }
        return(TRUE);
    }

    case 0x040b: { /* link key request reply */
        UBYTE rp[7];
        rp[0] = 0;
        CopyMem((APTR) p, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        /* auth completes */
        {
            UWORD n;
            for(n = 0; n < VBT_MAXLINKS; n++) {
                struct VBTLink *ln = &unit->vu_Links[n];
                const struct VBTFakeDevice *fd;
                if(!ln->ln_Used) {
                    continue;
                }
                fd = vbt_FakeDevice(ln->ln_DevIdx);
                if(fd && !memcmp(fd->fd_Addr, p, 6) && (ln->ln_PairStep == 11)) {
                    ln->ln_PairStep = 12;
                    vbt_Schedule(unit, VBTE_PAIR_STEP, n, 0, 50);
                }
            }
        }
        return(TRUE);
    }

    case 0x040c: { /* link key request negative reply: fall back to SSP */
        UBYTE rp[7];
        UWORD n;
        rp[0] = 0;
        CopyMem((APTR) p, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        for(n = 0; n < VBT_MAXLINKS; n++) {
            struct VBTLink *ln = &unit->vu_Links[n];
            const struct VBTFakeDevice *fd;
            if(!ln->ln_Used) {
                continue;
            }
            fd = vbt_FakeDevice(ln->ln_DevIdx);
            if(fd && !memcmp(fd->fd_Addr, p, 6) && (ln->ln_PairStep == 11)) {
                ln->ln_PairStep = 1;
                vbt_Schedule(unit, VBTE_PAIR_STEP, n, 0, 50);
            }
        }
        return(TRUE);
    }

    case 0x042b: { /* io capability request reply */
        UBYTE rp[7];
        UWORD n;
        rp[0] = 0;
        CopyMem((APTR) p, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        for(n = 0; n < VBT_MAXLINKS; n++) {
            struct VBTLink *ln = &unit->vu_Links[n];
            const struct VBTFakeDevice *fd;
            if(!ln->ln_Used) {
                continue;
            }
            fd = vbt_FakeDevice(ln->ln_DevIdx);
            if(fd && !memcmp(fd->fd_Addr, p, 6) && (ln->ln_PairStep == 2)) {
                ln->ln_PairStep = 3;
                vbt_Schedule(unit, VBTE_PAIR_STEP, n, 0, 50);
            }
        }
        return(TRUE);
    }

    case 0x042c: { /* user confirmation reply */
        UBYTE rp[7];
        UWORD n;
        rp[0] = 0;
        CopyMem((APTR) p, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        for(n = 0; n < VBT_MAXLINKS; n++) {
            struct VBTLink *ln = &unit->vu_Links[n];
            const struct VBTFakeDevice *fd;
            if(!ln->ln_Used) {
                continue;
            }
            fd = vbt_FakeDevice(ln->ln_DevIdx);
            if(fd && !memcmp(fd->fd_Addr, p, 6) && (ln->ln_PairStep == 4)) {
                ln->ln_PairStep = 5;
                vbt_Schedule(unit, VBTE_PAIR_STEP, n, 0, 50);
            }
        }
        return(TRUE);
    }

    case 0x042d: { /* user confirmation negative reply */
        UBYTE rp[7];
        UWORD n;
        rp[0] = 0;
        CopyMem((APTR) p, &rp[1], 6);
        vbt_CommandComplete(unit, opcode, rp, 7);
        for(n = 0; n < VBT_MAXLINKS; n++) {
            struct VBTLink *ln = &unit->vu_Links[n];
            const struct VBTFakeDevice *fd;
            if(!ln->ln_Used) {
                continue;
            }
            fd = vbt_FakeDevice(ln->ln_DevIdx);
            if(fd && !memcmp(fd->fd_Addr, p, 6)) {
                ln->ln_PairStep = 0;
                /* simple pairing complete with authentication failure */
                {
                    UBYTE ev[7];
                    ev[0] = 0x05;
                    CopyMem((APTR) fd->fd_Addr, &ev[1], 6);
                    vbt_SendEvent(unit, 0x36, ev, 7);
                }
                {
                    UBYTE ev[3];
                    ev[0] = 0x05;
                    ev[1] = ln->ln_Handle & 0xff;
                    ev[2] = ln->ln_Handle >> 8;
                    vbt_SendEvent(unit, 0x06, ev, 3);
                }
            }
        }
        return(TRUE);
    }

    case 0x0413: { /* set connection encryption */
        UWORD handle;
        struct VBTLink *ln;
        if(plen < 3) {
            vbt_CommandStatus(unit, opcode, 0x12);
            return(TRUE);
        }
        handle = (p[0] | (p[1] << 8)) & 0x0fff;
        ln = vbtp_FindLink(unit, handle);
        vbt_CommandStatus(unit, opcode, ln ? 0x00 : 0x02);
        if(ln) {
            UBYTE ev[4];
            ln->ln_Encrypted = p[2] ? TRUE : FALSE;
            ev[0] = 0;
            ev[1] = handle & 0xff;
            ev[2] = handle >> 8;
            ev[3] = ln->ln_Encrypted ? 1 : 0;
            vbt_SendEvent(unit, 0x08, ev, 4);
        }
        return(TRUE);
    }

    default:
        return(FALSE);
    }
}
/* \\\ */

/* *** timed events *** */

/* /// "vbtp_Timed()" */
void vbtp_Timed(struct VBTHCIUnit *unit, struct VBTTimedEvent *te)
{
    switch(te->te_Kind) {
    case VBTE_CONN_COMPLETE:
        if(te->te_Index & 0x8000) {
            if((te->te_Index & 0xff00) == 0xfe00) {
                /* LE failure */
                UBYTE ev[19];
                memset(ev, 0, sizeof(ev));
                ev[0] = 0x01; /* subevent */
                ev[1] = te->te_Index & 0xff;
                vbt_SendEvent(unit, 0x3e, ev, 19);
            } else {
                /* LE success */
                struct VBTLink *ln = &unit->vu_Links[te->te_Index & 0x0f];
                const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
                UBYTE ev[19];
                memset(ev, 0, sizeof(ev));
                ev[0] = 0x01;
                ev[1] = 0x00;
                ev[2] = ln->ln_Handle & 0xff;
                ev[3] = ln->ln_Handle >> 8;
                ev[4] = 0x00; /* central */
                ev[5] = fd->fd_AddrType;
                CopyMem((APTR) fd->fd_Addr, &ev[6], 6);
                ev[12] = 0x18; ev[13] = 0x00; /* interval */
                ev[14] = 0x00; ev[15] = 0x00; /* latency */
                ev[16] = 0xa0; ev[17] = 0x02; /* timeout */
                ev[18] = 0x00;
                vbt_SendEvent(unit, 0x3e, ev, 19);
            }
        } else if((te->te_Index & 0xff00) == 0xff00) {
            /* classic failure */
            UBYTE ev[11];
            memset(ev, 0, sizeof(ev));
            ev[0] = (te->te_Index & 0xff) ? (te->te_Index & 0xff) : 0x04;
            vbt_SendEvent(unit, 0x03, ev, 11);
        } else {
            struct VBTLink *ln = &unit->vu_Links[te->te_Index];
            const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
            UBYTE ev[11];
            ev[0] = 0x00;
            ev[1] = ln->ln_Handle & 0xff;
            ev[2] = ln->ln_Handle >> 8;
            CopyMem((APTR) fd->fd_Addr, &ev[3], 6);
            ev[9] = LINKTYPE_ACL;
            ev[10] = 0x00; /* encryption off */
            vbt_SendEvent(unit, 0x03, ev, 11);
        }
        break;

    case VBTE_DISCONN_COMPLETE: {
        struct VBTLink *ln = &unit->vu_Links[te->te_Index];
        if(ln->ln_Used) {
            UBYTE ev[4];
            ev[0] = 0x00;
            ev[1] = ln->ln_Handle & 0xff;
            ev[2] = ln->ln_Handle >> 8;
            ev[3] = te->te_Arg ? te->te_Arg : 0x16;
            memset(ln, 0, sizeof(*ln));
            vbt_SendEvent(unit, 0x05, ev, 4);
        }
        break;
    }

    case VBTE_PAIR_STEP: {
        struct VBTLink *ln = &unit->vu_Links[te->te_Index];
        const struct VBTFakeDevice *fd;
        if(!ln->ln_Used) {
            break;
        }
        fd = vbt_FakeDevice(ln->ln_DevIdx);
        switch(ln->ln_PairStep) {
        case 1: { /* SSP: io capability request */
            UBYTE ev[6];
            CopyMem((APTR) fd->fd_Addr, ev, 6);
            ln->ln_PairStep = 2;
            vbt_SendEvent(unit, 0x31, ev, 6);
            break;
        }
        case 3: { /* io cap response + user confirmation request */
            UBYTE ev[9];
            CopyMem((APTR) fd->fd_Addr, ev, 6);
            ev[6] = 0x03; /* no input no output */
            ev[7] = 0x00;
            ev[8] = 0x00;
            vbt_SendEvent(unit, 0x32, ev, 9);
            ln->ln_PairStep = 4;
            {
                UBYTE cv[10];
                CopyMem((APTR) fd->fd_Addr, cv, 6);
                cv[6] = 0x2e; cv[7] = 0x16; cv[8] = 0x03; cv[9] = 0x00; /* 202318 */
                vbt_SendEvent(unit, 0x33, cv, 10);
            }
            break;
        }
        case 5: { /* simple pairing complete + link key + auth complete */
            UBYTE ev[7];
            UBYTE lk[23];
            UBYTE ac[3];
            UWORD i;
            ev[0] = 0x00;
            CopyMem((APTR) fd->fd_Addr, &ev[1], 6);
            vbt_SendEvent(unit, 0x36, ev, 7);
            CopyMem((APTR) fd->fd_Addr, lk, 6);
            for(i = 0; i < 16; i++) {
                lk[6 + i] = 0x40 + ln->ln_DevIdx + i;
                unit->vu_LinkKeys[ln->ln_DevIdx & 7][i] = lk[6 + i];
            }
            unit->vu_HasLinkKey[ln->ln_DevIdx & 7] = TRUE;
            lk[22] = 0x05; /* authenticated combination key */
            vbt_SendEvent(unit, 0x18, lk, 23);
            ac[0] = 0x00;
            ac[1] = ln->ln_Handle & 0xff;
            ac[2] = ln->ln_Handle >> 8;
            ln->ln_PairStep = 0;
            vbt_SendEvent(unit, 0x06, ac, 3);
            break;
        }
        case 10: { /* bonded: link key request */
            UBYTE ev[6];
            CopyMem((APTR) fd->fd_Addr, ev, 6);
            ln->ln_PairStep = 11;
            vbt_SendEvent(unit, 0x17, ev, 6);
            break;
        }
        case 12: { /* auth complete after link key reply */
            UBYTE ac[3];
            ac[0] = 0x00;
            ac[1] = ln->ln_Handle & 0xff;
            ac[2] = ln->ln_Handle >> 8;
            ln->ln_PairStep = 0;
            vbt_SendEvent(unit, 0x06, ac, 3);
            break;
        }
        }
        break;
    }

    case VBTE_LE_NOTIFY: {
        struct VBTLink *ln = &unit->vu_Links[te->te_Index];
        if(ln->ln_Used && ln->ln_Notify) {
            /* handle value notification: one HID input report (mouse motion or
             * keystroke, per the device class) prefixed with the ATT notify
             * header for the report value handle (0x0014). */
            const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
            UBYTE pdu[3 + 8];
            ULONG rlen;
            pdu[0] = 0x1b;
            pdu[1] = 0x14; pdu[2] = 0x00;
            rlen = vbtp_HidInputReport(fd, ln->ln_MouseStep, &pdu[3]);
            ln->ln_MouseStep++;
            vbtp_SendPDU(unit, ln, 0x0004, pdu, 3 + rlen);
            vbt_Schedule(unit, VBTE_LE_NOTIFY, te->te_Index, 0, 1000);
        }
        break;
    }

    case VBTE_HID_REPORT: {
        /* classic HIDP: one input report on the interrupt channel (PSM 0x13),
         * as an HIDP DATA message (0xa1 = DATA transaction, Input report). */
        struct VBTLink *ln = &unit->vu_Links[te->te_Index];
        struct VBTChan *ch = NULL;
        UWORD n;
        if(ln->ln_Used) {
            for(n = 0; n < VBT_MAXCHANS; n++) {
                if((ln->ln_Chans[n].lc_State == 2) && (ln->ln_Chans[n].lc_PSM == 0x0013)) {
                    ch = &ln->ln_Chans[n];
                    break;
                }
            }
        }
        if(ch) {
            const struct VBTFakeDevice *fd = vbt_FakeDevice(ln->ln_DevIdx);
            UBYTE pdu[1 + 8];
            ULONG rlen;
            pdu[0] = 0xa1;
            rlen = vbtp_HidInputReport(fd, ln->ln_MouseStep, &pdu[1]);
            ln->ln_MouseStep++;
            vbtp_SendPDU(unit, ln, ch->lc_RemoteCID, pdu, 1 + rlen);
            vbt_Schedule(unit, VBTE_HID_REPORT, te->te_Index, 0, 1000);
        }
        break;
    }
    }
}
/* \\\ */
