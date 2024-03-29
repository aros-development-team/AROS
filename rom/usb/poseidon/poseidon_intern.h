/****************************************************************************

                 __   __                                    V/\V.       /\
                |" | |" |                                   mMMnw,      || []
                |  | |  |                                  (o  o)W   () || ||
                |__|_|_"|                                  | /  |Mw  || ||//
                ("  "  \|                                  \ -'_/mw   \\||/
                 \______)                                   ~%%/WM"    \||
 _____    ___     ______  _____  __  _____     ___  __  __/~~__ ~~\    _||
|"("  \()/\" \ ()/"_    )|"(___) ) )|"("  \ ()/\" \(__)/" ) /" ) " \  /_)O
|  )   )/" \  \ (_/"\__/ |  )_  ( ( |  )_  ) /" \  \  /  /|/  / �\  \/ ,|O
| (___/(  (_\__) _\  \_  | (__)  ) )| (__) |(  (_\__)/  /"/  /   |\   '_|O
|  |  _ \  /  / /" \_/ ) | ")__ ( ( |  )"  ) \  /  //  /|/  / . .|/\__/ ||
|__| (_) \/__/ (______/  |_(___) )_)|_(___/ . \/__/(__/ (__/ .:.:|      ||
                 _____
                |" __ \  Poseidon -- The divine USB stack for Amiga computers
                | (__) ) Version: 4.4 (06.04.23)
                |  __ (  Designed and written by
                |"(__) )   Chris Hodges <chrisly@platon42.de>
                |_____/  Copyright �2002-2009 Chris Hodges. All rights reserved.

 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 *                  Internal includes for poseidon.library
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 *
 */

#ifndef _LIBRARIES_POSEIDON_H
#define _LIBRARIES_POSEIDON_H

#if defined(__AROS__)
#include <oop/oop.h>
#endif

#include <libraries/poseidon.h>

/* Configuration stuff */

/* How this works:
   Configurations are stored in an IFF structure. It is managed by Trident
   and the stack itself. Class drivers may only inject, delete or read out
   certain sections of the configuration data. They either get pointer to the
   FORM sections or return pointers to this FORM. The form data has to be
   copied in each case (the length is given in the form).

   The configuration normally looks like this:
   FORM PSDC (Poseidon Config file)

[1*] FORM STKC (Stack config)
[n*]   FORM UDEV (USB hardware device)
         CHNK NAME [varlen] (device name)
         CHNK UNIT [4 bytes]
         [...]
[n*]   FORM UCLS (USB class driver)
         CHNK NAME [varlen] (library name)
         [...]
       [Chunks/Forms] (chunks containing some more data for the main stack file)

[n*] FORM CLSC (Class config)
       CHNK OWNR [varlen] name of class which stores this data
       FORM GCPD (global class private data) <- that's the form the class can modify
         [...]

[n*] FORM DEVC (Device configurations)
       CHNK DVID [varlen] DeviceID-String
       CHNK FBND [varlen] <classname> (forced binding)
       CHNK NAME [varlen] Custom name
[n*]   FORM DCFG (Device configuration data)
         CHNK OWNR [varlen] name of class for binding (AppName?)
         FORM DCPD (device config private data) <- that's the form the class can modify
           [...]
[n*]   FORM ICFG (Interface configuration data)
         CHNK OWNR [varlen] name of class for binding
         CHNK IFID [varlen] InterfaceID-String
         CHNK FBND [varlen] <classname> (forced binding)
         FORM ICPD (interface config private data) <- that's the form the class can modify

    psdReadCfg(pic, formdata):
        pic == NULL; replace (!) root form with given formdata
        pic != NULL; replace given pic with new form data.
    psdWriteCfg(pic)
        pic == NULL; generate the whole configuration buffer to save
        pic != NULL; generate the form buffer of the pic and its subform.
    psdFindCfgForm(pic, formid)
        pic == NULL -> pic = root form;
        find and return pic with given form ID or NULL if no such form exists.
    psdNextCfgForm(pic)
        get next form pic with same ID or return NULL if this was the only form with this ID.

    psdRemCfgForm(pic)
        pic == NULL -> pic = root form;
        delete form pic (effectly calls pFreeForm()).

    psdAddCfgChunk(pic, formdata)
        pic == NULL -> pic = root form;
        add the form OR CHUNK to the pic context form. Do not replace existing forms.
        Replaces existing chunks.

    psdRemCfgChunk(pic, chnkid)
        pic == NULL -> pic = root form;
        delete the chunk from the pic context form.

*/

