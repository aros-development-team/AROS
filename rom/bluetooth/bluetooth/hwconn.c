/*
 *----------------------------------------------------------------------------
 *      bluetooth.library: connections, channels, services (hardware task)
 *----------------------------------------------------------------------------
 *
 * Runs inside the hardware task (see hwtask.c). Manages ACL links to remote
 * devices, routes ACL data to the per-link L2CAP channel manager of btcore,
 * maps BtEndpoints onto L2CAP channels / GATT characteristics, enumerates
 * services (SDP / GATT) into BtService/BtEndpoint objects and drives classic
 * (SSP) pairing. Every request arrives as a BtChannel message from
 * bHandleChannel() and is replied with bReplyChannel().
 */

#include "debug.h"

#include "hwtask.h"
#include "aes128.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <string.h>

#define NewList(list) NEWLIST(list)
#define min(x,y) (((x) < (y)) ? (x) : (y))

#define DOSBase BluetoothBase->bt_DosBase
#define TimerBase BluetoothBase->bt_TimerIOReq.tr_node.io_Device

#define BCH_FROM_QNODE(mn) ((struct BtChannel *) (((UBYTE *) (mn)) - offsetof(struct BtChannel, bch_QueueNode)))

/* service enumeration states */
enum {
    ENUM_IDLE = 0,
    ENUM_SDP_CONNECT,
    ENUM_SDP_SEARCH,
    ENUM_SDP_ATTRS,
    ENUM_GATT_CONNECT,
    ENUM_GATT_SERVICES,
    ENUM_GATT_CHARS,
    ENUM_GATT_DESCS,        /* descriptors of each notifying / HID Report characteristic */
    ENUM_GATT_REPORTREF,    /* reading one HID Report Reference descriptor */
    ENUM_DONE
};

/* pairing states */
enum {
    PAIR_IDLE = 0,
    PAIR_CONNECTING,
    PAIR_AUTH,
    PAIR_WAITUSER,
    PAIR_ENCRYPT,
    PAIR_SMP,           /* LE: the Security Manager is running */
    PAIR_WAITENUM       /* LE: pairing requested while services are being enumerated */
};

static void bConnRunEnum(struct BtHWConn *cn);
static void bConnFinishEnum(struct BtHWConn *cn, LONG error);
static struct BtEndpoint * bNextDescEndpoint(struct BtHWConn *cn);
static void bEndpointEvent(struct bt_l2cap_channel_event_info *info, void *user_data);
static void bFlushWaitingRequests(struct BtHWConn *cn, LONG error);
static void bDispatchWaiting(struct BtHWConn *cn);
static void bStartPairing(struct BtHWConn *cn);
static void bPairingDone(struct BtHWConn *cn, LONG error, ULONG status);
static void bAskUser(struct BtHWConn *cn, UBYTE type, ULONG passkey);
static void bSMPChannelEvent(struct bt_l2cap_channel_event_info *info, void *user_data);
static void bLEReencrypt(struct BtHWConn *cn);
static BOOL bSMPSetup(struct BtHWConn *cn);
static void bSMPStart(struct BtHWConn *cn);
static void bSMPFail(struct BtHWConn *cn, UBYTE reason, CONST_STRPTR why);

/* how bEnsureConnection() may bring a link up */
enum {
    CONN_NONE = 0,      /* never: fail if the link is not already up */
    CONN_NOW,           /* page / initiate right away (user action) */
    CONN_AUTO           /* a class wants the device: bonded LE peers are
                           waited for (background scan) instead of paged */
};

/* *** small helpers *** */

/* /// "bFindConnByHandle()" */
static struct BtHWConn * bFindConnByHandle(struct BtHWCore *hc, UWORD handle)
{
    struct MinNode *mn;
    for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        if((cn->cn_Handle == handle) && (cn->cn_State != HCNS_FREE)) {
            return(cn);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bReqQueueAdd()" */
static void bReqQueueAdd(struct MinList *list, struct BtChannel *bch)
{
    bch->bch_Flags |= BCHF_QUEUED;
    AddTail((struct List *) list, (struct Node *) &bch->bch_QueueNode);
}
/* \\\ */

/* /// "bReqQueueRemove()" */
static BOOL bReqQueueRemove(struct MinList *list, struct BtChannel *bch)
{
    struct MinNode *mn;
    for(mn = list->mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        if(BCH_FROM_QNODE(mn) == bch) {
            Remove((struct Node *) mn);
            bch->bch_Flags &= ~BCHF_QUEUED;
            return(TRUE);
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "bReqQueueFlush()" */
static void bReqQueueFlush(struct BtBase *BluetoothBase, struct MinList *list, LONG error)
{
    struct MinNode *mn;
    while((mn = (struct MinNode *) RemHead((struct List *) list))) {
        bReplyChannel(BluetoothBase, BCH_FROM_QNODE(mn), error, 0);
    }
}
/* \\\ */

/* *** ACL transmit (btcore transport op) *** */

/* /// "bSendACLPacket()" */
/* Called by the L2CAP channel manager with a complete ACL packet (header +
   fragment). Queued and handed to the controller by bStartACLWrite() as
   buffers become available. */
int bSendACLPacket(struct bt_hci_transport *transport, const uint8_t *data, size_t length)
{
    struct BtHWCore *hc = transport->impl;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct HCACLTx *tx;

    if((length < 4) || (length > HC_ACLBUFSIZE)) {
        return(-1);
    }
    tx = btAllocVec(sizeof(struct HCACLTx) + length);
    if(!tx) {
        return(-1);
    }
    tx->tx_Handle = (data[0] | (data[1] << 8)) & 0x0fff;
    tx->tx_Length = length;
    CopyMem((APTR) data, tx->tx_Data, length);
    AddTail((struct List *) &hc->hc_ACLTxQueue, (struct Node *) tx);
    bStartACLWrite(hc);
    return(0);
}
/* \\\ */

/* *** endpoints (L2CAP channels / characteristics) *** */

/* /// "bFindHWEndpoint()" */
static struct BtHWEndpoint * bFindHWEndpoint(struct BtHWConn *cn, struct BtEndpoint *bep)
{
    struct MinNode *mn;
    for(mn = cn->cn_Endpoints.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWEndpoint *hep = (struct BtHWEndpoint *) mn;
        if(hep->hep_Endpoint == bep) {
            return(hep);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bAllocHWEndpoint()" */
static struct BtHWEndpoint * bAllocHWEndpoint(struct BtHWConn *cn, struct BtEndpoint *bep)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtHWEndpoint *hep = btAllocVec(sizeof(struct BtHWEndpoint));
    if(hep) {
        hep->hep_Conn = cn;
        hep->hep_Endpoint = bep;
        hep->hep_State = HEPS_CLOSED;
        NewList((struct List *) &hep->hep_ReadReqs);
        NewList((struct List *) &hep->hep_RxQueue);
        NewList((struct List *) &hep->hep_WriteReqs);
        AddTail((struct List *) &cn->cn_Endpoints, (struct Node *) hep);
        bep->bep_Chan = hep;
    }
    return(hep);
}
/* \\\ */

/* /// "bFreeHWEndpoint()" */
static void bFreeHWEndpoint(struct BtHWEndpoint *hep, LONG error)
{
    struct BtBase *BluetoothBase = hep->hep_Conn->cn_Core->hc_Base;
    struct HCRxSDU *rx;

    bReqQueueFlush(BluetoothBase, &hep->hep_ReadReqs, error);
    bReqQueueFlush(BluetoothBase, &hep->hep_WriteReqs, error);
    while((rx = (struct HCRxSDU *) RemHead((struct List *) &hep->hep_RxQueue))) {
        btFreeVec(rx);
    }
    if(hep->hep_Endpoint && (hep->hep_Endpoint->bep_Chan == hep)) {
        hep->hep_Endpoint->bep_Chan = NULL;
    }
    Remove((struct Node *) hep);
    btFreeVec(hep);
}
/* \\\ */

/* /// "bDeliverSDU()" */
/* Data arrived on an endpoint: complete the oldest read request or queue. */
static void bDeliverSDU(struct BtHWEndpoint *hep, const UBYTE *data, ULONG len)
{
    struct BtBase *BluetoothBase = hep->hep_Conn->cn_Core->hc_Base;
    struct MinNode *mn;

    if((mn = (struct MinNode *) RemHead((struct List *) &hep->hep_ReadReqs))) {
        struct BtChannel *bch = BCH_FROM_QNODE(mn);
        LONG err = 0;
        ULONG n = len;
        if(n > bch->bch_Length) {
            n = bch->bch_Length;
            err = BTIOERR_OVERFLOW;
        }
        if(n && bch->bch_Data) {
            CopyMem((APTR) data, bch->bch_Data, n);
        }
        bReplyChannel(BluetoothBase, bch, err, n);
        return;
    }
    if(hep->hep_RxCount >= HC_RXQUEUE_MAX) {
        struct HCRxSDU *old = (struct HCRxSDU *) RemHead((struct List *) &hep->hep_RxQueue);
        btFreeVec(old);
        hep->hep_RxCount--;
    }
    {
        struct HCRxSDU *rx = btAllocVec(sizeof(struct HCRxSDU) + len);
        if(rx) {
            rx->rx_Length = len;
            CopyMem((APTR) data, rx->rx_Data, len);
            AddTail((struct List *) &hep->hep_RxQueue, (struct Node *) rx);
            hep->hep_RxCount++;
        }
    }
}
/* \\\ */

/* /// "bEndpointWrite()" */
static void bEndpointWrite(struct BtHWEndpoint *hep, struct BtChannel *bch)
{
    struct BtHWConn *cn = hep->hep_Conn;
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    bt_status_t st;

    if(bch->bch_Length > BT_L2CAP_MAX_SEND_LEN) {
        bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
        return;
    }
    st = bt_l2cap_channel_manager_send(&cn->cn_L2CAP, hep->hep_LocalCID, bch->bch_Data, bch->bch_Length, bNowUS(hc));
    if(st != BT_OK) {
        bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
    } else {
        bReplyChannel(BluetoothBase, bch, 0, bch->bch_Length);
    }
}
/* \\\ */

/* /// "bEndpointEvent()" */
/* btcore L2CAP channel callback for endpoint channels */
static void bEndpointEvent(struct bt_l2cap_channel_event_info *info, void *user_data)
{
    struct BtHWEndpoint *hep = user_data;
    struct BtBase *BluetoothBase = hep->hep_Conn->cn_Core->hc_Base;
    struct MinNode *mn;

    switch(info->event) {
    case BT_L2CAP_CHANNEL_EVENT_OPENED:
        hep->hep_LocalCID = info->local_cid;
        hep->hep_State = HEPS_OPEN;
        KPRINTF(10, ("endpoint channel open cid %04lx\n", info->local_cid));
        /* flush queued writes */
        while((mn = (struct MinNode *) RemHead((struct List *) &hep->hep_WriteReqs))) {
            struct BtChannel *bch = BCH_FROM_QNODE(mn);
            bch->bch_Flags &= ~BCHF_QUEUED;
            bEndpointWrite(hep, bch);
        }
        break;
    case BT_L2CAP_CHANNEL_EVENT_DATA:
        bDeliverSDU(hep, info->data, info->data_len);
        break;
    case BT_L2CAP_CHANNEL_EVENT_CLOSED: {
        LONG err;
        switch(info->close_reason) {
        case BT_L2CAP_CLOSE_REFUSED:
            err = BTIOERR_REFUSED;
            break;
        case BT_L2CAP_CLOSE_TIMEOUT:
            err = BTIOERR_TIMEOUT;
            break;
        case BT_L2CAP_CLOSE_CONFIG_FAILED:
            err = BTIOERR_CHANNELFAILED;
            break;
        case BT_L2CAP_CLOSE_LOCAL:
            err = IOERR_ABORTED;
            break;
        default:
            err = BTIOERR_DISCONNECTED;
            break;
        }
        KPRINTF(10, ("endpoint channel closed (%ld)\n", info->close_reason));
        hep->hep_State = HEPS_CLOSED;
        hep->hep_LocalCID = 0;
        bReqQueueFlush(BluetoothBase, &hep->hep_ReadReqs, err);
        bReqQueueFlush(BluetoothBase, &hep->hep_WriteReqs, err);
        break;
    }
    }
}
/* \\\ */

/* /// "bEndpointOpen()" */
static BOOL bEndpointOpen(struct BtHWEndpoint *hep)
{
    struct BtHWConn *cn = hep->hep_Conn;
    struct BtHWCore *hc = cn->cn_Core;
    struct BtEndpoint *bep = hep->hep_Endpoint;
    uint16_t cid;

    if(hep->hep_State != HEPS_CLOSED) {
        return(TRUE);
    }
    switch(bep->bep_Type) {
    case BEPT_L2CAP:
        if(bt_l2cap_channel_manager_open(&cn->cn_L2CAP, bep->bep_PSM, bep->bep_MaxPktSize ? bep->bep_MaxPktSize : BT_L2CAP_DEFAULT_MTU,
                                         bEndpointEvent, hep, &cid, bNowUS(hc)) != BT_OK) {
            return(FALSE);
        }
        hep->hep_LocalCID = cid;
        hep->hep_State = HEPS_OPENING;
        return(TRUE);
    case BEPT_L2CAP_FIXED:
        if(bt_l2cap_channel_manager_open_fixed(&cn->cn_L2CAP, bep->bep_CID, bEndpointEvent, hep) != BT_OK) {
            return(FALSE);
        }
        return(TRUE);
    default:
        return(FALSE);
    }
}
/* \\\ */

/* /// "bEndpointClose()" */
static void bEndpointClose(struct BtHWEndpoint *hep)
{
    struct BtHWConn *cn = hep->hep_Conn;
    if(hep->hep_State == HEPS_CLOSED) {
        return;
    }
    hep->hep_State = HEPS_CLOSING;
    bt_l2cap_channel_manager_close(&cn->cn_L2CAP, hep->hep_LocalCID, bNowUS(cn->cn_Core));
    hep->hep_State = HEPS_CLOSED;
}
/* \\\ */

/* *** GATT endpoints *** */

/* /// "bGATTNotify()" */
static void bGATTNotify(uint16_t handle, const uint8_t *value, size_t len, bool indication, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct MinNode *mn;
    (void) indication;
    for(mn = cn->cn_Endpoints.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWEndpoint *hep = (struct BtHWEndpoint *) mn;
        if(hep->hep_Endpoint && (hep->hep_Endpoint->bep_Type == BEPT_GATT_CHAR) &&
           (hep->hep_Endpoint->bep_Handle == handle)) {
            bDeliverSDU(hep, value, len);
            return;
        }
    }
}
/* \\\ */

/* /// "bGATTOpComplete()" */
/* completion of a control channel GATT read/write */
static void bGATTOpComplete(struct bt_gatt_client_completion *completion, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtChannel *bch = cn->cn_CtrlReq;

    if(!bch) {
        return;
    }
    cn->cn_CtrlReq = NULL;
    if(completion->result != BT_GATT_CLIENT_OK) {
        bReplyChannel(BluetoothBase, bch, (completion->result == BT_GATT_CLIENT_ERROR_TIMEOUT) ? BTIOERR_TIMEOUT : BTIOERR_REMOTEERROR, completion->att_error_code);
        return;
    }
    if(completion->op == BT_GATT_CLIENT_OP_READ) {
        ULONG n = completion->value_len;
        LONG err = 0;
        if(n > bch->bch_Length) {
            n = bch->bch_Length;
            err = BTIOERR_OVERFLOW;
        }
        if(n) {
            CopyMem((APTR) completion->value, bch->bch_Data, n);
        }
        bReplyChannel(BluetoothBase, bch, err, n);
    } else {
        bReplyChannel(BluetoothBase, bch, 0, bch->bch_Length);
    }
    bDispatchWaiting(cn);
}
/* \\\ */

/* /// "bGATTCCCDComplete()" */
static void bGATTCCCDComplete(struct bt_gatt_client_completion *completion, void *user_data)
{
    struct BtHWEndpoint *hep = user_data;
    struct BtHWConn *cn = hep->hep_Conn;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtEndpoint *bep = hep->hep_Endpoint;
    hep->hep_CCCDWritten = TRUE;
    hep->hep_State = HEPS_OPEN;
    cn->cn_CCCDBusy = FALSE;
    if(completion->result != BT_GATT_CLIENT_OK) {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: enabling notifications on %s (handle %04lx, CCCD %04lx) failed (ATT 0x%02lx).",
                       hep->hep_Conn->cn_Device->bd_Name, bep->bep_Name, (ULONG) bep->bep_Handle,
                       (ULONG) (bep->bep_CCCDHandle ? bep->bep_CCCDHandle : bep->bep_Handle + 1), (ULONG) completion->att_error_code);
    } else {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: notifications enabled on %s (handle %04lx%s).",
                       hep->hep_Conn->cn_Device->bd_Name, bep->bep_Name, (ULONG) bep->bep_Handle,
                       bep->bep_ReportID ? (STRPTR) ", HID report" : (STRPTR) "");
    }
    bDispatchWaiting(cn);
}

/* re-issue requests that were parked while the GATT client was busy */
static void bDispatchWaiting(struct BtHWConn *cn)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct MinList again;
    struct MinNode *mn;
    NewList((struct List *) &again);
    while((mn = (struct MinNode *) RemHead((struct List *) &cn->cn_WaitReqs))) {
        AddTail((struct List *) &again, (struct Node *) mn);
    }
    while((mn = (struct MinNode *) RemHead((struct List *) &again))) {
        struct BtChannel *bch = BCH_FROM_QNODE(mn);
        bch->bch_Flags &= ~BCHF_QUEUED;
        if(!bConnHandleRequest(cn->cn_Core, bch)) {
            bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
        }
    }
}
/* \\\ */

/* *** connections *** */

/* A device can be connected on both bearers at once (dual mode): bd_Conns[0]
   is the BR/EDR link, bd_Conns[1] the LE link. */
#define BD_BEARERIDX(lt)  (((lt) == BDLT_LE) ? 1 : 0)
#define BD_CONN(bd, lt)   ((bd)->bd_Conns[BD_BEARERIDX(lt)])

/* /// "bDevConn()" */
/* the "primary" connection for generic use: an existing BR/EDR link if any,
   otherwise the LE one. */
static struct BtHWConn * bDevConn(struct BtDevice *bd)
{
    return bd->bd_Conns[0] ? bd->bd_Conns[0] : bd->bd_Conns[1];
}
/* \\\ */

/* /// "bAllocConn()" */
static struct BtHWConn * bAllocConn(struct BtHWCore *hc, struct BtDevice *bd, UBYTE linktype)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHWConn *cn = btAllocVec(sizeof(struct BtHWConn));
    if(cn) {
        cn->cn_Core = hc;
        cn->cn_Device = bd;
        cn->cn_LinkType = linktype;
        cn->cn_State = HCNS_CONNECTING;
        NewList((struct List *) &cn->cn_Endpoints);
        NewList((struct List *) &cn->cn_WaitReqs);
        AddTail((struct List *) &hc->hc_Conns, (struct Node *) cn);
        BD_CONN(bd, linktype) = cn;
        bd->bd_Flags |= BDFF_CONNECTING;
    }
    return(cn);
}
/* \\\ */

/* /// "bConnUp()" */
static void bConnUp(struct BtHWConn *cn, UWORD handle, UBYTE role)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd = cn->cn_Device;
    struct BtService *bsv;
    struct BtEndpoint *bep;
    ULONG fraglen;

    cn->cn_Handle = handle;
    cn->cn_Role = role;
    cn->cn_State = HCNS_CONNECTED;
    cn->cn_LastActivity = hc->hc_Tick;
    if(hc->hc_Connecting == cn) {
        hc->hc_Connecting = NULL;
    }
    bBgScanSchedule(hc);

    fraglen = (cn->cn_LinkType == BDLT_LE) ? bth->bth_LEACLMaxPktSize : bth->bth_ACLMaxPktSize;
    if(!fraglen) {
        fraglen = bth->bth_ACLMaxPktSize;
    }
    bt_l2cap_channel_manager_init(&cn->cn_L2CAP, &hc->hc_Transport, handle,
                                  (cn->cn_LinkType == BDLT_LE) ? BT_L2CAP_CID_SIGNALING_LE : BT_L2CAP_CID_SIGNALING_CLASSIC,
                                  fraglen);
    bt_sdp_client_init(&cn->cn_SDP, &cn->cn_L2CAP);
    bt_gatt_client_init(&cn->cn_GATT, &cn->cn_L2CAP);
    bt_gatt_client_set_notify_handler(&cn->cn_GATT, bGATTNotify, cn);

    /* listen on the known L2CAP PSMs so the device can open channels to us */
    btLockReadDevice(bd);
    for(bsv = (struct BtService *) bd->bd_Services.lh_Head; bsv->bsv_Node.ln_Succ; bsv = (struct BtService *) bsv->bsv_Node.ln_Succ) {
        for(bep = (struct BtEndpoint *) bsv->bsv_Endpoints.lh_Head; bep->bep_Node.ln_Succ; bep = (struct BtEndpoint *) bep->bep_Node.ln_Succ) {
            if((bep->bep_Type == BEPT_L2CAP) && (cn->cn_LinkType == BDLT_ACL)) {
                struct BtHWEndpoint *hep = bFindHWEndpoint(cn, bep);
                if(!hep) {
                    hep = bAllocHWEndpoint(cn, bep);
                }
                if(hep) {
                    bt_l2cap_channel_manager_listen(&cn->cn_L2CAP, bep->bep_PSM, BT_L2CAP_DEFAULT_MTU, bEndpointEvent, hep);
                }
            }
        }
    }
    btUnlockDevice(bd);

    btLockWriteDevice(bd);
    bd->bd_Flags &= ~BDFF_CONNECTING;
    bd->bd_Flags |= BDFF_CONNECTED;
    bd->bd_ConnHandle = handle;
    bd->bd_Role = role;
    bd->bd_LinkType = cn->cn_LinkType;
    bd->bd_DeadCount = 0;
    if(bd->bd_Flags & BDFF_DEAD) {
        bd->bd_Flags &= ~BDFF_DEAD;
    }
    if(bHaveDOS(BluetoothBase)) {
        DateStamp(&bd->bd_LastSeen);
    }
    btUnlockDevice(bd);

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "Connected to %s (%s link, handle %04lx, %s).", bd->bd_Name,
                   (cn->cn_LinkType == BDLT_LE) ? "LE" : "BR/EDR", handle,
                   (role == BDR_CENTRAL) ? "central" : "peripheral");
    btSendEvent(BEHMB_DEVICECONNECTED, bd, NULL);

    /* re-dispatch requests that waited for the link */
    {
        struct MinNode *mn;
        while((mn = (struct MinNode *) RemHead((struct List *) &cn->cn_WaitReqs))) {
            struct BtChannel *bch = BCH_FROM_QNODE(mn);
            bch->bch_Flags &= ~BCHF_QUEUED;
            if(!bConnHandleRequest(hc, bch)) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
            }
        }
    }
    if(cn->cn_LinkType == BDLT_LE) {
        /* always listen on the SMP channel (Security Requests) and re-encrypt a
           bonded peer BEFORE talking GATT to it: ATT traffic racing the LL
           encryption procedure is how a discovery comes back empty. */
        bt_l2cap_channel_manager_open_fixed(&cn->cn_L2CAP, BT_L2CAP_CID_SMP, bSMPChannelEvent, cn);
        if((role == BDR_CENTRAL) && (bd->bd_Keys.bkc_Flags & BKCF_LTK) && (cn->cn_PairState != PAIR_CONNECTING)) {
            bLEReencrypt(cn);
        }
    }
    /* enumerate this bearer's services once (each bearer of a dual-mode device
       enumerates independently, accumulating onto the one device). A pending
       re-encryption defers this until the Encryption Change event. */
    if(!cn->cn_Enumerated && (cn->cn_EnumState == ENUM_IDLE) && !cn->cn_EncryptPending) {
        cn->cn_EnumState = (cn->cn_LinkType == BDLT_LE) ? ENUM_GATT_CONNECT : ENUM_SDP_CONNECT;
        bConnRunEnum(cn);
    }
    if(cn->cn_PairState == PAIR_CONNECTING) {
        bStartPairing(cn);
    }
}
/* \\\ */

