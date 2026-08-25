/*
 *----------------------------------------------------------------------------
 *                bluetooth.library: the hardware task
 *----------------------------------------------------------------------------
 *
 * One task per radio, the counterpart of Poseidon's pDeviceTask(). It is the
 * only context that touches the bluetoothhci.device unit and it hosts the
 * protocol engine (the portable btcore: HCI command queue, timers, ...).
 *
 *   clients ---btSendChannel()/bSubmitCtrl()---> bth_TaskMsgPort ---+
 *                                                                 |
 *   bluetoothhci.device <---BTCMD_WRITEHCI/WRITEACL/READACL--- this task
 *                       ---BTHCIEventMsg (BTCMD_ADDMSGPORT)---> bth_EventMsgPort
 *                       ---IORequest replies-----------------> bth_DevMsgPort
 *
 * The task updates the BtHardware/BtDevice objects under the library base write
 * lock, replies channels to their owner's port and raises events. Class code
 * never runs in this task (class scans happen in the event handler task).
 */

#include "debug.h"

#include "hwtask.h"
#include "aes128.h"

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <string.h>
#include <stdio.h>

#define NewList(list) NEWLIST(list)
#define min(x,y) (((x) < (y)) ? (x) : (y))

#define DOSBase BluetoothBase->bt_DosBase
#define TimerBase BluetoothBase->bt_TimerIOReq.tr_node.io_Device

static void bStartDiscoveryPhase(struct BtHWCore *hc);
static void bFinishDiscovery(struct BtHWCore *hc);
static void bNextNameRequest(struct BtHWCore *hc);
static void bBringupStep(struct BtHWCore *hc);
static void bHandleDevReply(struct BtHWCore *hc, struct IOBTHCIReq *req);
static void bTick(struct BtHWCore *hc);
static void bArmTimer(struct BtHWCore *hc);
static void bHandleEvent(struct BtHWCore *hc, const UBYTE *data, ULONG len);

/* /// "bNowUS()" */
uint64_t bNowUS(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct timeval tv;
    GetSysTime(&tv);
    return((uint64_t) tv.tv_secs * 1000000ULL + tv.tv_micro);
}
/* \\\ */

/* /// "bReplyChannel()" */
void bReplyChannel(LIBBASETYPEPTR BluetoothBase, struct BtChannel *bch, LONG error, ULONG actual)
{
    bch->bch_Error = error;
    bch->bch_Actual = actual;
    bch->bch_Flags &= ~BCHF_QUEUED;
    if(bch->bch_Msg.mn_Node.ln_Type == NT_MESSAGE) {
        ReplyMsg(&bch->bch_Msg);
    }
}
/* \\\ */

/* *** transport (btcore -> bluetoothhci.device) *** */

/* /// "bSendCommand()" */
static int bSendCommand(struct bt_hci_transport *transport, const uint8_t *data, size_t length)
{
    struct BtHWCore *hc = transport->impl;
    struct BtHardware *bth = hc->hc_Hardware;
    UWORD n;

    if(length > sizeof(hc->hc_CmdBuf[0])) {
        return(-1);
    }
    for(n = 0; n < HC_NUMCMDREQS; n++) {
        if(!hc->hc_CmdPending[n]) {
            struct IOBTHCIReq *req = hc->hc_CmdReq[n];
            CopyMem((APTR) data, hc->hc_CmdBuf[n], length);
            req->iobt_Req.io_Command = BTCMD_WRITEHCI;
            req->iobt_Data = hc->hc_CmdBuf[n];
            req->iobt_Length = length;
            req->iobt_Actual = 0;
            req->iobt_Req.io_Error = 0;
            hc->hc_CmdPending[n] = TRUE;
            SendIO((struct IORequest *) req);
            bth->bth_MsgCount++;
            KPRINTF(5, ("HCI cmd %04lx (%ld bytes) sent\n", data[0]|(data[1]<<8), length));
            return(0);
        }
    }
    KPRINTF(20, ("No free command request!\n"));
    return(-1);
}
/* \\\ */

static int bTransportOpen(struct bt_hci_transport *transport) { (void) transport; return(0); }
static void bTransportClose(struct bt_hci_transport *transport) { (void) transport; }
static int bTransportStartReceive(struct bt_hci_transport *transport, bt_hci_transport_recv_fn recv, void *user_data)
{ (void) transport; (void) recv; (void) user_data; return(0); }
static void bTransportStopReceive(struct bt_hci_transport *transport) { (void) transport; }

static const struct bt_hci_transport_ops bTransportOps = {
    .open = bTransportOpen,
    .close = bTransportClose,
    .send_command = bSendCommand,
    .send_acl = bSendACLPacket,
    .send_sco = bSendACLPacket,
    .send_iso = bSendACLPacket,
    .start_receive = bTransportStartReceive,
    .stop_receive = bTransportStopReceive,
};

/* *** command helpers *** */

/* /// "bSubmitCmd()" */
BOOL bSubmitCmd(struct BtHWCore *hc, UWORD opcode, const UBYTE *params, UBYTE len,
                bt_cmdq_complete_fn cb, void *user)
{
    bt_status_t st = bt_cmdq_submit(&hc->hc_CmdQ, opcode, params, len, 0, cb, user);
    if(st != BT_OK) {
        KPRINTF(20, ("cmdq submit %04lx failed (%ld)\n", opcode, st));
        return(FALSE);
    }
    bt_cmdq_pump(&hc->hc_CmdQ, bNowUS(hc));
    return(TRUE);
}
/* \\\ */

/* /// "bIgnoreCompletion()" */
void bIgnoreCompletion(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        hc->hc_Hardware->bth_LastHCIError = completion->status;
        KPRINTF(10, ("cmd %04lx failed: result %ld status %02lx\n", completion->opcode, completion->result, completion->status));
    }
}
/* \\\ */

/* *** device bookkeeping *** */

/* /// "bFindDeviceByAddr()" */
struct BtDevice * bFindDeviceByAddr(struct BtHWCore *hc, const UBYTE *addr)
{
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
    while(bd->bd_Node.ln_Succ) {
        if(!memcmp(bd->bd_Address.bd_Addr, addr, 6)) {
            return(bd);
        }
        /* a bonded peer currently using a resolvable private address */
        if(bd->bd_CurAddrValid && !memcmp(bd->bd_CurAddr, addr, 6)) {
            return(bd);
        }
        bd = (struct BtDevice *) bd->bd_Node.ln_Succ;
    }
    return(NULL);
}
/* \\\ */

/* /// "bResolvePrivateAddr()" */
/* LE privacy: a bonded peer that gave us its IRK advertises from a random
   resolvable private address (top bits 01) that changes every few minutes.
   hash = ah(IRK, prand) - check the 24 bit hash against every stored IRK.
   On a match the device is returned and remembers this address as the one
   the peer answers to right now (bd_CurAddr). Base must be locked. */
static struct BtDevice * bResolvePrivateAddr(struct BtHWCore *hc, const UBYTE *addr)
{
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd;
    UBYTE rp[16];
    UBYTE key[16];
    UBYTE out[16];
    ULONG i;

    if((addr[5] & 0xc0) != 0x40) {
        return(NULL);
    }
    /* r' = padding || prand; prand most significant octet first */
    memset(rp, 0, sizeof(rp));
    rp[13] = addr[5];
    rp[14] = addr[4];
    rp[15] = addr[3];
    for(bd = (struct BtDevice *) bth->bth_Devices.lh_Head; bd->bd_Node.ln_Succ; bd = (struct BtDevice *) bd->bd_Node.ln_Succ) {
        if(!(bd->bd_Keys.bkc_Flags & BKCF_IRK)) {
            continue;
        }
        /* the IRK is stored as distributed (LSB first); AES wants MSB first */
        for(i = 0; i < 16; i++) {
            key[i] = bd->bd_Keys.bkc_IRK[15 - i];
        }
        bAES128Encrypt(key, rp, out);
        if((out[15] == addr[0]) && (out[14] == addr[1]) && (out[13] == addr[2])) {
            if(!bd->bd_CurAddrValid || memcmp(bd->bd_CurAddr, addr, 6)) {
                CopyMem((APTR) addr, bd->bd_CurAddr, 6);
                bd->bd_CurAddrType = BDAT_RANDOM;
                bd->bd_CurAddrValid = TRUE;
            }
            return(bd);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bSetDeviceName()" */
/* Sets the name reported by the device (bd_OrigName) and, unless the user
   assigned a custom name, the display name. Device must be write locked.
   Returns TRUE if the display name changed. */
static BOOL bSetDeviceName(struct BtHWCore *hc, struct BtDevice *bd, const UBYTE *name, ULONG len)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    STRPTR newname;
    BOOL custom;
    ULONG n;

    while(len && !name[len-1]) {
        len--;
    }
    for(n = 0; n < len; n++) {
        if(!name[n]) {
            len = n;
            break;
        }
    }
    if(!len) {
        return(FALSE);
    }
    if(bd->bd_OrigName && (strlen(bd->bd_OrigName) == len) && !memcmp(bd->bd_OrigName, name, len)) {
        return(FALSE);
    }
    newname = btAllocVec(len + 1);
    if(!newname) {
        return(FALSE);
    }
    CopyMem((APTR) name, newname, len);
    newname[len] = 0;
    bStripString(BluetoothBase, newname);
    custom = (bd->bd_Name && bd->bd_OrigName && strcmp(bd->bd_Name, bd->bd_OrigName)) ? TRUE : FALSE;
    if(bd->bd_Name && !bd->bd_OrigName && strcmp(bd->bd_Name, (STRPTR) bd->bd_AddrString)) {
        /* name came from the config -> custom */
        custom = TRUE;
    }
    btFreeVec(bd->bd_OrigName);
    bd->bd_OrigName = newname;
    if(!custom) {
        btFreeVec(bd->bd_Name);
        bd->bd_Name = btCopyStr(newname);
        bd->bd_Node.ln_Name = bd->bd_Name;
    }
    return(TRUE);
}
/* \\\ */

/* /// "bNoteDevice()" */
/*
 * Called for every inquiry result / advertising report. Creates or updates
 * the BtDevice and raises the ADDDEVICE/DEVICEUPDATE events.
 */
static BOOL bNoteDevice(struct BtHWCore *hc, const UBYTE *addr, UBYTE addrtype, BOOL isle,
                        ULONG cod, LONG rssi, const UBYTE *name, ULONG namelen,
                        const UBYTE *advdata, ULONG advlen, UWORD appearance,
                        const struct bt_le_adv_info *info)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd;
    BOOL isnew = FALSE;
    BOOL changed = FALSE;

    btLockWriteBase();
    bd = bFindDeviceByAddr(hc, addr);
    if(!bd && isle && (addrtype == BDAT_RANDOM)) {
        bd = bResolvePrivateAddr(hc, addr);
    }
    if(!bd && hc->hc_BgScanActive && !(bth->bth_Flags & BTHF_DISCOVERING)) {
        /* the background scan only exists to catch our own bonded devices
           waking up; strangers are not listed outside a discovery */
        btUnlockBase();
        return(FALSE);
    }
    if(!bd) {
        if(isle && (addrtype == BDAT_RANDOM)) {
            /* Private (rotating) random addresses would only clutter the list -
               unless the device tells us it wants to be found: a name, an
               appearance, the HID service, or the LE General/Limited Discoverable
               flag (a PC or phone in its "discoverable" mode advertises exactly
               that from a resolvable private address). */
            struct bt_addr ba;
            BOOL discoverable = info && info->has_flags && (info->flags & 0x03);
            BOOL hid = info && info->hid;
            CopyMem((APTR) addr, ba.b, 6);
            if(!bt_le_addr_is_stable(&ba, addrtype) && !namelen && !appearance && !discoverable && !hid) {
                btUnlockBase();
                hc->hc_DiagDropped++;
                return(FALSE);
            }
        }
        btUnlockBase();
        bd = btAllocDevice(bth);
        if(!bd) {
            return(FALSE);
        }
        btLockWriteBase();
        btLockWriteDevice(bd);
        CopyMem((APTR) addr, bd->bd_Address.bd_Addr, 6);
        bd->bd_AddrType = addrtype;
        bAddrToStr(bd->bd_Address.bd_Addr, (STRPTR) bd->bd_AddrString);
        bd->bd_IDString = btCopyStrFmt("BT:%s", bd->bd_AddrString);
        bApplyDevConfig(BluetoothBase, bd);
        if(!bd->bd_Name) {
            bd->bd_Name = btCopyStr((STRPTR) bd->bd_AddrString);
        }
        bd->bd_Node.ln_Name = bd->bd_Name;
        isnew = TRUE;
    } else {
        btLockWriteDevice(bd);
    }
    if(!(bd->bd_Flags & BDFF_DISCOVERED)) {
        bd->bd_Flags |= BDFF_DISCOVERED;
        changed = TRUE;
    }
    if(isle) {
        if(!(bd->bd_Flags & BDFF_LE)) {
            bd->bd_Flags |= BDFF_LE;
            changed = TRUE;
        }
    } else {
        if(!(bd->bd_Flags & BDFF_CLASSIC)) {
            bd->bd_Flags |= BDFF_CLASSIC;
            changed = TRUE;
        }
    }
    if(cod && (bd->bd_ClassOfDevice != cod)) {
        bd->bd_ClassOfDevice = cod;
        changed = TRUE;
    }
    if(appearance && (bd->bd_Appearance != appearance)) {
        bd->bd_Appearance = appearance;
        changed = TRUE;
    }
    if((rssi != 127) && (bd->bd_RSSI != rssi)) {
        bd->bd_RSSI = rssi;
        /* RSSI changes alone are frequent; only report them if nothing else did */
    }
    if(advdata && advlen) {
        ULONG len = min(advlen, BT_ADVDATA_MAX);
        if((bd->bd_AdvDataLen != len) || memcmp(bd->bd_AdvData, advdata, len)) {
            CopyMem((APTR) advdata, bd->bd_AdvData, len);
            bd->bd_AdvDataLen = len;
            changed = TRUE;
        }
    }
    if(name && namelen) {
        if(bSetDeviceName(hc, bd, name, namelen)) {
            changed = TRUE;
        }
    }
    if(bHaveDOS(BluetoothBase)) {
        DateStamp(&bd->bd_LastSeen);
    }
    btUnlockDevice(bd);
    btUnlockBase();
    if(isnew) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Found %s device %s (%s).", isle ? "LE" : "classic",
                       bd->bd_Name, bd->bd_AddrString);
        btSendEvent(BEHMB_ADDDEVICE, bd, NULL);
    } else if(changed) {
        btSendEvent(BEHMB_DEVICEUPDATE, bd, NULL);
    }
    if(isle && (bd->bd_Flags & BDFF_REGISTERED)) {
        /* one of ours is awake and advertising: reconnect it if wanted */
        bConnAdvertising(hc, bd);
    }
    return(TRUE);
}
/* \\\ */