/* Private stuff starts here */

#if defined(__AROS__)
struct USBController
{
    struct Node		  uc_Node;
};

struct USBDevice
{
    struct Node		  uc_Node;
};
#endif

struct PsdLockSem
{
    struct Node	        pls_Node;         /* Linkage */
    BOOL                pls_Dead;         /* Has Semaphore been deactivated? */
    struct List         pls_WaitQueue;    /* List of waiting tasks (ReadLock structs) */
    struct List         pls_ReadLocks;    /* List of obtained shared locks */
    struct Task        *pls_Owner;        /* Current owner of exclusive lock */
    UWORD               pls_ExclLockCount; /* Exclusive lock count */
    UWORD               pls_SharedLockCount; /* Count of *different* shared lock owners */
};

struct PsdReadLock
{
    struct Node         prl_Node;         /* Linkage */
    BOOL                prl_IsExcl;       /* Is this lock exclusive? */
    struct Task        *prl_Task;         /* Task waiting for or obtaining this lock */
    UWORD               prl_Count;        /* Shared lock count */
};

struct PsdSemaInfo
{
    struct Node         psi_Node;         /* Linkage */
    struct PsdLockSem  *psi_LockSem;      /* Pointer to semaphore */
};

struct PsdBorrowLock
{
    struct Node         pbl_Node;         /* Linkage */
    UWORD               pbl_ExclLockCount; /* Was exclusive before */
    UWORD               pbl_Count;        /* Shared lock count */
    struct PsdLockSem  *pbl_LockSem;      /* Pointer to semaphore */
    struct PsdReadLock *pbl_ReadLock;     /* Readlock that was changed */
};

struct PsdPoPo
{
    struct Task        *po_Task;          /* PoPo Task */
    struct MsgPort     *po_MsgPort;       /* Port for EventNote messages */
    LONG                po_ReadySignal;   /* Signal to respond to task changes */
    struct Task        *po_ReadySigTask;  /* task to signal */
    struct Library     *po_IntBase;       /* Intuition base for PoPo Task */
    struct Library     *po_MUIBase;       /* MUI base for PoPo Task */
    struct Library     *po_DTBase;        /* DataTypes base for PoPo Task */
    struct MsgPort     *po_TimerMsgPort;  /* Standard timer MsgPort */
    struct timerequest *po_TimerIOReq;    /* Standard timer request */
    struct MUI_CustomClass *po_PoPoClass; /* PoPo Action Class */
    ULONG              *po_PoPoObj;       /* PoPo Action Object */
    ULONG              *po_AppObj;        /* App Object */
    ULONG              *po_WindowObj;     /* Window Object */
    ULONG              *po_GroupObj;      /* Group Object */
    ULONG              *po_SaveObj;       /* Save Button Object */
    ULONG              *po_CloseObj;      /* Close Button Object */
    ULONG              *po_StickyObj;     /* Sticky Object */
    ULONG              *po_AboutMI;       /* About MenuItem */
    ULONG              *po_CloseMI;       /* Close MenuItem */
    ULONG              *po_TridentMI;     /* Trident MenuItem */
    ULONG              *po_MUIPrefsMI;    /* MUI Prefs MenuItem */
    struct PsdEventHook *po_EventHandler; /* Event handler */
    struct List         po_GadgetList;    /* List of gadgets for a device */
    BOOL                po_OpenRequest;   /* open window requested */
    BOOL                po_Sticky;        /* sticky entries */
    STRPTR              po_InsertSndFile; /* Path to insertion sound file */
    STRPTR              po_RemoveSndFile; /* Path to removal sound file */
    struct List         po_Sounds;        /* List of loaded soundfiles */
};

struct PsdHandlerTask
{
    struct Task        *ph_Task;          /* Event Handler Task */
    struct MsgPort     *ph_MsgPort;       /* Port for EventNote messages */
    LONG                ph_ReadySignal;   /* Signal to respond to task changes */
    struct Task        *ph_ReadySigTask;  /* task to signal */
    struct PsdEventHook *ph_EventHandler; /* Event handler */
    struct MsgPort     *ph_TimerMsgPort;  /* Port for timer requests */
    struct timerequest *ph_TimerIOReq;    /* Standard timer request */
};

struct PsdWStringMap
{
    WORD   psm_ID;
    STRPTR psm_String;
};

struct PsdUWStringMap
{
    UWORD  psm_ID;
    STRPTR psm_String;
};