/* /// "bConnDown()" */
static void bConnDown(struct BtHWConn *cn, LONG error, UBYTE reason)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    struct MinNode *mn;
    BOOL wasup = (cn->cn_State == HCNS_CONNECTED);

    if(hc->hc_Connecting == cn) {
        hc->hc_Connecting = NULL;
    }
    cn->cn_State = HCNS_FREE;
    cn->cn_Reason = reason;

    while((mn = cn->cn_Endpoints.mlh_Head)->mln_Succ) {
        bFreeHWEndpoint((struct BtHWEndpoint *) mn, error); /* removes the node */
    }
    bFlushWaitingRequests(cn, error);
    if(cn->cn_CtrlReq) {
        bReplyChannel(BluetoothBase, cn->cn_CtrlReq, error, 0);
        cn->cn_CtrlReq = NULL;
    }
    if(cn->cn_EnumState != ENUM_IDLE) {
        bConnFinishEnum(cn, error);
    }
    cn->cn_SMPActive = FALSE;
    cn->cn_SMPChanOpen = FALSE;
    if(cn->cn_PairState != PAIR_IDLE) {
        bPairingDone(cn, error, reason);
    }

    btLockWriteDevice(bd);
    if(BD_CONN(bd, cn->cn_LinkType) == cn) {
        BD_CONN(bd, cn->cn_LinkType) = NULL;
    }
    {
        /* a dual-mode device may still be up on the other bearer */
        struct BtHWConn *other = bDevConn(bd);
        if(other && (other->cn_State == HCNS_CONNECTED)) {
            bd->bd_ConnHandle = other->cn_Handle;
            bd->bd_LinkType = other->cn_LinkType;
        } else {
            bd->bd_Flags &= ~(BDFF_CONNECTED|BDFF_CONNECTING|BDFF_ENCRYPTED);
            bd->bd_ConnHandle = 0;
            bd->bd_Role = BDR_NONE;
            bd->bd_LinkType = BDLT_NONE;
        }
    }
    if(!wasup) {
        bd->bd_DeadCount += 4;
        if((bd->bd_DeadCount > 14) && !(bd->bd_Flags & BDFF_DEAD)) {
            bd->bd_Flags |= BDFF_DEAD;
            btUnlockDevice(bd);
            btSendEvent(BEHMB_DEVICEDEAD, bd, NULL);
            btLockWriteDevice(bd);
        }
    }
    btUnlockDevice(bd);

    if(wasup) {
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "Disconnected from %s (%s).", bd->bd_Name,
                       btNumToStr(BNTS_HCISTATUS, reason, "unknown reason"));
        btSendEvent(BEHMB_DEVICEDISCONNECTED, bd, (APTR) (IPTR) reason);
    } else {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "Connection to %s failed (%s).", bd->bd_Name,
                       btNumToStr(BNTS_HCISTATUS, reason, "unknown reason"));
    }
    Remove((struct Node *) cn);
    btFreeVec(cn);
    bBgScanSchedule(hc);
}
/* \\\ */

/* /// "bFlushWaitingRequests()" */
static void bFlushWaitingRequests(struct BtHWConn *cn, LONG error)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    bReqQueueFlush(BluetoothBase, &cn->cn_WaitReqs, error);
}
/* \\\ */

/* /// "bConnectStatusCB()" */
static void bConnectStatusCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtHWCore *hc = cn->cn_Core;
    if((completion->result != BT_CMDQ_RESULT_COMPLETE) || completion->status) {
        hc->hc_Hardware->bth_LastHCIError = completion->status;
        if(cn->cn_State == HCNS_CONNECTING) {
            bConnDown(cn, BTIOERR_CONNFAILED, completion->status ? completion->status : 0xff);
        }
    }
}
/* \\\ */

/* /// "bStartConnect()" */
static BOOL bStartConnect(struct BtHWConn *cn)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtDevice *bd = cn->cn_Device;
    UBYTE params[25];

    hc->hc_Connecting = cn;
    cn->cn_LastActivity = hc->hc_Tick;   /* connect attempts time out, see bConnTick() */
    if(cn->cn_LinkType == BDLT_LE) {
        struct bt_buf_writer w;
        /* initiate to the address the peer is using right now (a resolved
           private address) rather than its identity */
        const UBYTE *peer = bd->bd_CurAddrValid ? bd->bd_CurAddr : bd->bd_Address.bd_Addr;
        UBYTE peertype = bd->bd_CurAddrValid ? bd->bd_CurAddrType : bd->bd_AddrType;
        bBgScanStop(hc);   /* initiating is refused while a scan runs */
        bt_buf_writer_init(&w, params, sizeof(params));
        bt_buf_writer_write_le16(&w, 0x0060);           /* scan interval */
        bt_buf_writer_write_le16(&w, 0x0030);           /* scan window */
        bt_buf_writer_write_u8(&w, 0x00);               /* filter policy: peer address */
        bt_buf_writer_write_u8(&w, peertype & 1);       /* peer address type */
        bt_buf_writer_write_bytes(&w, peer, 6);
        bt_buf_writer_write_u8(&w, 0x00);               /* own address type public */
        bt_buf_writer_write_le16(&w, 0x0018);           /* conn interval min 30ms */
        bt_buf_writer_write_le16(&w, 0x0028);           /* conn interval max 50ms */
        bt_buf_writer_write_le16(&w, 0x0000);           /* latency */
        bt_buf_writer_write_le16(&w, 0x02a0);           /* supervision timeout 6.72s */
        bt_buf_writer_write_le16(&w, 0x0000);           /* min CE */
        bt_buf_writer_write_le16(&w, 0x0000);           /* max CE */
        return(bSubmitCmd(hc, HC_OP_LE_CREATE_CONNECTION, params, 25, bConnectStatusCB, cn));
    } else {
        CopyMem(bd->bd_Address.bd_Addr, params, 6);
        params[6] = 0x18;  /* packet types: DM1 DH1 */
        params[7] = 0xcc;  /* DM3 DH3 DM5 DH5 */
        params[8] = 0x01;  /* page scan repetition mode R1 */
        params[9] = 0x00;  /* reserved */
        params[10] = 0x00; /* clock offset */
        params[11] = 0x00;
        params[12] = 0x01; /* allow role switch */
        return(bSubmitCmd(hc, HC_OP_CREATE_CONNECTION, params, 13, bConnectStatusCB, cn));
    }
}
/* \\\ */

/* /// "bStartNextConnect()" */
static void bStartNextConnect(struct BtHWCore *hc)
{
    struct MinNode *mn;
    if(hc->hc_Connecting) {
        return;
    }
    for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        if((cn->cn_State == HCNS_CONNECTING) && !cn->cn_Handle && (cn->cn_Role == BDR_NONE) && !cn->cn_WaitAdv) {
            cn->cn_Role = BDR_CENTRAL;
            if(!bStartConnect(cn)) {
                bConnDown(cn, BTIOERR_HOSTERROR, 0xff);
            }
            return;
        }
    }
}
/* \\\ */

/* /// "bEnsureConnection()" */
/* Returns the connection for a device, starting one when needed. *pending
   is set when the caller must queue its request on cn_WaitReqs. NULL when
   connecting is not possible/allowed (error in *error). */
static struct BtHWConn * bEnsureConnection(struct BtHWCore *hc, struct BtDevice *bd, UBYTE wantlink,
                                            UBYTE mode, BOOL *pending, LONG *error)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtHWConn *cn;
    UBYTE linktype;

    *pending = FALSE;

    /* choose the bearer: an explicit request wins, otherwise prefer BR/EDR */
    if((wantlink == BDLT_ACL) && (bd->bd_Flags & BDFF_CLASSIC) && (bth->bth_Flags & BTHF_CLASSIC)) {
        linktype = BDLT_ACL;
    } else if((wantlink == BDLT_LE) && (bd->bd_Flags & BDFF_LE) && (bth->bth_Flags & BTHF_LE)) {
        linktype = BDLT_LE;
    } else if(wantlink != BDLT_NONE) {
        *error = BTIOERR_NOTSUPPORTED;
        return(NULL);
    } else if((bd->bd_Flags & BDFF_CLASSIC) && (bth->bth_Flags & BTHF_CLASSIC)) {
        linktype = BDLT_ACL;
    } else if((bd->bd_Flags & BDFF_LE) && (bth->bth_Flags & BTHF_LE)) {
        linktype = BDLT_LE;
    } else {
        *error = BTIOERR_NOTSUPPORTED;
        return(NULL);
    }

    cn = BD_CONN(bd, linktype);
    if(cn) {
        if(cn->cn_State == HCNS_CONNECTED) {
            return(cn);
        }
        if(cn->cn_State == HCNS_CONNECTING) {
            if(cn->cn_WaitAdv && (mode == CONN_NOW)) {
                /* someone wants it now: stop waiting, initiate */
                cn->cn_WaitAdv = FALSE;
                bStopDiscovery(hc);
                bStartNextConnect(hc);
            }
            *pending = TRUE;
            return(cn);
        }
        *error = BTIOERR_NOTCONNECTED;
        return(NULL);
    }
    if(mode == CONN_NONE) {
        *error = BTIOERR_NOTCONNECTED;
        return(NULL);
    }
    if(bth->bth_State != BHS_READY) {
        *error = BTIOERR_NOTREADY;
        return(NULL);
    }
    cn = bAllocConn(hc, bd, linktype);
    if(!cn) {
        *error = BTIOERR_OUTOFMEMORY;
        return(NULL);
    }
    if((mode == CONN_AUTO) && (linktype == BDLT_LE) && (bd->bd_Flags & BDFF_BONDED) &&
       BluetoothBase->bt_GlobalCfg->bgc_AutoConnect) {
        /* A class re-opening its channels to a bonded LE peripheral (the
         * keyboard went to sleep and dropped the link): do not page it - an
         * LE Create Connection to a sleeping peer just times out and blocks
         * the radio for everybody else. Wait for it to advertise instead;
         * the background scan sees it and bConnAdvertising() connects. */
        cn->cn_WaitAdv = TRUE;
        *pending = TRUE;
        bBgScanUpdate(hc);
        return(cn);
    }
    /* The radio cannot reliably run an inquiry / LE scan and service a
     * connection's ATT/GATT traffic at the same time, so stop any discovery
     * in progress before we page or initiate the link. */
    bStopDiscovery(hc);
    *pending = TRUE;
    bStartNextConnect(hc);
    return(cn);
}
/* \\\ */