/* /// "bScanDiagNote()" */
/* Journal one raw report (LE advertising/scan-response or classic inquiry) per
   address, whether or not bNoteDevice() accepted it, so the scan log can show
   exactly what each device sent and why it was or was not listed. */
static void bScanDiagNote(struct BtHWCore *hc, const UBYTE *addr, UBYTE addrtype, BOOL isle,
                          UBYTE evbit, UBYTE extbits, const UBYTE *ad, ULONG adlen,
                          const struct bt_le_adv_info *info, LONG rssi, ULONG cod, BOOL noted)
{
    struct BtScanDiag *sd = NULL;
    UWORD i;

    if(!(hc->hc_Base->bt_Flags & BTF_KLOG)) {
        return;                         /* journal only with the "btdebug" boot argument */
    }

    for(i = 0; i < hc->hc_ScanDiagCount; i++) {
        if(!memcmp(hc->hc_ScanDiag[i].sd_Addr, addr, 6) && (hc->hc_ScanDiag[i].sd_IsLE == (UBYTE) isle)) {
            sd = &hc->hc_ScanDiag[i];
            break;
        }
    }
    if(!sd) {
        if(hc->hc_ScanDiagCount >= HC_SCANDIAG_MAX) {
            hc->hc_ScanDiagOverflow++;
            return;
        }
        sd = &hc->hc_ScanDiag[hc->hc_ScanDiagCount++];
        memset(sd, 0, sizeof(*sd));
        CopyMem((APTR) addr, sd->sd_Addr, 6);
        sd->sd_AddrType = addrtype;
        sd->sd_IsLE = isle ? 1 : 0;
        if(isle) {
            struct bt_addr ba;
            CopyMem((APTR) addr, ba.b, 6);
            sd->sd_Stable = bt_le_addr_is_stable(&ba, addrtype) ? 1 : 0;
        } else {
            sd->sd_Stable = 1;
        }
        sd->sd_RSSI = 127;
    }
    if(sd->sd_Count < 0xffff) sd->sd_Count++;
    sd->sd_EvMask |= evbit;
    sd->sd_ExtMask |= extbits;
    if(noted) { if(sd->sd_Noted < 255) sd->sd_Noted++; }
    else      { if(sd->sd_Dropped < 255) sd->sd_Dropped++; }
    if(rssi != 127) sd->sd_RSSI = rssi;
    if(cod) sd->sd_CoD = cod;
    if(ad && adlen) {
        ULONG o = 0;
        while(o < adlen) {
            UBYTE flen = ad[o];
            if(!flen || (o + 1 + flen > adlen)) break;
            if(ad[o + 1] < 32) sd->sd_ADTypes |= (1UL << ad[o + 1]);
            else sd->sd_ADTypes |= (1UL << 31);
            o += 1 + flen;
        }
    }
    if(info) {
        if(info->has_flags) { sd->sd_HasFlags = 1; sd->sd_Flags = info->flags; }
        if(info->hid) sd->sd_HID = 1;
        if(info->appearance) sd->sd_Appearance = info->appearance;
        if(info->name && info->name_len && !sd->sd_Name[0]) {
            ULONG n = min(info->name_len, sizeof(sd->sd_Name) - 1);
            CopyMem((APTR) info->name, sd->sd_Name, n);
            sd->sd_Name[n] = 0;
        }
    }
}
/* \\\ */

/* /// "bScanDiagDump()" */
/* Append this discovery run's journal to SYS:BluetoothScan.log. Runs in the
   hardware task (a Process, so DOS I/O is fine). */
static void bScanDiagDump(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    static const char *legacyev[5] = { "ADV_IND", "ADV_DIRECT", "ADV_SCAN_IND", "ADV_NONCONN", "SCAN_RSP" };
    static const char *classicev[3] = { "std", "rssi", "eir" };
    char line[400];
    char addr[BT_ADDRSTR_LEN];
    struct DateStamp ds;
    BPTR fh;
    UWORD i;
    ULONG rpa_droppedonly = 0, rpa_discoverable = 0;

    if(!bHaveDOS(BluetoothBase)) {
        return;
    }
    if(!(fh = Open("SYS:BluetoothScan.log", MODE_READWRITE))) {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: could not open SYS:BluetoothScan.log for the scan diagnostics.",
                       bth->bth_DevName, bth->bth_Unit);
        return;
    }
    Seek(fh, 0, OFFSET_END);
    DateStamp(&ds);
    snprintf(line, sizeof(line),
             "==== scan on %s/%lu  day %ld %02ld:%02ld  controller %s LMP 0x%04lx  reports: LE legacy=%lu ext=%lu other=%lu scanrsp=%lu classic=%lu  dropped(RPA rule)=%lu  addresses=%u overflow=%lu\n",
             bth->bth_DevName, (unsigned long) bth->bth_Unit, (long) ds.ds_Days, (long)(ds.ds_Minute / 60), (long)(ds.ds_Minute % 60),
             btNumToStr(BNTS_MANUFACTURER, bth->bth_ManufacturerID, "?"), (unsigned long) bth->bth_LMPSubversion,
             (unsigned long) hc->hc_DiagAdvLegacy, (unsigned long) hc->hc_DiagAdvExt, (unsigned long) hc->hc_DiagAdvOther,
             (unsigned long) hc->hc_DiagScanRsp, (unsigned long) hc->hc_DiagInqResults, (unsigned long) hc->hc_DiagDropped,
             (unsigned) hc->hc_ScanDiagCount, (unsigned long) hc->hc_ScanDiagOverflow);
    FPuts(fh, line);
    for(i = 0; i < hc->hc_ScanDiagCount; i++) {
        struct BtScanDiag *sd = &hc->hc_ScanDiag[i];
        char ev[96], ad[160];
        ULONG b, p = 0;
        ev[0] = 0;
        if(sd->sd_IsLE) {
            for(b = 0; b < 5; b++) if(sd->sd_EvMask & (1 << b)) p += snprintf(ev + p, sizeof(ev) - p, "%s%s", p ? "," : "", legacyev[b]);
            if(sd->sd_ExtMask) p += snprintf(ev + p, sizeof(ev) - p, "%sext:0x%02x", p ? "," : "", sd->sd_ExtMask);
        } else {
            for(b = 0; b < 3; b++) if(sd->sd_EvMask & (1 << b)) p += snprintf(ev + p, sizeof(ev) - p, "%s%s", p ? "," : "", classicev[b]);
        }
        p = 0; ad[0] = 0;
        for(b = 0; b < 32; b++) if(sd->sd_ADTypes & (1UL << b)) p += snprintf(ad + p, sizeof(ad) - p, "%s%02lx", p ? "," : "", (b == 31) ? 0xffUL : (unsigned long) b);
        bAddrToStr(sd->sd_Addr, (STRPTR) addr);
        if(sd->sd_IsLE) {
            const char *kind = (sd->sd_AddrType == 0) ? "public" : (sd->sd_AddrType >= 2) ? "identity" : sd->sd_Stable ? "static-random" : "RPA";
            BOOL disc = sd->sd_HasFlags && (sd->sd_Flags & 0x03);
            if(!sd->sd_Noted && !sd->sd_Stable) { rpa_droppedonly++; if(disc || sd->sd_HID) rpa_discoverable++; }
            snprintf(line, sizeof(line),
                     "LE  %s %-13s ev=%-28s ad=[%s] flags=%s%02x%s name=\"%s\" appear=0x%04x hid=%c rssi=%ld reports=%u noted=%u dropped=%u -> %s\n",
                     addr, kind, ev, ad, sd->sd_HasFlags ? "" : "(none)", sd->sd_Flags,
                     disc ? "(discoverable)" : "", sd->sd_Name, sd->sd_Appearance, sd->sd_HID ? 'y' : 'n',
                     (long) sd->sd_RSSI, sd->sd_Count, sd->sd_Noted, sd->sd_Dropped,
                     sd->sd_Noted ? "LISTED" : "NOT LISTED (private address, not discoverable, no name/appearance/HID)");
        } else {
            snprintf(line, sizeof(line),
                     "BR  %s %-13s ev=%-28s cod=0x%06lx name=\"%s\" rssi=%ld reports=%u -> %s\n",
                     addr, "public", ev, (unsigned long) sd->sd_CoD, sd->sd_Name, (long) sd->sd_RSSI, sd->sd_Count,
                     sd->sd_Noted ? "LISTED" : "NOT LISTED");
        }
        FPuts(fh, line);
    }
    snprintf(line, sizeof(line),
             "---- %lu private-address devices never listed (%lu discoverable/HID - should be 0)\n\n",
             (unsigned long) rpa_droppedonly, (unsigned long) rpa_discoverable);
    FPuts(fh, line);
    Close(fh);
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: scan diagnostics: %ld addresses seen, %ld private-address devices unlisted (%ld of them discoverable) - details in SYS:BluetoothScan.log",
                   bth->bth_DevName, bth->bth_Unit, (ULONG) hc->hc_ScanDiagCount, rpa_droppedonly, rpa_discoverable);
}
/* \\\ */

/* *** discovery *** */

/* /// "bHasSvcBinding()" */
static BOOL bHasSvcBinding(struct BtDevice *bd)
{
    struct BtService *bsv = (struct BtService *) bd->bd_Services.lh_Head;
    while(bsv->bsv_Node.ln_Succ) {
        if(bsv->bsv_SvcBinding) {
            return(TRUE);
        }
        bsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
    }
    return(FALSE);
}
/* \\\ */

/* /// "bDiscoveryTimerCB()" */
static void bDiscoveryTimerCB(struct bt_timer *timer, void *user_data)
{
    struct BtHWCore *hc = user_data;
    (void) timer;
    KPRINTF(10, ("Discovery timer expired\n"));
    if(hc->hc_LEScanActive) {
        UBYTE params[2] = { 0, 0 };
        hc->hc_LEScanActive = FALSE;
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_ENABLE, params, 2, bIgnoreCompletion, hc);
    }
    if(hc->hc_InqActive) {
        /* the inquiry has its own duration; but be safe */
        bSubmitCmd(hc, HC_OP_INQUIRY_CANCEL, NULL, 0, bIgnoreCompletion, hc);
        hc->hc_InqActive = FALSE;
    }
    hc->hc_DiscoveryPending = FALSE;
    bStartDiscoveryPhase(hc);
}
/* \\\ */

/* /// "bInquiryStatusCB()" */
static void bInquiryStatusCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        struct BtBase *BluetoothBase = hc->hc_Base;
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Inquiry failed: %s (%ld).",
                       btNumToStr(BNTS_HCISTATUS, completion->status, "unknown"), completion->status);
        hc->hc_InqActive = FALSE;
        bStartDiscoveryPhase(hc);
    }
}
/* \\\ */

/* /// "bStartDiscoveryPhase()" */
/* Advances the discovery state machine: when neither inquiry nor LE scan is
   running any more, resolve names (if wanted) and finally finish. */