struct PsdULStringMap
{
    ULONG  psm_ID;
    STRPTR psm_String;
};

/* The library node - private
*/
struct PsdBase
{
    struct Library      ps_Library;       /* standard */
    UWORD               ps_Flags;         /* various flags */
    struct UtilityBase *ps_UtilityBase;   /* for tags etc */
    struct Library     *ps_DosBase;       /* for dos stuff */
    BOOL                ps_StackInit;     /* Did we initialize the stack yet? */
    APTR                ps_MemPool;       /* Public Memory Pool */
    APTR                ps_SemaMemPool;   /* Memory Pool exclusively for Semaphore ReadLocks */
    struct List         ps_DeadlockDebug; /* linked list of semaphore allocations */
    struct PsdLockSem   ps_Lock;          /* PBase lock */
    struct PsdLockSem   ps_ConfigLock;    /* Config semaphore */
    struct timerequest  ps_TimerIOReq;    /* Standard timer request */
    struct List         ps_Hardware;      /* List of Hardware Interfaces in use */
    struct List         ps_Classes;       /* List of Classes loaded */
    struct List         ps_ErrorMsgs;     /* List of Error Msgs */
    struct List         ps_EventHooks;    /* List of EventHandlers */
    struct MsgPort      ps_EventReplyPort; /* Replyport for Events */
    struct List         ps_ConfigRoot;    /* Configuration FORMs */
    struct List         ps_AlienConfigs;  /* Configuration FORM from outer space */
    BOOL                ps_CfgChangeMute; /* Don't generate config changed events */
    struct SignalSemaphore ps_ReentrantLock; /* Lock for non-reentrant stuff */
    struct SignalSemaphore ps_PoPoLock;   /* Lock for non-reentrant stuff */
    ULONG               ps_MemAllocated;  /* Bytes of memory allocated by stack */
    UWORD               ps_FunnyCount;    /* Funny Message Counter */
    BOOL                ps_ConfigRead;    /* Has a config been loaded? */
    BOOL                ps_CheckConfigReq; /* Set to true, to check if config changed */
    ULONG               ps_ConfigHash;    /* Last config hash value */
    ULONG               ps_SavedConfigHash; /* Hash sum of last saved config */
    struct PsdGlobalCfg *ps_GlobalCfg;    /* Global Config structure */
    struct PsdPoPo      ps_PoPo;
    ULONG               ps_ReleaseVersion; /* Release Version for update info */
    ULONG               ps_OSVersion;     /* Internal OS Version descriptor */
    BOOL                ps_StartedAsTask; /* Did we start in Task Mode before DOS was available? */
    struct PsdHandlerTask ps_EventHandler; /* Event handler */
#if defined(__AROS__)
    OOP_Class	       *ps_ContrClass;
    OOP_Class	       *ps_DevClass;
#endif
};

/* ps_Flags */
#define PSF_KLOG 0x0001

struct PsdEventHook
{
    struct Node         peh_Node;         /* Node linkage */
    struct MsgPort     *peh_MsgPort;      /* Target message port */
    ULONG               peh_MsgMask;      /* Mask of messages to send */
};

struct PsdEventNote
{
    struct Message      pen_Msg;          /* Intertask communication message */
    UWORD               pen_Event;        /* Event number as specified above */
    APTR                pen_Param1;       /* Parameter 1 for event */
    APTR                pen_Param2;       /* Parameter 2 */
};

struct PsdEventNoteInternal
{
    struct Node         peni_Node;        /* Node linkage */
    struct PsdEventNote peni_EventNote;   /* Encapsulated PsdEventNote */
};

struct PsdErrorMsg
{
    struct Node         pem_Node;         /* Node linkage */
    struct PsdBase     *pem_Base;         /* Uplinking */
    UWORD               pem_Level;        /* RC: 0=Note, 5=Warn, 10=Error, 20=Fail */
    STRPTR              pem_Origin;       /* From whom? */
    STRPTR              pem_Msg;          /* Actual error message */
    struct DateStamp    pem_DateStamp;    /* Date Stamp (if DOS available) */
};

struct PsdIFFContext
{
    struct Node         pic_Node;         /* Node linkage */
    //struct PsdIFFContext *pic_Parent;     /* Uplinking */
    struct List         pic_SubForms;     /* All sub forms */
    ULONG               pic_FormID;       /* 4 bytes FORM ID */
    ULONG               pic_FormLength;   /* Length of form */
    ULONG              *pic_Chunks;       /* Chunks (no forms) */
    ULONG               pic_ChunksLen;    /* Total length of chunks */
    ULONG               pic_BufferLen;    /* size of buffer allocated */
};