/* /// "bConnAdvertising()" */
/* A registered LE device was heard advertising (discovery or background
   scan). Connect to it when a class is waiting for it, or - if it is bonded
   and its policy allows - simply because it is one of ours that woke up. */
void bConnAdvertising(struct BtHWCore *hc, struct BtDevice *bd)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtHWConn *cn = bd->bd_Conns[1];
    BOOL pending;
    LONG err;

    if(!(bth->bth_Flags & BTHF_LE) || (bth->bth_State != BHS_READY)) {
        return;
    }
    if(hc->hc_Connecting || (bth->bth_Flags & BTHF_DISCOVERING)) {
        /* one connect at a time, and never hijack a discovery the user
           started - the device will advertise again */
        return;
    }
    if(cn) {
        if((cn->cn_State == HCNS_CONNECTING) && cn->cn_WaitAdv) {
            cn->cn_WaitAdv = FALSE;
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                           "%s is awake - connecting.", bd->bd_Name);
            bStartNextConnect(hc);
        }
        return;
    }
    if(!(bd->bd_Flags & BDFF_BONDED) || !bd->bd_PoPoCfg.bpc_AutoConnect ||
       !BluetoothBase->bt_GlobalCfg->bgc_AutoConnect) {
        return;
    }
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "%s is awake - reconnecting.", bd->bd_Name);
    bEnsureConnection(hc, bd, BDLT_LE, CONN_NOW, &pending, &err);
}
/* \\\ */

/* /// "bDisconnect()" */
static void bDisconnect(struct BtHWConn *cn, UBYTE reason)
{
    struct BtHWCore *hc = cn->cn_Core;
    UBYTE params[3];
    if(cn->cn_State == HCNS_CONNECTED) {
        cn->cn_State = HCNS_DISCONNECTING;
        params[0] = cn->cn_Handle & 0xff;
        params[1] = cn->cn_Handle >> 8;
        params[2] = reason;
        bSubmitCmd(hc, HC_OP_DISCONNECT, params, 3, bIgnoreCompletion, hc);
    } else if(cn->cn_State == HCNS_CONNECTING) {
        if(cn->cn_WaitAdv) {
            /* nothing in flight at the controller: just give up waiting */
            bConnDown(cn, IOERR_ABORTED, reason);
        } else if(cn->cn_LinkType == BDLT_LE) {
            bSubmitCmd(hc, HC_OP_LE_CREATE_CONN_CANCEL, NULL, 0, bIgnoreCompletion, hc);
        } else {
            bSubmitCmd(hc, HC_OP_CREATE_CONN_CANCEL, cn->cn_Device->bd_Address.bd_Addr, 6, bIgnoreCompletion, hc);
        }
    }
}
/* \\\ */

/* *** service enumeration *** */

/* /// "bReadSDPUint()" */
static ULONG bReadSDPUint(const struct bt_sdp_element *el)
{
    return(el->type == BT_SDP_ELEM_UINT ? el->uint : 0);
}
/* \\\ */

/* /// "bParseSDPRecord()" */
/* Builds a BtService (+ endpoints) from one attribute list. */
static void bParseSDPRecord(struct BtHWConn *cn, ULONG handle, const UBYTE *attrs, ULONG len)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    struct bt_sdp_element_iter it;
    struct bt_sdp_element id, val;
    struct BtService *bsv;
    UWORD psms[8];
    UWORD numpsms = 0;
    UWORD rfcomm = 0;
    UWORD classids[8];
    UWORD numclassids = 0;
    UWORD version = 0;
    UBYTE uuid128[16];
    BOOL have128 = FALSE;
    STRPTR name = NULL;
    const UBYTE *hiddesc = NULL;
    UWORD hiddesclen = 0;
    UWORD n;

    bt_sdp_element_iter_init(&it, attrs, len);
    while(bt_sdp_element_iter_next(&it, &id) == BT_OK) {
        if(bt_sdp_element_iter_next(&it, &val) != BT_OK) {
            break;
        }
        if(id.type != BT_SDP_ELEM_UINT) {
            continue;
        }
        switch(id.uint) {
        case 0x0001: /* ServiceClassIDList */
            if(val.type == BT_SDP_ELEM_SEQUENCE) {
                struct bt_sdp_element_iter sit;
                struct bt_sdp_element cls;
                bt_sdp_element_iter_init(&sit, val.seq_data, val.seq_len);
                while(bt_sdp_element_iter_next(&sit, &cls) == BT_OK) {
                    if(cls.type == BT_SDP_ELEM_UUID16) {
                        if(numclassids < 8) {
                            classids[numclassids++] = cls.uuid16;
                        }
                    } else if((cls.type == BT_SDP_ELEM_UUID128) && !have128 && !numclassids) {
                        CopyMem(cls.uuid128, uuid128, 16);
                        have128 = TRUE;
                    }
                }
            }
            break;
        case 0x0004: /* ProtocolDescriptorList */
        case 0x000d: /* AdditionalProtocolDescriptorLists */
            if(val.type == BT_SDP_ELEM_SEQUENCE) {
                /* 0x0004: sequence of protocol descriptors; 0x000d: sequence of such lists */
                struct bt_sdp_element_iter lit;
                struct bt_sdp_element lst;
                bt_sdp_element_iter_init(&lit, val.seq_data, val.seq_len);
                while(bt_sdp_element_iter_next(&lit, &lst) == BT_OK) {
                    struct bt_sdp_element_iter pit;
                    struct bt_sdp_element pd;
                    const UBYTE *ldata;
                    ULONG llen;
                    if(lst.type != BT_SDP_ELEM_SEQUENCE) {
                        continue;
                    }
                    if(id.uint == 0x000d) {
                        ldata = lst.seq_data;
                        llen = lst.seq_len;
                    } else {
                        /* for 0x0004 lst is already a protocol descriptor */
                        ldata = val.seq_data;
                        llen = val.seq_len;
                    }
                    bt_sdp_element_iter_init(&pit, ldata, llen);
                    while(bt_sdp_element_iter_next(&pit, &pd) == BT_OK) {
                        struct bt_sdp_element_iter eit;
                        struct bt_sdp_element proto, param;
                        if(pd.type != BT_SDP_ELEM_SEQUENCE) {
                            continue;
                        }
                        bt_sdp_element_iter_init(&eit, pd.seq_data, pd.seq_len);
                        if(bt_sdp_element_iter_next(&eit, &proto) != BT_OK) {
                            continue;
                        }
                        if(proto.type != BT_SDP_ELEM_UUID16) {
                            continue;
                        }
                        if(proto.uuid16 == 0x0100) { /* L2CAP */
                            if((bt_sdp_element_iter_next(&eit, &param) == BT_OK) && (param.type == BT_SDP_ELEM_UINT)) {
                                if(numpsms < 8) {
                                    for(n = 0; n < numpsms; n++) {
                                        if(psms[n] == param.uint) {
                                            break;
                                        }
                                    }
                                    if(n == numpsms) {
                                        psms[numpsms++] = param.uint;
                                    }
                                }
                            }
                        } else if(proto.uuid16 == 0x0003) { /* RFCOMM */
                            if((bt_sdp_element_iter_next(&eit, &param) == BT_OK) && (param.type == BT_SDP_ELEM_UINT)) {
                                rfcomm = param.uint;
                            }
                        }
                    }
                    if(id.uint == 0x0004) {
                        break; /* handled the whole list at once */
                    }
                }
            }
            break;
        case 0x0009: /* BluetoothProfileDescriptorList */
            if(val.type == BT_SDP_ELEM_SEQUENCE) {
                struct bt_sdp_element_iter pit;
                struct bt_sdp_element pd;
                bt_sdp_element_iter_init(&pit, val.seq_data, val.seq_len);
                if((bt_sdp_element_iter_next(&pit, &pd) == BT_OK) && (pd.type == BT_SDP_ELEM_SEQUENCE)) {
                    struct bt_sdp_element_iter eit;
                    struct bt_sdp_element u, v;
                    bt_sdp_element_iter_init(&eit, pd.seq_data, pd.seq_len);
                    if((bt_sdp_element_iter_next(&eit, &u) == BT_OK) && (bt_sdp_element_iter_next(&eit, &v) == BT_OK)) {
                        version = bReadSDPUint(&v);
                    }
                }
            }
            break;
        case 0x0100: /* ServiceName (language base 0x0100 + 0) */
            if((val.type == BT_SDP_ELEM_TEXT) && val.seq_len) {
                name = btAllocVec(val.seq_len + 1);
                if(name) {
                    CopyMem((APTR) val.seq_data, name, val.seq_len);
                    name[val.seq_len] = 0;
                    bStripString(BluetoothBase, name);
                }
            }
            break;
        case 0x0206: /* HIDDescriptorList: list of { descriptor type, descriptor value } */
            if(val.type == BT_SDP_ELEM_SEQUENCE) {
                struct bt_sdp_element_iter lit;
                struct bt_sdp_element rec;
                bt_sdp_element_iter_init(&lit, val.seq_data, val.seq_len);
                while(bt_sdp_element_iter_next(&lit, &rec) == BT_OK) {
                    struct bt_sdp_element_iter rit;
                    struct bt_sdp_element dtype, dval;
                    if(rec.type != BT_SDP_ELEM_SEQUENCE) {
                        continue;
                    }
                    bt_sdp_element_iter_init(&rit, rec.seq_data, rec.seq_len);
                    if((bt_sdp_element_iter_next(&rit, &dtype) == BT_OK) &&
                       (bt_sdp_element_iter_next(&rit, &dval) == BT_OK) &&
                       (dtype.type == BT_SDP_ELEM_UINT) && (dtype.uint == 0x22) &&
                       (dval.type == BT_SDP_ELEM_TEXT) && dval.seq_len) {
                        /* the Report (0x22) descriptor */
                        hiddesc = dval.seq_data;
                        hiddesclen = dval.seq_len;
                        break;
                    }
                }
            }
            break;
        }
    }

    if(!numclassids && !have128) {
        btFreeVec(name);
        return; /* not a service record we can describe */
    }

    btLockWriteDevice(bd);
    bsv = bAllocService(BluetoothBase, bd);
    if(bsv) {
        bsv->bsv_RecordHandle = handle;
        bsv->bsv_Version = version;
        if(numclassids) {
            bsv->bsv_UUID16 = classids[0];
            bUUID16To128(classids[0], bsv->bsv_UUID);
            bsv->bsv_ServiceClassIDs = btAllocVec((numclassids + 1) * sizeof(UWORD));
            if(bsv->bsv_ServiceClassIDs) {
                for(n = 0; n < numclassids; n++) {
                    bsv->bsv_ServiceClassIDs[n] = classids[n];
                }
                bsv->bsv_ServiceClassIDs[numclassids] = 0;
            }
        } else {
            CopyMem(uuid128, bsv->bsv_UUID, 16);
            bUUID128To16(uuid128, &bsv->bsv_UUID16);
        }
        if(name) {
            bsv->bsv_Name = name;
            name = NULL;
        } else {
            STRPTR nm = btNumToStr(BNTS_UUID16, bsv->bsv_UUID16, NULL);
            bsv->bsv_Name = btCopyStr(nm ? nm : (STRPTR) "Service");
        }
        bsv->bsv_Node.ln_Name = bsv->bsv_Name;
        if(hiddesc && hiddesclen) {
            bsv->bsv_HidDescriptor = btAllocVec(hiddesclen);
            if(bsv->bsv_HidDescriptor) {
                CopyMem((APTR) hiddesc, bsv->bsv_HidDescriptor, hiddesclen);
                bsv->bsv_HidDescriptorLen = hiddesclen;
            }
        }
        if(numpsms) {
            bsv->bsv_Protocol = BSVP_L2CAP;
            bsv->bsv_PSM = psms[0];
        } else if(rfcomm) {
            bsv->bsv_Protocol = BSVP_RFCOMM;
            bsv->bsv_RFCOMMChannel = rfcomm;
        }
        {
            UBYTE ustr[40];
            bUUIDToStr(bsv->bsv_UUID, (STRPTR) ustr);
            bsv->bsv_IDString = btCopyStrFmt("%s-%08lx", ustr, handle);
        }
        for(n = 0; n < numpsms; n++) {
            struct BtEndpoint *bep = bAllocEndpoint(BluetoothBase, bsv);
            if(bep) {
                bep->bep_Type = BEPT_L2CAP;
                bep->bep_CanRead = TRUE;
                bep->bep_CanWrite = TRUE;
                bep->bep_PSM = psms[n];
                bep->bep_MaxPktSize = BT_L2CAP_DEFAULT_MTU;
                bep->bep_Name = btCopyStrFmt("L2CAP PSM 0x%04lx", psms[n]);
                bep->bep_Node.ln_Name = bep->bep_Name;
            }
        }
        if(rfcomm) {
            struct BtEndpoint *bep = bAllocEndpoint(BluetoothBase, bsv);
            if(bep) {
                bep->bep_Type = BEPT_RFCOMM;
                bep->bep_CanRead = TRUE;
                bep->bep_CanWrite = TRUE;
                bep->bep_RFCOMMChannel = rfcomm;
                bep->bep_Name = btCopyStrFmt("RFCOMM channel %ld", rfcomm);
                bep->bep_Node.ln_Name = bep->bep_Name;
            }
        }
    }
    btUnlockDevice(bd);
    btFreeVec(name);
}
/* \\\ */

/* /// "bClearServices()" */
static void bClearServices(struct BtHWConn *cn)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    struct BtService *bsv;
    struct MinNode *mn, *next;

    /* only touch services (and endpoints) on THIS connection's bearer, so a
       dual-mode device keeps the other bearer's services when one re-enumerates:
       GATT/ATT services are LE, everything else BR/EDR. */
    BOOL isle = (cn->cn_LinkType == BDLT_LE);

    /* endpoint state of the old endpoints must go first */
    for(mn = cn->cn_Endpoints.mlh_Head; (next = mn->mln_Succ); mn = next) {
        struct BtHWEndpoint *hep = (struct BtHWEndpoint *) mn;
        if(hep->hep_State != HEPS_CLOSED) {
            bEndpointClose(hep);
        }
        bFreeHWEndpoint(hep, BTIOERR_CHANNELFAILED);
    }
    btLockWriteDevice(bd);
    for(bsv = (struct BtService *) bd->bd_Services.lh_Head; bsv->bsv_Node.ln_Succ; ) {
        struct BtService *nsv = (struct BtService *) bsv->bsv_Node.ln_Succ;
        BOOL svatt = (bsv->bsv_Protocol == BSVP_ATT);
        if((svatt == isle) && !bsv->bsv_SvcBinding && !bsv->bsv_BindingInProgress) {
            /* keep bound services (their binding owns channels) and services a
               class scan is currently binding (its subtask still holds the
               pointer); everything else on this bearer is refreshed */
            bFreeService(BluetoothBase, bsv);
            bd->bd_NumServices--;
        }
        bsv = nsv;
    }
    btUnlockDevice(bd);
}
/* \\\ */

/* /// "bSDPConnectCB()" */
static void bSDPConnectCB(bool success, void *user_data)
{
    struct BtHWConn *cn = user_data;
    cn->cn_SDPReady = success ? TRUE : FALSE;
    if(cn->cn_EnumState == ENUM_SDP_CONNECT) {
        if(!success) {
            bConnFinishEnum(cn, BTIOERR_CHANNELFAILED);
        } else {
            cn->cn_EnumState = ENUM_SDP_SEARCH;
            bConnRunEnum(cn);
        }
    } else if(cn->cn_CtrlReq && !success) {
        struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
        bReplyChannel(BluetoothBase, cn->cn_CtrlReq, BTIOERR_CHANNELFAILED, 0);
        cn->cn_CtrlReq = NULL;
    } else if(cn->cn_CtrlReq) {
        /* the control request waited for the SDP channel: re-dispatch */
        struct BtChannel *bch = cn->cn_CtrlReq;
        cn->cn_CtrlReq = NULL;
        bConnHandleRequest(cn->cn_Core, bch);
    }
}
/* \\\ */