static void bStartDiscoveryPhase(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;

    if(!(bth->bth_Flags & BTHF_DISCOVERING)) {
        return;
    }
    if(hc->hc_InqActive || hc->hc_LEScanActive || hc->hc_DiscoveryPending) {
        return;
    }
    if(hc->hc_ResolveNames) {
        struct BtDevice *bd;
        hc->hc_ResolveNames = FALSE;
        /* queue classic devices without a real name */
        btLockReadBase();
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
        while(bd->bd_Node.ln_Succ) {
            if((bd->bd_Flags & (BDFF_CLASSIC|BDFF_DISCOVERED)) == (BDFF_CLASSIC|BDFF_DISCOVERED) && !bd->bd_OrigName) {
                struct HCNameNode *nn = btAllocVec(sizeof(struct HCNameNode));
                if(nn) {
                    nn->nn_Device = bd;
                    AddTail((struct List *) &hc->hc_NameQueue, (struct Node *) nn);
                    hc->hc_NameQueueCount++;
                }
            }
            bd = (struct BtDevice *) bd->bd_Node.ln_Succ;
        }
        btUnlockBase();
        if(hc->hc_NameQueueCount) {
            KPRINTF(10, ("Resolving %ld names\n", hc->hc_NameQueueCount));
            bNextNameRequest(hc);
            return;
        }
    }
    if(hc->hc_NameQueueCount || hc->hc_NameReqDev) {
        /* still resolving */
        return;
    }
    bFinishDiscovery(hc);
}
/* \\\ */

/* /// "bFinishDiscovery()" */
static void bFinishDiscovery(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    if(bth->bth_Flags & BTHF_DISCOVERING) {
        bth->bth_Flags &= ~BTHF_DISCOVERING;
        bt_timer_list_cancel(&hc->hc_Timers, &hc->hc_DiscoveryTimer);
        /* the conclusive line: how many results actually came back off the radio.
           All zero => the controller reported nothing (init/transport/firmware);
           adv-ext>0 => it speaks BT 5/6 extended reports. */
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: discovery finished - %ld known; results: LE legacy=%ld, LE ext=%ld, LE other=%ld, classic=%ld.",
                       bth->bth_DevName, bth->bth_Unit, bth->bth_NumDevices,
                       hc->hc_DiagAdvLegacy, hc->hc_DiagAdvExt, hc->hc_DiagAdvOther, hc->hc_DiagInqResults);
        if(BluetoothBase->bt_Flags & BTF_KLOG) {
            bScanDiagDump(hc);          /* SYS:BluetoothScan.log, "btdebug" only */
        }
        btSendEvent(BEHMB_DISCOVERYSTOP, bth, NULL);
        bBgScanSchedule(hc);
    }
}
/* \\\ */

/* /// "bScanCmdLog()" */
/* Completion logger for the LE scan setup commands. Previously these used
   bIgnoreCompletion, so a controller that rejected legacy LE scan (some BT 5/6
   parts answer Command Disallowed and only accept the extended scan commands)
   failed silently - the #1 "scanning finds nothing" trap. Now the result is
   logged. */
static void bScanCmdLog(struct BtHWCore *hc, struct bt_cmdq_completion *completion, CONST_STRPTR what)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: %s FAILED: %s (0x%02lx).",
                       hc->hc_Hardware->bth_DevName, hc->hc_Hardware->bth_Unit, (STRPTR) what,
                       (completion->result == BT_CMDQ_RESULT_TIMEOUT) ? (STRPTR) "timeout" :
                       btNumToStr(BNTS_HCISTATUS, completion->status, "unknown"),
                       (ULONG) completion->status);
    } else {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: %s ok.",
                       hc->hc_Hardware->bth_DevName, hc->hc_Hardware->bth_Unit, (STRPTR) what);
    }
}
static void bScanParamCB(struct bt_cmdq_completion *completion, void *user_data)
{ bScanCmdLog((struct BtHWCore *) user_data, completion, "LE Set Scan Parameters"); }
static void bScanEnableCB(struct bt_cmdq_completion *completion, void *user_data)
{ bScanCmdLog((struct BtHWCore *) user_data, completion, "LE Set Scan Enable"); }
/* \\\ */

/* *** background LE scan *** */

/* /// "bBgScanCmdCB()" */
static void bBgScanCmdCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        struct BtBase *BluetoothBase = hc->hc_Base;
        hc->hc_BgScanActive = FALSE;
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Background LE scan refused by the controller: %s (%ld).",
                       btNumToStr(BNTS_HCISTATUS, completion->status, "unknown"), completion->status);
    }
}
/* \\\ */

/* /// "bBgScanStop()" */
/* Called before any LE scan or connect command: they are refused (Command
   Disallowed) while a scan is enabled on most controllers. */
void bBgScanStop(struct BtHWCore *hc)
{
    bt_timer_list_cancel(&hc->hc_Timers, &hc->hc_BgScanTimer);
    if(hc->hc_BgScanActive) {
        UBYTE params[2] = { 0, 0 };
        hc->hc_BgScanActive = FALSE;
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_ENABLE, params, 2, bIgnoreCompletion, hc);
    }
}
/* \\\ */

/* /// "bBgScanNeeded()" */
/* Is there a bonded LE device that should be reconnected when it shows up?
   Either a class is already waiting for it (cn_WaitAdv) or its auto-connect
   policy says so. */
static BOOL bBgScanNeeded(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd;
    BOOL needed = FALSE;

    if((bth->bth_State != BHS_READY) || !(bth->bth_Flags & BTHF_LE)) {
        return(FALSE);
    }
    if(!BluetoothBase->bt_GlobalCfg->bgc_AutoConnect) {
        return(FALSE);
    }
    btLockReadBase();
    for(bd = (struct BtDevice *) bth->bth_Devices.lh_Head; bd->bd_Node.ln_Succ; bd = (struct BtDevice *) bd->bd_Node.ln_Succ) {
        if((bd->bd_Flags & (BDFF_LE|BDFF_REGISTERED|BDFF_BONDED)) != (BDFF_LE|BDFF_REGISTERED|BDFF_BONDED)) {
            continue;
        }
        if(bd->bd_Conns[1] && (bd->bd_Conns[1]->cn_State == HCNS_CONNECTED)) {
            continue;
        }
        if(bd->bd_PoPoCfg.bpc_AutoConnect || (bd->bd_Conns[1] && bd->bd_Conns[1]->cn_WaitAdv)) {
            needed = TRUE;
            break;
        }
    }
    btUnlockBase();
    return(needed);
}
/* \\\ */

/* /// "bBgScanUpdate()" */
/* Start or stop the background scan according to the current state. */
void bBgScanUpdate(struct BtHWCore *hc)
{
    struct BtHardware *bth = hc->hc_Hardware;
    BOOL want;

    bt_timer_list_cancel(&hc->hc_Timers, &hc->hc_BgScanTimer);
    if((bth->bth_Flags & BTHF_DISCOVERING) || hc->hc_DiscoveryPending || hc->hc_LEScanActive ||
       hc->hc_Connecting || !hc->hc_BringupDone || hc->hc_BringupFailed) {
        want = FALSE;
    } else {
        want = bBgScanNeeded(hc);
    }
    if(want && !hc->hc_BgScanActive) {
        UBYTE params[7];
        struct bt_buf_writer w;
        bt_buf_writer_init(&w, params, sizeof(params));
        bt_buf_writer_write_u8(&w, 0x00);     /* passive: we only want to see who is there */
        bt_buf_writer_write_le16(&w, 0x0800); /* interval 1.28 s */
        bt_buf_writer_write_le16(&w, 0x0030); /* window 30 ms (~2% duty) */
        bt_buf_writer_write_u8(&w, 0x00);     /* public own address */
        bt_buf_writer_write_u8(&w, 0x00);     /* no filter policy (private addresses are resolved here) */
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_PARAMETERS, params, 7, bBgScanCmdCB, hc);
        params[0] = 0x01; /* enable */
        params[1] = 0x00; /* every report: a device may need a second look */
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_ENABLE, params, 2, bBgScanCmdCB, hc);
        hc->hc_BgScanActive = TRUE;
        KPRINTF(5, ("background LE scan on\n"));
    } else if(!want && hc->hc_BgScanActive) {
        bBgScanStop(hc);
        KPRINTF(5, ("background LE scan off\n"));
    }
}
/* \\\ */

/* /// "bBgScanTimerCB()" */
static void bBgScanTimerCB(struct bt_timer *timer, void *user_data)
{
    (void) timer;
    bBgScanUpdate((struct BtHWCore *) user_data);
}
/* \\\ */

/* /// "bBgScanSchedule()" */
/* Re-evaluate in a couple of seconds: links come and go in bursts (a class
   retrying a connect, a dual-mode device bringing up both bearers) and the
   scan should not flap on and off in between. */
void bBgScanSchedule(struct BtHWCore *hc)
{
    bt_timer_list_cancel(&hc->hc_Timers, &hc->hc_BgScanTimer);
    bt_timer_list_add(&hc->hc_Timers, &hc->hc_BgScanTimer, bNowUS(hc) + 2000000ULL);
}
/* \\\ */

/* /// "bStartDiscovery()" */
static LONG bStartDiscovery(struct BtHWCore *hc, struct BtDiscoveryParams *bdp)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    BOOL classic = bdp->bdp_Classic && (bth->bth_Flags & BTHF_CLASSIC);
    BOOL le = bdp->bdp_LE && (bth->bth_Flags & BTHF_LE);
    ULONG dur = bdp->bdp_Duration;

    if(bth->bth_State != BHS_READY) {
        return(BTIOERR_NOTREADY);
    }
    if(bth->bth_Flags & BTHF_DISCOVERING) {
        return(IOERR_UNITBUSY);
    }
    if(!classic && !le) {
        return(BTIOERR_NOTSUPPORTED);
    }
    if(dur > 60) {
        dur = 60;
    }
    if(bdp->bdp_ClearOld) {
        struct BtDevice *bd;
        struct BtDevice *next;
        btLockReadBase();
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
        while((next = (struct BtDevice *) bd->bd_Node.ln_Succ)) {
            if(!(bd->bd_Flags & (BDFF_REGISTERED|BDFF_CONNECTED|BDFF_CONNECTING)) && !bd->bd_DevBinding && !bd->bd_UseCnt &&
               !bHasSvcBinding(bd)) {
                btUnlockBase();
                btFreeDevice(bd);
                btSendEvent(BEHMB_REMDEVICE, bd, NULL);
                btLockReadBase();
                next = (struct BtDevice *) bth->bth_Devices.lh_Head;
            } else {
                bd->bd_Flags &= ~BDFF_DISCOVERED;
            }
            bd = next;
        }
        btUnlockBase();
    } else {
        struct BtDevice *bd;
        btLockReadBase();
        bd = (struct BtDevice *) bth->bth_Devices.lh_Head;
        while(bd->bd_Node.ln_Succ) {
            bd->bd_Flags &= ~BDFF_DISCOVERED;
            bd = (struct BtDevice *) bd->bd_Node.ln_Succ;
        }
        btUnlockBase();
    }

    bth->bth_Flags |= BTHF_DISCOVERING;
    hc->hc_ResolveNames = bdp->bdp_ResolveNames && classic;
    hc->hc_DiscoveryPending = TRUE;
    /* reset the scan diagnostics for this run */
    hc->hc_DiagAdvLegacy = hc->hc_DiagAdvExt = hc->hc_DiagAdvOther = 0;
    hc->hc_DiagInqResults = 0;
    hc->hc_DiagLESubeventMask = 0;
    hc->hc_DiagScanRsp = hc->hc_DiagDropped = 0;
    hc->hc_ScanDiagCount = 0;
    hc->hc_ScanDiagOverflow = 0;
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: discovery started (classic=%s, LE=%s, %lds).",
                   bth->bth_DevName, bth->bth_Unit,
                   classic ? (STRPTR) "yes" : (STRPTR) "no",
                   le ? (STRPTR) "yes" : (STRPTR) "no", dur);
    btSendEvent(BEHMB_DISCOVERYSTART, bth, NULL);

    bBgScanStop(hc);   /* scan parameters cannot be changed while scanning */
    if(le) {
        UBYTE params[7];
        struct bt_buf_writer w;
        bt_buf_writer_init(&w, params, sizeof(params));
        bt_buf_writer_write_u8(&w, 0x01);     /* active scan (get names) */
        bt_buf_writer_write_le16(&w, 0x0030); /* interval 30ms */
        bt_buf_writer_write_le16(&w, 0x0030); /* window 30ms */
        bt_buf_writer_write_u8(&w, 0x00);     /* public own address */
        bt_buf_writer_write_u8(&w, 0x00);     /* no filter policy */
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_PARAMETERS, params, 7, bScanParamCB, hc);
        params[0] = 0x01; /* enable */
        params[1] = 0x00; /* report duplicates: RSSI/name updates */
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_ENABLE, params, 2, bScanEnableCB, hc);
        hc->hc_LEScanActive = TRUE;
    }
    if(classic) {
        UBYTE params[5];
        struct bt_buf_writer w;
        ULONG inqlen = (dur * 100 + 127) / 128; /* 1.28s units */
        if(inqlen < 1) inqlen = 1;
        if(inqlen > 0x30) inqlen = 0x30;
        bt_buf_writer_init(&w, params, sizeof(params));
        bt_buf_writer_write_le24(&w, HC_GIAC_LAP);
        bt_buf_writer_write_u8(&w, inqlen);
        bt_buf_writer_write_u8(&w, 0); /* unlimited responses */
        bSubmitCmd(hc, HC_OP_INQUIRY, params, 5, bInquiryStatusCB, hc);
        hc->hc_InqActive = TRUE;
    }
    hc->hc_DiscoveryPending = FALSE;
    /* the LE scan needs a timer; the inquiry stops by itself but the timer
       also acts as a safety net */
    bt_timer_list_add(&hc->hc_Timers, &hc->hc_DiscoveryTimer, bNowUS(hc) + (uint64_t) dur * 1000000ULL);
    return(0);
}
/* \\\ */