struct PsdUsbClass
{
    struct Node         puc_Node;         /* Node linkage */
    struct PsdBase     *puc_Base;         /* Uplinking */
    struct Library     *puc_ClassBase;    /* Library pointer */
    STRPTR              puc_ClassName;    /* Name of class */
    STRPTR              puc_FullPath;     /* Full path and class name */
    UWORD               puc_UseCnt;       /* Number of bindings in use */
    BOOL                puc_RemoveMe;     /* Class scheduled for removal */
};

struct PsdAppBinding
{
    struct Node         pab_Node;         /* Node linkage */
    struct PsdDevice   *pab_Device;       /* Uplinking */
    struct Hook        *pab_ReleaseHook;  /* CallBackHook for releasing binding */
    IPTR                pab_UserData;     /* User Data */
    struct Task        *pab_Task;         /* Task bound to */
    BOOL                pab_ForceRelease; /* Force release of other app or class bindings */
};

struct PsdHardware
{
    struct Node         phw_Node;         /* Node linkage */
    struct PsdBase     *phw_Base;         /* Uplinking */
    struct Task        *phw_ReadySigTask; /* Task to send ready signal to */
    LONG                phw_ReadySignal;  /* Signal to send when ready */
    struct Task        *phw_Task;         /* Device task */
    STRPTR              phw_DevName;      /* Device name */
    ULONG               phw_Unit;         /* Unit number */

    STRPTR              phw_ProductName;  /* Product name */
    STRPTR              phw_Manufacturer; /* Manufacturer name */
    STRPTR              phw_Description;  /* Description string */
    STRPTR              phw_Copyright;    /* Copyright string */
    UWORD               phw_Version;      /* Version of device */
    UWORD               phw_Revision;     /* Device revision */
    UWORD               phw_DriverVers;   /* Driver version */
    ULONG               phw_Capabilities; /* Driver/HW capabilities */

    struct IOUsbHWReq  *phw_RootIOReq;    /* First IO Request */

    struct PsdDevice   *phw_RootDevice;   /* Link to root hub of this hardware */
    struct PsdDevice   *phw_DevArray[128]; /* DevAddress->Device mapping */
    struct List         phw_Devices;      /* List of devices */
    struct List         phw_DeadDevices;  /* List of disconnected devices */
    BOOL                phw_RemoveMe;     /* Hardware scheduled for removal */
    struct MsgPort      phw_DevMsgPort;   /* Quick device message port */
    struct MsgPort      phw_TaskMsgPort;  /* Quick task message port */
    volatile ULONG      phw_MsgCount;     /* Number of Messages pending */
};

/* Flags for pd_Flags */

#define PDFF_LOWSPEED    0x0001
#define PDFF_CONNECTED   0x0002
#define PDFF_HASDEVADDR  0x0004
#define PDFF_HASDEVDESC  0x0008
#define PDFF_CONFIGURED  0x0010
#define PDFF_HIGHSPEED   0x0020
#define PDFF_NEEDSSPLIT  0x0040
#define PDFF_LOWPOWER    0x0080
#define PDFF_DEAD        0x0100
#define PDFF_SUSPENDED   0x0200
#define PDFF_SUPERSPEED  0x0400
#define PDFF_APPBINDING  0x4000
#define PDFF_DELEXPUNGE  0x8000