/* /// "bSDPEnumCB()" */
static void bSDPEnumCB(struct bt_sdp_client_completion *completion, void *user_data)
{
    struct BtHWConn *cn = user_data;
    UWORD n;

    if(completion->result != BT_SDP_CLIENT_OK) {
        bConnFinishEnum(cn, BTIOERR_REMOTEERROR);
        return;
    }
    if(cn->cn_EnumState == ENUM_SDP_SEARCH) {
        cn->cn_EnumCount = 0;
        for(n = 0; (n * 4 + 3 < completion->data_len) && (cn->cn_EnumCount < 32); n++) {
            const uint8_t *h = &completion->data[n * 4];
            cn->cn_EnumHandles[cn->cn_EnumCount++] = ((ULONG) h[0] << 24) | (h[1] << 16) | (h[2] << 8) | h[3];
        }
        cn->cn_EnumIndex = 0;
        bClearServices(cn);
        cn->cn_EnumState = ENUM_SDP_ATTRS;
        bConnRunEnum(cn);
    } else if(cn->cn_EnumState == ENUM_SDP_ATTRS) {
        bParseSDPRecord(cn, cn->cn_EnumHandles[cn->cn_EnumIndex], completion->data, completion->data_len);
        cn->cn_EnumIndex++;
        bConnRunEnum(cn);
    }
}
/* \\\ */

/* /// "bGATTConnectCB()" */
static void bGATTConnectCB(bool success, void *user_data)
{
    struct BtHWConn *cn = user_data;
    cn->cn_GATTReady = success ? TRUE : FALSE;
    if(cn->cn_EnumState == ENUM_GATT_CONNECT) {
        if(!success) {
            bConnFinishEnum(cn, BTIOERR_CHANNELFAILED);
        } else {
            cn->cn_EnumState = ENUM_GATT_SERVICES;
            bConnRunEnum(cn);
        }
    } else if(cn->cn_CtrlReq) {
        struct BtChannel *bch = cn->cn_CtrlReq;
        cn->cn_CtrlReq = NULL;
        if(!success) {
            bReplyChannel(cn->cn_Core->hc_Base, bch, BTIOERR_CHANNELFAILED, 0);
        } else {
            bConnHandleRequest(cn->cn_Core, bch);
        }
    }
}
/* \\\ */

/* /// "bGATTEnumCB()" */
static void bGATTEnumCB(struct bt_gatt_client_completion *completion, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    struct bt_gatt_client *gc = &cn->cn_GATT;
    UWORD n;

    if(completion->result != BT_GATT_CLIENT_OK) {
        if((cn->cn_EnumState == ENUM_GATT_DESCS) || (cn->cn_EnumState == ENUM_GATT_REPORTREF)) {
            /* a characteristic without descriptors answers Attribute Not Found:
               not fatal, carry on with the next one */
            if(cn->cn_EnumEP) {
                cn->cn_EnumEP->bep_DescDone = TRUE;
            }
            cn->cn_EnumState = ENUM_GATT_DESCS;
            bConnRunEnum(cn);
            return;
        }
        if(cn->cn_EnumState == ENUM_GATT_CHARS) {
            /* a service whose characteristics need authentication (phones do
               this for some) is skipped, the rest of the device still enumerates */
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                           "%s: characteristics of service %ld are not readable (ATT 0x%02lx) - skipped.",
                           bd->bd_Name, (ULONG) cn->cn_EnumIndex, (ULONG) completion->att_error_code);
            cn->cn_EnumIndex++;
            bConnRunEnum(cn);
            return;
        }
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%s: GATT discovery failed (ATT 0x%02lx).",
                       bd->bd_Name, (ULONG) completion->att_error_code);
        bConnFinishEnum(cn, BTIOERR_REMOTEERROR);
        return;
    }
    (void) gc;
    switch(cn->cn_EnumState) {
    case ENUM_GATT_DESCS: {
        struct BtEndpoint *bep = cn->cn_EnumEP;
        if(bep) {
            bep->bep_DescDone = TRUE;
            for(n = 0; n < completion->count; n++) {
                if(completion->descriptors[n].uuid16 == 0x2902) {
                    bep->bep_CCCDHandle = completion->descriptors[n].handle;
                } else if(completion->descriptors[n].uuid16 == 0x2908) {
                    bep->bep_RefHandle = completion->descriptors[n].handle;
                }
            }
            if(bep->bep_RefHandle && (bep->bep_UUID16 == 0x2a4d)) {
                cn->cn_EnumState = ENUM_GATT_REPORTREF;   /* learn report id + type */
            }
        }
        bConnRunEnum(cn);
        break;
    }
    case ENUM_GATT_REPORTREF: {
        struct BtEndpoint *bep = cn->cn_EnumEP;
        if(bep && (completion->value_len >= 2)) {
            bep->bep_ReportID = completion->value[0];
            bep->bep_ReportType = completion->value[1];
        }
        cn->cn_EnumState = ENUM_GATT_DESCS;
        bConnRunEnum(cn);
        break;
    }
    case ENUM_GATT_SERVICES:
        bClearServices(cn);
        cn->cn_EnumCount = completion->count;
        if(cn->cn_EnumCount > BT_GATT_CLIENT_MAX_SERVICES) {
            cn->cn_EnumCount = BT_GATT_CLIENT_MAX_SERVICES;
        }
        for(n = 0; n < cn->cn_EnumCount; n++) {
            cn->cn_Services[n] = completion->services[n];
        }
        btLockWriteDevice(bd);
        for(n = 0; n < cn->cn_EnumCount; n++) {
            struct BtService *bsv = bAllocService(BluetoothBase, bd);
            if(bsv) {
                UBYTE ustr[40];
                STRPTR nm;
                bsv->bsv_Protocol = BSVP_ATT;
                bsv->bsv_UUID16 = cn->cn_Services[n].uuid16;
                bUUID16To128(bsv->bsv_UUID16, bsv->bsv_UUID);
                bsv->bsv_StartHandle = cn->cn_Services[n].start_handle;
                bsv->bsv_EndHandle = cn->cn_Services[n].end_handle;
                bsv->bsv_IsPrimary = TRUE;
                nm = btNumToStr(BNTS_UUID16, bsv->bsv_UUID16, NULL);
                bsv->bsv_Name = btCopyStr(nm ? nm : (STRPTR) "GATT service");
                bsv->bsv_Node.ln_Name = bsv->bsv_Name;
                bUUIDToStr(bsv->bsv_UUID, (STRPTR) ustr);
                bsv->bsv_IDString = btCopyStrFmt("%s-%04lx", ustr, bsv->bsv_StartHandle);
            }
        }
        btUnlockDevice(bd);
        cn->cn_EnumIndex = 0;
        cn->cn_EnumState = ENUM_GATT_CHARS;
        bConnRunEnum(cn);
        break;

    case ENUM_GATT_CHARS: {
        /* characteristics of service cn_EnumIndex */
        struct BtService *bsv;
        UWORD idx = 0;
        btLockWriteDevice(bd);
        for(bsv = (struct BtService *) bd->bd_Services.lh_Head; bsv->bsv_Node.ln_Succ; bsv = (struct BtService *) bsv->bsv_Node.ln_Succ) {
            if(bsv->bsv_Protocol != BSVP_ATT) {
                continue;
            }
            if(idx == cn->cn_EnumIndex) {
                for(n = 0; n < completion->count; n++) {
                    struct BtEndpoint *bep = bAllocEndpoint(BluetoothBase, bsv);
                    if(bep) {
                        UBYTE props = completion->characteristics[n].properties;
                        STRPTR nm;
                        bep->bep_Type = BEPT_GATT_CHAR;
                        bep->bep_Handle = completion->characteristics[n].value_handle;
                        bep->bep_UUID16 = completion->characteristics[n].uuid16;
                        bUUID16To128(bep->bep_UUID16, bep->bep_UUID);
                        bep->bep_Properties = props;
                        bep->bep_CanRead = (props & 0x30) ? TRUE : FALSE;  /* notify | indicate */
                        bep->bep_CanWrite = (props & 0x0c) ? TRUE : FALSE; /* write | write without response */
                        bep->bep_MaxPktSize = 20;
                        /* the characteristic's descriptors live between its value
                           handle and the next declaration (or the service end) */
                        bep->bep_EndHandle = (n + 1 < completion->count) ?
                                             (completion->characteristics[n + 1].declaration_handle - 1) : bsv->bsv_EndHandle;
                        nm = btNumToStr(BNTS_UUID16, bep->bep_UUID16, NULL);
                        bep->bep_Name = nm ? btCopyStr(nm) : btCopyStrFmt("Characteristic 0x%04lx", bep->bep_UUID16);
                        bep->bep_Node.ln_Name = bep->bep_Name;
                    }
                }
                break;
            }
            idx++;
        }
        btUnlockDevice(bd);
        cn->cn_EnumIndex++;
        bConnRunEnum(cn);
        break;
    }
    default:
        bConnRunEnum(cn);
        break;
    }
}
/* \\\ */

/* /// "bConnRunEnum()" */
static void bConnRunEnum(struct BtHWConn *cn)
{
    struct BtHWCore *hc = cn->cn_Core;
    uint64_t now = bNowUS(hc);

    switch(cn->cn_EnumState) {
    case ENUM_SDP_CONNECT:
        if(cn->cn_SDPReady) {
            cn->cn_EnumState = ENUM_SDP_SEARCH;
            bConnRunEnum(cn);
        } else if(bt_sdp_client_connect(&cn->cn_SDP, bSDPConnectCB, cn, now) != BT_OK) {
            bConnFinishEnum(cn, BTIOERR_CHANNELFAILED);
        }
        break;
    case ENUM_SDP_SEARCH: {
        /* search pattern: sequence { UUID16 L2CAP (0x0100) } */
        static const uint8_t pattern[] = { 0x35, 0x03, 0x19, 0x01, 0x00 };
        if(bt_sdp_client_search(&cn->cn_SDP, pattern, sizeof(pattern), 32, bSDPEnumCB, cn, now) != BT_OK) {
            bConnFinishEnum(cn, BTIOERR_HOSTERROR);
        }
        break;
    }
    case ENUM_SDP_ATTRS:
        if(cn->cn_EnumIndex >= cn->cn_EnumCount) {
            bConnFinishEnum(cn, 0);
        } else {
            /* all attributes: sequence { UINT32 0x0000FFFF } */
            static const uint8_t attrids[] = { 0x35, 0x05, 0x0a, 0x00, 0x00, 0xff, 0xff };
            if(bt_sdp_client_get_attributes(&cn->cn_SDP, cn->cn_EnumHandles[cn->cn_EnumIndex], 1000,
                                            attrids, sizeof(attrids), bSDPEnumCB, cn, now) != BT_OK) {
                bConnFinishEnum(cn, BTIOERR_HOSTERROR);
            }
        }
        break;
    case ENUM_GATT_CONNECT:
        if(cn->cn_GATTReady) {
            cn->cn_EnumState = ENUM_GATT_SERVICES;
            bConnRunEnum(cn);
        } else if(bt_gatt_client_connect(&cn->cn_GATT, bGATTConnectCB, cn, now) != BT_OK) {
            bConnFinishEnum(cn, BTIOERR_CHANNELFAILED);
        }
        break;
    case ENUM_GATT_SERVICES:
        if(bt_gatt_client_discover_services(&cn->cn_GATT, bGATTEnumCB, cn, now) != BT_OK) {
            bConnFinishEnum(cn, BTIOERR_HOSTERROR);
        }
        break;
    case ENUM_GATT_CHARS:
        if(cn->cn_EnumIndex >= cn->cn_EnumCount) {
            cn->cn_EnumEP = NULL;
            cn->cn_EnumState = ENUM_GATT_DESCS;
            bConnRunEnum(cn);
        } else {
            struct bt_gatt_service *svc = &cn->cn_Services[cn->cn_EnumIndex];
            if(bt_gatt_client_discover_characteristics(&cn->cn_GATT, svc->start_handle, svc->end_handle,
                                                       bGATTEnumCB, cn, now) != BT_OK) {
                bConnFinishEnum(cn, BTIOERR_HOSTERROR);
            }
        }
        break;
    case ENUM_GATT_DESCS: {
        /* every characteristic that can notify (needs its CCCD) or is an HID
           Report (needs its Report Reference) gets its descriptors looked up */
        struct BtEndpoint *bep = bNextDescEndpoint(cn);
        cn->cn_EnumEP = bep;
        if(!bep) {
            bConnFinishEnum(cn, 0);
        } else if((bep->bep_Handle + 1 > bep->bep_EndHandle) ||
                  (bt_gatt_client_discover_descriptors(&cn->cn_GATT, bep->bep_Handle + 1, bep->bep_EndHandle,
                                                       bGATTEnumCB, cn, now) != BT_OK)) {
            bep->bep_DescDone = TRUE;
            bConnRunEnum(cn);
        }
        break;
    }
    case ENUM_GATT_REPORTREF:
        if(!cn->cn_EnumEP || (bt_gatt_client_read(&cn->cn_GATT, cn->cn_EnumEP->bep_RefHandle, bGATTEnumCB, cn, now) != BT_OK)) {
            cn->cn_EnumState = ENUM_GATT_DESCS;
            bConnRunEnum(cn);
        }
        break;
    default:
        break;
    }
}

/* next GATT characteristic endpoint whose descriptors have not been examined */
static struct BtEndpoint * bNextDescEndpoint(struct BtHWConn *cn)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    struct BtService *bsv;
    struct BtEndpoint *found = NULL;

    btLockReadDevice(bd);
    for(bsv = (struct BtService *) bd->bd_Services.lh_Head; !found && bsv->bsv_Node.ln_Succ; bsv = (struct BtService *) bsv->bsv_Node.ln_Succ) {
        struct BtEndpoint *bep;
        if(bsv->bsv_Protocol != BSVP_ATT) {
            continue;
        }
        for(bep = (struct BtEndpoint *) bsv->bsv_Endpoints.lh_Head; bep->bep_Node.ln_Succ; bep = (struct BtEndpoint *) bep->bep_Node.ln_Succ) {
            if((bep->bep_Type == BEPT_GATT_CHAR) && !bep->bep_DescDone &&
               ((bep->bep_Properties & 0x30) || (bep->bep_UUID16 == 0x2a4d))) {
                found = bep;
                break;
            }
        }
    }
    btUnlockDevice(bd);
    return(found);
}
/* \\\ */

/* /// "bConnFinishEnum()" */
static void bConnFinishEnum(struct BtHWConn *cn, LONG error)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;

    cn->cn_EnumState = ENUM_IDLE;
    if(!error) {
        cn->cn_Enumerated = TRUE;
        btLockWriteDevice(bd);
        bd->bd_Flags |= BDFF_SERVICESKNOWN;
        btUnlockDevice(bd);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: %ld service(s) found.", bd->bd_Name, bd->bd_NumServices);
        btSendEvent(BEHMB_SERVICESCHG, bd, NULL);
        /* if a registered device is connected and services are known,
           the event handler task runs the class scan */
    } else {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: service enumeration failed (%s).", bd->bd_Name,
                       btNumToStr(BNTS_IOERR, error, "unknown"));
    }
    if(cn->cn_EnumReq) {
        bReplyChannel(BluetoothBase, cn->cn_EnumReq, error, bd->bd_NumServices);
        cn->cn_EnumReq = NULL;
    }
    if((cn->cn_PairState == PAIR_WAITENUM) && (cn->cn_State == HCNS_CONNECTED)) {
        cn->cn_PairState = PAIR_IDLE;
        bStartPairing(cn);
    }
}
/* \\\ */

/* *** classic pairing (SSP / legacy) *** */

/* /// "bPairingDone()" */
static void bPairingDone(struct BtHWConn *cn, LONG error, ULONG status)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;

    if(cn->cn_PairState == PAIR_IDLE) {
        /* nothing in progress - but never leave a submitted pair request
         * unanswered, or the caller waits on it forever. */
        if(cn->cn_PairReq) {
            bReplyChannel(BluetoothBase, cn->cn_PairReq, error ? error : BTIOERR_NOTSUPPORTED, status);
            cn->cn_PairReq = NULL;
        }
        return;
    }
    cn->cn_PairState = PAIR_IDLE;
    btLockWriteDevice(bd);
    bd->bd_PairingRequest = BPRT_NONE;
    if(!error) {
        bd->bd_PairingState = BDPS_DONE;
        bd->bd_Flags |= BDFF_BONDED|BDFF_REGISTERED;
    } else {
        bd->bd_PairingState = BDPS_FAILED;
    }
    btUnlockDevice(bd);
    if(!error) {
        bStoreDevConfig(BluetoothBase, bd, TRUE);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Paired with %s.", bd->bd_Name);
        btSendEvent(BEHMB_DEVICEREGISTERED, bd, NULL);
    } else {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Pairing with %s failed (%s).",
                       bd->bd_Name, btNumToStr(BNTS_HCISTATUS, status, "unknown"));
    }
    btSendEvent(BEHMB_PAIRINGDONE, bd, (APTR) (IPTR) status);
    bShowPairingPopup(BluetoothBase, bd, BPRT_NONE, 0);   /* close a popup still showing for this device */
    if(cn->cn_PairReq) {
        bReplyChannel(BluetoothBase, cn->cn_PairReq, error, status);
        cn->cn_PairReq = NULL;
    }
}
/* \\\ */

