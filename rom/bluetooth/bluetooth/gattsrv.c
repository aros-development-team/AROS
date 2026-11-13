/*
 *----------------------------------------------------------------------------
 *            bluetooth.library: GATT server (hardware task)
 *----------------------------------------------------------------------------
 *
 * Offers the GATT services registered with btAddServiceRecord() (BSVP_ATT)
 * to the LE devices connected to a radio. The requests arrive on the ATT
 * channel the GATT client of the link owns (hwconn.c) and are answered by
 * btcore's GATT server from the service records: only the enabled ones exist
 * as far as a device can tell. Which ones are enabled, and whether the radio
 * advertises so that devices can find and connect to it, is decided by the
 * GATT server class (btgatt.class) and its user.
 *
 * Everything here runs in the hardware task, but for bGattSrvCollect().
 * Other tasks change the records and values and bump a sequence number in
 * the library base; bGattSrvPoll() picks that up when they signal the task,
 * or with the next tick.
 */

#include "debug.h"

#include "hwtask.h"

#include <proto/exec.h>

#include <string.h>

#define GATT_UUID_PRIMARY_SERVICE 0x2800
#define GATT_UUID_CHARACTERISTIC  0x2803
#define GATT_UUID_CLIENT_CONFIG   0x2902
#define GATT_UUID_DEVICE_NAME     0x2a00

#define ATT_ERR_INVALID_VALUE_LEN 0x0d
#define ATT_ERR_NO_RESOURCES      0x11

/* ATT transaction timeout: a device that has not confirmed an indication by
   then is not going to */
#define GATT_INDICATION_TIMEOUT_MS 30000