struct PsdDevice
{
    struct Node         pd_Node;          /* Node linkage */
    struct PsdHardware *pd_Hardware;      /* Interfacing hardware */
    struct PsdLockSem   pd_Lock;          /* Access locking */
    struct PsdDevice   *pd_Hub;           /* Hub for device */
    APTR                pd_DevBinding;    /* Device binding */
    struct PsdUsbClass *pd_ClsBinding;    /* Which class has the bond? */
    struct PsdConfig   *pd_CurrentConfig; /* Direct pointer to currently set config */
    UWORD               pd_UseCnt;        /* Usage counter */
    UWORD               pd_DevAddr;       /* Device address */
    UWORD               pd_CurrCfg;       /* Current Configuration Number */
    UWORD               pd_NumCfgs;       /* Number of configurations available */
    UWORD               pd_PowerDrain;    /* Current power usage */
    UWORD               pd_PowerSupply;   /* Power provided from parent */
    UWORD               pd_CurrLangID;    /* Current Language ID */
    UWORD              *pd_LangIDArray;   /* Array of supported languages */
    UWORD               pd_Flags;         /* Lowspeed? */
    UWORD               pd_HubPort;       /* Port number at parent hub */
    UWORD               pd_HubThinkTime;  /* Think time for TT inter-transaction gap */
    UWORD               pd_USBVers;       /* USB Version */
    UWORD               pd_DevClass;      /* Class code */
    UWORD               pd_DevSubClass;   /* Subclass code */
    UWORD               pd_DevProto;      /* Device protocol code */
    UWORD               pd_MaxPktSize0;   /* Packet size for EP0 */
    UWORD               pd_VendorID;      /* Vendor ID */
    UWORD               pd_ProductID;     /* Product ID */
    UWORD               pd_DevVers;       /* Device release version */
    UWORD               pd_CloneCount;    /* Running Number to distinguish same devices */
    UWORD               pd_DeadCount;     /* Number of timeouts on the device */
    UWORD               pd_IOBusyCount;   /* Number of busy IOs (not including interrupt transfers) */
    struct timeval      pd_LastActivity;  /* Timestamp of last IO access (start or end) */
    STRPTR              pd_MnfctrStr;     /* Manufacturer string */
    STRPTR              pd_ProductStr;    /* Product string (custom?) */
    STRPTR              pd_OldProductStr; /* Original product string */
    STRPTR              pd_SerNumStr;     /* Serial number string */
    STRPTR              pd_IDString;      /* Whole Device ID string */
    struct List         pd_Configs;       /* List of configurations */
    BOOL                pd_IsNewToMe;     /* Whether the device is connected the first time */
    struct PsdPoPoCfg   pd_PoPoCfg;       /* Inhibit PopUp and Class scan Config */
    struct List         pd_Descriptors;   /* Descriptors collected */
    struct List         pd_RTIsoHandlers; /* List of RTIsoHandlers */
};

struct PsdDescriptor
{
    struct Node         pdd_Node;         /* Node linkage */
    struct PsdDevice   *pdd_Device;       /* Up linkage */
    struct PsdConfig   *pdd_Config;       /* Up linkage (optional, depending on type of desc) */
    struct PsdInterface *pdd_Interface;   /* Up linkage (optional, depending on type of desc) */
    struct PsdEndpoint *pdd_Endpoint;     /* Up linkage (optional, depending on type of desc) */
    STRPTR              pdd_Name;         /* Supposed Descriptor Type Name */
    UWORD               pdd_Type;         /* Descriptor Type */
    UWORD               pdd_Length;       /* Number of bytes in descriptor */
    UWORD               pdd_CSSubType;    /* Subtype of ClassSpecific Descriptor */
    UBYTE              *pdd_Data;         /* Pointer to data */
};

struct PsdConfig
{
    struct Node         pc_Node;          /* Node linkage */
    struct PsdDevice   *pc_Device;        /* Up linkage */
    UWORD               pc_CfgNum;        /* Config number */
    UWORD               pc_Attr;          /* Attributes */
    UWORD               pc_MaxPower;      /* MaxPower (in mA) */
    UWORD               pc_NumIfs;        /* Number of interfaces */
    STRPTR              pc_CfgStr;        /* Name of config */
    struct List         pc_Interfaces;    /* List of interfaces */
};

struct PsdInterface
{
    struct Node         pif_Node;         /* Node linkage */
    struct PsdConfig   *pif_Config;       /* Up linkage */
    APTR                pif_IfBinding;    /* Interface Binding */
    struct PsdUsbClass *pif_ClsBinding;   /* Which class has the bond? */
    UWORD               pif_IfNum;        /* Interface number */
    UWORD               pif_Alternate;    /* Alternate setting */
    UWORD               pif_NumEPs;       /* Number of Endpoints */
    UWORD               pif_IfClass;      /* Interface Class */
    UWORD               pif_IfSubClass;   /* Interface Subclass */
    UWORD               pif_IfProto;      /* Interface Protocol */
    STRPTR              pif_IfStr;        /* Interface String */
    STRPTR              pif_IDString;     /* Interface ID String */
    struct PsdInterface *pif_ParentIf;    /* If interface is an alternate, uplink */
    struct List         pif_AlterIfs;     /* List of alternate interface not in use */
    struct List         pif_EPs;          /* List of endpoints */
};

