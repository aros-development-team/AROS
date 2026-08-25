/*
 *----------------------------------------------------------------------------
 *                Internal includes for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * Structure and naming follow rom/usb/poseidon/poseidon_intern.h so that the
 * two stacks can be read side by side.
 */

#ifndef _LIBRARIES_BLUETOOTH_H
#define _LIBRARIES_BLUETOOTH_H

#include <libraries/bluetooth.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <utility/hooks.h>

/* Configuration stuff

   Configurations are stored in an IFF structure managed by the prefs program
   and the stack itself; class drivers only inject, delete or read out their
   own sections. The layout mirrors poseidon.prefs:

   FORM BTLC/BTBC (Bluetooth config file)

[1*] FORM STKC (Stack config)
[n*]   FORM BHWD (Bluetooth hardware device)
         CHNK NAME [varlen] (exec device name)
         CHNK UNIT [4 bytes]
         CHNK OFFL (present if the hardware is not to be started)
[n*]   FORM BCLS (Bluetooth class driver)
         CHNK NAME [varlen] (library name)
       CHNK GCFG (struct BtGlobalCfg)

[n*] FORM CLSC (Class config)
       CHNK OWNR [varlen] name of class which stores this data
       FORM GCPD (global class private data) <- the form the class can modify

[n*] FORM DEVC (Device configurations, one per known remote device)
       CHNK DVID [varlen] DeviceID-String ("BT:xx:xx:xx:xx:xx:xx")
       CHNK NAME [varlen] Custom name
       CHNK FBND [varlen] <classname> (forced device binding)
       CHNK POPO (struct BtPoPoCfg)
       CHNK DREG (struct BtRegDevCfg, present iff the device is registered)
       CHNK KEYS (opaque bond key blob, present iff bonded)
[n*]   FORM DCFG (Device configuration data)
         CHNK OWNR [varlen] name of class for binding
         FORM DCPD (device config private data) <- the form the class can modify
[n*]   FORM SCFG (Service configuration data)
         CHNK OWNR [varlen] name of class for binding
         CHNK SVID [varlen] ServiceID-String
         CHNK FBND [varlen] <classname> (forced binding)
         FORM SCPD (service config private data) <- the form the class can modify

   The bt*Cfg*() calls behave exactly like their psd*Cfg*() counterparts.
*/

/* Private stuff starts here */

struct BtLockSem
{
    struct Node         bls_Node;         /* Linkage */
    BOOL                bls_Dead;         /* Has Semaphore been deactivated? */
    struct List         bls_WaitQueue;    /* List of waiting tasks (ReadLock structs) */
    struct List         bls_ReadLocks;    /* List of obtained shared locks */
    struct Task        *bls_Owner;        /* Current owner of exclusive lock */
    UWORD               bls_ExclLockCount; /* Exclusive lock count */
    UWORD               bls_SharedLockCount; /* Count of *different* shared lock owners */
};

struct BtReadLock
{
    struct Node         brl_Node;         /* Linkage */
    BOOL                brl_IsExcl;       /* Is this lock exclusive? */
    struct Task        *brl_Task;         /* Task waiting for or obtaining this lock */
    UWORD               brl_Count;        /* Shared lock count */
};

struct BtSemaInfo
{
    struct Node         bsi_Node;         /* Linkage */
    struct BtLockSem   *bsi_LockSem;      /* Pointer to semaphore */
};

struct BtBorrowLock
{
    struct Node         bbl_Node;         /* Linkage */
    UWORD               bbl_ExclLockCount; /* Was exclusive before */
    UWORD               bbl_Count;        /* Shared lock count */
    struct BtLockSem   *bbl_LockSem;      /* Pointer to semaphore */
    struct BtReadLock  *bbl_ReadLock;     /* Readlock that was changed */
};

struct BtHandlerTask
{
    struct Task        *bh_Task;          /* Event Handler Task */
    struct MsgPort     *bh_MsgPort;       /* Port for EventNote messages */
    LONG                bh_ReadySignal;   /* Signal to respond to task changes */
    struct Task        *bh_ReadySigTask;  /* task to signal */
    struct BtEventHook *bh_EventHandler;  /* Event handler */
    struct MsgPort     *bh_TimerMsgPort;  /* Port for timer requests */
    struct timerequest *bh_TimerIOReq;    /* Standard timer request */
};

/* PoPo-style popup GUI task, spawned on demand to show pairing requests
   from inside the library, the way poseidon.library's PoPo does for USB. */