/* /// "bGattSub()" */
/* The client configuration a device wrote for one characteristic. */
static UWORD *bGattSub(struct BtHWConn *cn, UWORD handle, BOOL create)
{
    ULONG n;

    for(n = 0; n < HC_GATT_MAXSUBS; n++) {
        if(cn->cn_GATTSubs[n].handle == handle) {
            return(&cn->cn_GATTSubs[n].value);
        }
    }
    if(create) {
        for(n = 0; n < HC_GATT_MAXSUBS; n++) {
            if(!cn->cn_GATTSubs[n].handle) {
                cn->cn_GATTSubs[n].handle = handle;
                cn->cn_GATTSubs[n].value = 0;
                return(&cn->cn_GATTSubs[n].value);
            }
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "bGattSetType()" */
static void bGattSetType(struct bt_gatt_server_attr *a, UWORD uuid16)
{
    a->type_len = 2;
    a->type[0] = uuid16 & 0xff;
    a->type[1] = uuid16 >> 8;
}
/* \\\ */

/* /// "bGattValue()" */
/* A stable copy of a characteristic's value for this request. */
static UWORD bGattValue(struct BtHWConn *cn, struct BtGattChar *bgc)
{
    UWORD len;

    if((bgc->bgc_UUIDLen == 2) && !bgc->bgc_Len &&
       ((bgc->bgc_UUID[0] | (bgc->bgc_UUID[1] << 8)) == GATT_UUID_DEVICE_NAME)) {
        /* a Device Name left empty is the name of this radio */
        STRPTR name = cn->cn_Core->hc_Hardware->bth_LocalName;
        len = name ? strlen((char *) name) : 0;
        if(len > 248) {
            len = 248;
        }
        if(len) {
            CopyMem(name, cn->cn_GATTVal, len);
        }
        return(len);
    }
    if(bgc->bgc_Properties & BGDP_STREAM) {
        return(0);                      /* past events are not a value */
    }
    len = bgc->bgc_Len;
    if(len) {
        CopyMem(bgc->bgc_Value, cn->cn_GATTVal, len);
    }
    return(len);
}
/* \\\ */

/* /// "bGattNext()" */
/* btcore asks for the first attribute at or after a handle. The attributes
   are those of the enabled records, in the order they were registered (which
   is the order of their handles). The library base is read-locked. */
static bool bGattNext(void *context, uint16_t from, struct bt_gatt_server_attr *a)
{
    struct BtHWConn *cn = context;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtServiceRecord *bsr;

    for(bsr = (struct BtServiceRecord *) BluetoothBase->bt_ServiceRecords.lh_Head; bsr->bsr_Node.ln_Succ;
        bsr = (struct BtServiceRecord *) bsr->bsr_Node.ln_Succ) {
        ULONG h;

        if((bsr->bsr_Protocol != BSVP_ATT) || !bsr->bsr_Enabled || (bsr->bsr_LastHandle < from)) {
            continue;
        }
        for(h = (from > bsr->bsr_FirstHandle) ? from : bsr->bsr_FirstHandle; h <= bsr->bsr_LastHandle; h++) {
            ULONG idx = h - bsr->bsr_FirstHandle;
            struct BtGattChar *bgc;
            ULONG k;

            a->handle = h;
            a->ref = bsr;
            if(!idx) {
                bGattSetType(a, GATT_UUID_PRIMARY_SERVICE);
                a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_SERVICE;
                a->group_end = bsr->bsr_LastHandle;
                a->value = bsr->bsr_UUID;
                a->value_len = bsr->bsr_UUIDLen;
                a->ref_kind = HC_GATT_KIND_DECL;
                return(true);
            }
            k = (idx - 1) / 3;
            bgc = &bsr->bsr_Chars[k];
            a->ref_index = k;
            switch((idx - 1) % 3) {
            case 0: /* the declaration: properties, value handle, UUID */
                bGattSetType(a, GATT_UUID_CHARACTERISTIC);
                a->flags = BT_GATT_ATTR_READ;
                a->scratch[0] = bgc->bgc_Properties & 0xff;
                a->scratch[1] = (h + 1) & 0xff;
                a->scratch[2] = (h + 1) >> 8;
                CopyMem(bgc->bgc_UUID, &a->scratch[3], bgc->bgc_UUIDLen);
                a->value = a->scratch;
                a->value_len = 3 + bgc->bgc_UUIDLen;
                a->ref_kind = HC_GATT_KIND_DECL;
                return(true);

            case 1: /* the value */
                a->type_len = bgc->bgc_UUIDLen;
                CopyMem(bgc->bgc_UUID, a->type, bgc->bgc_UUIDLen);
                a->flags = ((bgc->bgc_Properties & BGDP_READ) ? BT_GATT_ATTR_READ : 0) |
                           ((bgc->bgc_Properties & BGDP_WRITE) ? BT_GATT_ATTR_WRITE : 0) |
                           ((bgc->bgc_Properties & BGDP_WRITENR) ? BT_GATT_ATTR_WRITE_NR : 0);
                a->value_len = bGattValue(cn, bgc);
                a->value = cn->cn_GATTVal;
                a->ref_kind = HC_GATT_KIND_VALUE;
                return(true);

            default: { /* the client configuration, kept per connection */
                UWORD *sub;
                if(!(bgc->bgc_Properties & (BGDP_NOTIFY|BGDP_INDICATE))) {
                    continue;
                }
                bGattSetType(a, GATT_UUID_CLIENT_CONFIG);
                a->flags = BT_GATT_ATTR_READ | BT_GATT_ATTR_WRITE;
                sub = bGattSub(cn, h, FALSE);
                a->scratch[0] = sub ? (*sub & 0xff) : 0;
                a->scratch[1] = 0;
                a->value = a->scratch;
                a->value_len = 2;
                a->ref_kind = HC_GATT_KIND_CONFIG;
                return(true);
            }
            }
        }
    }
    return(false);
}
/* \\\ */

/* /// "bGattWrite()" */
static uint8_t bGattWrite(void *context, const struct bt_gatt_server_attr *a, const uint8_t *data, size_t len)
{
    struct BtHWConn *cn = context;
    struct BtServiceRecord *bsr = a->ref;
    struct BtGattChar *bgc = &bsr->bsr_Chars[a->ref_index];

    if(a->ref_kind == HC_GATT_KIND_CONFIG) {
        UWORD *sub;
        if(len != 2) {
            return(ATT_ERR_INVALID_VALUE_LEN);
        }
        UWORD old;
        if(!(sub = bGattSub(cn, a->handle, TRUE))) {
            return(ATT_ERR_NO_RESOURCES);
        }
        old = *sub & 3;
        /* only what the characteristic offers can be subscribed to */
        *sub = data[0] & (((bgc->bgc_Properties & BGDP_NOTIFY) ? 1 : 0) |
                          ((bgc->bgc_Properties & BGDP_INDICATE) ? 2 : 0));
        /* bt_GattLock is held (bGattRequest()) */
        if(!old && *sub) {
            bgc->bgc_Subscribers++;
        } else if(old && !*sub && bgc->bgc_Subscribers) {
            bgc->bgc_Subscribers--;
        }
        return(0);
    }
    if(len > bgc->bgc_MaxLen) {
        return(ATT_ERR_INVALID_VALUE_LEN);
    }
    if(len) {
        CopyMem((APTR) data, bgc->bgc_Value, len);
    }
    bgc->bgc_Len = len;
    /* the owner hears about it once the base is unlocked again */
    cn->cn_GATTWrRec = bsr;
    cn->cn_GATTWrIdx = a->ref_index;
    cn->cn_GATTWrData = data;
    cn->cn_GATTWrLen = len;
    return(0);
}
/* \\\ */

/* /// "bGattRequest()" */
/* An ATT request, command or confirmation from the device. */
static void bGattRequest(const uint8_t *pdu, size_t len, uint64_t now_us, void *user_data)
{
    struct BtHWConn *cn = user_data;
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    UBYTE rsp[BT_GATT_SERVER_RX_MTU];
    size_t n;

    if(!cn->cn_GATTSeen) {
        cn->cn_GATTSeen = TRUE;
        btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s is looking at our GATT services.",
                       cn->cn_Device->bd_Name);
    }
    cn->cn_GATTWrRec = NULL;
    cn->cn_GATTWrData = NULL;
    cn->cn_GATTWrLen = 0;
    btLockReadBase();
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    n = bt_gatt_server_handle_pdu(&cn->cn_GATTServer, pdu, len, rsp, sizeof(rsp));
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    btUnlockBase();
    if(n) {
        bt_l2cap_channel_manager_send(&cn->cn_L2CAP, BT_L2CAP_CID_ATT, rsp, n, now_us);
    }
    if((len >= 1) && (pdu[0] == BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION) &&
       (cn->cn_GATTSeq != BluetoothBase->bt_GattSeq)) {
        /* the next indication may go now, not a tick later */
        bGattSrvPoll(cn->cn_Core);
    }
    if(cn->cn_GATTWrRec) {
        bSendServiceWriteEvent(BluetoothBase, cn->cn_GATTWrRec, cn->cn_GATTWrIdx,
                               cn->cn_GATTWrData, cn->cn_GATTWrLen, cn->cn_Device);
        cn->cn_GATTWrRec = NULL;
        cn->cn_GATTWrData = NULL;
        cn->cn_GATTWrLen = 0;
    }
}
/* \\\ */

/* /// "bGattSrvInit()" */
/* A link came up: serve our GATT services on it. Only LE links have the ATT
   channel from the start; nothing is offered over BR/EDR. */
void bGattSrvInit(struct BtHWConn *cn)
{
    memset(cn->cn_GATTSubs, 0, sizeof(cn->cn_GATTSubs));
    cn->cn_GATTSeen = FALSE;
    /* what was queued before the link existed is not for it */
    cn->cn_GATTSeq = cn->cn_Core->hc_Base->bt_GattSeq;
    cn->cn_GATTIndSince = 0;
    cn->cn_GATTIndDead = FALSE;
    cn->cn_GATTWrRec = NULL;
    cn->cn_GATTWrData = NULL;
    cn->cn_GATTWrLen = 0;
    bt_gatt_server_init(&cn->cn_GATTServer, bGattNext, bGattWrite, cn);
    bt_gatt_client_set_request_handler(&cn->cn_GATT, bGattRequest, cn);
    if(cn->cn_LinkType == BDLT_LE) {
        bt_gatt_client_listen(&cn->cn_GATT);
    }
}
/* \\\ */

/* /// "bGattSrvLinkDown()" */
/* A link is gone: its subscriptions no longer count. */
void bGattSrvLinkDown(struct BtHWConn *cn)
{
    struct BtBase *BluetoothBase = cn->cn_Core->hc_Base;
    struct BtServiceRecord *bsr;
    ULONG n;

    btLockReadBase();
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    for(n = 0; n < HC_GATT_MAXSUBS; n++) {
        UWORD h = cn->cn_GATTSubs[n].handle;

        if(!h || !(cn->cn_GATTSubs[n].value & 3)) {
            continue;
        }
        /* the handles are never handed out twice: the record that has this
           one is the one subscribed to, if it is still there */
        for(bsr = (struct BtServiceRecord *) BluetoothBase->bt_ServiceRecords.lh_Head; bsr->bsr_Node.ln_Succ;
            bsr = (struct BtServiceRecord *) bsr->bsr_Node.ln_Succ) {
            ULONG idx;
            if((bsr->bsr_Protocol != BSVP_ATT) || (h <= bsr->bsr_FirstHandle) || (h > bsr->bsr_LastHandle)) {
                continue;
            }
            idx = h - bsr->bsr_FirstHandle - 1;
            if(((idx % 3) == 2) && (idx / 3 < bsr->bsr_NumChars) && bsr->bsr_Chars[idx / 3].bgc_Subscribers) {
                bsr->bsr_Chars[idx / 3].bgc_Subscribers--;
            }
            break;
        }
    }
    memset(cn->cn_GATTSubs, 0, sizeof(cn->cn_GATTSubs));
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    btUnlockBase();
}
/* \\\ */

/* /// "bGattNotifyLink()" */
/*
 * Send one link what was queued for it since it was last served, in order.
 * An indication needs the device's confirmation before the next one may go:
 * the link stays behind at that snapshot, which is kept, and is served again
 * once the confirmation came (bGattRequest()) or with the next poll. The
 * base and bt_GattLock are held.
 */
static void bGattNotifyLink(struct BtHWConn *cn, ULONG newseq)
{
    struct BtHWCore *hc = cn->cn_Core;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtGattNotification *bgn;
    UBYTE pdu[BT_GATT_SERVER_RX_MTU];

    if(cn->cn_GATTServer.indication_pending && !cn->cn_GATTIndDead &&
       ((LONG) (hc->hc_Tick - cn->cn_GATTIndSince) >= GATT_INDICATION_TIMEOUT_MS)) {
        cn->cn_GATTIndDead = TRUE;
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s does not confirm our indications - sending it no more.", cn->cn_Device->bd_Name);
    }
    for(bgn = (struct BtGattNotification *) BluetoothBase->bt_GattNotifications.lh_Head;
        bgn->bgn_Node.ln_Succ;
        bgn = (struct BtGattNotification *) bgn->bgn_Node.ln_Succ) {
        struct BtServiceRecord *bsr = bgn->bgn_Record;
        UWORD *sub;
        UWORD vh;
        BOOL indicate;
        size_t n;

        if((LONG) (bgn->bgn_Seq - cn->cn_GATTSeq) <= 0) {
            continue;   /* sent already */
        }
        if((LONG) (bgn->bgn_Seq - newseq) > 0) {
            break;      /* queued after this round started */
        }
        vh = bsr->bsr_FirstHandle + 2 + 3 * bgn->bgn_Index;
        if(!bsr->bsr_Enabled || (bgn->bgn_Index >= bsr->bsr_NumChars) ||
           !(sub = bGattSub(cn, vh + 1, FALSE)) || !(*sub & 3)) {
            cn->cn_GATTSeq = bgn->bgn_Seq;
            continue;
        }
        indicate = (*sub & 1) ? FALSE : TRUE;
        if(indicate && cn->cn_GATTIndDead) {
            cn->cn_GATTSeq = bgn->bgn_Seq;
            continue;
        }
        if(indicate && cn->cn_GATTServer.indication_pending) {
            return;     /* wait for the confirmation */
        }
        n = bt_gatt_server_encode_handle_value(&cn->cn_GATTServer, vh, bgn->bgn_Data, bgn->bgn_Length,
                                                indicate ? true : false, pdu, sizeof(pdu));
        if(n) {
            bt_l2cap_channel_manager_send(&cn->cn_L2CAP, BT_L2CAP_CID_ATT, pdu, n, bNowUS(hc));
            if(indicate) {
                cn->cn_GATTIndSince = hc->hc_Tick;
            }
        }
        cn->cn_GATTSeq = bgn->bgn_Seq;
    }
    cn->cn_GATTSeq = newseq;
}
/* \\\ */

/* /// "bGattNotifyChanged()" */
/* Values set since the last look: tell the devices that subscribed. Then
   hc_GattSeq says how far every link of this radio got. */
static void bGattNotifyChanged(struct BtHWCore *hc, ULONG newseq)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct MinNode *mn;
    ULONG low = newseq;

    btLockReadBase();
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    for(mn = hc->hc_Conns.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        struct BtHWConn *cn = (struct BtHWConn *) mn;

        if((cn->cn_State != HCNS_CONNECTED) || (cn->cn_LinkType != BDLT_LE)) {
            continue;
        }
        if(cn->cn_GATTSeq != newseq) {
            bGattNotifyLink(cn, newseq);
        }
        if((LONG) (cn->cn_GATTSeq - low) < 0) {
            low = cn->cn_GATTSeq;
        }
    }
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    btUnlockBase();
    hc->hc_GattSeq = low;
}
/* \\\ */

/* /// "bGattSrvCollect()" */
/* A snapshot may be freed once every running radio is past its sequence.
   With no radio left, none will ever be sent: they all go. */
void bGattSrvCollect(struct BtBase *BluetoothBase)
{
    struct BtGattNotification *bgn, *next;
    struct BtHardware *bth;

    btLockReadBase();
    ObtainSemaphore(&BluetoothBase->bt_GattLock);
    bgn = (struct BtGattNotification *) BluetoothBase->bt_GattNotifications.lh_Head;
    while(bgn->bgn_Node.ln_Succ) {
        BOOL consumed = TRUE;

        for(bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
            bth->bth_Node.ln_Succ; bth = (struct BtHardware *) bth->bth_Node.ln_Succ) {
            struct BtHWCore *other = bth->bth_Core;
            if(other && bth->bth_Task && !other->hc_Shutdown &&
               ((LONG) (other->hc_GattSeq - bgn->bgn_Seq) < 0)) {
                consumed = FALSE;
                break;
            }
        }
        if(!consumed) {
            break;
        }
        next = (struct BtGattNotification *) bgn->bgn_Node.ln_Succ;
        Remove(&bgn->bgn_Node);
        BluetoothBase->bt_GattNotificationCount--;
        btFreeVec(bgn);
        bgn = next;
    }
    ReleaseSemaphore(&BluetoothBase->bt_GattLock);
    btUnlockBase();
}
/* \\\ */

/* /// "bLEAdvEnabled()" */
/* The controller's answer to "advertise". It may refuse: not every one can
   advertise while it scans, initiates or already has a link as peripheral. */
static void bLEAdvEnabled(struct bt_cmdq_completion *completion, void *user_data)
{
    struct BtHWCore *hc = user_data;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;

    if((completion->result == BT_CMDQ_RESULT_COMPLETE) && !completion->status) {
        if(!hc->hc_LEAdvOn) {
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s/%ld: advertising to LE devices as '%s'.",
                           bth->bth_DevName, bth->bth_Unit,
                           bth->bth_LocalName ? bth->bth_LocalName : (STRPTR) "AROS");
        }
        hc->hc_LEAdvOn = TRUE;
        hc->hc_LEAdvWarned = FALSE;
        return;
    }
    hc->hc_LEAdvOn = FALSE;
    bth->bth_LastHCIError = completion->status;
    if(!hc->hc_LEAdvWarned) {
        hc->hc_LEAdvWarned = TRUE;
        btAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "%s/%ld: the controller does not advertise right now (HCI status 0x%02lx).",
                       bth->bth_DevName, bth->bth_Unit, (ULONG) completion->status);
    }
}
/* \\\ */

