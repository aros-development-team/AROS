/*
 *----------------------------------------------------------------------------
 *        bluetooth.library: hardware task internals (hwtask.c, hwconn.c)
 *----------------------------------------------------------------------------
 */

#ifndef BLUETOOTH_HWTASK_H
#define BLUETOOTH_HWTASK_H

#include "bluetooth.library.h"

#include <bluetooth/hci.h>

#include <btcore/hci.h>
#include <btcore/buffer.h>
#include <btcore/command_queue.h>
#include <btcore/timer.h>
#include <btcore/transport.h>
#include <btcore/device_registry.h>
#include <btcore/l2cap.h>
#include <btcore/l2cap_channel.h>
#include <btcore/sdp.h>
#include <btcore/sdp_client.h>
#include <btcore/att.h>
#include <btcore/gatt_client.h>
#include <btcore/smp.h>
#include <btcore/smp_manager.h>

#define HC_NUMCMDREQS   2
#define HC_NUMACLREADS  4
#define HC_NUMACLWRITES 4
#define HC_ACLBUFSIZE   MAX_ACL_IN_BUFFER_SIZE
#define HC_TICK_MS      100
#define HC_MAXCONNS     8
#define HC_RXQUEUE_MAX  8      /* SDUs kept per L2CAP channel when no read request is pending */
#define HC_CHANCLOSE_MS 2000   /* grace period before an unused L2CAP channel is closed */

/* HCI opcodes */
#define HC_OP(ogf,ocf) BT_HCI_OPCODE(ogf, ocf)
#define HC_OP_INQUIRY                 HC_OP(0x01, 0x0001)
#define HC_OP_INQUIRY_CANCEL          HC_OP(0x01, 0x0002)
#define HC_OP_CREATE_CONNECTION       HC_OP(0x01, 0x0005)
#define HC_OP_DISCONNECT              HC_OP(0x01, 0x0006)
#define HC_OP_CREATE_CONN_CANCEL      HC_OP(0x01, 0x0008)
#define HC_OP_ACCEPT_CONN_REQ         HC_OP(0x01, 0x0009)
#define HC_OP_REJECT_CONN_REQ         HC_OP(0x01, 0x000a)
#define HC_OP_LINK_KEY_REQ_REPLY      HC_OP(0x01, 0x000b)
#define HC_OP_LINK_KEY_REQ_NEG_REPLY  HC_OP(0x01, 0x000c)
#define HC_OP_PIN_CODE_REQ_REPLY      HC_OP(0x01, 0x000d)
#define HC_OP_PIN_CODE_REQ_NEG_REPLY  HC_OP(0x01, 0x000e)
#define HC_OP_AUTH_REQUESTED          HC_OP(0x01, 0x0011)
#define HC_OP_SET_CONN_ENCRYPTION     HC_OP(0x01, 0x0013)
#define HC_OP_REMOTE_NAME_REQUEST     HC_OP(0x01, 0x0019)
#define HC_OP_READ_REMOTE_VERSION     HC_OP(0x01, 0x001d)
#define HC_OP_IO_CAP_REQ_REPLY        HC_OP(0x01, 0x002b)
#define HC_OP_USER_CONFIRM_REPLY      HC_OP(0x01, 0x002c)
#define HC_OP_USER_CONFIRM_NEG_REPLY  HC_OP(0x01, 0x002d)
#define HC_OP_USER_PASSKEY_REPLY      HC_OP(0x01, 0x002e)
#define HC_OP_USER_PASSKEY_NEG_REPLY  HC_OP(0x01, 0x002f)
#define HC_OP_IO_CAP_REQ_NEG_REPLY    HC_OP(0x01, 0x0034)
#define HC_OP_SET_EVENT_MASK          HC_OP(0x03, 0x0001)
#define HC_OP_RESET                   HC_OP(0x03, 0x0003)
#define HC_OP_WRITE_LOCAL_NAME        HC_OP(0x03, 0x0013)
#define HC_OP_READ_LOCAL_NAME         HC_OP(0x03, 0x0014)
#define HC_OP_WRITE_SCAN_ENABLE       HC_OP(0x03, 0x001a)
#define HC_OP_WRITE_CLASS_OF_DEVICE   HC_OP(0x03, 0x0024)
#define HC_OP_WRITE_INQUIRY_MODE      HC_OP(0x03, 0x0045)
#define HC_OP_WRITE_SIMPLE_PAIRING    HC_OP(0x03, 0x0056)
#define HC_OP_WRITE_LE_HOST_SUPPORT   HC_OP(0x03, 0x006d)
#define HC_OP_READ_LOCAL_VERSION      HC_OP(0x04, 0x0001)
#define HC_OP_READ_LOCAL_FEATURES     HC_OP(0x04, 0x0003)
#define HC_OP_READ_BUFFER_SIZE        HC_OP(0x04, 0x0005)
#define HC_OP_READ_BD_ADDR            HC_OP(0x04, 0x0009)
#define HC_OP_READ_RSSI               HC_OP(0x05, 0x0005)
#define HC_OP_LE_SET_EVENT_MASK       HC_OP(0x08, 0x0001)
#define HC_OP_LE_READ_BUFFER_SIZE     HC_OP(0x08, 0x0002)
#define HC_OP_LE_READ_LOCAL_FEATURES  HC_OP(0x08, 0x0003)
#define HC_OP_LE_SET_SCAN_PARAMETERS  HC_OP(0x08, 0x000b)
#define HC_OP_LE_SET_SCAN_ENABLE      HC_OP(0x08, 0x000c)
#define HC_OP_LE_RAND                 HC_OP(0x08, 0x0018)
#define HC_OP_LE_READ_LOCAL_P256      HC_OP(0x08, 0x0025)
#define HC_OP_LE_GENERATE_DHKEY       HC_OP(0x08, 0x0026)