/* /// "bStopDiscovery()" */
LONG bStopDiscovery(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct HCNameNode *nn;

    if(!(bth->bth_Flags & BTHF_DISCOVERING)) {
        return(0);
    }
    bt_timer_list_cancel(&hc->hc_Timers, &hc->hc_DiscoveryTimer);
    if(hc->hc_LEScanActive) {
        UBYTE params[2] = { 0, 0 };
        hc->hc_LEScanActive = FALSE;
        bSubmitCmd(hc, HC_OP_LE_SET_SCAN_ENABLE, params, 2, bIgnoreCompletion, hc);
    }
    if(hc->hc_InqActive) {
        hc->hc_InqActive = FALSE;
        bSubmitCmd(hc, HC_OP_INQUIRY_CANCEL, NULL, 0, bIgnoreCompletion, hc);
    }
    hc->hc_ResolveNames = FALSE;
    while((nn = (struct HCNameNode *) RemHead((struct List *) &hc->hc_NameQueue))) {
        btFreeVec(nn);
    }
    hc->hc_NameQueueCount = 0;
    hc->hc_DiscoveryPending = FALSE;
    if(!hc->hc_NameReqDev) {
        bFinishDiscovery(hc);
    }
    return(0);
}
/* \\\ */

/* *** remote name requests *** */

/* /// "bNameReqStatusCB()" */
static void bNameReqStatusCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    struct BtBase *BluetoothBase = hc->hc_Base;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        /* request refused: finish it now, the completion event will not come */
        struct BtChannel *bch = hc->hc_NameReqChannel;
        KPRINTF(10, ("Remote name request refused (%02lx)\n", completion->status));
        hc->hc_NameReqDev = NULL;
        hc->hc_NameReqChannel = NULL;
        if(bch) {
            bReplyChannel(BluetoothBase, bch, (completion->result == BT_CMDQ_RESULT_TIMEOUT) ? BTIOERR_TIMEOUT : BTIOERR_REMOTEERROR, 0);
        }
        bNextNameRequest(hc);
    }
}
/* \\\ */

/* /// "bNextNameRequest()" */
static void bNextNameRequest(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = NULL;
    struct BtChannel *bch = NULL;
    struct HCNameNode *nn;
    UBYTE params[10];

    if(hc->hc_NameReqDev || hc->hc_Shutdown) {
        return;
    }
    /* client channels first */
    if(hc->hc_NameChannels.mlh_Head->mln_Succ) {
        struct MinNode *mn = hc->hc_NameChannels.mlh_Head;
        Remove((struct Node *) mn);
        bch = (struct BtChannel *) (((UBYTE *) mn) - offsetof(struct BtChannel, bch_QueueNode));
        bd = bch->bch_Device;
    } else if((nn = (struct HCNameNode *) RemHead((struct List *) &hc->hc_NameQueue))) {
        bd = nn->nn_Device;
        btFreeVec(nn);
        hc->hc_NameQueueCount--;
    }
    if(!bd) {
        bStartDiscoveryPhase(hc);
        return;
    }
    hc->hc_NameReqDev = bd;
    hc->hc_NameReqChannel = bch;
    CopyMem(bd->bd_Address.bd_Addr, params, 6);
    params[6] = 0x01; /* page scan repetition mode R1 */
    params[7] = 0x00; /* reserved */
    params[8] = 0x00; /* clock offset */
    params[9] = 0x00;
    if(!bSubmitCmd(hc, HC_OP_REMOTE_NAME_REQUEST, params, 10, bNameReqStatusCB, hc)) {
        hc->hc_NameReqDev = NULL;
        hc->hc_NameReqChannel = NULL;
        if(bch) {
            bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
        }
        bNextNameRequest(hc);
    }
}
/* \\\ */

/* /// "bNameReqComplete()" */
static void bNameReqComplete(struct BtHWCore *hc, const UBYTE *params, ULONG len)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd;
    struct BtChannel *bch;
    UBYTE status;

    if(len < 7) {
        return;
    }
    status = params[0];
    btLockReadBase();
    bd = bFindDeviceByAddr(hc, &params[1]);
    btUnlockBase();
    if(bd && !status && (len > 7)) {
        BOOL changed;
        btLockWriteDevice(bd);
        changed = bSetDeviceName(hc, bd, &params[7], len - 7);
        btUnlockDevice(bd);
        if(changed) {
            btSendEvent(BEHMB_DEVICEUPDATE, bd, NULL);
        }
    }
    if(bd && (bd == hc->hc_NameReqDev)) {
        bch = hc->hc_NameReqChannel;
        hc->hc_NameReqDev = NULL;
        hc->hc_NameReqChannel = NULL;
        if(bch) {
            if(status) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_REMOTEERROR, status);
            } else {
                ULONG n = 0;
                if(bch->bch_Data && bch->bch_Length) {
                    while((n < len - 7) && (n < bch->bch_Length - 1) && params[7 + n]) {
                        ((UBYTE *) bch->bch_Data)[n] = params[7 + n];
                        n++;
                    }
                    ((UBYTE *) bch->bch_Data)[n] = 0;
                }
                bReplyChannel(BluetoothBase, bch, 0, n);
            }
        }
        bNextNameRequest(hc);
    }
}
/* \\\ */

/* *** HCI event dispatch *** */

/* /// "bHandleInquiryResult()" */
static void bHandleInquiryResult(struct BtHWCore *hc, UBYTE code, const UBYTE *params, ULONG len)
{
    ULONG n, count;

    switch(code) {
    case BT_HCI_EVENT_INQUIRY_RESULT: {
        struct bt_hci_inquiry_result_iter it;
        struct bt_hci_inquiry_result_entry entry;
        if(bt_hci_inquiry_result_iter_init(&it, params, len) != BT_OK) {
            return;
        }
        while(bt_hci_inquiry_result_iter_next(&it, &entry) == BT_OK) {
            hc->hc_DiagInqResults++;
            bScanDiagNote(hc, entry.bd_addr.b, 0xfe, FALSE, 0x01, 0, NULL, 0, NULL, 127, entry.class_of_device,
                          bNoteDevice(hc, entry.bd_addr.b, BDAT_PUBLIC, FALSE, entry.class_of_device, 127, NULL, 0, NULL, 0, 0, NULL));
        }
        break;
    }
    case HC_EVT_INQUIRY_RESULT_RSSI:
        /* num_responses, then bd_addr[n], psrm[n], reserved[n], cod[n], clock[n], rssi[n] */
        if(len < 1) {
            return;
        }
        count = params[0];
        if(len < 1 + count * 14) {
            return;
        }
        for(n = 0; n < count; n++) {
            const UBYTE *addr = &params[1 + n * 6];
            const UBYTE *cod = &params[1 + count * 8 + n * 3];
            LONG rssi = (BYTE) params[1 + count * 13 + n];
            ULONG codv = cod[0] | (cod[1] << 8) | (cod[2] << 16);
            hc->hc_DiagInqResults++;
            bScanDiagNote(hc, addr, 0xfe, FALSE, 0x02, 0, NULL, 0, NULL, rssi, codv,
                          bNoteDevice(hc, addr, BDAT_PUBLIC, FALSE, codv, rssi, NULL, 0, NULL, 0, 0, NULL));
        }
        break;
    case HC_EVT_EXTENDED_INQUIRY_RESULT: {
        /* num_responses(1) bd_addr(6) psrm(1) reserved(1) cod(3) clock(2) rssi(1) eir(240) */
        const UBYTE *addr;
        ULONG codv;
        LONG rssi;
        struct bt_le_adv_info info;
        if(len < 15) {
            return;
        }
        addr = &params[1];
        codv = params[9] | (params[10] << 8) | (params[11] << 16);
        rssi = (BYTE) params[14];
        bt_le_adv_parse(&params[15], len - 15, &info);
        hc->hc_DiagInqResults++;
        bScanDiagNote(hc, addr, 0xfe, FALSE, 0x04, 0, &params[15], len - 15, &info, rssi, codv,
                      bNoteDevice(hc, addr, BDAT_PUBLIC, FALSE, codv, rssi, info.name, info.name_len, &params[15], len - 15, 0, &info));
        break;
    }
    }
}
/* \\\ */

/* /// "bHandleLEMeta()" */
/* Advertising-report handler. Connection-related LE Meta subevents are consumed
   earlier by bConnHandleEvent(); this deals with the scan results. It accepts
   BOTH the legacy advertising report (subevent 0x02, BT 4.x) and the extended
   advertising report (subevent 0x0D) used by BT 5.0 / 6.0 controllers, feeding
   both into the same device-discovery path. The first time each subevent code
   is seen during a run it is logged, which conclusively shows whether a real
   adapter is emitting legacy or extended reports (or nothing). */
static void bHandleLEMeta(struct BtHWCore *hc, const UBYTE *params, ULONG len)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    UBYTE subevent;

    if(len < 1) {
        return;
    }
    subevent = params[0];

    if((subevent < 32) && !(hc->hc_DiagLESubeventMask & (1UL << subevent))) {
        hc->hc_DiagLESubeventMask |= (1UL << subevent);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: first LE Meta subevent 0x%02lx (%s).",
                       hc->hc_Hardware->bth_DevName, hc->hc_Hardware->bth_Unit, (ULONG) subevent,
                       (subevent == 0x02) ? (STRPTR) "legacy advertising report" :
                       (subevent == 0x0d) ? (STRPTR) "extended advertising report - BT 5/6" :
                       (STRPTR) "non-advertising");
    }

    if(subevent == BT_HCI_LE_META_SUBEVENT_ADVERTISING_REPORT) {   /* 0x02 legacy */
        struct bt_hci_le_adv_report_iter it;
        struct bt_hci_le_adv_report report;
        if(bt_hci_le_adv_report_iter_init(&it, params, len) != BT_OK) {
            return;
        }
        while(bt_hci_le_adv_report_iter_next(&it, &report) == BT_OK) {
            struct bt_le_adv_info info;
            bt_le_adv_parse(report.data, report.data_len, &info);
            hc->hc_DiagAdvLegacy++;
            if(report.event_type == 0x04) hc->hc_DiagScanRsp++;
            {
                BOOL noted = bNoteDevice(hc, report.address.b, report.address_type & 3, TRUE, 0, report.rssi,
                                         info.name, info.name_len, report.data, report.data_len, info.appearance, &info);
                bScanDiagNote(hc, report.address.b, report.address_type & 3, TRUE, (UBYTE)(1u << (report.event_type & 7)), 0,
                              report.data, report.data_len, &info, report.rssi, 0, noted);
            }
        }
    } else if(subevent == 0x0d) {   /* LE Extended Advertising Report (BT 5.0+) */
        ULONG num = (len >= 2) ? params[1] : 0;
        ULONG o = 2;
        ULONG i;
        /* Reports are laid out sequentially (variable-length Data forbids parallel
           arrays); each has a 24-byte fixed header then Data_Length bytes. */
        for(i = 0; i < num; i++) {
            const UBYTE *rep = &params[o];
            UBYTE addrtype;
            LONG rssi;
            ULONG dlen;
            struct bt_le_adv_info info;
            if(o + 24 > len) {
                break;   /* truncated event */
            }
            addrtype = rep[2];
            rssi = (BYTE) rep[13];
            dlen = rep[23];
            if(o + 24 + dlen > len) {
                dlen = (len > o + 24) ? (len - (o + 24)) : 0;   /* clamp to what arrived */
            }
            hc->hc_DiagAdvExt++;
            if(addrtype != 0xff) {   /* 0xff == anonymous advertiser (no address) */
                bt_le_adv_parse(&rep[24], dlen, &info);
                if(rep[0] & 0x08) hc->hc_DiagScanRsp++;
                {
                    BOOL noted = bNoteDevice(hc, &rep[3], addrtype & 3, TRUE, 0, rssi,
                                             info.name, info.name_len, &rep[24], dlen, info.appearance, &info);
                    bScanDiagNote(hc, &rep[3], addrtype & 3, TRUE, 0, rep[0], &rep[24], dlen, &info, rssi, 0, noted);
                }
            }
            o += 24 + rep[23];
        }
    } else {
        hc->hc_DiagAdvOther++;
    }
}
/* \\\ */