struct BtPopupTask
{
    struct Task        *bp_Task;          /* the popup GUI task */
    struct MsgPort     *bp_Port;          /* port for pairing popup requests */
    LONG                bp_ReadySignal;
    struct Task        *bp_ReadySigTask;
    struct Library     *bp_MUIMasterBase; /* muimaster, opened by the task */
};

/* A pairing request handed to the popup task. */
struct BtPopupMsg
{
    struct Message      bpm_Msg;
    struct BtDevice    *bpm_Device;
    ULONG               bpm_Type;         /* BPRT_xxx */
    ULONG               bpm_Passkey;
};

struct BtWStringMap
{
    WORD   bsm_ID;
    STRPTR bsm_String;
};

struct BtUWStringMap
{
    UWORD  bsm_ID;
    STRPTR bsm_String;
};

struct BtULStringMap
{
    ULONG  bsm_ID;
    STRPTR bsm_String;
};

/* The library node - private
*/
struct BtBase
{
    struct Library      bt_Library;       /* standard */
    UWORD               bt_Flags;         /* various flags */
    struct UtilityBase *bt_UtilityBase;   /* for tags etc */
    struct Library     *bt_DosBase;       /* for dos stuff */
    BOOL                bt_StackInit;     /* Did we initialize the stack yet? */
    APTR                bt_MemPool;       /* Public Memory Pool */
    APTR                bt_SemaMemPool;   /* Memory Pool exclusively for Semaphore ReadLocks */
    struct List         bt_DeadlockDebug; /* linked list of semaphore allocations */
    struct BtLockSem    bt_Lock;          /* Library base lock */
    struct BtLockSem    bt_ConfigLock;    /* Config semaphore */
    struct timerequest  bt_TimerIOReq;    /* Standard timer request */
    struct List         bt_Hardware;      /* List of Hardware Interfaces in use */
    struct List         bt_Classes;       /* List of Classes loaded */
    struct List         bt_FirmwareLoaders; /* List of struct BtFirmwareLoader (plugins) */
    struct SignalSemaphore bt_FirmwareLock; /* Guards bt_FirmwareLoaders */
    struct List         bt_ErrorMsgs;     /* List of Error Msgs */
    struct List         bt_EventHooks;    /* List of EventHandlers */
    struct MsgPort      bt_EventReplyPort; /* Replyport for Events */
    struct List         bt_ConfigRoot;    /* Configuration FORMs */
    struct List         bt_AlienConfigs;  /* Configuration FORM from outer space */
    BOOL                bt_CfgChangeMute; /* Don't generate config changed events */
    struct SignalSemaphore bt_ReentrantLock; /* Lock for non-reentrant stuff */
    ULONG               bt_MemAllocated;  /* Bytes of memory allocated by stack */
    BOOL                bt_ConfigRead;    /* Has a config been loaded? */
    BOOL                bt_CheckConfigReq; /* Set to true, to check if config changed */
    BOOL                bt_SaveConfigReq; /* device registration/bond changed: write the config to disk */
    ULONG               bt_ConfigHash;    /* Last config hash value */
    ULONG               bt_SavedConfigHash; /* Hash sum of last saved config */
    struct BtGlobalCfg *bt_GlobalCfg;     /* Global Config structure */
    ULONG               bt_ReleaseVersion; /* Release Version for update info */
    ULONG               bt_OSVersion;     /* Internal OS Version descriptor */
    BOOL                bt_StartedAsTask; /* Did we start in Task Mode before DOS was available? */
    struct BtHandlerTask bt_EventHandler; /* Event handler */
    struct BtPopupTask   bt_Popup;        /* PoPo-style pairing popup task */
};

/* bt_Flags */
#define BTF_KLOG 0x0001

struct BtEventHook
{
    struct Node         beh_Node;         /* Node linkage */
    struct MsgPort     *beh_MsgPort;      /* Target message port */
    ULONG               beh_MsgMask;      /* Mask of messages to send */
};

struct BtEventNote
{
    struct Message      ben_Msg;          /* Intertask communication message */
    UWORD               ben_Event;        /* Event number as specified above */
    APTR                ben_Param1;       /* Parameter 1 for event */
    APTR                ben_Param2;       /* Parameter 2 */
};

struct BtEventNoteInternal
{
    struct Node         beni_Node;        /* Node linkage */
    struct BtEventNote  beni_EventNote;   /* Encapsulated BtEventNote */
};