#define HC_OP_LE_CREATE_CONNECTION    HC_OP(0x08, 0x000d)
#define HC_OP_LE_CREATE_CONN_CANCEL   HC_OP(0x08, 0x000e)
#define HC_OP_LE_START_ENCRYPTION     HC_OP(0x08, 0x0019)
#define HC_OP_LE_LTK_REQ_REPLY        HC_OP(0x08, 0x001a)
#define HC_OP_LE_LTK_REQ_NEG_REPLY    HC_OP(0x08, 0x001b)
#define HC_GIAC_LAP                   0x9E8B33

/* HCI events */
#define HC_EVT_INQUIRY_COMPLETE       0x01
#define HC_EVT_INQUIRY_RESULT         0x02
#define HC_EVT_CONN_COMPLETE          0x03
#define HC_EVT_CONN_REQUEST           0x04
#define HC_EVT_DISCONN_COMPLETE       0x05
#define HC_EVT_AUTH_COMPLETE          0x06
#define HC_EVT_REMOTE_NAME_COMPLETE   0x07
#define HC_EVT_ENCRYPTION_CHANGE      0x08
#define HC_EVT_ENCRYPTION_KEY_REFRESH 0x30
#define HC_EVT_READ_REMOTE_VERSION    0x0c
#define HC_EVT_HARDWARE_ERROR         0x10
#define HC_EVT_NUM_COMPLETED_PACKETS  0x13
#define HC_EVT_PIN_CODE_REQUEST       0x16
#define HC_EVT_LINK_KEY_REQUEST       0x17
#define HC_EVT_LINK_KEY_NOTIFICATION  0x18
#define HC_EVT_INQUIRY_RESULT_RSSI    0x22
#define HC_EVT_EXTENDED_INQUIRY_RESULT 0x2f
#define HC_EVT_IO_CAP_REQUEST         0x31
#define HC_EVT_IO_CAP_RESPONSE        0x32
#define HC_EVT_USER_CONFIRM_REQUEST   0x33
#define HC_EVT_USER_PASSKEY_REQUEST   0x34
#define HC_EVT_SIMPLE_PAIRING_COMPLETE 0x36
#define HC_EVT_USER_PASSKEY_NOTIFY    0x3b
#define HC_EVT_LE_META                0x3e
#define HC_LE_SUB_CONN_COMPLETE       0x01
#define HC_LE_SUB_ADV_REPORT          0x02
#define HC_LE_SUB_LTK_REQUEST         0x05
#define HC_LE_SUB_P256_COMPLETE       0x08
#define HC_LE_SUB_DHKEY_COMPLETE      0x09
#define HC_LE_SUB_ENH_CONN_COMPLETE   0x0a