/* /// "bHandleEvent()" */
static void bHandleEvent(struct BtHWCore *hc, const UBYTE *data, ULONG len)
{
    struct bt_buf_reader r;
    struct bt_hci_event_header hdr;
    const uint8_t *params;

    bt_cmdq_on_event(&hc->hc_CmdQ, data, len, bNowUS(hc));

    bt_buf_reader_init(&r, data, len);
    if(bt_hci_parse_event_header(&r, &hdr) != BT_OK) {
        return;
    }
    params = bt_buf_reader_peek(&r, hdr.param_len);
    if(!params) {
        return;
    }
    KPRINTF(5, ("HCI event %02lx (%ld)\n", hdr.event_code, hdr.param_len));

    switch(hdr.event_code) {
    case BT_HCI_EVENT_INQUIRY_COMPLETE:
        hc->hc_InqActive = FALSE;
        bStartDiscoveryPhase(hc);
        break;
    case BT_HCI_EVENT_INQUIRY_RESULT:
    case HC_EVT_INQUIRY_RESULT_RSSI:
    case HC_EVT_EXTENDED_INQUIRY_RESULT:
        bHandleInquiryResult(hc, hdr.event_code, params, hdr.param_len);
        break;
    case HCIEVT_REMOTE_NAME_REQ_CMPL:
        bNameReqComplete(hc, params, hdr.param_len);
        break;
    case HC_EVT_LE_META:
        if(!bConnHandleEvent(hc, hdr.event_code, params, hdr.param_len)) {
            bHandleLEMeta(hc, params, hdr.param_len);
        }
        break;
    case HCIEVT_HARDWARE_ERROR: {
        struct BtBase *BluetoothBase = hc->hc_Base;
        hc->hc_Hardware->bth_ErrorCount++;
        btAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld reported hardware error %02lx.",
                       hc->hc_Hardware->bth_DevName, hc->hc_Hardware->bth_Unit,
                       hdr.param_len ? params[0] : 0);
        btSendEvent(BEHMB_HARDWAREERROR, hc->hc_Hardware, NULL);
        break;
    }
    default:
        bConnHandleEvent(hc, hdr.event_code, params, hdr.param_len);
        break;
    }
}
/* \\\ */

/* *** bring-up *** */

/* /// "bSyncCmdCB()" */
/* completion callback for bHciDoSync() - records the outcome for the waiter. */
static void bSyncCmdCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;

    hc->hc_SyncOK = (completion->result == BT_CMDQ_RESULT_COMPLETE);
    hc->hc_SyncStatus = completion->status;
    hc->hc_SyncRespLen = 0;
    if(hc->hc_SyncResp && hc->hc_SyncRespMax && completion->return_params) {
        UWORD n = (UWORD) completion->return_params_len;
        if(n > hc->hc_SyncRespMax) {
            n = hc->hc_SyncRespMax;
        }
        CopyMem((APTR) completion->return_params, hc->hc_SyncResp, n);
        hc->hc_SyncRespLen = n;
    }
    hc->hc_SyncDone = TRUE;
}
/* \\\ */

/* /// "bHciDoSync()" */
/* Send a single HCI command and block (pumping this hwtask's device/event/timer
   ports) until it completes or the command queue times it out. This is the
   channel pluggable firmware loaders use via BtFirmwareContext.fwc_HciCommand;
   it must only be called from the hwtask itself. Returns 0 on a completed,
   zero-status command, else the HCI status (or -1 on timeout / send failure). */
LONG bHciDoSync(struct BtHWCore *hc, UWORD opcode, CONST_APTR params, UWORD plen,
                UBYTE *status, UBYTE *resp, UWORD *resplen, UWORD respmax)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct IOBTHCIReq *req;
    struct BTHCIEventMsg *bem;
    ULONG sigmask, sigs;

    hc->hc_SyncDone = FALSE;
    hc->hc_SyncOK = FALSE;
    hc->hc_SyncStatus = 0xff;
    hc->hc_SyncResp = resp;
    hc->hc_SyncRespMax = respmax;
    hc->hc_SyncRespLen = 0;

    if(!bSubmitCmd(hc, opcode, (const UBYTE *) params, (UBYTE) plen, bSyncCmdCB, hc)) {
        if(status) *status = 0xff;
        if(resplen) *resplen = 0;
        return(-1);
    }

    sigmask = (1UL<<bth->bth_DevMsgPort.mp_SigBit) |
              (1UL<<bth->bth_EventMsgPort.mp_SigBit) |
              (1UL<<hc->hc_TimerPort->mp_SigBit);
    while(!hc->hc_SyncDone) {
        sigs = Wait(sigmask | SIGBREAKF_CTRL_C);
        while((req = (struct IOBTHCIReq *) GetMsg(&bth->bth_DevMsgPort))) {
            bHandleDevReply(hc, req);
        }
        while((bem = (struct BTHCIEventMsg *) GetMsg(&bth->bth_EventMsgPort))) {
            bHandleEvent(hc, (UBYTE *) &bem->bem_Event, bem->bem_Event.bhe_PayloadLength + 2);
            ReplyMsg(&bem->bem_Msg);
        }
        if(sigs & (1UL<<hc->hc_TimerPort->mp_SigBit)) {
            while(GetMsg(hc->hc_TimerPort)) {
                hc->hc_TimerPending = FALSE;
            }
            bTick(hc);
            bArmTimer(hc);
        }
        if(sigs & SIGBREAKF_CTRL_C) {
            /* leave CTRL_C pending for the caller's outer loop and give up */
            SetSignal(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
            break;
        }
    }

    if(status) *status = hc->hc_SyncStatus;
    if(resplen) *resplen = hc->hc_SyncRespLen;
    if(!hc->hc_SyncDone || !hc->hc_SyncOK) {
        return(-1);
    }
    return((LONG) hc->hc_SyncStatus);
}
/* \\\ */