struct PsdEndpoint
{
    struct Node         pep_Node;         /* Node linkage */
    struct PsdInterface *pep_Interface;   /* Up linkage */
    UWORD               pep_EPNum;        /* Endpoint address */
    UWORD               pep_Direction;    /* Direction (0=OUT, 1=IN) */
    UWORD               pep_TransType;    /* TransferType, see USEA-Flags */
    UWORD               pep_MaxPktSize;   /* Maximum packet size for EP */
    UWORD               pep_NumTransMuFr; /* Number of transactions per �Frame */
    UWORD               pep_Interval;     /* Interval for polling in ms */
    UWORD               pep_SyncType;     /* Iso Synchronization Type */
    UWORD               pep_UsageType;    /* Iso Usage Type */
};

/* Flags for pp_Flags */
#define PFF_INPLACE     0x0001            /* streams: buffer is in place, needs no copying */

struct PsdPipe
{
    struct Message      pp_Msg;           /* Intertask communication message */
    struct PsdDevice   *pp_Device;        /* Up linkage */
    struct PsdEndpoint *pp_Endpoint;      /* Endpoint linkage or NULL for default pipe */
    struct MsgPort     *pp_MsgPort;       /* Msg Port of task allocated pipe */
    struct PsdPipe     *pp_AbortPipe;     /* Pipe to abort */
    ULONG               pp_Num;           /* internal pipe number (used for streams) */
    UWORD               pp_Flags;         /* internal flags (used for streams) */
    struct IOUsbHWReq   pp_IOReq;         /* IO Request allocated for this pipe */
};

/* Flags for pps_Flags */
#define PSFF_ASYNCIO     0x0001           /* async task (r/w) */
#define PSFF_SHORTTERM   0x0002           /* terminate read on short packet (r) */
#define PSFF_READAHEAD   0x0004           /* read from usb device until buffer full (r) */
#define PSFF_BUFFERREAD  0x0008           /* reads smaller than maxpktsize are buffered (r) */
#define PSFF_BUFFERWRITE 0x0010           /* writes smaller than maxpktsize are buffered -- flush required (w) */
#define PSFF_NOSHORTPKT  0x0020           /* don't terminate writes with a short packet (w) */
#define PSFF_NAKTIMEOUT  0x0040           /* enable nak timeout (r/w) */
#define PSFF_ALLOWRUNT   0x0080           /* allow reading of runt packets (r) */
#define PSFF_DONOTWAIT   0x0100           /* non blocking IO (r) */
#define PSFF_OWNMSGPORT  0x8000           /* internal flag */

struct PsdPipeStream
{
    struct Node         pps_Node;         /* Node linkage */
    struct PsdDevice   *pps_Device;       /* Up linkage */
    struct PsdEndpoint *pps_Endpoint;     /* Endpoint linkage */
    struct MsgPort     *pps_MsgPort;      /* Msg Port of task allocated pipe */
    struct PsdPipe    **pps_Pipes;        /* Array of pipes */
    struct List         pps_FreePipes;    /* Inactive pipes */
    struct List         pps_ReadyPipes;   /* Ready pipes */
    struct Task        *pps_AsyncTask;    /* Task used for asynchroneous transfers */
    struct SignalSemaphore pps_AccessLock; /* Semaphore for reading & writing */
    struct PsdPipe     *pps_ActivePipe;   /* Pipe currently active (w) */
    ULONG               pps_NumPipes;     /* Number of pipes */
    ULONG               pps_NakTimeoutTime; /* Nak Timeout time for pipe */
    ULONG               pps_AbortSigMask; /* Signal mask for waking up read or write */
    UWORD               pps_Flags;        /* transfer flags */

    UBYTE              *pps_Buffer;       /* globally allocated buffer for all pipes */
    ULONG               pps_BufferSize;   /* Size of each pipe buffer */
    ULONG               pps_Offset;       /* read or write offset into the first returned buffer */
    ULONG               pps_BytesPending; /* bytes in read or write buffer */
    ULONG               pps_ReqBytes;     /* number of bytes currently requested */
    LONG                pps_Error;        /* last error occurred */
    UBYTE              *pps_TermArray;    /* termination char array */
};

struct PsdRTIsoHandler
{
    struct Node         prt_Node;         /* Node linkage */
    struct PsdDevice   *prt_Device;       /* Up linkage */
    struct PsdEndpoint *prt_Endpoint;     /* Endpoint linkage */
    struct PsdPipe     *prt_Pipe;         /* Pipe */
    struct Hook        *prt_ReleaseHook;  /* Hook to be called when device gets removed */
    struct IOUsbHWRTIso prt_RTIso;        /* RT Iso structure */
};

#endif /* _LIBRARIES_POSEIDON_H */