/* bring-up steps */
enum {
    HCB_RESET = 0,
    HCB_READ_VERSION,
    HCB_FIRMWARE,           /* offer the controller to the pluggable firmware loaders */
    HCB_READ_FEATURES,
    HCB_READ_BUFFER_SIZE,
    HCB_READ_BD_ADDR,
    HCB_SET_EVENT_MASK,
    HCB_LE_SET_EVENT_MASK,
    HCB_LE_READ_BUFFER_SIZE,
    HCB_LE_READ_LOCAL_FEATURES,
    HCB_WRITE_LE_HOST_SUPPORT,
    HCB_WRITE_INQUIRY_MODE,
    HCB_WRITE_SIMPLE_PAIRING,
    HCB_WRITE_CLASS_OF_DEVICE,
    HCB_WRITE_LOCAL_NAME,
    HCB_READ_LOCAL_NAME,
    HCB_WRITE_SCAN_ENABLE,
    HCB_DONE
};

/* one queued outbound ACL packet (waiting for a credit or a free request) */
struct HCACLTx
{
    struct MinNode  tx_Node;
    UWORD           tx_Handle;
    UWORD           tx_Length;
    UBYTE           tx_Data[1];   /* variable */
};

/* one received SDU kept until a read request arrives */
struct HCRxSDU
{
    struct MinNode  rx_Node;
    UWORD           rx_Length;
    UBYTE           rx_Data[1];   /* variable */
};

/* per L2CAP channel / characteristic state, hangs off BtEndpoint bep_Chan */
struct BtHWEndpoint
{
    struct MinNode      hep_Node;      /* in cn_Endpoints */
    struct BtHWConn    *hep_Conn;
    struct BtEndpoint  *hep_Endpoint;
    UWORD               hep_State;     /* HEPS_xxx */
    UWORD               hep_LocalCID;  /* L2CAP */
    ULONG               hep_UseCnt;    /* BtChannel objects allocated on it */
    ULONG               hep_CloseTick; /* ms tick after which an unused channel closes */
    struct MinList      hep_ReadReqs;  /* BtChannel read requests */
    struct MinList      hep_RxQueue;   /* HCRxSDU */
    ULONG               hep_RxCount;
    struct MinList      hep_WriteReqs; /* BtChannel write requests waiting for open */
    BOOL                hep_CCCDWritten;
};

#define HEPS_CLOSED   0
#define HEPS_OPENING  1
#define HEPS_OPEN     2
#define HEPS_CLOSING  3