struct BtErrorMsg
{
    struct Node         bem_Node;         /* Node linkage */
    struct BtBase      *bem_Base;         /* Uplinking */
    UWORD               bem_Level;        /* RC: 0=Note, 5=Warn, 10=Error, 20=Fail */
    STRPTR              bem_Origin;       /* From whom? */
    STRPTR              bem_Msg;          /* Actual error message */
    struct DateStamp    bem_DateStamp;    /* Date Stamp (if DOS available) */
};

struct BtIFFContext
{
    struct Node         bic_Node;         /* Node linkage */
    struct List         bic_SubForms;     /* All sub forms */
    ULONG               bic_FormID;       /* 4 bytes FORM ID */
    ULONG               bic_FormLength;   /* Length of form */
    ULONG              *bic_Chunks;       /* Chunks (no forms) */
    ULONG               bic_ChunksLen;    /* Total length of chunks */
    ULONG               bic_BufferLen;    /* size of buffer allocated */
};

struct BtClass
{
    struct Node         bc_Node;          /* Node linkage */
    struct BtBase      *bc_Base;          /* Uplinking */
    struct Library     *bc_ClassBase;     /* Library pointer */
    STRPTR              bc_ClassName;     /* Name of class */
    STRPTR              bc_FullPath;      /* Full path and class name */
    UWORD               bc_UseCnt;        /* Number of bindings in use */
    BOOL                bc_RemoveMe;      /* Class scheduled for removal */
};

struct BtAppBinding
{
    struct Node         bab_Node;         /* Node linkage */
    struct BtDevice    *bab_Device;       /* Uplinking */
    struct Hook        *bab_ReleaseHook;  /* CallBackHook for releasing binding */
    IPTR                bab_UserData;     /* User Data */
    struct Task        *bab_Task;         /* Task bound to */
    BOOL                bab_ForceRelease; /* Force release of other app or class bindings */
};

/* Flags for bth_Flags */
#define BTHF_CLASSIC        0x0001        /* BR/EDR supported */
#define BTHF_LE             0x0002        /* LE supported */
#define BTHF_DISCOVERING    0x0004        /* discovery running */
#define BTHF_DISCOVERABLE   0x0008        /* inquiry scan on */
#define BTHF_CONNECTABLE    0x0010        /* page scan on */
#define BTHF_REMOVEME       0x0100        /* scheduled for removal */
#define BTHF_FWLOADED       0x0200        /* firmware download done (or not needed) */
#define BTHF_FWPENDING      0x0400        /* firmware load requested, awaiting a loader */

#define BT_ADDRSTR_LEN      18            /* "xx:xx:xx:xx:xx:xx" + NUL */
#define BT_NAME_MAX         248           /* HCI local/remote name */

struct BtHardware
{
    struct Node         bth_Node;               /* Node linkage */
    struct BtBase      *bth_Base;               /* Uplinking */
    struct Task        *bth_ReadySigTask;       /* Task to send ready signal to */
    LONG                bth_ReadySignal;        /* Signal to send when ready */
    struct Task        *bth_Task;               /* Device task */
    STRPTR              bth_DevName;            /* Device name */
    ULONG               bth_Unit;               /* Unit number */

    STRPTR              bth_ProductName;        /* Product name */
    STRPTR              bth_Manufacturer;       /* Manufacturer name */
    STRPTR              bth_Description;        /* Description string */
    STRPTR              bth_Copyright;          /* Copyright string */
    UWORD               bth_Version;            /* Version of device */
    UWORD               bth_Revision;           /* Device revision */
    UWORD               bth_DriverVers;         /* Driver version */

    UWORD               bth_State;              /* BHS_xxx */
    UWORD               bth_Flags;              /* BTHF_xxx */
    BD_ADDR             bth_Address;            /* Local BD_ADDR */
    UBYTE               bth_AddrString[BT_ADDRSTR_LEN];
    STRPTR              bth_LocalName;          /* Local name */
    ULONG               bth_ClassOfDevice;      /* Local class of device */
    UWORD               bth_HCIVersion;
    UWORD               bth_HCIRevision;
    UWORD               bth_LMPVersion;
    UWORD               bth_LMPSubversion;
    UWORD               bth_ManufacturerID;
    UBYTE               bth_Features[8];        /* LMP features page 0 */
    UBYTE               bth_LEFeatures[8];      /* LE controller features (bit 6 of byte 0 = LE Secure Connections) */
    UWORD               bth_ACLMaxPktSize;
    UWORD               bth_ACLNumPkts;
    UWORD               bth_SCOMaxPktSize;
    UWORD               bth_SCONumPkts;
    UWORD               bth_LEACLMaxPktSize;
    UWORD               bth_LEACLNumPkts;
    BOOL                bth_RemoveMe;           /* Hardware scheduled for removal */
    ULONG               bth_ErrorCount;         /* transport errors */
    ULONG               bth_LastHCIError;       /* last HCI status */