/* /// "bStartPairing()" */
static void bStartPairing(struct BtHWConn *cn)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    UBYTE params[2];

    if(cn->cn_LinkType != BDLT_ACL) {
        /* LE: run SMP as the central over fixed CID 6 */
        ULONG want;
        if(cn->cn_EnumState != ENUM_IDLE) {
            /* let service discovery finish first; bConnFinishEnum() resumes us */
            cn->cn_PairState = PAIR_WAITENUM;
            btLockWriteDevice(bd);
            bd->bd_PairingState = BDPS_INPROGRESS;
            btUnlockDevice(bd);
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s: pairing waits for service discovery to finish.", bd->bd_Name);
            return;
        }
        cn->cn_PairState = PAIR_SMP;
        btLockWriteDevice(bd);
        bd->bd_PairingState = BDPS_INPROGRESS;
        btUnlockDevice(bd);
        if(cn->cn_Role != BDR_CENTRAL) {
            btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%s: LE pairing as the peripheral is not supported.", bd->bd_Name);
            bPairingDone(cn, BTIOERR_NOTSUPPORTED, 0x05);
            return;
        }
        if(!bSMPSetup(cn)) {
            bPairingDone(cn, BTIOERR_HOSTERROR, 0x05);
            return;
        }
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s: LE pairing started (%s).", bd->bd_Name,
                       (cn->cn_SMP.config.features.auth_req & BT_SMP_AUTHREQ_SC) ? (STRPTR) "secure connections offered" : (STRPTR) "legacy");
        /* fresh controller entropy for the pairing randoms, then go */
        want = (hc->hc_RandAvail >= 32) ? 0 : bConnRequestEntropy(hc, 4);
        if(want) {
            cn->cn_SMPRandWait = (UBYTE) want;
        } else {
            bSMPStart(cn);
        }
        return;
    }
    cn->cn_PairState = PAIR_AUTH;
    btLockWriteDevice(bd);
    bd->bd_PairingState = BDPS_INPROGRESS;
    btUnlockDevice(bd);
    params[0] = cn->cn_Handle & 0xff;
    params[1] = cn->cn_Handle >> 8;
    if(!bSubmitCmd(hc, HC_OP_AUTH_REQUESTED, params, 2, bIgnoreCompletion, hc)) {
        bPairingDone(cn, BTIOERR_HOSTERROR, 0);
    }
}
/* \\\ */

/* /// "bAskUser()" */
static void bAskUser(struct BtHWConn *cn, UBYTE type, ULONG passkey)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    if(cn->cn_PairState == PAIR_IDLE) {
        /* remote initiated */
        cn->cn_PairState = PAIR_AUTH;
    }
    cn->cn_PairState = PAIR_WAITUSER;
    btLockWriteDevice(bd);
    bd->bd_PairingState = BDPS_WAITUSER;
    bd->bd_PairingRequest = type;
    bd->bd_PairingPasskey = passkey;
    btUnlockDevice(bd);
    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s: pairing needs %s%s%s.", bd->bd_Name,
                   btNumToStr(BNTS_PAIRINGREQ, type, "input"),
                   ((type == BPRT_NUMERICCOMPARE) || (type == BPRT_PASSKEYDISPLAY)) ? " " : "",
                   ((type == BPRT_NUMERICCOMPARE) || (type == BPRT_PASSKEYDISPLAY)) ? (STRPTR) "(see BDA_PairingPasskey)" : (STRPTR) "");
    btSendEvent(BEHMB_PAIRINGREQUEST, bd, (APTR) (IPTR) type);
    /* let the library show its own PoPo-style popup (gated by bgc_PopupPairing) */
    bShowPairingPopup(BluetoothBase, bd, type, passkey);
}
/* \\\ */

/* /// "bPairingReply()" */
static LONG bPairingReply(struct BtHWConn *cn, struct BtPairParams *bpp)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    UBYTE params[23];
    UBYTE type = bd->bd_PairingRequest;

    if(cn->cn_PairState != PAIR_WAITUSER) {
        return(BTIOERR_BADPARAMS);
    }
    if(cn->cn_LinkType == BDLT_LE) {
        uint64_t now = bNowUS(hc);
        cn->cn_PairState = PAIR_SMP;
        btLockWriteDevice(bd);
        bd->bd_PairingState = BDPS_INPROGRESS;
        bd->bd_PairingRequest = BPRT_NONE;
        btUnlockDevice(bd);
        if(!cn->cn_SMPActive) {
            return(0);                      /* pairing already finished */
        }
        switch(type) {
        case BPRT_PASSKEYDISPLAY:
            return(0);                      /* nothing to feed back; the peer types it */
        case BPRT_NUMERICCOMPARE:
        case BPRT_CONSENT:
            bt_smp_manager_confirm_numeric(&cn->cn_SMP, bpp->bpp_HaveConfirm ? (bpp->bpp_Confirm ? true : false) : true, now);
            return(0);
        case BPRT_PASSKEYENTRY:
            if(bpp->bpp_HavePasskey) {
                bt_smp_manager_provide_passkey(&cn->cn_SMP, bpp->bpp_Passkey, now);
            } else {
                bSMPFail(cn, 0x01, "no passkey entered");
            }
            return(0);
        default:
            return(BTIOERR_BADPARAMS);
        }
    }
    cn->cn_PairState = PAIR_AUTH;
    btLockWriteDevice(bd);
    bd->bd_PairingState = BDPS_INPROGRESS;
    bd->bd_PairingRequest = BPRT_NONE;
    btUnlockDevice(bd);
    CopyMem(bd->bd_Address.bd_Addr, params, 6);
    switch(type) {
    case BPRT_CONSENT:
    case BPRT_NUMERICCOMPARE:
        bSubmitCmd(hc, (bpp->bpp_HaveConfirm && !bpp->bpp_Confirm) ? HC_OP_USER_CONFIRM_NEG_REPLY : HC_OP_USER_CONFIRM_REPLY,
                   params, 6, bIgnoreCompletion, hc);
        return(0);
    case BPRT_PASSKEYENTRY:
        if(!bpp->bpp_HavePasskey) {
            bSubmitCmd(hc, HC_OP_USER_PASSKEY_NEG_REPLY, params, 6, bIgnoreCompletion, hc);
        } else {
            params[6] = bpp->bpp_Passkey;
            params[7] = bpp->bpp_Passkey >> 8;
            params[8] = bpp->bpp_Passkey >> 16;
            params[9] = bpp->bpp_Passkey >> 24;
            bSubmitCmd(hc, HC_OP_USER_PASSKEY_REPLY, params, 10, bIgnoreCompletion, hc);
        }
        return(0);
    case BPRT_PINCODE:
        if(!bpp->bpp_PINCode || !*bpp->bpp_PINCode) {
            bSubmitCmd(hc, HC_OP_PIN_CODE_REQ_NEG_REPLY, params, 6, bIgnoreCompletion, hc);
        } else {
            ULONG len = strlen(bpp->bpp_PINCode);
            if(len > 16) {
                len = 16;
            }
            memset(&params[7], 0, 16);
            params[6] = len;
            CopyMem(bpp->bpp_PINCode, &params[7], len);
            bSubmitCmd(hc, HC_OP_PIN_CODE_REQ_REPLY, params, 23, bIgnoreCompletion, hc);
        }
        return(0);
    default:
        return(BTIOERR_BADPARAMS);
    }
}
/* \\\ */


/* *** LE pairing: Security Manager Protocol *** */

/* The btcore SMP manager runs the pairing state machine; this glue supplies
   what it needs from the port: AES-128, random numbers, the L2CAP fixed
   channel (CID 6), the controller's crypto commands (LE Start Encryption,
   P-256 public key, DH key) and the user-interaction popups. We only pair as
   the central/initiator. Keys are kept in the device's BtKeyCfg (HCI byte
   order) so a bonded device is re-encrypted on every reconnect. */

#define SMP_IO_DISPLAYYESNO 0x01

static void bReverseBytes(UBYTE *dst, const UBYTE *src, ULONG len)
{
    ULONG i;
    for(i = 0; i < len; i++) {
        dst[i] = src[len - 1 - i];
    }
}

/* /// "bRandomBytes()" */
static ULONG bRand32(struct BtHWCore *hc)
{
    /* xorshift128 over the seeded state */
    ULONG t = hc->hc_RandState[3];
    ULONG s = hc->hc_RandState[0];
    hc->hc_RandState[3] = hc->hc_RandState[2];
    hc->hc_RandState[2] = hc->hc_RandState[1];
    hc->hc_RandState[1] = s;
    t ^= t << 11;
    t ^= t >> 8;
    hc->hc_RandState[0] = t ^ s ^ (s >> 19);
    return(hc->hc_RandState[0]);
}

static void bRandomBytes(struct BtHWCore *hc, UBYTE *out, ULONG len)
{
    ULONG i;
    for(i = 0; i < len; i++) {
        UBYTE b = (UBYTE) bRand32(hc);
        if(hc->hc_RandAvail) {
            b ^= hc->hc_RandPool[--hc->hc_RandAvail];   /* controller entropy */
        }
        out[i] = b;
    }
}

static void bRandFillCB(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    struct MinNode *mn, *next;

    if((completion->result == BT_CMDQ_RESULT_COMPLETE) && !completion->status &&
       completion->return_params && (completion->return_params_len >= 9)) {
        ULONG i;
        for(i = 0; i < 8; i++) {
            UBYTE b = completion->return_params[1 + i];
            if(hc->hc_RandAvail < sizeof(hc->hc_RandPool)) {
                hc->hc_RandPool[hc->hc_RandAvail++] = b;
            }
            hc->hc_RandState[i & 3] ^= ((ULONG) b) << ((i >> 2) * 8 + (i & 3) * 2);
        }
    }
    if(hc->hc_RandRequests) {
        hc->hc_RandRequests--;
    }
    /* pairings that waited for fresh entropy before sending Pairing Request */
    for(mn = hc->hc_Conns.mlh_Head; (next = mn->mln_Succ); mn = next) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        if(cn->cn_SMPActive && cn->cn_SMPRandWait) {
            if(--cn->cn_SMPRandWait == 0) {
                bSMPStart(cn);
            }
        }
    }
}

ULONG bConnRequestEntropy(struct BtHWCore *hc, ULONG count)
{
    ULONG n = 0;
    if(!(hc->hc_Hardware->bth_Flags & BTHF_LE)) {
        return(0);
    }
    while(count--) {
        if(!bSubmitCmd(hc, HC_OP_LE_RAND, NULL, 0, bRandFillCB, hc)) {
            break;
        }
        hc->hc_RandRequests++;
        n++;
    }
    return(n);
}
/* \\\ */

/* /// "SMP port callbacks" */
static bt_status_t bSMPAES(void *context, const uint8_t key[16], const uint8_t pt[16], uint8_t ct[16])
{
    (void) context;
    bAES128Encrypt(key, pt, ct);
    return(BT_OK);
}

static bt_status_t bSMPSend(void *context, const uint8_t *pdu, size_t len)
{
    struct BtHWConn *cn = context;
    if(!cn->cn_SMPChanOpen) {
        return(BT_ERR_NO_RESOURCES);
    }
    return(bt_l2cap_channel_manager_send(&cn->cn_L2CAP, BT_L2CAP_CID_SMP, pdu, len, bNowUS(cn->cn_Core)));
}

static bt_status_t bSMPRandom(void *context, uint8_t *out, size_t len)
{
    struct BtHWConn *cn = context;
    bRandomBytes(cn->cn_Core, out, len);
    return(BT_OK);
}

/* LE Start Encryption: handle(2) rand(8) ediv(2) ltk(16), all HCI byte order */
static BOOL bLEStartEncryption(struct BtHWConn *cn, const UBYTE *ltk, const UBYTE *rand, UWORD ediv)
{
    UBYTE p[28];
    p[0] = cn->cn_Handle & 0xff;
    p[1] = cn->cn_Handle >> 8;
    CopyMem((APTR) rand, &p[2], 8);
    p[10] = ediv & 0xff;
    p[11] = ediv >> 8;
    CopyMem((APTR) ltk, &p[12], 16);
    return(bSubmitCmd(cn->cn_Core, HC_OP_LE_START_ENCRYPTION, p, 28, bIgnoreCompletion, cn->cn_Core));
}

static bt_status_t bSMPStartEncryption(void *context, const uint8_t stk[16], uint8_t key_size)
{
    struct BtHWConn *cn = context;
    static const UBYTE zero8[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    /* the manager keeps keys most-significant octet first; HCI wants them LSB first */
    bReverseBytes(cn->cn_SMPLTK, stk, 16);
    cn->cn_SMPKeySize = key_size;
    return(bLEStartEncryption(cn, cn->cn_SMPLTK, zero8, 0) ? BT_OK : BT_ERR_NO_RESOURCES);
}

static bt_status_t bSMPGenPublicKey(void *context)
{
    struct BtHWConn *cn = context;
    return(bSubmitCmd(cn->cn_Core, HC_OP_LE_READ_LOCAL_P256, NULL, 0, bIgnoreCompletion, cn->cn_Core) ? BT_OK : BT_ERR_NO_RESOURCES);
}

static bt_status_t bSMPGenDHKey(void *context, const uint8_t peer_x[32], const uint8_t peer_y[32])
{
    struct BtHWConn *cn = context;
    UBYTE p[64];
    bReverseBytes(&p[0], peer_x, 32);
    bReverseBytes(&p[32], peer_y, 32);
    return(bSubmitCmd(cn->cn_Core, HC_OP_LE_GENERATE_DHKEY, p, 64, bIgnoreCompletion, cn->cn_Core) ? BT_OK : BT_ERR_NO_RESOURCES);
}

static bt_status_t bSMPGetLocalKeys(void *context, uint8_t mask, struct bt_smp_distributed_keys *out)
{
    struct BtHWConn *cn = context;
    struct BtHWCore *hc = cn->cn_Core;
    memset(out, 0, sizeof(*out));
    if(mask & BT_SMP_KEYDIST_ENC_KEY) {
        UBYTE e[2];
        bRandomBytes(hc, out->ltk, 16);
        bRandomBytes(hc, out->rand, 8);
        bRandomBytes(hc, e, 2);
        out->ediv = e[0] | (e[1] << 8);
    }
    if(mask & BT_SMP_KEYDIST_ID_KEY) {
        /* we use no private addresses: identity is the public address, IRK all zero */
        out->identity_address_type = 0;
        CopyMem(hc->hc_Hardware->bth_Address.bd_Addr, out->identity_address, 6);
    }
    if(mask & BT_SMP_KEYDIST_SIGN_KEY) {
        bRandomBytes(hc, out->csrk, 16);
    }
    out->key_mask = mask;
    return(BT_OK);
}

static void bSMPKeysComplete(void *context, const struct bt_smp_distributed_keys *peer,
                             const struct bt_smp_distributed_keys *local)
{
    struct BtHWConn *cn = context;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    (void) local;
    btLockWriteDevice(bd);
    if(peer->key_mask & BT_SMP_KEYDIST_ENC_KEY) {
        /* distributed as on the wire = HCI byte order */
        CopyMem((APTR) peer->ltk, bd->bd_Keys.bkc_LTK, 16);
        CopyMem((APTR) peer->rand, bd->bd_Keys.bkc_Rand, 8);
        bd->bd_Keys.bkc_EDIV[0] = peer->ediv & 0xff;
        bd->bd_Keys.bkc_EDIV[1] = peer->ediv >> 8;
        bd->bd_Keys.bkc_Flags |= BKCF_LTK;
        bd->bd_Keys.bkc_Flags &= ~BKCF_SC;
    }
    if(peer->key_mask & BT_SMP_KEYDIST_ID_KEY) {
        static const UBYTE noaddr[6] = { 0, 0, 0, 0, 0, 0 };
        CopyMem((APTR) peer->irk, bd->bd_Keys.bkc_IRK, 16);
        bd->bd_Keys.bkc_Flags |= BKCF_IRK;
        /* the peer paired from a private address: file it under the
           identity address it just told us, so the bond is found again
           after the private address has rotated */
        if(memcmp(peer->identity_address, noaddr, 6)) {
            bRekeyDevice(BluetoothBase, bd, peer->identity_address,
                         peer->identity_address_type ? BDAT_RANDOM : BDAT_PUBLIC);
        }
    }
    if(peer->key_mask & BT_SMP_KEYDIST_SIGN_KEY) {
        CopyMem((APTR) peer->csrk, bd->bd_Keys.bkc_CSRK, 16);
        bd->bd_Keys.bkc_Flags |= BKCF_CSRK;
    }
    btUnlockDevice(bd);
}

static void bSMPFail(struct BtHWConn *cn, UBYTE reason, CONST_STRPTR why)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    UBYTE pdu[2];
    pdu[0] = BT_SMP_PAIRING_FAILED;
    pdu[1] = reason;
    bSMPSend(cn, pdu, 2);
    cn->cn_SMPActive = FALSE;
    btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%s: LE pairing aborted - %s.", cn->cn_Device->bd_Name, why);
    bPairingDone(cn, BTIOERR_SECURITY, 0x05);
}