/* /// "bLEAdvUpdate()" */
/*
 * Connectable LE advertising, as far as the stack is asked to advertise:
 * the flags, a "computer" appearance and the 16-bit UUIDs of the enabled
 * services in the advertisement, the name in the scan response. The
 * controller stops advertising when a device connects; this is called again
 * when the link is gone.
 */
static void bLEAdvUpdate(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtServiceRecord *bsr;
    STRPTR name = bth->bth_LocalName ? bth->bth_LocalName : (STRPTR) "AROS";
    UBYTE p[32];
    ULONG pos, lenpos, lenpos128, nlen;
    BOOL incomplete16 = FALSE, incomplete128 = FALSE;

    if(!(bth->bth_Flags & BTHF_LE) || !hc->hc_BringupDone || hc->hc_Shutdown) {
        return;
    }
    if(!BluetoothBase->bt_LEAdvertising && !hc->hc_LEAdvAsked) {
        return; /* never asked for, nothing to take back */
    }
    /* the data can only be changed while it is off */
    p[0] = 0x00;
    bSubmitCmd(hc, HC_OP_LE_SET_ADV_ENABLE, p, 1, bIgnoreCompletion, hc);
    if(!BluetoothBase->bt_LEAdvertising) {
        if(hc->hc_LEAdvOn) {
            btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "%s/%ld: no longer advertising to LE devices.",
                           bth->bth_DevName, bth->bth_Unit);
        }
        hc->hc_LEAdvOn = FALSE;
        hc->hc_LEAdvAsked = FALSE;
        hc->hc_LEAdvWarned = FALSE;
        return;
    }
    hc->hc_LEAdvAsked = TRUE;

    memset(p, 0, sizeof(p));
    p[0] = 0xa0; p[1] = 0x00;       /* interval 100 ms ... */
    p[2] = 0xf0; p[3] = 0x00;       /* ... to 150 ms */
    p[4] = 0x00;                    /* connectable, scannable, undirected */
    p[5] = 0x00;                    /* our public address */
    p[13] = 0x07;                   /* all three advertising channels */
    p[14] = 0x00;                   /* anyone may scan and connect */
    bSubmitCmd(hc, HC_OP_LE_SET_ADV_PARAMS, p, 15, bIgnoreCompletion, hc);

    memset(p, 0, sizeof(p));
    pos = 1;
    p[pos++] = 2; p[pos++] = 0x01;  /* flags: general discoverable */
    p[pos++] = (bth->bth_Flags & BTHF_CLASSIC) ? 0x1a : 0x06;
    p[pos++] = 3; p[pos++] = 0x19;  /* appearance: computer */
    p[pos++] = 0x80; p[pos++] = 0x00;
    /* A BLE MIDI client discovers the service from its 128-bit UUID. Put one
       or more vendor UUIDs first, then use the remaining bytes for adopted
       16-bit services. UUIDs in advertising use ATT little-endian order. */
    lenpos128 = pos;
    pos += 2;
    btLockReadBase();
    for(bsr = (struct BtServiceRecord *) BluetoothBase->bt_ServiceRecords.lh_Head; bsr->bsr_Node.ln_Succ;
        bsr = (struct BtServiceRecord *) bsr->bsr_Node.ln_Succ) {
        if((bsr->bsr_Protocol != BSVP_ATT) || !bsr->bsr_Enabled || (bsr->bsr_UUIDLen != 16)) {
            continue;
        }
        if(pos + 16 > 32) {
            incomplete128 = TRUE;
            continue;
        }
        CopyMem(bsr->bsr_UUID, &p[pos], 16);
        pos += 16;
    }
    if(pos > lenpos128 + 2) {
        p[lenpos128] = pos - lenpos128 - 1;
        p[lenpos128 + 1] = incomplete128 ? 0x06 : 0x07;
    } else {
        pos = lenpos128;
        p[pos] = p[pos + 1] = 0;
    }
    lenpos = pos;
    pos += 2;
    for(bsr = (struct BtServiceRecord *) BluetoothBase->bt_ServiceRecords.lh_Head; bsr->bsr_Node.ln_Succ;
        bsr = (struct BtServiceRecord *) bsr->bsr_Node.ln_Succ) {
        if((bsr->bsr_Protocol != BSVP_ATT) || !bsr->bsr_Enabled || (bsr->bsr_UUIDLen != 2) ||
           (bsr->bsr_UUID16 == 0x1800) || (bsr->bsr_UUID16 == 0x1801)) {
            continue; /* the two every device has are not worth the room */
        }
        if(pos + 2 > 32) {
            incomplete16 = TRUE;
            continue;
        }
        p[pos++] = bsr->bsr_UUID[0];
        p[pos++] = bsr->bsr_UUID[1];
    }
    btUnlockBase();
    if(pos > lenpos + 2) {
        p[lenpos] = pos - lenpos - 1;
        p[lenpos + 1] = incomplete16 ? 0x02 : 0x03;
    } else {
        pos = lenpos;
        p[pos] = p[pos + 1] = 0;
    }
    p[0] = pos - 1;
    bSubmitCmd(hc, HC_OP_LE_SET_ADV_DATA, p, 32, bIgnoreCompletion, hc);

    memset(p, 0, sizeof(p));
    nlen = strlen((char *) name);
    p[2] = 0x09;                    /* complete local name */
    if(nlen > 29) {
        nlen = 29;
        p[2] = 0x08;                /* shortened */
    }
    p[1] = nlen + 1;
    CopyMem(name, &p[3], nlen);
    p[0] = nlen + 2;
    bSubmitCmd(hc, HC_OP_LE_SET_SCAN_RSP_DATA, p, 32, bIgnoreCompletion, hc);

    p[0] = 0x01;
    bSubmitCmd(hc, HC_OP_LE_SET_ADV_ENABLE, p, 1, bLEAdvEnabled, hc);
}
/* \\\ */

/* /// "bGattSrvRefresh()" */
/* Have the next tick set the advertising up again (bring-up done, a link
   that ended the advertising is gone, the name changed). */
void bGattSrvRefresh(struct BtHWCore *hc)
{
    hc->hc_LEAdvSeq = hc->hc_Base->bt_LEAdvSeq - 1;
}
/* \\\ */

/* /// "bGattSrvPoll()" */
/* Every tick: follow what the other tasks changed. */
void bGattSrvPoll(struct BtHWCore *hc)
{
    struct BtBase *BluetoothBase = hc->hc_Base;
    ULONG seq;

    seq = BluetoothBase->bt_LEAdvSeq;
    if(hc->hc_LEAdvSeq != seq) {
        hc->hc_LEAdvSeq = seq;
        bLEAdvUpdate(hc);
    }
    seq = BluetoothBase->bt_GattSeq;
    if(hc->hc_GattSeq != seq) {
        /* new values, or a link still behind (an unconfirmed indication) */
        bGattNotifyChanged(hc, seq);
        bGattSrvCollect(BluetoothBase);
    }
}
/* \\\ */