/* connection (ACL link) */
struct BtHWConn
{
    struct MinNode      cn_Node;       /* in hc_Conns */
    struct BtHWCore    *cn_Core;
    struct BtDevice    *cn_Device;
    UWORD               cn_Handle;
    UWORD               cn_State;      /* HCNS_xxx */
    UBYTE               cn_LinkType;   /* BDLT_ACL / BDLT_LE */
    UBYTE               cn_Role;
    UBYTE               cn_Encrypted;
    UBYTE               cn_Reason;     /* disconnect reason */
    ULONG               cn_Credits;    /* ACL packets in flight (for accounting) */
    struct bt_l2cap_channel_manager cn_L2CAP;
    struct bt_sdp_client cn_SDP;
    struct bt_gatt_client cn_GATT;
    BOOL                cn_SDPReady;
    BOOL                cn_GATTReady;
    struct MinList      cn_Endpoints;  /* BtHWEndpoint */
    struct MinList      cn_WaitReqs;   /* BtChannel requests waiting for the link */
    /* service enumeration */
    UWORD               cn_EnumState;
    BOOL                cn_Enumerated;  /* this bearer's services have been enumerated once */
    UWORD               cn_EnumIndex;
    UWORD               cn_EnumCount;
    ULONG               cn_EnumHandles[32];
    struct bt_gatt_service cn_Services[BT_GATT_CLIENT_MAX_SERVICES];
    struct BtChannel   *cn_EnumReq;    /* control request waiting for enumeration */
    struct BtEndpoint  *cn_EnumEP;     /* GATT endpoint whose descriptors are being looked at */
    BOOL                cn_CCCDBusy;     /* a CCCD write is in flight on the GATT client */
    /* control channel requests in flight on the SDP/GATT clients */
    struct BtChannel   *cn_CtrlReq;
    /* pairing */
    UWORD               cn_PairState;
    struct BtChannel   *cn_PairReq;
    /* LE pairing: btcore Security Manager, driven over fixed CID 6 */
    struct bt_smp_manager     cn_SMP;
    struct bt_smp_cmac_aes128 cn_SMPCmac;
    BOOL                cn_SMPActive;     /* manager initialised for a pairing in progress */
    BOOL                cn_SMPChanOpen;   /* fixed SMP channel registered on this link */
    BOOL                cn_EncryptPending;/* LE Start Encryption with the stored key in flight */
    UBYTE               cn_SMPRandWait;   /* LE Rand completions to collect before starting */
    UBYTE               cn_SMPKeySize;
    UBYTE               cn_SMPLTK[16];    /* key handed to LE Start Encryption (HCI byte order) */
    ULONG               cn_LastActivity;
    /* HCNS_CONNECTING without an HCI connect in flight: a class wants the
       (bonded, sleeping) LE peer and the background scan connects as soon as
       it advertises - no 10 s timeout, no controller time wasted paging. */
    BOOL                cn_WaitAdv;
};

#define HCNS_FREE       0
#define HCNS_CONNECTING 1
#define HCNS_CONNECTED  2
#define HCNS_DISCONNECTING 3

#define HC_SCANDIAG_MAX 96
struct BtScanDiag
{
    UBYTE  sd_Addr[6];
    UBYTE  sd_AddrType;    /* HCI address type (0 public, 1 random, 2/3 identity); 0xfe = BR/EDR */
    UBYTE  sd_Stable;      /* bt_le_addr_is_stable(): public or static random */
    UBYTE  sd_IsLE;
    UBYTE  sd_EvMask;      /* LE legacy: bit n = event_type n seen (0 ADV_IND 1 DIRECT 2 SCAN_IND 3 NONCONN 4 SCAN_RSP); classic: 0 std 1 rssi 2 eir */
    UBYTE  sd_ExtMask;     /* LE extended report event_type bits OR'd (bit0 conn bit1 scan bit2 dir bit3 scanrsp bit4 legacy) */
    UBYTE  sd_HasFlags;
    UBYTE  sd_Flags;       /* AD Flags value */
    UBYTE  sd_HID;
    UBYTE  sd_Noted;       /* reports that reached the device list (capped) */
    UBYTE  sd_Dropped;     /* reports discarded by the private-address rule (capped) */
    UWORD  sd_Count;
    UWORD  sd_Appearance;
    ULONG  sd_ADTypes;     /* bitmask of AD types < 32 seen across all reports */
    ULONG  sd_CoD;
    LONG   sd_RSSI;        /* last non-127 */
    char   sd_Name[32];
};

struct BtHWCore
{
    struct BtBase      *hc_Base;
    struct BtHardware  *hc_Hardware;
    struct Task        *hc_Task;

    struct bt_hci_transport hc_Transport;
    struct bt_timer_list hc_Timers;
    struct bt_cmdq      hc_CmdQ;