    struct IOBTHCIReq  *bth_RootIOReq;          /* First IO Request (query/flush) */
    struct List         bth_Devices;            /* List of devices */
    struct List         bth_DeadDevices;        /* Devices being freed */
    ULONG               bth_NumDevices;
    struct MsgPort      bth_TaskMsgPort;        /* Channels and control messages in */
    struct MsgPort      bth_DevMsgPort;         /* IORequest replies from the HCI device */
    struct MsgPort      bth_EventMsgPort;       /* BTHCIEventMsg in */
    volatile ULONG      bth_MsgCount;           /* Number of IORequests pending at the driver */
    APTR                bth_Core;               /* struct BtHWCore (hwtask.c private) */
    ULONG               bth_DiscoveryEnd;       /* discovery deadline (secs), 0 = none */
};

/* Flags for bd_Flags */

#define BDFF_CLASSIC        0x0001
#define BDFF_LE             0x0002
#define BDFF_DISCOVERED     0x0004
#define BDFF_REGISTERED     0x0008
#define BDFF_BONDED         0x0010
#define BDFF_CONNECTED      0x0020
#define BDFF_ENCRYPTED      0x0040
#define BDFF_DEAD           0x0080
#define BDFF_SERVICESKNOWN  0x0100
#define BDFF_CONNECTING     0x0200
#define BDFF_APPBINDING     0x4000
#define BDFF_DELEXPUNGE     0x8000

#define BT_ADVDATA_MAX      62            /* adv + scan response */

/* Bond keys of a device (private; persisted as the KEYS chunk of its DEVC
   form, never handed out through btGetAttrs) */
struct BtKeyCfg
{
    ULONG bkc_ChunkID;                    /* IFFCHNK_KEYS */
    ULONG bkc_Length;                     /* sizeof(struct BtKeyCfg)-8 */
    UBYTE bkc_Flags;                      /* BKCF_xxx */
    UBYTE bkc_LinkKeyType;                /* BR/EDR link key type */
    UBYTE bkc_LinkKey[16];                /* BR/EDR link key */
    UBYTE bkc_LTK[16];                    /* LE long term key */
    UBYTE bkc_EDIV[2];
    UBYTE bkc_Rand[8];
    UBYTE bkc_IRK[16];
    UBYTE bkc_CSRK[16];
    UBYTE bkc_Reserved[4];
};

#define BKCF_LINKKEY 0x01
#define BKCF_LTK     0x02
#define BKCF_IRK     0x04
#define BKCF_CSRK    0x08
#define BKCF_SC      0x10                 /* LE secure connections key */

