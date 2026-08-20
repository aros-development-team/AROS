/*
 * vbthci.device -- virtual Bluetooth HCI radio for bluetooth.library.
 *
 * Implements the bluetoothhci.device command set (devices/bluetoothhci.h)
 * on top of a simulated dual-mode controller with a handful of fake remote
 * devices, so that the stack, its shell commands, the prefs program and the
 * classes can be exercised without real hardware.
 */

#ifndef VBTHCI_INTERN_H
#define VBTHCI_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <devices/bluetoothhci.h>

#define VBTHCI_NUMUNITS 1

struct VBTHCIUnit;

struct VBTHCIBase
{
    struct Device        vb_Device;
    struct VBTHCIUnit   *vb_Units[VBTHCI_NUMUNITS];
    struct SignalSemaphore vb_Lock;
};

/* simulated remote device */
struct VBTFakeDevice
{
    UBYTE  fd_Addr[6];        /* wire order (LSB first) */
    UBYTE  fd_AddrType;       /* 0 public, 1 random */
    UBYTE  fd_IsLE;           /* primary bearer is LE */
    UBYTE  fd_DualMode;       /* dual-mode: reachable on BOTH BR/EDR and LE (same address) */
    ULONG  fd_CoD;            /* classic Class-of-Device (inquiry) */
    UWORD  fd_Appearance;     /* LE appearance (advertising) */
    UWORD  fd_ServiceUUID;    /* 16 bit UUID advertised */
    CONST_STRPTR fd_Name;
    BYTE   fd_RSSI;
};

/* which bearers a fake device is reachable on */
#define VBT_HASCLASSIC(fd)  (!(fd)->fd_IsLE || (fd)->fd_DualMode)
#define VBT_HASLE(fd)       ((fd)->fd_IsLE || (fd)->fd_DualMode)

/* a scheduled event */
struct VBTTimedEvent
{
    struct MinNode te_Node;
    ULONG          te_Due;     /* ms tick when due */
    UWORD          te_Kind;    /* VBTE_xxx */
    UWORD          te_Index;   /* fake device index or repeat counter */
    UWORD          te_Arg;
    UBYTE          te_Addr[6]; /* remote name requests for unknown devices */
};

#define VBTE_INQUIRY_RESULT   1
#define VBTE_INQUIRY_COMPLETE 2
#define VBTE_LE_ADV_REPORT    3
#define VBTE_REMOTE_NAME      4
#define VBTE_CONN_COMPLETE    5   /* te_Index = fake device, te_Arg = link */
#define VBTE_DISCONN_COMPLETE 6   /* te_Index = link */
#define VBTE_HID_REPORT       7   /* te_Index = link */
#define VBTE_PAIR_STEP        8   /* te_Index = link, te_Arg = step */
#define VBTE_LE_NOTIFY        9   /* te_Index = link */

#define VBT_MAXLINKS   4
#define VBT_MAXCHANS   6
#define VBT_RXBUFSIZE  1100

/* simulated L2CAP channel on the peer side */
struct VBTChan
{
    UWORD  lc_State;      /* 0 free, 1 configuring, 2 open */
    UWORD  lc_PSM;
    UWORD  lc_LocalCID;   /* the peer's endpoint */
    UWORD  lc_RemoteCID;  /* the host's endpoint */
    UWORD  lc_ConfIn;     /* host configured us */
    UWORD  lc_ConfOut;    /* we configured host */
};

/* simulated ACL link */
struct VBTLink
{
    BOOL   ln_Used;
    UWORD  ln_Handle;
    UWORD  ln_DevIdx;
    BOOL   ln_LE;
    BOOL   ln_Encrypted;
    UWORD  ln_PairStep;
    struct VBTChan ln_Chans[VBT_MAXCHANS];
    UBYTE  ln_NextCID;
    UBYTE  ln_RxBuf[VBT_RXBUFSIZE];
    UWORD  ln_RxLen;
    UWORD  ln_RxExpected;
    UWORD  ln_MTU;         /* ATT MTU */
    BOOL   ln_Notify;      /* CCCD of the HID report enabled */
    UBYTE  ln_KeyPhase;
    UBYTE  ln_MouseStep;
};

/* ACL data waiting for a READACL request */
struct VBTACLData
{
    struct MinNode ad_Node;
    UWORD          ad_Length;
    UBYTE          ad_Data[1];
};

struct VBTHCIUnit
{
    struct Unit          vu_Unit;          /* message port for BeginIO */
    struct VBTHCIBase   *vu_Base;
    ULONG                vu_UnitNo;
    struct Task         *vu_Task;
    struct Task         *vu_ReadySigTask;
    LONG                 vu_ReadySignal;
    BOOL                 vu_Open;
    BOOL                 vu_Shutdown;
    struct MsgPort      *vu_EventPort;     /* client's port (BTCMD_ADDMSGPORT) */
    struct MsgPort      *vu_EventReplyPort;
    ULONG                vu_EventsPending;
    struct MsgPort      *vu_TimerPort;
    struct timerequest  *vu_TimerReq;
    BOOL                 vu_TimerPending;
    ULONG                vu_Tick;          /* ms since start */
    struct MinList       vu_ReadQueue;     /* pending BTCMD_READACL */
    struct MinList       vu_Timed;         /* VBTTimedEvent list */
    struct MinList       vu_ACLToHost;     /* VBTACLData */
    struct VBTLink       vu_Links[VBT_MAXLINKS];
    UWORD                vu_NextHandle;
    UBYTE                vu_LinkKeys[8][16]; /* per fake device */
    UBYTE                vu_HasLinkKey[8];
    /* controller state */
    BOOL                 vu_Inquiring;
    BOOL                 vu_LEScanning;
    UBYTE                vu_ScanEnable;
    UBYTE                vu_LocalName[248];
    ULONG                vu_CoD;
    UBYTE                vu_EventMask[8];
};

void vbthci_UnitTask(void);
LONG vbthci_QueueRequest(struct VBTHCIUnit *unit, struct IOBTHCIReq *ioreq);

/* vbthci_unit.c helpers used by the peer simulation */
void vbt_SendEvent(struct VBTHCIUnit *unit, UBYTE code, const UBYTE *params, ULONG len);
void vbt_CommandComplete(struct VBTHCIUnit *unit, UWORD opcode, const UBYTE *rp, ULONG rplen);
void vbt_CommandStatus(struct VBTHCIUnit *unit, UWORD opcode, UBYTE status);
void vbt_Schedule(struct VBTHCIUnit *unit, UWORD kind, UWORD index, UWORD arg, ULONG delayms);
void vbt_CancelKind(struct VBTHCIUnit *unit, UWORD kind);
ULONG vbt_NumFakeDevices(void);
const struct VBTFakeDevice *vbt_FakeDevice(ULONG idx);

/* vbthci_peer.c */
BOOL vbtp_HandleCommand(struct VBTHCIUnit *unit, UWORD opcode, const UBYTE *p, UBYTE plen);
void vbtp_HandleACL(struct VBTHCIUnit *unit, const UBYTE *data, ULONG len);
void vbtp_Timed(struct VBTHCIUnit *unit, struct VBTTimedEvent *te);
void vbtp_DeliverReads(struct VBTHCIUnit *unit);
void vbtp_Reset(struct VBTHCIUnit *unit);

#endif /* VBTHCI_INTERN_H */