/* /// "bBringupCB()" */
static void bBringupCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    const uint8_t *rp = completion->return_params;
    size_t rplen = completion->return_params_len;
    BOOL ok = (completion->result == BT_CMDQ_RESULT_COMPLETE) && !completion->status;
    BOOL optional = FALSE;

    switch(hc->hc_BringupStep) {
    case HCB_RESET:
    case HCB_READ_VERSION:
    case HCB_READ_FEATURES:
    case HCB_READ_BUFFER_SIZE:
    case HCB_READ_BD_ADDR:
        optional = FALSE;
        break;
    default:
        optional = TRUE;
        break;
    }
    /* trace every init step so a real adapter's bring-up is visible end to end */
    btAddErrorMsg(ok ? RETURN_OK : (optional ? RETURN_WARN : RETURN_FAIL), (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: init step %ld -> %s (0x%02lx).",
                   bth->bth_DevName, bth->bth_Unit, (ULONG) hc->hc_BringupStep,
                   ok ? (STRPTR) "ok" : (completion->result == BT_CMDQ_RESULT_TIMEOUT) ? (STRPTR) "timeout" : (STRPTR) "error",
                   (ULONG) completion->status);
    if(!ok) {
        bth->bth_LastHCIError = completion->status;
        if(!optional) {
            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "%s/%ld: initialisation failed at step %ld (%s).",
                           bth->bth_DevName, bth->bth_Unit, hc->hc_BringupStep,
                           (completion->result == BT_CMDQ_RESULT_TIMEOUT) ? (STRPTR) "timeout" :
                           btNumToStr(BNTS_HCISTATUS, completion->status, "unknown"));
            hc->hc_BringupFailed = TRUE;
            hc->hc_BringupDone = TRUE;
            return;
        }
        KPRINTF(10, ("optional bring-up step %ld failed (%02lx)\n", hc->hc_BringupStep, completion->status));
    } else {
        switch(hc->hc_BringupStep) {
        case HCB_READ_VERSION: {
            struct bt_hci_local_version ver;
            if(bt_hci_parse_local_version(rp, rplen, &ver) == BT_OK) {
                bth->bth_HCIVersion = ver.hci_version;
                bth->bth_HCIRevision = ver.hci_revision;
                bth->bth_LMPVersion = ver.lmp_pal_version;
                bth->bth_LMPSubversion = ver.lmp_pal_subversion;
                bth->bth_ManufacturerID = ver.manufacturer_name;
            }
            break;
        }
        case HCB_READ_FEATURES: {
            struct bt_hci_local_features feat;
            if(bt_hci_parse_local_features(rp, rplen, &feat) == BT_OK) {
                CopyMem(feat.features, bth->bth_Features, 8);
                bth->bth_Flags &= ~(BTHF_CLASSIC|BTHF_LE);
                if(!(feat.features[4] & 0x20)) {   /* BR/EDR not supported */
                    bth->bth_Flags |= BTHF_CLASSIC;
                }
                if(feat.features[4] & 0x40) {      /* LE supported (controller) */
                    bth->bth_Flags |= BTHF_LE;
                }
            }
            break;
        }
        case HCB_READ_BUFFER_SIZE: {
            struct bt_hci_buffer_size bs;
            if(bt_hci_parse_buffer_size(rp, rplen, &bs) == BT_OK) {
                bth->bth_ACLMaxPktSize = bs.acl_data_packet_length;
                bth->bth_ACLNumPkts = bs.total_num_acl_data_packets;
                bth->bth_SCOMaxPktSize = bs.sco_data_packet_length;
                bth->bth_SCONumPkts = bs.total_num_sco_data_packets;
            }
            break;
        }
        case HCB_READ_BD_ADDR:
            if(rplen >= 7) {
                CopyMem((APTR) &rp[1], bth->bth_Address.bd_Addr, 6);
                bAddrToStr(bth->bth_Address.bd_Addr, (STRPTR) bth->bth_AddrString);
            }
            break;
        case HCB_LE_READ_LOCAL_FEATURES:
            if(rplen >= 9) {
                CopyMem((APTR) &rp[1], bth->bth_LEFeatures, 8);
            }
            break;
        case HCB_LE_READ_BUFFER_SIZE:
            if(rplen >= 4) {
                bth->bth_LEACLMaxPktSize = rp[1] | (rp[2] << 8);
                bth->bth_LEACLNumPkts = rp[3];
                if(!bth->bth_LEACLMaxPktSize) {
                    /* shares the BR/EDR buffers */
                    bth->bth_LEACLMaxPktSize = bth->bth_ACLMaxPktSize;
                    bth->bth_LEACLNumPkts = bth->bth_ACLNumPkts;
                }
            }
            break;
        case HCB_READ_LOCAL_NAME:
            if(rplen >= 2) {
                ULONG n = 0;
                while((n < rplen - 1) && rp[1 + n]) {
                    n++;
                }
                if(n) {
                    STRPTR name = btAllocVec(n + 1);
                    if(name) {
                        CopyMem((APTR) &rp[1], name, n);
                        name[n] = 0;
                        btFreeVec(bth->bth_LocalName);
                        bth->bth_LocalName = name;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    hc->hc_BringupStep++;
    bBringupStep(hc);
}
/* \\\ */

/* /// "bBringupStep()" */
static void bBringupStep(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    UBYTE params[248];
    UBYTE len = 0;
    UWORD opcode = 0;

    while(!opcode) {
        switch(hc->hc_BringupStep) {
        case HCB_RESET:
            opcode = HC_OP_RESET;
            break;
        case HCB_READ_VERSION:
            opcode = HC_OP_READ_LOCAL_VERSION;
            break;
        case HCB_FIRMWARE:
            /* Hand off to any pluggable firmware loader that matches this
               controller. The load is synchronous and must run outside this
               async callback chain, so flag it and let the hwtask main loop
               run bDoFirmware() before continuing the bring-up. */
            hc->hc_FirmwarePending = TRUE;
            return;
        case HCB_READ_FEATURES:
            opcode = HC_OP_READ_LOCAL_FEATURES;
            break;
        case HCB_READ_BUFFER_SIZE:
            opcode = HC_OP_READ_BUFFER_SIZE;
            break;
        case HCB_READ_BD_ADDR:
            opcode = HC_OP_READ_BD_ADDR;
            break;
        case HCB_SET_EVENT_MASK:
            opcode = HC_OP_SET_EVENT_MASK;
            memset(params, 0xff, 8);
            params[7] = 0x3f;
            len = 8;
            break;
        case HCB_LE_SET_EVENT_MASK:
            if(!(bth->bth_Flags & BTHF_LE)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_LE_SET_EVENT_MASK;
            memset(params, 0, 8);
            params[0] = 0xff;
            params[1] = 0x1f;
            len = 8;
            break;
        case HCB_LE_READ_BUFFER_SIZE:
            if(!(bth->bth_Flags & BTHF_LE)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_LE_READ_BUFFER_SIZE;
            break;
        case HCB_LE_READ_LOCAL_FEATURES:
            if(!(bth->bth_Flags & BTHF_LE)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_LE_READ_LOCAL_FEATURES;
            break;
        case HCB_WRITE_LE_HOST_SUPPORT:
            if(!(bth->bth_Flags & BTHF_LE) || !(bth->bth_Flags & BTHF_CLASSIC)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_WRITE_LE_HOST_SUPPORT;
            params[0] = 1; /* LE supported host */
            params[1] = 0; /* simultaneous LE host (deprecated) */
            len = 2;
            break;
        case HCB_WRITE_INQUIRY_MODE:
            if(!(bth->bth_Flags & BTHF_CLASSIC)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_WRITE_INQUIRY_MODE;
            params[0] = 0x02; /* with RSSI and EIR */
            len = 1;
            break;
        case HCB_WRITE_SIMPLE_PAIRING:
            if(!(bth->bth_Flags & BTHF_CLASSIC) || (bth->bth_LMPVersion < 4)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_WRITE_SIMPLE_PAIRING;
            params[0] = 0x01;
            len = 1;
            break;
        case HCB_WRITE_CLASS_OF_DEVICE:
            if(!(bth->bth_Flags & BTHF_CLASSIC)) {
                hc->hc_BringupStep++;
                continue;
            }
            if(!bth->bth_ClassOfDevice) {
                bth->bth_ClassOfDevice = 0x000104; /* computer, desktop */
            }
            opcode = HC_OP_WRITE_CLASS_OF_DEVICE;
            params[0] = bth->bth_ClassOfDevice;
            params[1] = bth->bth_ClassOfDevice >> 8;
            params[2] = bth->bth_ClassOfDevice >> 16;
            len = 3;
            break;
        case HCB_WRITE_LOCAL_NAME: {
            STRPTR name = (STRPTR) BluetoothBase->bt_GlobalCfg->bgc_LocalName;
            if(!(bth->bth_Flags & BTHF_CLASSIC) || !name[0]) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_WRITE_LOCAL_NAME;
            memset(params, 0, 248);
            strncpy((char *) params, name, 247);
            len = 248;
            break;
        }
        case HCB_READ_LOCAL_NAME:
            if(!(bth->bth_Flags & BTHF_CLASSIC)) {
                hc->hc_BringupStep++;
                continue;
            }
            opcode = HC_OP_READ_LOCAL_NAME;
            break;
        case HCB_WRITE_SCAN_ENABLE:
            if(!(bth->bth_Flags & BTHF_CLASSIC)) {
                hc->hc_BringupStep++;
                continue;
            }
            bth->bth_Flags &= ~(BTHF_DISCOVERABLE|BTHF_CONNECTABLE);
            if(BluetoothBase->bt_GlobalCfg->bgc_Discoverable) {
                bth->bth_Flags |= BTHF_DISCOVERABLE;
            }
            if(BluetoothBase->bt_GlobalCfg->bgc_Connectable) {
                bth->bth_Flags |= BTHF_CONNECTABLE;
            }
            opcode = HC_OP_WRITE_SCAN_ENABLE;
            params[0] = ((bth->bth_Flags & BTHF_DISCOVERABLE) ? 1 : 0) | ((bth->bth_Flags & BTHF_CONNECTABLE) ? 2 : 0);
            len = 1;
            break;
        default:
            if(hc->hc_Reinit) {
                /* re-initialisation after a late firmware load: back in service */
                hc->hc_Reinit = FALSE;
                hc->hc_ACLCredits = bth->bth_ACLNumPkts;
                hc->hc_LEACLCredits = bth->bth_LEACLNumPkts;
                bth->bth_State = BHS_READY;
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "%s/%ld: controller re-initialised on the new firmware.",
                               bth->bth_DevName, bth->bth_Unit);
            }
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "%s/%ld: bring-up complete - HCI %ld.%ld LMP %ld.%ld, %s%s%s, addr %s.",
                           bth->bth_DevName, bth->bth_Unit,
                           (ULONG) bth->bth_HCIVersion, (ULONG) bth->bth_HCIRevision,
                           (ULONG) bth->bth_LMPVersion, (ULONG) bth->bth_LMPSubversion,
                           (bth->bth_Flags & BTHF_CLASSIC) ? (STRPTR) "BR/EDR" : (STRPTR) "",
                           ((bth->bth_Flags & BTHF_CLASSIC) && (bth->bth_Flags & BTHF_LE)) ? (STRPTR) "+" : (STRPTR) "",
                           (bth->bth_Flags & BTHF_LE) ? (STRPTR) "LE" : (STRPTR) "",
                           bth->bth_AddrString);
            hc->hc_BringupDone = TRUE;
            /* stock up on controller entropy for LE pairing */
            bConnRequestEntropy(hc, 4);
            /* and start listening for our bonded LE devices */
            bBgScanSchedule(hc);
            return;
        }
    }
    if(!bSubmitCmd(hc, opcode, len ? params : NULL, len, bBringupCB, hc)) {
        hc->hc_BringupFailed = TRUE;
        hc->hc_BringupDone = TRUE;
    }
}
/* \\\ */

/* *** channel / control handling *** */

/* /// "bHandleChannel()" */
void bHandleChannel(LIBBASETYPEPTR BluetoothBase, struct BtHardware *bth, struct BtChannel *bch, BOOL direct)
{
    struct BtHWCore *hc = bth->bth_Core;
    struct BtDevice *bd = bch->bch_Device;

    (void) direct;

    if(bch->bch_AbortChannel) {
        struct BtChannel *victim = bch->bch_AbortChannel;
        struct MinNode *mn;
        BOOL found = bConnAbortRequest(hc, bch->bch_AbortChannel);
        /* look for the channel in the queues */
        for(mn = hc->hc_NameChannels.mlh_Head; !found && mn->mln_Succ; mn = mn->mln_Succ) {
            struct BtChannel *qbch = (struct BtChannel *) (((UBYTE *) mn) - offsetof(struct BtChannel, bch_QueueNode));
            if(qbch == victim) {
                Remove((struct Node *) mn);
                found = TRUE;
                break;
            }
        }
        if(!found && (hc->hc_NameReqChannel == victim)) {
            hc->hc_NameReqChannel = NULL;
            found = TRUE;
        }
        if(found) {
            bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
        }
        bReplyChannel(BluetoothBase, bch, 0, 0);
        return;
    }

    if(hc->hc_Shutdown) {
        bReplyChannel(BluetoothBase, bch, IOERR_ABORTED, 0);
        return;
    }

    if(!bch->bch_Endpoint) {
        switch(bch->bch_Request) {
        case BTPRI_DISCOVERY:
            if(!bch->bch_Data || (bch->bch_Length < sizeof(struct BtDiscoveryParams))) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
            } else {
                bReplyChannel(BluetoothBase, bch, bStartDiscovery(hc, (struct BtDiscoveryParams *) bch->bch_Data), 0);
            }
            return;

        case BTPRI_STOPDISCOVERY:
            bReplyChannel(BluetoothBase, bch, bStopDiscovery(hc), 0);
            return;

        case BTPRI_SETLOCALNAME: {
            UBYTE params[248];
            STRPTR name = (STRPTR) bch->bch_Data;
            if(!(bth->bth_Flags & BTHF_CLASSIC) || (bth->bth_State != BHS_READY)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return;
            }
            memset(params, 0, 248);
            if(name) {
                strncpy((char *) params, name, 247);
            }
            btFreeVec(bth->bth_LocalName);
            bth->bth_LocalName = btCopyStr((STRPTR) params);
            bSubmitCmd(hc, HC_OP_WRITE_LOCAL_NAME, params, 248, bIgnoreCompletion, hc);
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return;
        }

        case BTPRI_SETSCANMODE: {
            UBYTE params[1];
            if(!(bth->bth_Flags & BTHF_CLASSIC) || (bth->bth_State != BHS_READY)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return;
            }
            bth->bth_Flags &= ~(BTHF_DISCOVERABLE|BTHF_CONNECTABLE);
            if(bch->bch_Value) {
                bth->bth_Flags |= BTHF_DISCOVERABLE;
            }
            if(bch->bch_Index) {
                bth->bth_Flags |= BTHF_CONNECTABLE;
            }
            params[0] = (bch->bch_Value ? 1 : 0) | (bch->bch_Index ? 2 : 0);
            bSubmitCmd(hc, HC_OP_WRITE_SCAN_ENABLE, params, 1, bIgnoreCompletion, hc);
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return;
        }

        case BTPRI_SETCOD: {
            UBYTE params[3];
            ULONG cod;
            if(!bch->bch_Data || (bch->bch_Length < sizeof(ULONG)) || !(bth->bth_Flags & BTHF_CLASSIC)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
                return;
            }
            cod = *((ULONG *) bch->bch_Data);
            bth->bth_ClassOfDevice = cod;
            params[0] = cod;
            params[1] = cod >> 8;
            params[2] = cod >> 16;
            bSubmitCmd(hc, HC_OP_WRITE_CLASS_OF_DEVICE, params, 3, bIgnoreCompletion, hc);
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return;
        }

        case BTPRI_REGISTER:
            if(!bd) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
                return;
            }
            btLockWriteDevice(bd);
            if(!(bd->bd_Flags & BDFF_REGISTERED)) {
                bd->bd_Flags |= BDFF_REGISTERED;
                btUnlockDevice(bd);
                bStoreDevConfig(BluetoothBase, bd, TRUE);
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Device %s registered.", bd->bd_Name);
                btSendEvent(BEHMB_DEVICEREGISTERED, bd, NULL);
            } else {
                btUnlockDevice(bd);
            }
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return;

        case BTPRI_UNREGISTER:
            if(!bd) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
                return;
            }
            btLockWriteDevice(bd);
            if(bd->bd_Flags & BDFF_REGISTERED) {
                bd->bd_Flags &= ~(BDFF_REGISTERED|BDFF_BONDED);
                memset(&bd->bd_Keys, 0, sizeof(bd->bd_Keys));
                bd->bd_CurAddrValid = FALSE;
                btUnlockDevice(bd);
                bStoreDevConfig(BluetoothBase, bd, TRUE);
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                               "Device %s unregistered.", bd->bd_Name);
                /* bindings are released by the event handler task */
                btSendEvent(BEHMB_DEVICEUNREGISTERED, bd, NULL);
            } else {
                btUnlockDevice(bd);
            }
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return;

        case BTPR_REMOTENAME:
            if(!bd) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
                return;
            }
            if(!(bth->bth_Flags & BTHF_CLASSIC) || !(bd->bd_Flags & BDFF_CLASSIC)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return;
            }
            bch->bch_Flags |= BCHF_QUEUED;
            AddTail((struct List *) &hc->hc_NameChannels, (struct Node *) &bch->bch_QueueNode);
            bNextNameRequest(hc);
            return;

        default:
            /* connection, pairing, service and channel requests */
            if(!bConnHandleRequest(hc, bch)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
            }
            return;
        }
    }
    /* endpoint channels */
    if(!bConnHandleRequest(hc, bch)) {
        bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
    }
}
/* \\\ */

/* *** task *** */

/* /// "bOpenHCIDevice()" */
static LONG bOpenHCIDevice(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    STRPTR devname = bth->bth_DevName;
    LONG ioerr = -1;

    while(*devname) {
        if(!(ioerr = OpenDevice(devname, bth->bth_Unit, (struct IORequest *) bth->bth_RootIOReq, 0))) {
            break;
        }
        do {
            if((*devname == '/') || (*devname == ':')) {
                ++devname;
                break;
            }
        } while(*(++devname));
    }
    return(ioerr);
}
/* \\\ */

/* /// "bQueryHCIDevice()" */
static void bQueryHCIDevice(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct TagItem taglist[8];
    STRPTR prodname = NULL;
    STRPTR manufacturer = NULL;
    STRPTR description = NULL;
    STRPTR copyright = NULL;
    IPTR version = 0;
    IPTR revision = 0;
    IPTR driververs = 0x0100;

    taglist[0].ti_Tag = BTA_ProductName;   taglist[0].ti_Data = (IPTR) &prodname;
    taglist[1].ti_Tag = BTA_Author;        taglist[1].ti_Data = (IPTR) &manufacturer;
    taglist[2].ti_Tag = BTA_Description;   taglist[2].ti_Data = (IPTR) &description;
    taglist[3].ti_Tag = BTA_Version;       taglist[3].ti_Data = (IPTR) &version;
    taglist[4].ti_Tag = BTA_Revision;      taglist[4].ti_Data = (IPTR) &revision;
    taglist[5].ti_Tag = BTA_Copyright;     taglist[5].ti_Data = (IPTR) &copyright;
    taglist[6].ti_Tag = BTA_DriverVersion; taglist[6].ti_Data = (IPTR) &driververs;
    taglist[7].ti_Tag = TAG_END;
    bth->bth_RootIOReq->iobt_Data = taglist;
    bth->bth_RootIOReq->iobt_Length = sizeof(taglist);
    bth->bth_RootIOReq->iobt_Req.io_Command = BTCMD_QUERYDEVICE;
    DoIO((struct IORequest *) bth->bth_RootIOReq);

    bth->bth_ProductName = btCopyStr(prodname ? prodname : (STRPTR) "n/a");
    bth->bth_Manufacturer = btCopyStr(manufacturer ? manufacturer : (STRPTR) "n/a");
    bth->bth_Description = btCopyStr(description ? description : (STRPTR) "n/a");
    bth->bth_Copyright = btCopyStr(copyright ? copyright : (STRPTR) "n/a");
    bth->bth_Version = version;
    bth->bth_Revision = revision;
    bth->bth_DriverVers = driververs;
    strcpy((char *) bth->bth_AddrString, "00:00:00:00:00:00");
}
/* \\\ */

/* /// "bAllocCore()" */
static struct BtHWCore * bAllocCore(struct BtBase *BluetoothBase, struct BtHardware *bth)
{
    struct BtHWCore *hc;
    UWORD n;

    if(!(hc = btAllocVec(sizeof(struct BtHWCore)))) {
        return(NULL);
    }
    hc->hc_Base = BluetoothBase;
    hc->hc_Hardware = bth;
    NewList((struct List *) &hc->hc_NameChannels);
    NewList((struct List *) &hc->hc_NameQueue);
    NewList((struct List *) &hc->hc_ACLTxQueue);
    NewList((struct List *) &hc->hc_Conns);
    hc->hc_Transport.ops = &bTransportOps;
    hc->hc_Transport.impl = hc;
    bt_timer_list_init(&hc->hc_Timers);
    bt_cmdq_init(&hc->hc_CmdQ, &hc->hc_Transport, &hc->hc_Timers);
    bt_timer_init(&hc->hc_DiscoveryTimer, bDiscoveryTimerCB, hc);
    bt_timer_init(&hc->hc_BgScanTimer, bBgScanTimerCB, hc);

    for(n = 0; n < HC_NUMCMDREQS; n++) {
        hc->hc_CmdReq[n] = (struct IOBTHCIReq *) CreateIORequest(&bth->bth_DevMsgPort, sizeof(struct IOBTHCIReq));
        if(!hc->hc_CmdReq[n]) {
            goto fail;
        }
    }
    for(n = 0; n < HC_NUMACLREADS; n++) {
        hc->hc_ACLReadReq[n] = (struct IOBTHCIReq *) CreateIORequest(&bth->bth_DevMsgPort, sizeof(struct IOBTHCIReq));
        hc->hc_ACLReadBuf[n] = btAllocVec(HC_ACLBUFSIZE);
        if(!hc->hc_ACLReadReq[n] || !hc->hc_ACLReadBuf[n]) {
            goto fail;
        }
    }
    for(n = 0; n < HC_NUMACLWRITES; n++) {
        hc->hc_ACLWriteReq[n] = (struct IOBTHCIReq *) CreateIORequest(&bth->bth_DevMsgPort, sizeof(struct IOBTHCIReq));
        hc->hc_ACLWriteBuf[n] = btAllocVec(HC_ACLBUFSIZE);
        if(!hc->hc_ACLWriteReq[n] || !hc->hc_ACLWriteBuf[n]) {
            goto fail;
        }
    }
    hc->hc_CtlReq = (struct IOBTHCIReq *) CreateIORequest(&bth->bth_DevMsgPort, sizeof(struct IOBTHCIReq));
    if(!hc->hc_CtlReq) {
        goto fail;
    }
    if(!(hc->hc_TimerPort = CreateMsgPort())) {
        goto fail;
    }
    hc->hc_TimerReq = (struct timerequest *) CreateIORequest(hc->hc_TimerPort, sizeof(struct timerequest));
    if(!hc->hc_TimerReq) {
        goto fail;
    }
    if(OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) hc->hc_TimerReq, 0)) {
        DeleteIORequest((struct IORequest *) hc->hc_TimerReq);
        hc->hc_TimerReq = NULL;
        goto fail;
    }
    return(hc);

fail:
    if(hc->hc_TimerReq) {
        CloseDevice((struct IORequest *) hc->hc_TimerReq);
        DeleteIORequest((struct IORequest *) hc->hc_TimerReq);
    }
    if(hc->hc_TimerPort) {
        DeleteMsgPort(hc->hc_TimerPort);
    }
    if(hc->hc_CtlReq) {
        DeleteIORequest((struct IORequest *) hc->hc_CtlReq);
    }
    for(n = 0; n < HC_NUMACLWRITES; n++) {
        if(hc->hc_ACLWriteReq[n]) {
            DeleteIORequest((struct IORequest *) hc->hc_ACLWriteReq[n]);
        }
        btFreeVec(hc->hc_ACLWriteBuf[n]);
    }
    for(n = 0; n < HC_NUMACLREADS; n++) {
        if(hc->hc_ACLReadReq[n]) {
            DeleteIORequest((struct IORequest *) hc->hc_ACLReadReq[n]);
        }
        btFreeVec(hc->hc_ACLReadBuf[n]);
    }
    for(n = 0; n < HC_NUMCMDREQS; n++) {
        if(hc->hc_CmdReq[n]) {
            DeleteIORequest((struct IORequest *) hc->hc_CmdReq[n]);
        }
    }
    btFreeVec(hc);
    return(NULL);
}
/* \\\ */

/* /// "bFreeCore()" */
static void bFreeCore(struct BtBase *BluetoothBase, struct BtHWCore *hc)
{
    UWORD n;
    struct HCNameNode *nn;

    while((nn = (struct HCNameNode *) RemHead((struct List *) &hc->hc_NameQueue))) {
        btFreeVec(nn);
    }
    if(hc->hc_TimerReq) {
        CloseDevice((struct IORequest *) hc->hc_TimerReq);
        DeleteIORequest((struct IORequest *) hc->hc_TimerReq);
    }
    if(hc->hc_TimerPort) {
        DeleteMsgPort(hc->hc_TimerPort);
    }
    DeleteIORequest((struct IORequest *) hc->hc_CtlReq);
    for(n = 0; n < HC_NUMACLWRITES; n++) {
        DeleteIORequest((struct IORequest *) hc->hc_ACLWriteReq[n]);
        btFreeVec(hc->hc_ACLWriteBuf[n]);
    }
    for(n = 0; n < HC_NUMACLREADS; n++) {
        DeleteIORequest((struct IORequest *) hc->hc_ACLReadReq[n]);
        btFreeVec(hc->hc_ACLReadBuf[n]);
    }
    for(n = 0; n < HC_NUMCMDREQS; n++) {
        DeleteIORequest((struct IORequest *) hc->hc_CmdReq[n]);
    }
    btFreeVec(hc);
}
/* \\\ */

/* /// "bStartACLRead()" */
static void bStartACLRead(struct BtHWCore *hc, UWORD n)
{
    struct IOBTHCIReq *req = hc->hc_ACLReadReq[n];
    req->iobt_Req.io_Command = BTCMD_READACL;
    req->iobt_Data = hc->hc_ACLReadBuf[n];
    req->iobt_Length = HC_ACLBUFSIZE;
    req->iobt_Actual = 0;
    req->iobt_Req.io_Error = 0;
    hc->hc_ACLReadPending[n] = TRUE;
    SendIO((struct IORequest *) req);
    hc->hc_Hardware->bth_MsgCount++;
}
/* \\\ */

/* /// "bStartACLWrite()" */
/* Hands queued outbound ACL packets to the controller as long as write
   requests and controller buffers (credits) are available. */
void bStartACLWrite(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct HCACLTx *tx;
    UWORD n;

    while((tx = (struct HCACLTx *) hc->hc_ACLTxQueue.mlh_Head)->tx_Node.mln_Succ) {
        struct BtHWConn *cn = NULL;
        struct MinNode *mn;
        ULONG *credits = &hc->hc_ACLCredits;
        struct IOBTHCIReq *req = NULL;

        for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
            if(((struct BtHWConn *) mn)->cn_Handle == tx->tx_Handle) {
                cn = (struct BtHWConn *) mn;
                break;
            }
        }
        if(cn && (cn->cn_LinkType == BDLT_LE) && bth->bth_LEACLNumPkts) {
            credits = &hc->hc_LEACLCredits;
        }
        if(!*credits) {
            return;
        }
        for(n = 0; n < HC_NUMACLWRITES; n++) {
            if(!hc->hc_ACLWritePending[n]) {
                req = hc->hc_ACLWriteReq[n];
                break;
            }
        }
        if(!req) {
            return;
        }
        Remove((struct Node *) tx);
        CopyMem(tx->tx_Data, hc->hc_ACLWriteBuf[n], tx->tx_Length);
        req->iobt_Req.io_Command = BTCMD_WRITEACL;
        req->iobt_Data = hc->hc_ACLWriteBuf[n];
        req->iobt_Length = tx->tx_Length;
        req->iobt_Actual = 0;
        req->iobt_Req.io_Error = 0;
        hc->hc_ACLWritePending[n] = TRUE;
        (*credits)--;
        if(cn) {
            cn->cn_Credits++;
        }
        SendIO((struct IORequest *) req);
        bth->bth_MsgCount++;
        btFreeVec(tx);
    }
}
/* \\\ */