static void bSMPUserAction(void *context, enum bt_smp_user_action action, uint32_t passkey)
{
    struct BtHWConn *cn = context;
    switch(action) {
    case BT_SMP_USER_DISPLAY_PASSKEY:
        /* the peer (a keyboard) types this in */
        bAskUser(cn, BPRT_PASSKEYDISPLAY, passkey);
        break;
    case BT_SMP_USER_REQUEST_PASSKEY:
        bAskUser(cn, BPRT_PASSKEYENTRY, 0);
        break;
    case BT_SMP_USER_CONFIRM_NUMERIC:
        bAskUser(cn, BPRT_NUMERICCOMPARE, passkey);
        break;
    default:
        bSMPFail(cn, 0x05, "out-of-band pairing is not supported");
        break;
    }
}

static void bSMPComplete(void *context, enum bt_smp_manager_result result,
                         const struct bt_smp_pairing_negotiation *neg)
{
    struct BtHWConn *cn = context;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    static const char *resname[] = { "ok", "protocol error", "confirm value mismatch", "timeout",
                                     "crypto failure", "encryption failed", "unsupported by the device" };
    static const char *assocname[] = { "just works", "out of band", "numeric comparison",
                                       "passkey (we displayed)", "passkey (device displayed)", "passkey (both entered)" };

    cn->cn_SMPActive = FALSE;
    if(result == BT_SMP_MANAGER_OK) {
        btLockWriteDevice(bd);
        if(neg && neg->secure_connections) {
            /* LE Secure Connections: the LTK is the f5-derived key we encrypted with */
            CopyMem(cn->cn_SMPLTK, bd->bd_Keys.bkc_LTK, 16);
            memset(bd->bd_Keys.bkc_Rand, 0, 8);
            bd->bd_Keys.bkc_EDIV[0] = bd->bd_Keys.bkc_EDIV[1] = 0;
            bd->bd_Keys.bkc_Flags |= BKCF_LTK | BKCF_SC;
        }
        btUnlockDevice(bd);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                       "%s: LE pairing complete (%s, %s, %ld-byte key%s%s).", bd->bd_Name,
                       (neg && neg->secure_connections) ? (STRPTR) "secure connections" : (STRPTR) "legacy",
                       neg ? (STRPTR) assocname[neg->association] : (STRPTR) "?",
                       neg ? (ULONG) neg->encryption_key_size : 0UL,
                       (neg && neg->mitm_requested) ? (STRPTR) ", authenticated" : (STRPTR) "",
                       (bd->bd_Keys.bkc_Flags & BKCF_LTK) ? (STRPTR) ", key stored" : (STRPTR) "");
        bPairingDone(cn, 0, 0);
    } else {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%s: LE pairing failed - %s.",
                       bd->bd_Name, (result < 7) ? resname[result] : "unknown");
        bPairingDone(cn, BTIOERR_SECURITY, 0x05);
    }
}

static const struct bt_smp_manager_ops bSMPOps = {
    bSMPSend, bSMPRandom, bSMPStartEncryption, bSMPGenPublicKey, bSMPGenDHKey,
    bSMPGetLocalKeys, bSMPKeysComplete, bSMPUserAction, bSMPComplete
};
/* \\\ */

/* /// "bLEReencrypt()" */
/* Bonded device reconnected: encrypt straight away with the stored key. */
static void bLEReencrypt(struct BtHWConn *cn)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;
    UWORD ediv = bd->bd_Keys.bkc_EDIV[0] | (bd->bd_Keys.bkc_EDIV[1] << 8);
    if(bLEStartEncryption(cn, bd->bd_Keys.bkc_LTK, bd->bd_Keys.bkc_Rand, ediv)) {
        cn->cn_EncryptPending = TRUE;
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s: encrypting the link with the stored LE key.", bd->bd_Name);
    }
}
/* \\\ */

/* /// "bSMPChannelEvent()" */
static void bSMPChannelEvent(struct bt_l2cap_channel_event_info *info, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtDevice *bd = cn->cn_Device;

    switch(info->event) {
    case BT_L2CAP_CHANNEL_EVENT_OPENED:
        cn->cn_SMPChanOpen = TRUE;
        break;
    case BT_L2CAP_CHANNEL_EVENT_CLOSED:
        cn->cn_SMPChanOpen = FALSE;
        break;
    case BT_L2CAP_CHANNEL_EVENT_DATA:
        if(!info->data_len) {
            break;
        }
        if(cn->cn_SMPActive) {
            if((info->data[0] == BT_SMP_PAIRING_FAILED) && (info->data_len >= 2)) {
                static const char *why[] = { "?", "passkey entry failed", "OOB data not available",
                    "authentication requirements not met", "confirm value failed", "pairing not supported",
                    "encryption key size", "command not supported", "unspecified reason", "repeated attempts",
                    "invalid parameters", "DHKey check failed", "numeric comparison failed",
                    "BR/EDR pairing in progress", "cross-transport key not allowed" };
                UBYTE r = info->data[1];
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "%s refused the pairing: %s (0x%02lx).",
                               bd->bd_Name, (r < 15) ? why[r] : "unknown", (ULONG) r);
            }
            bt_smp_manager_on_pdu(&cn->cn_SMP, info->data, info->data_len, info->now_us);
        } else if(info->data[0] == BT_SMP_SECURITY_REQUEST) {
            /* the peripheral wants a secure link */
            if((bd->bd_Keys.bkc_Flags & BKCF_LTK) && !cn->cn_Encrypted && !cn->cn_EncryptPending) {
                bLEReencrypt(cn);
            } else if((cn->cn_PairState == PAIR_IDLE) && !(bd->bd_Flags & BDFF_BONDED)) {
                btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s requests security - pairing.", bd->bd_Name);
                bStartPairing(cn);
            }
        } else if(info->data[0] == BT_SMP_PAIRING_REQUEST) {
            /* the peer wants to pair us as the peripheral: not supported */
            UBYTE pdu[2];
            pdu[0] = BT_SMP_PAIRING_FAILED;
            pdu[1] = 0x05;
            bSMPSend(cn, pdu, 2);
        }
        break;
    }
}
/* \\\ */

/* /// "bSMPSetup()" */
static BOOL bSMPSetup(struct BtHWConn *cn)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtDevice *bd = cn->cn_Device;
    struct bt_smp_manager_config cfg;
    struct bt_smp_aes128 aes;
    struct bt_smp_aes_cmac cmac;

    memset(&cfg, 0, sizeof(cfg));
    cfg.features.io_capability = SMP_IO_DISPLAYYESNO;  /* a keyboard peer -> we display the passkey */
    cfg.features.oob_data_flag = 0;
    cfg.features.auth_req = BT_SMP_AUTHREQ_BONDING | BT_SMP_AUTHREQ_MITM |
                            ((bth->bth_LEFeatures[0] & 0x40) ? BT_SMP_AUTHREQ_SC : 0);
    cfg.features.max_encryption_key_size = 16;
    cfg.features.initiator_key_distribution = 0;
    cfg.features.responder_key_distribution = BT_SMP_KEYDIST_ENC_KEY | BT_SMP_KEYDIST_ID_KEY;
    /* the manager wants addresses most-significant octet first */
    cfg.initiator_address_type = 0;
    bReverseBytes(cfg.initiator_address, bth->bth_Address.bd_Addr, 6);
    cfg.responder_address_type = bd->bd_AddrType & 1;
    bReverseBytes(cfg.responder_address, bd->bd_Address.bd_Addr, 6);

    aes.encrypt = bSMPAES;
    aes.context = hc;
    bt_smp_manager_init(&cn->cn_SMP, &cfg, &aes, &bSMPOps, cn);
    bt_smp_cmac_aes128_init(&cn->cn_SMPCmac, &aes);
    cmac.calculate = bt_smp_cmac_aes128_calculate;
    cmac.context = &cn->cn_SMPCmac;
    bt_smp_manager_set_cmac(&cn->cn_SMP, &cmac);

    if(!cn->cn_SMPChanOpen) {
        if(bt_l2cap_channel_manager_open_fixed(&cn->cn_L2CAP, BT_L2CAP_CID_SMP, bSMPChannelEvent, cn) != BT_OK) {
            return(FALSE);
        }
    }
    cn->cn_SMPActive = TRUE;
    cn->cn_SMPRandWait = 0;
    return(TRUE);
}

static void bSMPStart(struct BtHWConn *cn)
{
    if(bt_smp_manager_start(&cn->cn_SMP, bNowUS(cn->cn_Core)) != BT_OK) {
        cn->cn_SMPActive = FALSE;
        bPairingDone(cn, BTIOERR_HOSTERROR, 0x05);
    }
}
/* \\\ */

/* *** HCI events *** */