    struct IOBTHCIReq  *hc_CmdReq[HC_NUMCMDREQS];
    UBYTE               hc_CmdBuf[HC_NUMCMDREQS][BT_HCI_COMMAND_HEADER_LEN + BT_HCI_MAX_PARAM_LEN];
    BOOL                hc_CmdPending[HC_NUMCMDREQS];

    struct IOBTHCIReq  *hc_ACLReadReq[HC_NUMACLREADS];
    UBYTE              *hc_ACLReadBuf[HC_NUMACLREADS];
    BOOL                hc_ACLReadPending[HC_NUMACLREADS];

    struct IOBTHCIReq  *hc_ACLWriteReq[HC_NUMACLWRITES];
    UBYTE              *hc_ACLWriteBuf[HC_NUMACLWRITES];
    BOOL                hc_ACLWritePending[HC_NUMACLWRITES];
    struct MinList      hc_ACLTxQueue;    /* HCACLTx waiting */
    ULONG               hc_ACLCredits;    /* BR/EDR buffers free at the controller */
    ULONG               hc_LEACLCredits;  /* LE buffers free */

    struct IOBTHCIReq  *hc_CtlReq;        /* ADDMSGPORT / REMMSGPORT / FLUSH */
    BOOL                hc_EventPortAdded;

    struct MsgPort     *hc_TimerPort;
    struct timerequest *hc_TimerReq;
    BOOL                hc_TimerPending;
    ULONG               hc_Tick;          /* ms since start */

    UWORD               hc_BringupStep;
    BOOL                hc_BringupDone;
    BOOL                hc_BringupFailed;
    BOOL                hc_Shutdown;

    /* firmware-loader hook: HCB_FIRMWARE sets hc_FirmwarePending so the hwtask
       main loop runs the (synchronous) loader outside event processing. */
    BOOL                hc_FirmwarePending;
    BOOL                hc_Reinit;        /* bring-up re-run after a late firmware load */
    /* entropy for SMP: controller LE Rand output whitening a seeded PRNG */
    ULONG               hc_RandState[4];
    UBYTE               hc_RandPool[64];
    UWORD               hc_RandAvail;
    UWORD               hc_RandRequests;
    /* one in-flight synchronous HCI command (bHciDoSync / firmware loaders) */
    BOOL                hc_SyncDone;
    BOOL                hc_SyncOK;        /* command completed (vs timeout/send error) */
    UBYTE               hc_SyncStatus;    /* HCI status byte */
    UBYTE              *hc_SyncResp;      /* caller's return-param buffer, or NULL */
    UWORD               hc_SyncRespMax;
    UWORD               hc_SyncRespLen;

    /* discovery */
    BOOL                hc_InqActive;
    BOOL                hc_LEScanActive;
    BOOL                hc_ResolveNames;
    BOOL                hc_DiscoveryPending; /* discovery requested, commands in flight */
    struct bt_timer     hc_DiscoveryTimer;

    /* background LE scan: a low duty passive scan that runs whenever a
       bonded LE device with auto-connect is not connected, so a keyboard
       waking up (advertising) is reconnected without anyone asking. Off
       during discovery and while an outgoing connect is in flight. */
    BOOL                hc_BgScanActive;
    struct bt_timer     hc_BgScanTimer;      /* re-arm hysteresis after link changes */
    ULONG               hc_BgScanCheckTick;  /* periodic re-evaluation (hc_Tick based) */

    /* scan diagnostics (conclusive "why is nothing found" instrumentation) */
    ULONG               hc_DiagAdvLegacy;    /* LE advertising reports parsed (subevent 0x02) */
    ULONG               hc_DiagAdvExt;       /* LE extended advertising reports parsed (0x0D) */
    ULONG               hc_DiagAdvOther;     /* LE Meta events of some other subevent */
    ULONG               hc_DiagInqResults;   /* classic inquiry results seen */
    ULONG               hc_DiagLESubeventMask; /* bitmask of LE Meta subevent codes seen this run */
    ULONG               hc_DiagScanRsp;      /* LE scan responses received (active scan working) */
    ULONG               hc_DiagDropped;      /* reports discarded by the private-address rule */
    /* per-address journal of everything the radio reported this run, dumped to
       SYS:BluetoothScan.log when discovery finishes ("btdebug" boot argument only) */
    struct BtScanDiag   hc_ScanDiag[HC_SCANDIAG_MAX];
    UWORD               hc_ScanDiagCount;
    ULONG               hc_ScanDiagOverflow; /* reports from addresses beyond the table */

