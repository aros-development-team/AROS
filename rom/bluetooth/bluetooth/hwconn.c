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
    ENUM_GATT_DESCS,
    ENUM_DONE
};

/* pairing states */
enum {
    PAIR_IDLE = 0,
    PAIR_CONNECTING,
    PAIR_AUTH,
    PAIR_WAITUSER,
    PAIR_ENCRYPT
};

static void bConnRunEnum(struct BtHWConn *cn);
static void bConnFinishEnum(struct BtHWConn *cn, LONG error);
static void bEndpointEvent(struct bt_l2cap_channel_event_info *info, void *user_data);
static void bFlushWaitingRequests(struct BtHWConn *cn, LONG error);
static void bStartPairing(struct BtHWConn *cn);
static void bPairingDone(struct BtHWConn *cn, LONG error, ULONG status);

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
}
/* \\\ */

/* /// "bGATTCCCDComplete()" */
static void bGATTCCCDComplete(struct bt_gatt_client_completion *completion, void *user_data)
{
    struct BtHWEndpoint *hep = user_data;
    (void) completion;
    hep->hep_CCCDWritten = TRUE;
    hep->hep_State = HEPS_OPEN;
}
/* \\\ */

/* *** connections *** */

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
        bd->bd_Conn = cn;
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
    /* enumerate services once for registered devices we know nothing about */
    if(!(bd->bd_Flags & BDFF_SERVICESKNOWN) && (cn->cn_EnumState == ENUM_IDLE)) {
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
    if(cn->cn_PairState != PAIR_IDLE) {
        bPairingDone(cn, error, reason);
    }

    btLockWriteDevice(bd);
    bd->bd_Flags &= ~(BDFF_CONNECTED|BDFF_CONNECTING|BDFF_ENCRYPTED);
    bd->bd_ConnHandle = 0;
    bd->bd_Role = BDR_NONE;
    bd->bd_LinkType = BDLT_NONE;
    if(bd->bd_Conn == cn) {
        bd->bd_Conn = NULL;
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
    if(cn->cn_LinkType == BDLT_LE) {
        struct bt_buf_writer w;
        bt_buf_writer_init(&w, params, sizeof(params));
        bt_buf_writer_write_le16(&w, 0x0060);           /* scan interval */
        bt_buf_writer_write_le16(&w, 0x0030);           /* scan window */
        bt_buf_writer_write_u8(&w, 0x00);               /* filter policy: peer address */
        bt_buf_writer_write_u8(&w, bd->bd_AddrType & 1); /* peer address type */
        bt_buf_writer_write_bytes(&w, bd->bd_Address.bd_Addr, 6);
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
        if((cn->cn_State == HCNS_CONNECTING) && !cn->cn_Handle && (cn->cn_Role == BDR_NONE)) {
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
static struct BtHWConn * bEnsureConnection(struct BtHWCore *hc, struct BtDevice *bd, BOOL autoconnect,
                                            BOOL *pending, LONG *error)
{
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtHWConn *cn = bd->bd_Conn;
    UBYTE linktype;

    *pending = FALSE;
    if(cn) {
        if(cn->cn_State == HCNS_CONNECTED) {
            return(cn);
        }
        if(cn->cn_State == HCNS_CONNECTING) {
            *pending = TRUE;
            return(cn);
        }
        *error = BTIOERR_NOTCONNECTED;
        return(NULL);
    }
    if(!autoconnect) {
        *error = BTIOERR_NOTCONNECTED;
        return(NULL);
    }
    if(bth->bth_State != BHS_READY) {
        *error = BTIOERR_NOTREADY;
        return(NULL);
    }
    if((bd->bd_Flags & BDFF_CLASSIC) && (bth->bth_Flags & BTHF_CLASSIC)) {
        linktype = BDLT_ACL;
    } else if((bd->bd_Flags & BDFF_LE) && (bth->bth_Flags & BTHF_LE)) {
        linktype = BDLT_LE;
    } else {
        *error = BTIOERR_NOTSUPPORTED;
        return(NULL);
    }
    cn = bAllocConn(hc, bd, linktype);
    if(!cn) {
        *error = BTIOERR_OUTOFMEMORY;
        return(NULL);
    }
    *pending = TRUE;
    bStartNextConnect(hc);
    return(cn);
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
        if(cn->cn_LinkType == BDLT_LE) {
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

    /* endpoint state of the old endpoints must go first */
    for(mn = cn->cn_Endpoints.mlh_Head; (next = mn->mln_Succ); mn = next) {
        struct BtHWEndpoint *hep = (struct BtHWEndpoint *) mn;
        if(hep->hep_State != HEPS_CLOSED) {
            bEndpointClose(hep);
        }
        bFreeHWEndpoint(hep, BTIOERR_CHANNELFAILED);
    }
    btLockWriteDevice(bd);
    while((bsv = (struct BtService *) bd->bd_Services.lh_Head)->bsv_Node.ln_Succ) {
        if(bsv->bsv_SvcBinding) {
            /* keep bound services (their binding owns channels); refresh is
               not possible without releasing the binding first */
            break;
        }
        bFreeService(BluetoothBase, bsv);
        bd->bd_NumServices--;
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
        bConnFinishEnum(cn, BTIOERR_REMOTEERROR);
        return;
    }
    (void) gc;
    switch(cn->cn_EnumState) {
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
            bConnFinishEnum(cn, 0);
        } else {
            struct bt_gatt_service *svc = &cn->cn_Services[cn->cn_EnumIndex];
            if(bt_gatt_client_discover_characteristics(&cn->cn_GATT, svc->start_handle, svc->end_handle,
                                                       bGATTEnumCB, cn, now) != BT_OK) {
                bConnFinishEnum(cn, BTIOERR_HOSTERROR);
            }
        }
        break;
    default:
        break;
    }
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
        bStoreDevConfig(BluetoothBase, bd);
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Paired with %s.", bd->bd_Name);
        btSendEvent(BEHMB_DEVICEREGISTERED, bd, NULL);
    } else {
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Pairing with %s failed (%s).",
                       bd->bd_Name, btNumToStr(BNTS_HCISTATUS, status, "unknown"));
    }
    btSendEvent(BEHMB_PAIRINGDONE, bd, (APTR) (IPTR) status);
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
        /* LE pairing (SMP) is not wired up yet */
        bPairingDone(cn, BTIOERR_NOTSUPPORTED, 0);
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
        cn = bd->bd_Conn;
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
        if(accept && bd && !bd->bd_Conn) {
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

    case HC_EVT_ENCRYPTION_CHANGE: {
        UWORD handle;
        if(len < 4) {
            return(TRUE);
        }
        handle = (params[1] | (params[2] << 8)) & 0x0fff;
        cn = bFindConnByHandle(hc, handle);
        if(cn) {
            cn->cn_Encrypted = (!params[0] && params[3]) ? TRUE : FALSE;
            btLockWriteDevice(cn->cn_Device);
            if(cn->cn_Encrypted) {
                cn->cn_Device->bd_Flags |= BDFF_ENCRYPTED;
            } else {
                cn->cn_Device->bd_Flags &= ~BDFF_ENCRYPTED;
            }
            btUnlockDevice(cn->cn_Device);
            if(cn->cn_PairState == PAIR_ENCRYPT) {
                bPairingDone(cn, params[0] ? BTIOERR_SECURITY : 0, params[0]);
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
                bStoreDevConfig(BluetoothBase, bd);
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
        if(bd && bd->bd_Conn) {
            bAskUser(bd->bd_Conn, BPRT_PINCODE, 0);
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
            if(bd->bd_Conn && (bd->bd_Conn->cn_PairState == PAIR_IDLE)) {
                bd->bd_Conn->cn_PairState = PAIR_AUTH; /* remote initiated */
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
        if(bd && bd->bd_Conn) {
            if(BluetoothBase->bt_GlobalCfg->bgc_PopupPairing) {
                bAskUser(bd->bd_Conn, BPRT_NUMERICCOMPARE, passkey);
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
        if(bd && bd->bd_Conn) {
            bAskUser(bd->bd_Conn, BPRT_PASSKEYENTRY, 0);
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
        if(bd && bd->bd_Conn) {
            struct BtHWConn *pcn = bd->bd_Conn;
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
        if(bd && bd->bd_Conn && params[0] && (bd->bd_Conn->cn_PairState != PAIR_IDLE)) {
            bPairingDone(bd->bd_Conn, BTIOERR_SECURITY, params[0]);
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
            cn = bd->bd_Conn;
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
            cn = bEnsureConnection(hc, bd, TRUE, &pending, &err);
            if(!cn) {
                bReplyChannel(BluetoothBase, bch, err, 0);
            } else if(pending) {
                bReqQueueAdd(&cn->cn_WaitReqs, bch);
            } else {
                bReplyChannel(BluetoothBase, bch, 0, 0);
            }
            return(TRUE);

        case BTPRI_DISCONNECT:
            cn = bd->bd_Conn;
            if(cn) {
                bDisconnect(cn, 0x13);
            }
            bReplyChannel(BluetoothBase, bch, 0, 0);
            return(TRUE);

        case BTPRI_ENUMSERVICES:
            cn = bEnsureConnection(hc, bd, TRUE, &pending, &err);
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
            cn = bEnsureConnection(hc, bd, TRUE, &pending, &err);
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
            cn = bd->bd_Conn;
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
            btUnlockDevice(bd);
            bStoreDevConfig(BluetoothBase, bd);
            if(bd->bd_Conn) {
                bDisconnect(bd->bd_Conn, 0x13);
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
            cn = bEnsureConnection(hc, bd, (bch->bch_Flags & BCHF_AUTOCONNECT) ? TRUE : FALSE, &pending, &err);
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
            cn = bEnsureConnection(hc, bd, (bch->bch_Flags & BCHF_AUTOCONNECT) ? TRUE : FALSE, &pending, &err);
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
            cn = bd->bd_Conn;
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
        cn = bEnsureConnection(hc, bd, (bch->bch_Flags & BCHF_AUTOCONNECT) ? TRUE : FALSE, &pending, &err);
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
                    /* enable notifications/indications on the CCCD (value handle + 1 by convention) */
                    UBYTE cccd[2];
                    cccd[0] = (bep->bep_Properties & 0x20) ? 0x02 : 0x01;
                    cccd[1] = 0;
                    hep->hep_State = HEPS_OPENING;
                    if(!cn->cn_CtrlReq && (cn->cn_EnumState == ENUM_IDLE)) {
                        bt_gatt_client_write(&cn->cn_GATT, bep->bep_Handle + 1, cccd, 2, bGATTCCCDComplete, hep, now);
                    } else {
                        hep->hep_State = HEPS_OPEN; /* try without */
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
        if(cn->cn_State != HCNS_CONNECTED) {
            continue;
        }
        bt_l2cap_channel_manager_tick(&cn->cn_L2CAP, now);
        bt_gatt_client_tick(&cn->cn_GATT, now);
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
    if(bd->bd_Conn) {
        bDisconnect(bd->bd_Conn, 0x13);
    }
}
/* \\\ */

/* /// "bConnInit()" */
void bConnInit(struct BtHWCore *hc)
{
    NewList((struct List *) &hc->hc_Conns);
    NewList((struct List *) &hc->hc_ACLTxQueue);
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