/* /// "bConnHandleEvent()" */
BOOL bConnHandleEvent(struct BtHWCore *hc, UBYTE code, const UBYTE *params, ULONG len)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtHWConn *cn;
    struct BtDevice *bd;

    switch(code) {
    case HC_EVT_CONN_COMPLETE: {
        UWORD handle;
        UBYTE status;
        if(len < 11) {
            return(TRUE);
        }
        status = params[0];
        handle = (params[1] | (params[2] << 8)) & 0x0fff;
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, &params[3]);
        btUnlockBase();
        if(params[9] != LINKTYPE_ACL) {
            return(TRUE); /* SCO: not handled */
        }
        if(!bd) {
            KPRINTF(10, ("connection complete for unknown device\n"));
            return(TRUE);
        }
        cn = bd->bd_Conns[0];
        if(!cn) {
            if(status) {
                return(TRUE);
            }
            /* connection we did not ask for (accepted incoming) */
            cn = bAllocConn(hc, bd, BDLT_ACL);
            if(!cn) {
                return(TRUE);
            }
            cn->cn_Role = BDR_PERIPHERAL;
        }
        if(status) {
            bConnDown(cn, BTIOERR_CONNFAILED, status);
            bStartNextConnect(hc);
        } else {
            bConnUp(cn, handle, cn->cn_Role ? cn->cn_Role : BDR_CENTRAL);
            if(params[10]) {
                cn->cn_Encrypted = TRUE;
            }
            bStartNextConnect(hc);
        }
        return(TRUE);
    }

    case HC_EVT_CONN_REQUEST: {
        UBYTE reply[7];
        BOOL accept = FALSE;
        if(len < 10) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(params[9] != LINKTYPE_ACL) {
            /* SCO/eSCO: reject */
            CopyMem((APTR) params, reply, 6);
            reply[6] = 0x0f;
            bSubmitCmd(hc, HC_OP_REJECT_CONN_REQ, reply, 7, bIgnoreCompletion, hc);
            return(TRUE);
        }
        if(bd) {
            if((bd->bd_Flags & BDFF_REGISTERED) || bd->bd_PoPoCfg.bpc_Trusted || (bth->bth_Flags & BTHF_DISCOVERABLE)) {
                accept = TRUE;
            }
        } else if(bth->bth_Flags & BTHF_DISCOVERABLE) {
            /* while discoverable, let unknown devices in (pairing from a phone) */
            ULONG cod = params[6] | (params[7] << 8) | (params[8] << 16);
            bd = btAllocDevice(bth);
            if(bd) {
                btLockWriteDevice(bd);
                CopyMem((APTR) params, bd->bd_Address.bd_Addr, 6);
                bAddrToStr(bd->bd_Address.bd_Addr, (STRPTR) bd->bd_AddrString);
                bd->bd_IDString = btCopyStrFmt("BT:%s", bd->bd_AddrString);
                bd->bd_ClassOfDevice = cod;
                bd->bd_Flags |= BDFF_CLASSIC;
                bApplyDevConfig(BluetoothBase, bd);
                if(!bd->bd_Name) {
                    bd->bd_Name = btCopyStr((STRPTR) bd->bd_AddrString);
                }
                bd->bd_Node.ln_Name = bd->bd_Name;
                btUnlockDevice(bd);
                btSendEvent(BEHMB_ADDDEVICE, bd, NULL);
                accept = TRUE;
            }
        }
        CopyMem((APTR) params, reply, 6);
        if(accept && bd && !bd->bd_Conns[0]) {
            cn = bAllocConn(hc, bd, BDLT_ACL);
            if(cn) {
                cn->cn_Role = BDR_PERIPHERAL;
                reply[6] = 0x01; /* remain peripheral */
                bSubmitCmd(hc, HC_OP_ACCEPT_CONN_REQ, reply, 7, bIgnoreCompletion, hc);
                return(TRUE);
            }
        }
        reply[6] = 0x0f; /* unacceptable BD_ADDR */
        bSubmitCmd(hc, HC_OP_REJECT_CONN_REQ, reply, 7, bIgnoreCompletion, hc);
        return(TRUE);
    }

    case HC_EVT_DISCONN_COMPLETE: {
        UWORD handle;
        if(len < 4) {
            return(TRUE);
        }
        handle = (params[1] | (params[2] << 8)) & 0x0fff;
        cn = bFindConnByHandle(hc, handle);
        if(cn) {
            bConnDown(cn, BTIOERR_DISCONNECTED, params[3]);
        }
        bStartNextConnect(hc);
        return(TRUE);
    }

    case HC_EVT_NUM_COMPLETED_PACKETS: {
        UWORD n, count;
        if(len < 1) {
            return(TRUE);
        }
        count = params[0];
        for(n = 0; (n < count) && (1 + n * 4 + 3 < len); n++) {
            const UBYTE *e = &params[1 + n * 4];
            UWORD handle = (e[0] | (e[1] << 8)) & 0x0fff;
            UWORD done = e[2] | (e[3] << 8);
            cn = bFindConnByHandle(hc, handle);
            if(cn) {
                cn->cn_Credits = (cn->cn_Credits > done) ? cn->cn_Credits - done : 0;
                if((cn->cn_LinkType == BDLT_LE) && bth->bth_LEACLNumPkts) {
                    hc->hc_LEACLCredits += done;
                    if(hc->hc_LEACLCredits > bth->bth_LEACLNumPkts) {
                        hc->hc_LEACLCredits = bth->bth_LEACLNumPkts;
                    }
                } else {
                    hc->hc_ACLCredits += done;
                    if(hc->hc_ACLCredits > bth->bth_ACLNumPkts) {
                        hc->hc_ACLCredits = bth->bth_ACLNumPkts;
                    }
                }
            } else {
                hc->hc_ACLCredits += done;
                if(hc->hc_ACLCredits > bth->bth_ACLNumPkts) {
                    hc->hc_ACLCredits = bth->bth_ACLNumPkts;
                }
            }
        }
        bStartACLWrite(hc);
        return(TRUE);
    }

    case HC_EVT_ENCRYPTION_CHANGE:
    case HC_EVT_ENCRYPTION_KEY_REFRESH: {
        UWORD handle;
        BOOL enabled;
        if(len < ((code == HC_EVT_ENCRYPTION_CHANGE) ? 4 : 3)) {
            return(TRUE);
        }
        handle = (params[1] | (params[2] << 8)) & 0x0fff;
        enabled = (code == HC_EVT_ENCRYPTION_CHANGE) ? (!params[0] && params[3]) : !params[0];
        cn = bFindConnByHandle(hc, handle);
        if(cn) {
            BOOL was = cn->cn_Encrypted;
            cn->cn_Encrypted = enabled ? TRUE : FALSE;
            btLockWriteDevice(cn->cn_Device);
            if(cn->cn_Encrypted) {
                cn->cn_Device->bd_Flags |= BDFF_ENCRYPTED;
            } else {
                cn->cn_Device->bd_Flags &= ~BDFF_ENCRYPTED;
            }
            btUnlockDevice(cn->cn_Device);
            if(cn->cn_PairState == PAIR_ENCRYPT) {
                bPairingDone(cn, params[0] ? BTIOERR_SECURITY : 0, params[0]);
            } else if(cn->cn_SMPActive) {
                bt_smp_manager_on_encryption_changed(&cn->cn_SMP, cn->cn_Encrypted ? true : false, bNowUS(hc));
            } else if(cn->cn_EncryptPending) {
                cn->cn_EncryptPending = FALSE;
                if(cn->cn_Encrypted) {
                    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s: link encrypted with the stored key.", cn->cn_Device->bd_Name);
                } else {
                    btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "%s: the stored LE key was rejected (%s) - remove the device and pair it again.",
                                   cn->cn_Device->bd_Name, btNumToStr(BNTS_HCISTATUS, params[0], "unknown"));
                }
                /* the enumeration bConnUp() held back can run now */
                if(!cn->cn_Enumerated && (cn->cn_EnumState == ENUM_IDLE) && (cn->cn_State == HCNS_CONNECTED)) {
                    cn->cn_EnumState = ENUM_GATT_CONNECT;
                    bConnRunEnum(cn);
                }
            }
            if(cn->cn_Encrypted && !was && (cn->cn_LinkType == BDLT_LE) && !cn->cn_SMPActive) {
                /* protected characteristics (HID report maps...) are readable now: let the classes retry */
                btSendEvent(BEHMB_SERVICESCHG, cn->cn_Device, NULL);
            }
        }
        return(TRUE);
    }

    case HC_EVT_AUTH_COMPLETE: {
        UWORD handle;
        if(len < 3) {
            return(TRUE);
        }
        handle = (params[1] | (params[2] << 8)) & 0x0fff;
        cn = bFindConnByHandle(hc, handle);
        if(cn && (cn->cn_PairState != PAIR_IDLE)) {
            if(params[0]) {
                bPairingDone(cn, BTIOERR_SECURITY, params[0]);
            } else {
                /* authenticated: switch on encryption */
                UBYTE p[3];
                cn->cn_PairState = PAIR_ENCRYPT;
                p[0] = handle & 0xff;
                p[1] = handle >> 8;
                p[2] = 0x01;
                bSubmitCmd(hc, HC_OP_SET_CONN_ENCRYPTION, p, 3, bIgnoreCompletion, hc);
            }
        }
        return(TRUE);
    }

    case HC_EVT_LINK_KEY_REQUEST: {
        UBYTE reply[22];
        if(len < 6) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        CopyMem((APTR) params, reply, 6);
        if(bd && bd->bd_Keys.bkc_Flags & BKCF_LINKKEY) {
            CopyMem(bd->bd_Keys.bkc_LinkKey, &reply[6], 16);
            bSubmitCmd(hc, HC_OP_LINK_KEY_REQ_REPLY, reply, 22, bIgnoreCompletion, hc);
        } else {
            bSubmitCmd(hc, HC_OP_LINK_KEY_REQ_NEG_REPLY, reply, 6, bIgnoreCompletion, hc);
        }
        return(TRUE);
    }

    case HC_EVT_LINK_KEY_NOTIFICATION:
        if(len < 23) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(bd) {
            btLockWriteDevice(bd);
            CopyMem((APTR) &params[6], bd->bd_Keys.bkc_LinkKey, 16);
            bd->bd_Keys.bkc_LinkKeyType = params[22];
            bd->bd_Keys.bkc_Flags |= BKCF_LINKKEY;
            bd->bd_Flags |= BDFF_BONDED;
            btUnlockDevice(bd);
            if(bd->bd_Flags & BDFF_REGISTERED) {
                bStoreDevConfig(BluetoothBase, bd, TRUE);
            }
        }
        return(TRUE);

    case HC_EVT_PIN_CODE_REQUEST:
        if(len < 6) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(bd && bd->bd_Conns[0]) {
            bAskUser(bd->bd_Conns[0], BPRT_PINCODE, 0);
        } else {
            UBYTE reply[6];
            CopyMem((APTR) params, reply, 6);
            bSubmitCmd(hc, HC_OP_PIN_CODE_REQ_NEG_REPLY, reply, 6, bIgnoreCompletion, hc);
        }
        return(TRUE);

    case HC_EVT_IO_CAP_REQUEST: {
        UBYTE reply[9];
        if(len < 6) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        CopyMem((APTR) params, reply, 6);
        if(bd) {
            reply[6] = BPIO_DISPLAYYESNO;  /* IO capability */
            reply[7] = 0x00;               /* no OOB data */
            reply[8] = 0x02;               /* dedicated bonding, MITM not required */
            if(bd->bd_Conns[0] && (bd->bd_Conns[0]->cn_PairState == PAIR_IDLE)) {
                bd->bd_Conns[0]->cn_PairState = PAIR_AUTH; /* remote initiated */
                btLockWriteDevice(bd);
                bd->bd_PairingState = BDPS_INPROGRESS;
                btUnlockDevice(bd);
            }
            bSubmitCmd(hc, HC_OP_IO_CAP_REQ_REPLY, reply, 9, bIgnoreCompletion, hc);
        } else {
            reply[6] = 0x18; /* pairing not allowed */
            bSubmitCmd(hc, HC_OP_IO_CAP_REQ_NEG_REPLY, reply, 7, bIgnoreCompletion, hc);
        }
        return(TRUE);
    }

    case HC_EVT_IO_CAP_RESPONSE:
        return(TRUE);

    case HC_EVT_USER_CONFIRM_REQUEST: {
        ULONG passkey;
        if(len < 10) {
            return(TRUE);
        }
        passkey = params[6] | (params[7] << 8) | (params[8] << 16) | ((ULONG) params[9] << 24);
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(bd && bd->bd_Conns[0]) {
            if(BluetoothBase->bt_GlobalCfg->bgc_PopupPairing) {
                bAskUser(bd->bd_Conns[0], BPRT_NUMERICCOMPARE, passkey);
            } else {
                /* nobody to ask: accept (just works) */
                UBYTE reply[6];
                CopyMem((APTR) params, reply, 6);
                bSubmitCmd(hc, HC_OP_USER_CONFIRM_REPLY, reply, 6, bIgnoreCompletion, hc);
            }
        }
        return(TRUE);
    }

    case HC_EVT_USER_PASSKEY_REQUEST:
        if(len < 6) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(bd && bd->bd_Conns[0]) {
            bAskUser(bd->bd_Conns[0], BPRT_PASSKEYENTRY, 0);
        }
        return(TRUE);

    case HC_EVT_USER_PASSKEY_NOTIFY: {
        ULONG passkey;
        if(len < 10) {
            return(TRUE);
        }
        passkey = params[6] | (params[7] << 8) | (params[8] << 16) | ((ULONG) params[9] << 24);
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, params);
        btUnlockBase();
        if(bd && bd->bd_Conns[0]) {
            struct BtHWConn *pcn = bd->bd_Conns[0];
            btLockWriteDevice(bd);
            bd->bd_PairingRequest = BPRT_PASSKEYDISPLAY;
            bd->bd_PairingPasskey = passkey;
            btUnlockDevice(bd);
            if(pcn->cn_PairState == PAIR_IDLE) {
                pcn->cn_PairState = PAIR_AUTH;
            }
            btSendEvent(BEHMB_PAIRINGREQUEST, bd, (APTR) (IPTR) BPRT_PASSKEYDISPLAY);
        }
        return(TRUE);
    }

    case HC_EVT_SIMPLE_PAIRING_COMPLETE:
        if(len < 7) {
            return(TRUE);
        }
        btLockReadBase();
        bd = bFindDeviceByAddr(hc, &params[1]);
        btUnlockBase();
        if(bd && bd->bd_Conns[0] && params[0] && (bd->bd_Conns[0]->cn_PairState != PAIR_IDLE)) {
            bPairingDone(bd->bd_Conns[0], BTIOERR_SECURITY, params[0]);
        }
        return(TRUE);

    case HC_EVT_LE_META: {
        UBYTE sub;
        if(len < 1) {
            return(FALSE);
        }
        sub = params[0];
        if((sub == HC_LE_SUB_CONN_COMPLETE) || (sub == HC_LE_SUB_ENH_CONN_COMPLETE)) {
            UWORD handle;
            UBYTE status;
            if(len < 12) {
                return(TRUE);
            }
            status = params[1];
            handle = (params[2] | (params[3] << 8)) & 0x0fff;
            btLockReadBase();
            bd = bFindDeviceByAddr(hc, &params[6]);
            btUnlockBase();
            hc->hc_LEConnecting = NULL;
            if(!bd && status && hc->hc_Connecting && (hc->hc_Connecting->cn_LinkType == BDLT_LE) &&
               (hc->hc_Connecting->cn_State == HCNS_CONNECTING)) {
                /* our own initiate failed or was cancelled: the event carries
                   no usable peer address then (all zero) - it is the connect
                   in flight that failed */
                cn = hc->hc_Connecting;
                bConnDown(cn, BTIOERR_CONNFAILED, status);
                bStartNextConnect(hc);
                return(TRUE);
            }
            if(!bd) {
                /* the address may be unknown to us when the peer used a
                   private address; nothing to attach the link to */
                if(!status) {
                    UBYTE p[3];
                    p[0] = handle & 0xff;
                    p[1] = handle >> 8;
                    p[2] = 0x13;
                    bSubmitCmd(hc, HC_OP_DISCONNECT, p, 3, bIgnoreCompletion, hc);
                }
                return(TRUE);
            }
            cn = bd->bd_Conns[1];
            if(!cn) {
                if(status) {
                    return(TRUE);
                }
                cn = bAllocConn(hc, bd, BDLT_LE);
                if(!cn) {
                    return(TRUE);
                }
                cn->cn_Role = BDR_PERIPHERAL;
            }
            if(status) {
                bConnDown(cn, BTIOERR_CONNFAILED, status);
            } else {
                bConnUp(cn, handle, params[4] ? BDR_PERIPHERAL : BDR_CENTRAL);
            }
            bStartNextConnect(hc);
            return(TRUE);
        }
        if((sub == HC_LE_SUB_P256_COMPLETE) || (sub == HC_LE_SUB_DHKEY_COMPLETE)) {
            /* crypto results for the (single) pairing in progress */
            struct MinNode *mn;
            for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
                cn = (struct BtHWConn *) mn;
                if(!cn->cn_SMPActive) {
                    continue;
                }
                if(sub == HC_LE_SUB_P256_COMPLETE) {
                    UBYTE x[32], y[32];
                    BOOL ok = (len >= 66) && !params[1];
                    if(ok) {
                        bReverseBytes(x, &params[2], 32);
                        bReverseBytes(y, &params[34], 32);
                    }
                    bt_smp_manager_on_local_public_key(&cn->cn_SMP, ok ? true : false, x, y, bNowUS(hc));
                } else {
                    UBYTE k[32];
                    BOOL ok = (len >= 34) && !params[1];
                    if(ok) {
                        bReverseBytes(k, &params[2], 32);
                    }
                    bt_smp_manager_on_dhkey(&cn->cn_SMP, ok ? true : false, k, bNowUS(hc));
                }
                break;
            }
            return(TRUE);
        }
        if(sub == HC_LE_SUB_LTK_REQUEST) {
            /* we do not act as LE peripheral yet: negative reply */
            UBYTE p[2];
            if(len < 3) {
                return(TRUE);
            }
            p[0] = params[1];
            p[1] = params[2];
            bSubmitCmd(hc, HC_OP_LE_LTK_REQ_NEG_REPLY, p, 2, bIgnoreCompletion, hc);
            return(TRUE);
        }
        return(FALSE);
    }

    default:
        return(FALSE);
    }
}
/* \\\ */

/* *** ACL receive *** */

/* /// "bConnHandleACL()" */
void bConnHandleACL(struct BtHWCore *hc, const UBYTE *data, ULONG len)
{
    struct bt_buf_reader r;
    struct bt_hci_acl_header hdr;
    const uint8_t *payload;
    struct BtHWConn *cn;

    bt_buf_reader_init(&r, data, len);
    if(bt_hci_parse_acl_header(&r, &hdr) != BT_OK) {
        return;
    }
    payload = bt_buf_reader_peek(&r, hdr.data_len);
    if(!payload) {
        return;
    }
    cn = bFindConnByHandle(hc, hdr.handle);
    if(!cn || (cn->cn_State != HCNS_CONNECTED)) {
        KPRINTF(10, ("ACL data for unknown handle %04lx\n", hdr.handle));
        return;
    }
    cn->cn_LastActivity = hc->hc_Tick;
    bt_l2cap_channel_manager_on_acl(&cn->cn_L2CAP, hdr.pb_flag, payload, hdr.data_len, bNowUS(hc));
}
/* \\\ */

/* *** requests *** */

/* /// "bSDPCtrlCB()" */
static void bSDPCtrlCB(struct bt_sdp_client_completion *completion, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtChannel *bch = cn->cn_CtrlReq;
    if(!bch) {
        return;
    }
    cn->cn_CtrlReq = NULL;
    if(completion->result != BT_SDP_CLIENT_OK) {
        bReplyChannel(BluetoothBase, bch, BTIOERR_REMOTEERROR, completion->result);
    } else {
        ULONG n = completion->data_len;
        LONG err = 0;
        if(n > bch->bch_Length) {
            n = bch->bch_Length;
            err = BTIOERR_OVERFLOW;
        }
        if(n) {
            CopyMem((APTR) completion->data, bch->bch_Data, n);
        }
        bReplyChannel(BluetoothBase, bch, err, n);
    }
}
/* \\\ */

/* /// "bConnHandleRequest()" */
/* Handles connection related control requests and endpoint channel
   requests. Returns FALSE if the request is not one of ours. */