/* /// "bArmTimer()" */
static void bArmTimer(struct BtHWCore *hc)
{
    if(hc->hc_TimerPending) {
        return;
    }
    hc->hc_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
    hc->hc_TimerReq->tr_time.tv_secs = 0;
    hc->hc_TimerReq->tr_time.tv_micro = HC_TICK_MS * 1000;
    SendIO((struct IORequest *) hc->hc_TimerReq);
    hc->hc_TimerPending = TRUE;
}
/* \\\ */

/* /// "bTick()" */
static void bTick(struct BtHWCore *hc)
{
    uint64_t now = bNowUS(hc);
    struct bt_timer *t;

    hc->hc_Tick += HC_TICK_MS;
    bt_cmdq_tick(&hc->hc_CmdQ, now);
    while((t = bt_timer_list_pop_expired(&hc->hc_Timers, now))) {
        if(t->callback) {
            t->callback(t, t->user_data);
        }
    }
    bConnTick(hc);
    /* registrations and policies change from other tasks: re-check the
       background scan every few seconds as a catch-all */
    if((LONG) (hc->hc_Tick - hc->hc_BgScanCheckTick) >= 5000) {
        hc->hc_BgScanCheckTick = hc->hc_Tick;
        if(!hc->hc_BgScanTimer.pending) {
            bBgScanUpdate(hc);
        }
    }
}
/* \\\ */

/* /// "bHandleDevReply()" */
static void bHandleDevReply(struct BtHWCore *hc, struct IOBTHCIReq *req)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    UWORD n;

    bth->bth_MsgCount--;
    for(n = 0; n < HC_NUMCMDREQS; n++) {
        if(req == hc->hc_CmdReq[n]) {
            hc->hc_CmdPending[n] = FALSE;
            if(req->iobt_Req.io_Error) {
                bth->bth_ErrorCount++;
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "%s/%ld: HCI command transmission failed: %s (%ld).",
                               bth->bth_DevName, bth->bth_Unit,
                               btNumToStr(BNTS_IOERR, req->iobt_Req.io_Error, "unknown"), req->iobt_Req.io_Error);
            }
            return;
        }
    }
    for(n = 0; n < HC_NUMACLREADS; n++) {
        if(req == hc->hc_ACLReadReq[n]) {
            hc->hc_ACLReadPending[n] = FALSE;
            if(!req->iobt_Req.io_Error) {
                if(!hc->hc_Shutdown) {
                    bConnHandleACL(hc, hc->hc_ACLReadBuf[n], req->iobt_Actual);
                }
            } else if(req->iobt_Req.io_Error != IOERR_ABORTED) {
                bth->bth_ErrorCount++;
            }
            if(!hc->hc_Shutdown && (req->iobt_Req.io_Error != IOERR_ABORTED)) {
                bStartACLRead(hc, n);
            }
            return;
        }
    }
    for(n = 0; n < HC_NUMACLWRITES; n++) {
        if(req == hc->hc_ACLWriteReq[n]) {
            hc->hc_ACLWritePending[n] = FALSE;
            if(req->iobt_Req.io_Error && (req->iobt_Req.io_Error != IOERR_ABORTED)) {
                bth->bth_ErrorCount++;
                KPRINTF(10, ("ACL write failed (%ld)\n", req->iobt_Req.io_Error));
            }
            if(!hc->hc_Shutdown) {
                bStartACLWrite(hc);
            }
            return;
        }
    }
    if(req == hc->hc_CtlReq) {
        return;
    }
    KPRINTF(20, ("Unknown IORequest %p replied!\n", req));
}
/* \\\ */