struct BtDevice
{
    struct Node         bd_Node;          /* Node linkage */
    struct BtHardware  *bd_Hardware;      /* Interfacing hardware */
    struct BtLockSem    bd_Lock;          /* Access locking */
    APTR                bd_DevBinding;    /* Device binding */
    struct BtClass     *bd_ClsBinding;    /* Which class has the bond? */
    UWORD               bd_UseCnt;        /* Usage counter */
    UWORD               bd_Flags;         /* BDFF_xxx */
    BD_ADDR             bd_Address;       /* Remote address */
    UBYTE               bd_AddrType;      /* BDAT_xxx */
    UBYTE               bd_Role;          /* BDR_xxx */
    UBYTE               bd_LinkType;      /* BDLT_xxx */
    UBYTE               bd_PairingState;  /* BDPS_xxx */
    UBYTE               bd_PairingRequest;/* BPRT_xxx */
    UBYTE               bd_AdvDataLen;
    UBYTE               bd_AddrString[BT_ADDRSTR_LEN];
    UWORD               bd_ConnHandle;    /* ACL connection handle */
    UWORD               bd_Appearance;    /* LE appearance */
    UWORD               bd_DeadCount;     /* Number of failures on the device */
    UWORD               bd_LMPVersion;
    UWORD               bd_ManufacturerID;
    UWORD               bd_VendorIDSource;
    UWORD               bd_VendorID;
    UWORD               bd_ProductID;
    UWORD               bd_ProductVersion;
    ULONG               bd_ClassOfDevice;
    ULONG               bd_PairingPasskey;
    LONG                bd_RSSI;          /* dBm, 127 unknown */
    STRPTR              bd_Name;          /* Name (custom?) */
    STRPTR              bd_OrigName;      /* Name reported by the device */
    STRPTR              bd_IDString;      /* Whole Device ID string */
    struct List         bd_Services;      /* List of services */
    ULONG               bd_NumServices;
    struct DateStamp    bd_FirstSeen;
    struct DateStamp    bd_LastSeen;
    BOOL                bd_IsNewToMe;     /* Whether the device is seen the first time */
    struct BtPoPoCfg    bd_PoPoCfg;       /* Inhibit PopUp and Class scan Config */
    UBYTE               bd_AdvData[BT_ADVDATA_MAX];
    struct BtKeyCfg     bd_Keys;          /* bond keys */
    struct BtHWConn    *bd_Conns[2];      /* per-bearer link state: [0]=BR/EDR, [1]=LE (hwconn.c private) */
    /* The address the peer is using right now when it differs from
       bd_Address: a bonded LE peer advertising from a resolvable private
       address that its IRK resolved to this device. Links are created to
       and matched against this address while bd_CurAddrValid. */
    UBYTE               bd_CurAddr[6];
    UBYTE               bd_CurAddrType;
    BOOL                bd_CurAddrValid;
};

struct BtService
{
    struct Node         bsv_Node;         /* Node linkage */
    struct BtDevice    *bsv_Device;       /* Up linkage */
    APTR                bsv_SvcBinding;   /* Service Binding */
    struct BtClass     *bsv_ClsBinding;   /* Which class has the bond? */
    BOOL                bsv_BindingInProgress; /* a class scan is binding this service; do not free it */
    UBYTE               bsv_UUID[16];     /* 128 bit UUID, big endian */
    UWORD               bsv_UUID16;       /* 16 bit UUID or 0 */
    UWORD               bsv_Protocol;     /* BSVP_xxx */
    UWORD               bsv_PSM;          /* L2CAP PSM */
    UWORD               bsv_RFCOMMChannel;      /* RFCOMM channel */
    UWORD               bsv_Version;      /* Profile version */
    UWORD               bsv_StartHandle;  /* GATT */
    UWORD               bsv_EndHandle;    /* GATT */
    UWORD               bsv_NumEPs;
    BOOL                bsv_IsPrimary;
    ULONG               bsv_RecordHandle; /* SDP record handle */
    STRPTR              bsv_Name;         /* Service name */
    STRPTR              bsv_IDString;     /* Service ID string */
    UWORD              *bsv_ServiceClassIDs; /* 0 terminated array of 16 bit ids */
    UBYTE              *bsv_HidDescriptor; /* SDP HIDDescriptorList report descriptor (classic HID), or NULL */
    UWORD               bsv_HidDescriptorLen;
    struct List         bsv_Endpoints;    /* List of endpoints */
};

struct BtHWConn;
struct BtHWEndpoint;

struct BtEndpoint
{
    struct Node         bep_Node;         /* Node linkage */
    struct BtService   *bep_Service;      /* Up linkage */
    UWORD               bep_Type;         /* BEPT_xxx */
    UWORD               bep_CanRead;      /* device -> host possible */
    UWORD               bep_CanWrite;     /* host -> device possible */
    UWORD               bep_PSM;
    UWORD               bep_CID;
    UWORD               bep_RFCOMMChannel;
    UWORD               bep_Handle;       /* GATT value handle */
    UWORD               bep_UUID16;
    UWORD               bep_Properties;
    UWORD               bep_MaxPktSize;   /* MTU */
    UWORD               bep_EndHandle;    /* GATT: last handle belonging to this characteristic */
    UWORD               bep_CCCDHandle;   /* GATT: Client Characteristic Configuration descriptor (0 = unknown) */
    UWORD               bep_RefHandle;    /* GATT: HID Report Reference descriptor (0 = none) */
    UWORD               bep_ReportID;     /* HID report id from the Report Reference */
    UWORD               bep_ReportType;   /* 1 input, 2 output, 3 feature */
    UWORD               bep_DescDone;     /* descriptors have been looked at */
    UBYTE               bep_UUID[16];
    STRPTR              bep_Name;
    struct BtHWEndpoint *bep_Chan;        /* open channel state (hwconn.c private) */
};