    /* remote name requests */
    struct MinList      hc_NameChannels;   /* BtChannel queue (bch_QueueNode) */
    struct BtDevice    *hc_NameReqDev;     /* device of the request in flight */
    struct BtChannel   *hc_NameReqChannel; /* client channel of that request or NULL */
    struct MinList      hc_NameQueue;      /* devices queued for discovery name resolution */
    ULONG               hc_NameQueueCount;

    /* connections */
    struct MinList      hc_Conns;          /* BtHWConn */
    struct BtHWConn    *hc_Connecting;     /* outgoing connection in progress (one at a time) */
    struct BtDevice    *hc_LEConnecting;
};

/* queue node for discovery name resolution */
struct HCNameNode
{
    struct MinNode   nn_Node;
    struct BtDevice *nn_Device;
};

/* hwtask.c */
uint64_t bNowUS(struct BtHWCore *hc);
BOOL bSubmitCmd(struct BtHWCore *hc, UWORD opcode, const UBYTE *params, UBYTE len,
                bt_cmdq_complete_fn cb, void *user);
void bIgnoreCompletion(struct bt_cmdq_completion *completion, void *user_data);
/* hwconn.c: ask the controller for LE Rand entropy; returns commands submitted */
ULONG bConnRequestEntropy(struct BtHWCore *hc, ULONG count);
/* synchronous HCI command used by firmware loaders (hwtask.c) */
LONG bHciDoSync(struct BtHWCore *hc, UWORD opcode, CONST_APTR params, UWORD plen,
                UBYTE *status, UBYTE *resp, UWORD *resplen, UWORD respmax);
/* firmware.c: offer the controller to the registered firmware loaders;
   TRUE if a loader downloaded firmware (the controller restarted on it) */
BOOL bDoFirmware(struct BtHWCore *hc);
struct BtDevice * bFindDeviceByAddr(struct BtHWCore *hc, const UBYTE *addr);
LONG bStopDiscovery(struct BtHWCore *hc);
void bReplyChannel(struct BtBase *BluetoothBase, struct BtChannel *bch, LONG error, ULONG actual);
void bStartACLWrite(struct BtHWCore *hc);
/* background LE scan (hwtask.c): stop it before an LE connect/scan command,
   re-evaluate (now / after a short delay) when links or registrations change */
void bBgScanStop(struct BtHWCore *hc);
void bBgScanUpdate(struct BtHWCore *hc);
void bBgScanSchedule(struct BtHWCore *hc);

/* hwconn.c */
/* a bonded/registered LE device is advertising: connect to it when a class
   is waiting for it or its auto-connect policy says so */
void bConnAdvertising(struct BtHWCore *hc, struct BtDevice *bd);
void bConnInit(struct BtHWCore *hc);
void bConnShutdown(struct BtHWCore *hc);
BOOL bConnHandleEvent(struct BtHWCore *hc, UBYTE code, const UBYTE *params, ULONG len);
void bConnHandleACL(struct BtHWCore *hc, const UBYTE *data, ULONG len);
BOOL bConnHandleRequest(struct BtHWCore *hc, struct BtChannel *bch);
BOOL bConnAbortRequest(struct BtHWCore *hc, struct BtChannel *victim);
void bConnTick(struct BtHWCore *hc);
void bConnDeviceGone(struct BtHWCore *hc, struct BtDevice *bd);
int  bSendACLPacket(struct bt_hci_transport *transport, const uint8_t *data, size_t length);

#endif /* BLUETOOTH_HWTASK_H */