BOOL bConnHandleRequest(struct BtHWCore *hc, struct BtChannel *bch)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtDevice *bd = bch->bch_Device;
    struct BtHWConn *cn;
    BOOL pending;
    LONG err;
    uint64_t now = bNowUS(hc);

    if(!bd) {
        return(FALSE);
    }

    if(!bch->bch_Endpoint) {
        switch(bch->bch_Request) {
        case BTPRI_CONNECT:
            cn = bEnsureConnection(hc, bd, BDLT_NONE, CONN_NOW, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
            } else if(pending) {
                bReqQueueAdd(&cn->cn_WaitReqs, bch);
            } else {
                bReplyChannel(BluetoothBase, bch, 0, 0);
            }
            /* dual-mode device: also bring up the OTHER bearer (fire and forget)
               so both bearers' services get enumerated and listed. */
            if(cn && (bd->bd_Flags & BDFF_CLASSIC) && (bd->bd_Flags & BDFF_LE)) {
                UBYTE other = (cn->cn_LinkType == BDLT_LE) ? BDLT_ACL : BDLT_LE;
                BOOL p2;
                LONG e2;
                bEnsureConnection(hc, bd, other, CONN_NOW, &p2, &e2);
            }
            return(TRUE);

        case BTPRI_DISCONNECT:
            /* drop every bearer of a (possibly dual-mode) device */
            if(bd->bd_Conns[0]) {
                bDisconnect(bd->bd_Conns[0], 0x13);
            }
            if(bd->bd_Conns[1]) {
                bDisconnect(bd->bd_Conns[1], 0x13);
            }
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return(TRUE);

        case BTPRI_ENUMSERVICES:
            cn = bEnsureConnection(hc, bd, BDLT_NONE, CONN_NOW, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
            } else if(pending) {
                bReqQueueAdd(&cn->cn_WaitReqs, bch);
            } else if(cn->cn_EnumState != ENUM_IDLE) {
                if(cn->cn_EnumReq) {
                    bReplyChannel(BluetoothBase, bch, IOERR_UNITBUSY, 0);
                } else {
                    cn->cn_EnumReq = bch;
                }
            } else {
                btLockWriteDevice(bd);
                bd->bd_Flags &= ~BDFF_SERVICESKNOWN;
                btUnlockDevice(bd);
                cn->cn_EnumReq = bch;
                cn->cn_EnumState = (cn->cn_LinkType == BDLT_LE) ? ENUM_GATT_CONNECT : ENUM_SDP_CONNECT;
                bConnRunEnum(cn);
            }
            return(TRUE);

        case BTPRI_PAIR:
            cn = bEnsureConnection(hc, bd, BDLT_NONE, CONN_NOW, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
            } else if(cn->cn_PairState != PAIR_IDLE) {
                bReplyChannel(BluetoothBase, bch, IOERR_UNITBUSY, 0);
            } else {
                cn->cn_PairReq = bch;
                if(pending) {
                    cn->cn_PairState = PAIR_CONNECTING;
                } else {
                    bStartPairing(cn);
                }
            }
            return(TRUE);

        case BTPRI_PAIRREPLY:
            cn = (bd->bd_Conns[0] && (bd->bd_Conns[0]->cn_PairState == PAIR_WAITUSER)) ? bd->bd_Conns[0] : bd->bd_Conns[1];
            if(!cn || !bch->bch_Data || (bch->bch_Length < sizeof(struct BtPairParams))) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
            } else {
                bReplyChannel(BluetoothBase, bch, bPairingReply(cn, (struct BtPairParams *) bch->bch_Data), 0);
            }
            return(TRUE);

        case BTPRI_UNPAIR:
            btLockWriteDevice(bd);
            memset(&bd->bd_Keys, 0, sizeof(bd->bd_Keys));
            bd->bd_Flags &= ~BDFF_BONDED;
            bd->bd_CurAddrValid = FALSE;
            btUnlockDevice(bd);
            bStoreDevConfig(BluetoothBase, bd, TRUE);
            if(bd->bd_Conns[0]) {
                bDisconnect(bd->bd_Conns[0], 0x13);
            }
            if(bd->bd_Conns[1]) {
                bDisconnect(bd->bd_Conns[1], 0x13);
            }
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Keys of %s removed.", bd->bd_Name);
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return(TRUE);

        case BTPR_READRSSI:
            /* answered from the last inquiry / advertising report */
            if(bch->bch_Data && bch->bch_Length) {
                *((BYTE *) bch->bch_Data) = (BYTE) bd->bd_RSSI;
                bReplyChannel(BluetoothBase, bch, 0, 1);
            } else {
                bReplyChannel(BluetoothBase, bch, BTIOERR_BADPARAMS, 0);
            }
            return(TRUE);

        case BTPR_SDPSEARCH:
        case BTPR_SDPATTRIBUTES:
            cn = bEnsureConnection(hc, bd, BDLT_ACL, (bch->bch_Flags & BCHF_AUTOCONNECT) ? CONN_AUTO : CONN_NONE, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
                return(TRUE);
            }
            if(pending) {
                bReqQueueAdd(&cn->cn_WaitReqs, bch);
                return(TRUE);
            }
            if(cn->cn_LinkType != BDLT_ACL) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return(TRUE);
            }
            if(cn->cn_CtrlReq || (cn->cn_EnumState != ENUM_IDLE)) {
                bReplyChannel(BluetoothBase, bch, IOERR_UNITBUSY, 0);
                return(TRUE);
            }
            if(!cn->cn_SDPReady) {
                cn->cn_CtrlReq = bch;
                if(bt_sdp_client_connect(&cn->cn_SDP, bSDPConnectCB, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_CHANNELFAILED, 0);
                }
                return(TRUE);
            }
            cn->cn_CtrlReq = bch;
            if(bch->bch_Request == BTPR_SDPSEARCH) {
                uint8_t pattern[5];
                pattern[0] = 0x35; pattern[1] = 0x03; pattern[2] = 0x19;
                pattern[3] = bch->bch_Value >> 8; pattern[4] = bch->bch_Value & 0xff;
                if(bt_sdp_client_search(&cn->cn_SDP, pattern, 5, 32, bSDPCtrlCB, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
                }
            } else {
                static const uint8_t attrids[] = { 0x35, 0x05, 0x0a, 0x00, 0x00, 0xff, 0xff };
                ULONG handle = bch->bch_Value | ((ULONG) bch->bch_Index << 16);
                if(bt_sdp_client_get_attributes(&cn->cn_SDP, handle, 1000, attrids, sizeof(attrids), bSDPCtrlCB, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
                }
            }
            return(TRUE);

        case BTPR_GATTREAD:
        case BTPR_GATTWRITE:
        case BTPR_GATTWRITENORSP:
            cn = bEnsureConnection(hc, bd, BDLT_LE, (bch->bch_Flags & BCHF_AUTOCONNECT) ? CONN_AUTO : CONN_NONE, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
                return(TRUE);
            }
            if(pending) {
                bReqQueueAdd(&cn->cn_WaitReqs, bch);
                return(TRUE);
            }
            if(cn->cn_LinkType != BDLT_LE) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return(TRUE);
            }
            if(cn->cn_CtrlReq || (cn->cn_EnumState != ENUM_IDLE)) {
                bReplyChannel(BluetoothBase, bch, IOERR_UNITBUSY, 0);
                return(TRUE);
            }
            if(!cn->cn_GATTReady) {
                cn->cn_CtrlReq = bch;
                if(bt_gatt_client_connect(&cn->cn_GATT, bGATTConnectCB, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_CHANNELFAILED, 0);
                }
                return(TRUE);
            }
            cn->cn_CtrlReq = bch;
            if(bch->bch_Request == BTPR_GATTREAD) {
                if(bt_gatt_client_read(&cn->cn_GATT, bch->bch_Value, bGATTOpComplete, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
                }
            } else {
                if(bt_gatt_client_write(&cn->cn_GATT, bch->bch_Value, bch->bch_Data, bch->bch_Length,
                                        bGATTOpComplete, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
                }
            }
            return(TRUE);

        case BTPRI_OPENCHANNEL:
        case BTPRI_CLOSECHANNEL: {
            struct BtEndpoint *bep = (struct BtEndpoint *) bch->bch_Data;
            cn = bep ? BD_CONN(bd, (bep->bep_Type == BEPT_GATT_CHAR) ? BDLT_LE : BDLT_ACL) : NULL;
            if(cn && bep && (cn->cn_State == HCNS_CONNECTED)) {
                struct BtHWEndpoint *hep = bFindHWEndpoint(cn, bep);
                if(hep) {
                    if(bch->bch_Request == BTPRI_CLOSECHANNEL) {
                        if(hep->hep_UseCnt) {
                            hep->hep_UseCnt--;
                        }
                        if(!hep->hep_UseCnt) {
                            hep->hep_CloseTick = hc->hc_Tick + HC_CHANCLOSE_MS;
                        }
                    } else {
                        hep->hep_UseCnt++;
                        hep->hep_CloseTick = 0;
                    }
                }
            }
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return(TRUE);
        }

        default:
            return(FALSE);
        }
    }

    /* endpoint channels */
    {
        struct BtEndpoint *bep = bch->bch_Endpoint;
        struct BtHWEndpoint *hep;
        UWORD rq = bch->bch_Request & 0x80 ? BTPR_WRITE : BTPR_READ;

        if((rq == BTPR_READ) && !bep->bep_CanRead) {
            bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
            return(TRUE);
        }
        if((rq == BTPR_WRITE) && !bep->bep_CanWrite) {
            bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
            return(TRUE);
        }
        cn = bEnsureConnection(hc, bd, (bep->bep_Type == BEPT_GATT_CHAR) ? BDLT_LE : BDLT_ACL,
                               (bch->bch_Flags & BCHF_AUTOCONNECT) ? CONN_AUTO : CONN_NONE, &pending, &err);
        if(!cn) {
            bReplyChannel(BluetoothBase, bch, err, 0);
            return(TRUE);
        }
        if(pending) {
            bReqQueueAdd(&cn->cn_WaitReqs, bch);
            return(TRUE);
        }
        hep = bFindHWEndpoint(cn, bep);
        if(!hep) {
            hep = bAllocHWEndpoint(cn, bep);
            if(!hep) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_OUTOFMEMORY, 0);
                return(TRUE);
            }
        }
        hep->hep_CloseTick = 0;

        switch(bep->bep_Type) {
        case BEPT_L2CAP:
        case BEPT_L2CAP_FIXED:
            if(cn->cn_LinkType == BDLT_LE && bep->bep_Type == BEPT_L2CAP) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return(TRUE);
            }
            if(hep->hep_State == HEPS_CLOSED) {
                if(!bEndpointOpen(hep)) {
                    bReplyChannel(BluetoothBase, bch, BTIOERR_CHANNELFAILED, 0);
                    return(TRUE);
                }
            }
            if(rq == BTPR_READ) {
                struct HCRxSDU *rx;
                if((hep->hep_State == HEPS_OPEN) && (rx = (struct HCRxSDU *) RemHead((struct List *) &hep->hep_RxQueue))) {
                    ULONG n = rx->rx_Length;
                    LONG e = 0;
                    hep->hep_RxCount--;
                    if(n > bch->bch_Length) {
                        n = bch->bch_Length;
                        e = BTIOERR_OVERFLOW;
                    }
                    if(n) {
                        CopyMem(rx->rx_Data, bch->bch_Data, n);
                    }
                    btFreeVec(rx);
                    bReplyChannel(BluetoothBase, bch, e, n);
                } else if((bch->bch_Flags & BCHF_NOWAIT) && (hep->hep_State == HEPS_OPEN)) {
                    bReplyChannel(BluetoothBase, bch, 0, 0);
                } else {
                    bReqQueueAdd(&hep->hep_ReadReqs, bch);
                }
            } else {
                if(hep->hep_State == HEPS_OPEN) {
                    bEndpointWrite(hep, bch);
                } else {
                    bReqQueueAdd(&hep->hep_WriteReqs, bch);
                }
            }
            return(TRUE);

        case BEPT_GATT_CHAR:
            if(cn->cn_LinkType != BDLT_LE) {
                bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
                return(TRUE);
            }
            if(!cn->cn_GATTReady) {
                /* bring the ATT channel up first, then retry */
                if(cn->cn_CtrlReq) {
                    bReqQueueAdd(&cn->cn_WaitReqs, bch);
                    return(TRUE);
                }
                cn->cn_CtrlReq = bch;
                if(bt_gatt_client_connect(&cn->cn_GATT, bGATTConnectCB, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_CHANNELFAILED, 0);
                }
                return(TRUE);
            }
            if(rq == BTPR_READ) {
                struct HCRxSDU *rx;
                if(!hep->hep_CCCDWritten && (hep->hep_State == HEPS_CLOSED)) {
                    /* enable notifications/indications on the CCCD found during
                       enumeration (fall back to value handle + 1). The GATT client
                       does one operation at a time: if it is busy, park the request
                       and come back when the current operation completes. */
                    UBYTE cccd[2];
                    UWORD ch = bep->bep_CCCDHandle ? bep->bep_CCCDHandle : (bep->bep_Handle + 1);
                    if(cn->cn_CtrlReq || cn->cn_CCCDBusy || (cn->cn_EnumState != ENUM_IDLE)) {
                        bReqQueueAdd(&cn->cn_WaitReqs, bch);
                        return(TRUE);
                    }
                    cccd[0] = (bep->bep_Properties & 0x20) ? 0x02 : 0x01;
                    cccd[1] = 0;
                    hep->hep_State = HEPS_OPENING;
                    cn->cn_CCCDBusy = TRUE;
                    if(bt_gatt_client_write(&cn->cn_GATT, ch, cccd, 2, bGATTCCCDComplete, hep, now) != BT_OK) {
                        cn->cn_CCCDBusy = FALSE;
                        hep->hep_State = HEPS_OPEN;   /* try without */
                        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "%s: could not submit the CCCD write for %s.", bd->bd_Name, bep->bep_Name);
                    }
                }
                if((rx = (struct HCRxSDU *) RemHead((struct List *) &hep->hep_RxQueue))) {
                    ULONG n = rx->rx_Length;
                    LONG e = 0;
                    hep->hep_RxCount--;
                    if(n > bch->bch_Length) {
                        n = bch->bch_Length;
                        e = BTIOERR_OVERFLOW;
                    }
                    if(n) {
                        CopyMem(rx->rx_Data, bch->bch_Data, n);
                    }
                    btFreeVec(rx);
                    bReplyChannel(BluetoothBase, bch, e, n);
                } else {
                    bReqQueueAdd(&hep->hep_ReadReqs, bch);
                }
            } else {
                if(cn->cn_CtrlReq || (cn->cn_EnumState != ENUM_IDLE)) {
                    bReplyChannel(BluetoothBase, bch, IOERR_UNITBUSY, 0);
                    return(TRUE);
                }
                cn->cn_CtrlReq = bch;
                if(bt_gatt_client_write(&cn->cn_GATT, bep->bep_Handle, bch->bch_Data, bch->bch_Length,
                                        bGATTOpComplete, cn, now) != BT_OK) {
                    cn->cn_CtrlReq = NULL;
                    bReplyChannel(BluetoothBase, bch, BTIOERR_HOSTERROR, 0);
                }
            }
            return(TRUE);

        default:
            bReplyChannel(BluetoothBase, bch, BTIOERR_NOTSUPPORTED, 0);
            return(TRUE);
        }
    }
}
/* \\\ */

/* /// "bConnAbortRequest()" */
BOOL bConnAbortRequest(struct BtHWCore *hc, struct BtChannel *victim)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct MinNode *mn;
    for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        struct MinNode *en;
        if(bReqQueueRemove(&cn->cn_WaitReqs, victim)) {
            bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
            return(TRUE);
        }
        if(cn->cn_CtrlReq == victim) {
            cn->cn_CtrlReq = NULL;
            bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
            return(TRUE);
        }
        if(cn->cn_EnumReq == victim) {
            cn->cn_EnumReq = NULL;
            bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
            return(TRUE);
        }
        if(cn->cn_PairReq == victim) {
            cn->cn_PairReq = NULL;
            bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
            return(TRUE);
        }
        for(en = cn->cn_Endpoints.mlh_Head; en->mln_Succ; en = en->mln_Succ) {
            struct BtHWEndpoint *hep = (struct BtHWEndpoint *) en;
            if(bReqQueueRemove(&hep->hep_ReadReqs, victim) || bReqQueueRemove(&hep->hep_WriteReqs, victim)) {
                bReplyChannel(BluetoothBase, victim, IOERR_ABORTED, 0);
                return(TRUE);
            }
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "bConnTick()" */
void bConnTick(struct BtHWCore *hc)
{
    struct MinNode *mn, *next;
    uint64_t now = bNowUS(hc);

    for(mn = hc->hc_Conns.mlh_Head; (next = mn->mln_Succ); mn = next) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        struct MinNode *en, *enext;
        if(cn->cn_State == HCNS_CONNECTING) {
            /* a peer that never answers (off, out of range, a stale address)
               must not hang whoever asked for the connection: cancel after
               10 s, and give up outright if even the cancel gets no answer */
            LONG waited = (LONG) (hc->hc_Tick - cn->cn_LastActivity);
            if(cn->cn_WaitAdv) {
                continue;   /* not paging: waits for the peer to advertise */
            }
            if(waited > 20000) {
                bConnDown(cn, BTIOERR_TIMEOUT, 0x08);
                bStartNextConnect(hc);
            } else if((waited > 10000) && !cn->cn_Reason) {
                struct BtBase *BluetoothBase = hc->hc_Base;
                cn->cn_Reason = 0x08;   /* marks the cancel as sent */
                btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "%s does not answer - cancelling the connection attempt.", cn->cn_Device->bd_Name);
                bDisconnect(cn, 0x13);
            }
            continue;
        }
        if(cn->cn_State != HCNS_CONNECTED) {
            continue;
        }
        bt_l2cap_channel_manager_tick(&cn->cn_L2CAP, now);
        bt_gatt_client_tick(&cn->cn_GATT, now);
        if(cn->cn_SMPActive) {
            bt_smp_manager_tick(&cn->cn_SMP, now);
        }
        for(en = cn->cn_Endpoints.mlh_Head; (enext = en->mln_Succ); en = enext) {
            struct BtHWEndpoint *hep = (struct BtHWEndpoint *) en;
            if(hep->hep_CloseTick && !hep->hep_UseCnt && ((LONG) (hc->hc_Tick - hep->hep_CloseTick) >= 0)) {
                hep->hep_CloseTick = 0;
                if(hep->hep_Endpoint->bep_Type == BEPT_L2CAP) {
                    bEndpointClose(hep);
                }
            }
        }
    }
}
/* \\\ */

/* /// "bConnDeviceGone()" */
void bConnDeviceGone(struct BtHWCore *hc, struct BtDevice *bd)
{
    (void) hc;
    if(bd->bd_Conns[0]) {
        bDisconnect(bd->bd_Conns[0], 0x13);
    }
    if(bd->bd_Conns[1]) {
        bDisconnect(bd->bd_Conns[1], 0x13);
    }
}
/* \\\ */

/* /// "bConnInit()" */
void bConnInit(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct timeval tv;
    NewList((struct List *) &hc->hc_Conns);
    NewList((struct List *) &hc->hc_ACLTxQueue);
    /* seed the SMP PRNG; the controller's LE Rand output is mixed in later */
    GetSysTime(&tv);
    hc->hc_RandState[0] = tv.tv_secs ^ 0x9e3779b9UL;
    hc->hc_RandState[1] = tv.tv_micro ^ 0x7f4a7c15UL;
    hc->hc_RandState[2] = (ULONG) (IPTR) hc ^ 0x94d049bbUL;
    hc->hc_RandState[3] = *(ULONG *) &hc->hc_Hardware->bth_Address.bd_Addr[2] ^ 0xbf58476dUL;
    if(!hc->hc_RandState[0]) hc->hc_RandState[0] = 1;
    hc->hc_RandAvail = 0;
    hc->hc_RandRequests = 0;
}
/* \\\ */

/* /// "bConnShutdown()" */
void bConnShutdown(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct MinNode *mn;
    struct HCACLTx *tx;

    while((mn = hc->hc_Conns.mlh_Head)->mln_Succ) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;
        if(cn->cn_State == HCNS_CONNECTED) {
            UBYTE p[3];
            p[0] = cn->cn_Handle & 0xff;
            p[1] = cn->cn_Handle >> 8;
            p[2] = 0x13;
            bSubmitCmd(hc, HC_OP_DISCONNECT, p, 3, bIgnoreCompletion, hc);
        }
        bConnDown(cn, IOERR_ABORTED, 0x16);
    }
    while((tx = (struct HCACLTx *) RemHead((struct List *) &hc->hc_ACLTxQueue))) {
        btFreeVec(tx);
    }
}
/* \\\ */