/* /// "bHWTask()" */
AROS_UFH0(void, bHWTask)
{
    AROS_USERFUNC_INIT
    LIBBASETYPEPTR BluetoothBase;
    struct BtHardware *bth;
    struct BtHWCore *hc = NULL;
    struct Task *thistask;
    ULONG sigs;
    ULONG sigmask;
    LONG ioerr;
    struct BtChannel *bch;
    struct IOBTHCIReq *req;
    struct BTHCIEventMsg *bem;
    ULONG cnt;
    UWORD n;

    thistask = FindTask(NULL);
    bth = thistask->tc_UserData;
    BluetoothBase = bth->bth_Base;
    SetTaskPri(thistask, 21);

    /* keep the library from being expunged while the task runs */
    if(!OpenLibrary("bluetooth.library", 1)) {
        Alert(AG_OpenLib);
        return;
    }

    memset(&bth->bth_TaskMsgPort, 0, sizeof(bth->bth_TaskMsgPort));
    bth->bth_TaskMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    bth->bth_TaskMsgPort.mp_Node.ln_Name = (APTR) bth;
    bth->bth_TaskMsgPort.mp_Flags = PA_SIGNAL;
    bth->bth_TaskMsgPort.mp_SigTask = thistask;
    bth->bth_TaskMsgPort.mp_SigBit = AllocSignal(-1L);
    NewList(&bth->bth_TaskMsgPort.mp_MsgList);

    memset(&bth->bth_DevMsgPort, 0, sizeof(bth->bth_DevMsgPort));
    bth->bth_DevMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    bth->bth_DevMsgPort.mp_Node.ln_Name = (APTR) bth;
    bth->bth_DevMsgPort.mp_Flags = PA_SIGNAL;
    bth->bth_DevMsgPort.mp_SigTask = thistask;
    bth->bth_DevMsgPort.mp_SigBit = AllocSignal(-1L);
    NewList(&bth->bth_DevMsgPort.mp_MsgList);

    memset(&bth->bth_EventMsgPort, 0, sizeof(bth->bth_EventMsgPort));
    bth->bth_EventMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    bth->bth_EventMsgPort.mp_Node.ln_Name = (APTR) bth;
    bth->bth_EventMsgPort.mp_Flags = PA_SIGNAL;
    bth->bth_EventMsgPort.mp_SigTask = thistask;
    bth->bth_EventMsgPort.mp_SigBit = AllocSignal(-1L);
    NewList(&bth->bth_EventMsgPort.mp_MsgList);

    if((bth->bth_RootIOReq = (struct IOBTHCIReq *) CreateIORequest(&bth->bth_DevMsgPort, sizeof(struct IOBTHCIReq)))) {
        if((hc = bAllocCore(BluetoothBase, bth))) {
            bth->bth_Core = hc;
            hc->hc_Task = thistask;
            ioerr = bOpenHCIDevice(hc);
            if(!ioerr) {
                bth->bth_Node.ln_Name = bth->bth_RootIOReq->iobt_Req.io_Device->dd_Library.lib_Node.ln_Name;
                bQueryHCIDevice(hc);

                /* copy the opened device into the other requests */
                for(n = 0; n < HC_NUMCMDREQS; n++) {
                    hc->hc_CmdReq[n]->iobt_Req.io_Device = bth->bth_RootIOReq->iobt_Req.io_Device;
                    hc->hc_CmdReq[n]->iobt_Req.io_Unit = bth->bth_RootIOReq->iobt_Req.io_Unit;
                }
                for(n = 0; n < HC_NUMACLREADS; n++) {
                    hc->hc_ACLReadReq[n]->iobt_Req.io_Device = bth->bth_RootIOReq->iobt_Req.io_Device;
                    hc->hc_ACLReadReq[n]->iobt_Req.io_Unit = bth->bth_RootIOReq->iobt_Req.io_Unit;
                }
                for(n = 0; n < HC_NUMACLWRITES; n++) {
                    hc->hc_ACLWriteReq[n]->iobt_Req.io_Device = bth->bth_RootIOReq->iobt_Req.io_Device;
                    hc->hc_ACLWriteReq[n]->iobt_Req.io_Unit = bth->bth_RootIOReq->iobt_Req.io_Unit;
                }
                hc->hc_CtlReq->iobt_Req.io_Device = bth->bth_RootIOReq->iobt_Req.io_Device;
                hc->hc_CtlReq->iobt_Req.io_Unit = bth->bth_RootIOReq->iobt_Req.io_Unit;

                /* register the event port */
                hc->hc_CtlReq->iobt_Req.io_Command = BTCMD_ADDMSGPORT;
                hc->hc_CtlReq->iobt_Data = &bth->bth_EventMsgPort;
                hc->hc_CtlReq->iobt_Length = sizeof(struct MsgPort);
                DoIO((struct IORequest *) hc->hc_CtlReq);
                hc->hc_EventPortAdded = hc->hc_CtlReq->iobt_Req.io_Error ? FALSE : TRUE;
                if(!hc->hc_EventPortAdded) {
                    btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                                   "%s/%ld does not accept an event message port (%ld).",
                                   bth->bth_DevName, bth->bth_Unit, hc->hc_CtlReq->iobt_Req.io_Error);
                }

                for(n = 0; n < HC_NUMACLREADS; n++) {
                    bStartACLRead(hc, n);
                }
                bConnInit(hc);

                sigmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F |
                          (1UL<<bth->bth_DevMsgPort.mp_SigBit) |
                          (1UL<<bth->bth_TaskMsgPort.mp_SigBit) |
                          (1UL<<bth->bth_EventMsgPort.mp_SigBit) |
                          (1UL<<hc->hc_TimerPort->mp_SigBit);

                /* HCI bring-up */
                bth->bth_State = BHS_STARTING;
                if(hc->hc_EventPortAdded) {
                    hc->hc_BringupStep = HCB_RESET;
                    bBringupStep(hc);
                } else {
                    hc->hc_BringupFailed = TRUE;
                    hc->hc_BringupDone = TRUE;
                }
                bArmTimer(hc);
                while(!hc->hc_BringupDone) {
                    if(hc->hc_FirmwarePending) {
                        /* HCB_FIRMWARE reached: run the (synchronous) firmware
                           loader here, then resume the bring-up sequence. */
                        hc->hc_FirmwarePending = FALSE;
                        bDoFirmware(hc);
                        hc->hc_BringupStep++;
                        bBringupStep(hc);
                        continue;
                    }
                    sigs = Wait(sigmask & ~(1UL<<bth->bth_TaskMsgPort.mp_SigBit));
                    while((req = (struct IOBTHCIReq *) GetMsg(&bth->bth_DevMsgPort))) {
                        bHandleDevReply(hc, req);
                    }
                    while((bem = (struct BTHCIEventMsg *) GetMsg(&bth->bth_EventMsgPort))) {
                        bHandleEvent(hc, (UBYTE *) &bem->bem_Event, bem->bem_Event.bhe_PayloadLength + 2);
                        ReplyMsg(&bem->bem_Msg);
                    }
                    if(sigs & (1UL<<hc->hc_TimerPort->mp_SigBit)) {
                        while(GetMsg(hc->hc_TimerPort)) {
                            hc->hc_TimerPending = FALSE;
                        }
                        bTick(hc);
                        bArmTimer(hc);
                    }
                    if(sigs & SIGBREAKF_CTRL_C) {
                        hc->hc_BringupFailed = TRUE;
                        hc->hc_BringupDone = TRUE;
                    }
                }

                if(!hc->hc_BringupFailed) {
                    hc->hc_ACLCredits = bth->bth_ACLNumPkts;
                    hc->hc_LEACLCredits = bth->bth_LEACLNumPkts;
                    bth->bth_State = BHS_READY;
                    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "%s/%ld ready: %s, HCI %s, LMP %s (%s), %s%s.",
                                   bth->bth_DevName, bth->bth_Unit, bth->bth_AddrString,
                                   btNumToStr(BNTS_LMPVERSION, bth->bth_HCIVersion, "?"),
                                   btNumToStr(BNTS_LMPVERSION, bth->bth_LMPVersion, "?"),
                                   btNumToStr(BNTS_MANUFACTURER, bth->bth_ManufacturerID, "unknown vendor"),
                                   (bth->bth_Flags & BTHF_CLASSIC) ? "BR/EDR" : "",
                                   (bth->bth_Flags & BTHF_LE) ? ((bth->bth_Flags & BTHF_CLASSIC) ? " + LE" : "LE") : "");
                    KPRINTF(10, ("%s ready!\n", thistask->tc_Node.ln_Name));
                    bth->bth_Task = thistask;

                    btLockWriteBase();
                    AddTail(&BluetoothBase->bt_Hardware, &bth->bth_Node);
                    btUnlockBase();

                    Forbid();
                    if(bth->bth_ReadySigTask) {
                        Signal(bth->bth_ReadySigTask, 1L<<bth->bth_ReadySignal);
                    }
                    Permit();
                    do {
                        while((bch = (struct BtChannel *) GetMsg(&bth->bth_TaskMsgPort))) {
                            bHandleChannel(BluetoothBase, bth, bch, FALSE);
                        }
                        while((req = (struct IOBTHCIReq *) GetMsg(&bth->bth_DevMsgPort))) {
                            bHandleDevReply(hc, req);
                        }
                        while((bem = (struct BTHCIEventMsg *) GetMsg(&bth->bth_EventMsgPort))) {
                            bHandleEvent(hc, (UBYTE *) &bem->bem_Event, bem->bem_Event.bhe_PayloadLength + 2);
                            ReplyMsg(&bem->bem_Msg);
                        }
                        while(GetMsg(hc->hc_TimerPort)) {
                            hc->hc_TimerPending = FALSE;
                        }
                        if(!hc->hc_TimerPending) {
                            bTick(hc);
                            bArmTimer(hc);
                        }
                        if(hc->hc_FirmwarePending) {
                            /* HCB_FIRMWARE reached during a re-run bring-up */
                            hc->hc_FirmwarePending = FALSE;
                            bDoFirmware(hc);
                            hc->hc_BringupStep++;
                            bBringupStep(hc);
                        }
                        if(sigs & SIGBREAKF_CTRL_F) {
                            /* a firmware loader bound after this controller was
                               already up (the USB class adds the radio long before
                               BTStackLoader runs): load the firmware now. The
                               controller restarts on it at HCI defaults, so every
                               event mask/mode set during bring-up is gone - run the
                               bring-up again from Reset. */
                            if(bDoFirmware(hc)) {
                                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                               "%s/%ld: firmware loaded after bring-up - re-initialising the controller.",
                                               bth->bth_DevName, bth->bth_Unit);
                                bth->bth_State = BHS_STARTING;
                                hc->hc_Reinit = TRUE;
                                hc->hc_BringupDone = FALSE;
                                hc->hc_BringupFailed = FALSE;
                                hc->hc_BringupStep = HCB_RESET;
                                bBringupStep(hc);
                            }
                        }
                        sigs = Wait(sigmask);
                    } while(!(sigs & SIGBREAKF_CTRL_C));

                    hc->hc_Shutdown = TRUE;
                    bth->bth_State = BHS_OFFLINE;
                    btLockWriteBase();
                    Remove(&bth->bth_Node);
                    btUnlockBase();
                } else {
                    bth->bth_State = BHS_ERROR;
                    hc->hc_Shutdown = TRUE;
                }

                /* shutdown: drop connections and abort pending channels */
                bConnShutdown(hc);
                {
                    struct MinNode *mn;
                    while((mn = (struct MinNode *) RemHead((struct List *) &hc->hc_NameChannels))) {
                        bch = (struct BtChannel *) (((UBYTE *) mn) - offsetof(struct BtChannel, bch_QueueNode));
                        bReplyChannel(BluetoothBase, bch, IOERR_ABORTED, 0);
                    }
                    if(hc->hc_NameReqChannel) {
                        bReplyChannel(BluetoothBase, hc->hc_NameReqChannel, IOERR_ABORTED, 0);
                        hc->hc_NameReqChannel = NULL;
                    }
                }
                while((bch = (struct BtChannel *) GetMsg(&bth->bth_TaskMsgPort))) {
                    bReplyChannel(BluetoothBase, bch, IOERR_ABORTED, 0);
                }
                if(hc->hc_TimerPending) {
                    AbortIO((struct IORequest *) hc->hc_TimerReq);
                    WaitIO((struct IORequest *) hc->hc_TimerReq);
                    hc->hc_TimerPending = FALSE;
                }
                if(hc->hc_EventPortAdded) {
                    hc->hc_CtlReq->iobt_Req.io_Command = BTCMD_REMMSGPORT;
                    hc->hc_CtlReq->iobt_Data = &bth->bth_EventMsgPort;
                    hc->hc_CtlReq->iobt_Length = sizeof(struct MsgPort);
                    DoIO((struct IORequest *) hc->hc_CtlReq);
                }
                /* Flush all pending IO Requests */
                bth->bth_RootIOReq->iobt_Req.io_Command = CMD_FLUSH;
                DoIO((struct IORequest *) bth->bth_RootIOReq);
                cnt = 0;
                while(bth->bth_MsgCount) {
                    KPRINTF(20, ("Still %ld iorequests pending!\n", bth->bth_MsgCount));
                    btDelayMS(100);
                    if(++cnt == 50) {
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "There are still %ld IORequests pending, before unit can go down. Driver buggy?",
                                       bth->bth_MsgCount);
                    }
                    if(cnt == 300) {
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Okay, I've waited long enough, sod these %ld IORequests.",
                                       bth->bth_MsgCount);
                        bth->bth_MsgCount = 0;
                        break;
                    }
                    while((req = (struct IOBTHCIReq *) GetMsg(&bth->bth_DevMsgPort))) {
                        bHandleDevReply(hc, req);
                    }
                }
                while((bem = (struct BTHCIEventMsg *) GetMsg(&bth->bth_EventMsgPort))) {
                    ReplyMsg(&bem->bem_Msg);
                }
                CloseDevice((struct IORequest *) bth->bth_RootIOReq);
            } else {
                btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                               "Opening %s unit %ld failed %s (%ld).",
                               bth->bth_DevName, bth->bth_Unit, btNumToStr(BNTS_IOERR, ioerr, "unknown"), ioerr);
            }
            bFreeCore(BluetoothBase, hc);
            bth->bth_Core = NULL;
        }
        DeleteIORequest((struct IORequest *) bth->bth_RootIOReq);
        bth->bth_RootIOReq = NULL;
    }
    FreeSignal((LONG) bth->bth_TaskMsgPort.mp_SigBit);
    FreeSignal((LONG) bth->bth_DevMsgPort.mp_SigBit);
    FreeSignal((LONG) bth->bth_EventMsgPort.mp_SigBit);

    CloseLibrary((struct Library *) BluetoothBase);
    Forbid();
    bth->bth_Task = NULL;
    if(bth->bth_ReadySigTask) {
        Signal(bth->bth_ReadySigTask, 1L<<bth->bth_ReadySignal);
    }
    AROS_USERFUNC_EXIT
}
/* \\\ */