/* Flags for bch_Flags */
#define BCHF_AUTOCONNECT    0x0001
#define BCHF_NOWAIT         0x0002
#define BCHF_QUEUED         0x0100        /* internal: pending in the hardware task */
#define BCHF_ABORTED        0x0200        /* internal: abort requested */

/* Internal request codes for the default channel (bch_Request); public codes are
   BTPR_xxx from libraries/bluetooth.h */
#define BTPRI_BASE          0x8000
#define BTPRI_DISCOVERY     (BTPRI_BASE + 0x01) /* data = struct BtDiscoveryParams */
#define BTPRI_STOPDISCOVERY (BTPRI_BASE + 0x02)
#define BTPRI_CONNECT       (BTPRI_BASE + 0x03)
#define BTPRI_DISCONNECT    (BTPRI_BASE + 0x04)
#define BTPRI_PAIR          (BTPRI_BASE + 0x05) /* data = struct BtPairParams */
#define BTPRI_PAIRREPLY     (BTPRI_BASE + 0x06) /* data = struct BtPairParams */
#define BTPRI_UNPAIR        (BTPRI_BASE + 0x07)
#define BTPRI_REGISTER      (BTPRI_BASE + 0x08)
#define BTPRI_UNREGISTER    (BTPRI_BASE + 0x09)
#define BTPRI_ENUMSERVICES  (BTPRI_BASE + 0x0a)
#define BTPRI_SETLOCALNAME  (BTPRI_BASE + 0x0b) /* data = name */
#define BTPRI_SETSCANMODE   (BTPRI_BASE + 0x0c) /* val = discoverable, idx = connectable */
#define BTPRI_SETCOD        (BTPRI_BASE + 0x0d) /* data = &ULONG */
#define BTPRI_OPENCHANNEL   (BTPRI_BASE + 0x0e) /* endpoint channel: open channel */
#define BTPRI_CLOSECHANNEL  (BTPRI_BASE + 0x0f)

struct BtDiscoveryParams
{
    ULONG bdp_Duration;                   /* seconds */
    BOOL  bdp_Classic;
    BOOL  bdp_LE;
    BOOL  bdp_ResolveNames;
    BOOL  bdp_ClearOld;
};

struct BtPairParams
{
    BOOL  bpp_Bond;
    BOOL  bpp_MITM;
    UWORD bpp_IOCapability;
    STRPTR bpp_PINCode;
    ULONG bpp_Passkey;
    BOOL  bpp_Confirm;
    BOOL  bpp_HaveConfirm;
    BOOL  bpp_HavePasskey;
};

struct BtChannel
{
    struct Message      bch_Msg;           /* Intertask communication message */
    struct BtDevice    *bch_Device;        /* Up linkage */
    struct BtEndpoint  *bch_Endpoint;      /* Endpoint linkage or NULL for default channel */
    struct MsgPort     *bch_MsgPort;       /* Msg Port of task allocated channel */
    struct BtChannel      *bch_AbortChannel;     /* Channel to abort */
    struct BtHardware  *bch_Hardware;      /* Hardware (for hardware level requests) */
    UWORD               bch_Flags;         /* BCHF_xxx */
    UWORD               bch_Request;       /* default channel: BTPR_xxx / BTPRI_xxx */
    UWORD               bch_Value;
    UWORD               bch_Index;
    APTR                bch_Data;          /* transfer buffer */
    ULONG               bch_Length;        /* transfer length */
    ULONG               bch_Actual;        /* bytes transferred */
    LONG                bch_Error;         /* result */
    ULONG               bch_Timeout;       /* ms */
    struct MinNode      bch_QueueNode;     /* hardware task internal queue linkage */
    ULONG               bch_Deadline;      /* hardware task internal */
    APTR                bch_UserData;      /* caller private */
};

/* helpers shared inside the library */
void bAddrToStr(const UBYTE *addr, STRPTR buf);
BOOL bStrToAddr(CONST_STRPTR str, UBYTE *addr);
void bUUID16To128(UWORD uuid16, UBYTE *uuid128);
BOOL bUUID128To16(const UBYTE *uuid128, UWORD *uuid16);
void bUUIDToStr(const UBYTE *uuid128, STRPTR buf); /* buf >= 37 bytes */

#endif /* _LIBRARIES_BLUETOOTH_H */
