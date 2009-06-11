#ifndef MASSSTORAGE_H
#define MASSSTORAGE_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/hardblocks.h>

#define NIL_PTR 0xffffffff

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab
#define ID_SELECT_LUN   0x22222222
#define ID_AUTODTXMAXTX 0x11111111

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    IPTR  cdc_NakTimeout;
    IPTR  cdc_PatchFlags;
    char  cdc_FATFSName[64];
    ULONG cdc_FATDosType;
    IPTR  cdc_StartupDelay;
    char  cdc_FATControl[64];
    ULONG cdc_MaxTransfer;
    char  cdc_CDFSName[64];
    ULONG cdc_CDDosType;
    char  cdc_CDControl[64];
    char  cdc_NTFSName[64];
    ULONG cdc_NTFSDosType;
    char  cdc_NTFSControl[64];
};

struct ClsUnitCfg
{
    ULONG cuc_ChunkID;
    ULONG cuc_Length;
    IPTR  cuc_AutoMountFAT;
    char  cuc_FATDOSName[32];
    IPTR  cuc_FATBuffers;
    IPTR  cuc_AutoMountRDB;
    IPTR  cuc_BootRDB;
    IPTR  cuc_DefaultUnit;
    IPTR  cuc_AutoUnmount;
    IPTR  cuc_MountAllFAT;
    IPTR  cuc_AutoMountCD;
};

#if 0
struct PartitionEntry
{
    UBYTE pe_Flags;               /* Offset 0 */
    UBYTE pe_StartCHS[3];         /* Offset 1 */
    UBYTE pe_Type;                /* Offset 4 */
    UBYTE pe_EndCHS[3];           /* Offset 5 */
    ULONG pe_StartLBA;            /* Offset 8 */
    ULONG pe_SectorCount;         /* Offset 12 */
};

#define PE_FLAGB_ACTIVE 7
#define PE_FLAGF_ACTIVE (1 << PE_FLAGB_ACTIVE)

struct MasterBootRecord {
    UBYTE mbr_pad0[446];
    struct PartitionEntry mbr_Partition[4];
    UBYTE mbr_Signature[2];
};

#define MBR_SIGNATURE   0x55aa

struct FATSuperBlock
{
    UBYTE fsb_Jump[3];                   // 0000
    UBYTE fsb_Vendor[8];                 // 0003
    UBYTE fsb_BytesPerSector[2];         // 000B
    UBYTE fsb_SectorsPerCluster;         // 000D
    UBYTE fsb_ReservedSectors[2];        // 000E
    UBYTE fsb_NumberFATs;                // 0010
    UBYTE fsb_NumberRootEntries[2];      // 0011
    UBYTE fsb_SectorsPerVolume[2];       // 0013
    UBYTE fsb_MediaDescriptor;           // 0015
    UBYTE fsb_SectorsPerFAT[2];          // 0016
    UBYTE fsb_SectorsPerTrack[2];        // 0018
    UBYTE fsb_Heads[2];                  // 001A
    UBYTE fsb_FirstVolumeSector[2];      // 001C
    UBYTE fsb_pad0[13];                  // 001E
    UBYTE fsb_Label[11];                 // 002B
    UBYTE fsb_FileSystem[8];             // 0036
    UBYTE fsb_pad1[9];                   // 003E
    UBYTE fsb_Label2[11];                // 0047
    UBYTE fsb_FileSystem2[8];            // 0052
    UBYTE fsb_BootCode[512 - 90];        // 005A
};
#endif

#if defined(__GNUC__)
# pragma pack()
#endif

#if 0
struct RigidDisk
{
    struct RigidDiskBlock rdsk_RDB;
    struct PartitionBlock rdsk_PART;
    struct FileSysHeaderBlock rdsk_FSHD;
};
#endif

#define PFF_SINGLE_LUN     0x000001 /* allow access only to LUN 0 */
#define PFF_MODE_XLATE     0x000002 /* translate 6 byte commands to 10 byte commands */
#define PFF_EMUL_LARGE_BLK 0x000004 /* Emulate access on larger block sizes */
#define PFF_REM_SUPPORT    0x000010 /* extended removable support */
#define PFF_FIX_INQ36      0x000040 /* inquiry request needs fixing */
#define PFF_DELAY_DATA     0x000080 /* delay data phase */
#define PFF_SIMPLE_SCSI    0x000100 /* kill all complicated SCSI commands that might crash firmware */
#define PFF_NO_RESET       0x000200 /* do not perform initial bulk reset */
#define PFF_FAKE_INQUIRY   0x000400 /* filter inquiry and fake it */
#define PFF_FIX_CAPACITY   0x000800 /* fix capacity by one */
#define PFF_NO_FALLBACK    0x001000 /* don't fall back automatically */
#define PFF_CSS_BROKEN     0x002000 /* olympus command status signature fix */
#define PFF_CLEAR_EP       0x004000 /* clear endpoint halt */
#define PFF_DEBUG          0x008000 /* more debug output */

struct NepClassMS
{
    struct Unit         ncm_Unit;         /* Unit structure */
    struct NepClassMS  *ncm_UnitLUN0;     /* Head of list */
    ULONG               ncm_UnitNo;       /* Unit number */
    struct NepMSBase   *ncm_ClsBase;      /* Up linkage */
    struct NepMSDevBase *ncm_DevBase;     /* Device base */
    struct Library     *ncm_Base;         /* Poseidon base */
    struct PsdDevice   *ncm_Device;       /* Up linkage */
    struct PsdConfig   *ncm_Config;       /* Up linkage */
    struct PsdInterface *ncm_Interface;   /* Up linkage */
    struct Task        *ncm_ReadySigTask; /* Task to send ready signal to */
    LONG                ncm_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncm_Task;         /* Subtask */
    struct MsgPort     *ncm_TaskMsgPort;  /* Message Port of Subtask */
    struct SignalSemaphore ncm_XFerLock;  /* LUN allowed to talk to the device */
    struct PsdPipe     *ncm_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncm_EPOut;        /* Endpoint OUT */
    struct PsdPipe     *ncm_EPOutPipe;    /* Endpoint OUT pipe */
    struct PsdEndpoint *ncm_EPIn;         /* Endpoint IN */
    struct PsdPipe     *ncm_EPInPipe;     /* Endpoint IN pipe */
    struct PsdEndpoint *ncm_EPInt;        /* Optional Endpoint INT */
    struct PsdPipe     *ncm_EPIntPipe;    /* Optional Endpoint INT pipe */
    UWORD               ncm_EPOutNum;     /* Endpoint OUT number */
    UWORD               ncm_EPInNum;      /* Endpoint IN number */
    UWORD               ncm_EPIntNum;     /* Endpoint INT number */
    struct MsgPort     *ncm_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncm_UnitProdID;   /* ProductID of unit */
    UWORD               ncm_UnitVendorID; /* VendorID of unit */
    UWORD               ncm_UnitCfgNum;   /* Config of unit */
    UWORD               ncm_UnitIfNum;    /* Interface number */
    UWORD               ncm_UnitLUN;      /* LUN */
    UWORD               ncm_MaxLUN;       /* Number of LUNs */
    BOOL                ncm_DenyRequests; /* Device is gone! */
    BOOL                ncm_HasMounted;   /* Has mounted some partitions */
    BOOL                ncm_UnitReady;    /* Unit is ready */
    ULONG               ncm_ChangeCount;  /* Number of disk changes */
    ULONG               ncm_LastChange;   /* Change number last interrupt created */
    struct List         ncm_DCInts;       /* DiskChange Interrupt IORequests */
    ULONG               ncm_BlockSize;    /* BlockSize = 512 */
    ULONG               ncm_BlockShift;   /* Log2 BlockSize */
    BOOL                ncm_WriteProtect; /* Is Disk write protected? */
    BOOL                ncm_Removable;    /* Is disk removable? */
    BOOL                ncm_ForceRTCheck; /* Force removable task to be restarted */
    UWORD               ncm_DeviceType;   /* Peripheral Device Type (from Inquiry data) */
    UWORD               ncm_TPType;       /* Transport type */
    UWORD               ncm_CSType;       /* SCSI Commandset type */
    ULONG               ncm_TagCount;     /* Tag for CBW */
    struct DriveGeometry ncm_Geometry;    /* Drive Geometry */
    ULONG               ncm_GeoChangeCount; /* when did we last obtained the geometry for caching */
    BOOL                ncm_BulkResetBorks; /* Bulk Reset is broken, don't try to use it */

    UBYTE              *ncm_OneBlock;     /* buffer for one block */
    ULONG               ncm_OneBlockSize; /* size of one block buffer */

    STRPTR              ncm_DevIDString;  /* Device ID String */
    STRPTR              ncm_IfIDString;   /* Interface ID String */

    struct IOStdReq    *ncm_XFerPending;  /* XFer IORequest pending */
    struct List         ncm_XFerQueue;    /* List of xfer requests */

    char                ncm_LUNIDStr[18];
    char                ncm_LUNNumStr[4];
    UBYTE               ncm_ModePageBuf[256];
    UBYTE               ncm_FATControlBSTR[68];

    BOOL                ncm_UsingDefaultCfg;

    BOOL                ncm_IOStarted;    /* IO Running */
    BOOL                ncm_Running;      /* Not suspended */

    struct ClsDevCfg   *ncm_CDC;
    struct ClsUnitCfg  *ncm_CUC;

    struct Library     *ncm_MUIBase;      /* MUI master base */
    struct Library     *ncm_PsdBase;      /* Poseidon base */
    struct Library     *ncm_IntBase;      /* Intuition base */
    struct Task        *ncm_GUITask;      /* GUI Task */
    struct NepClassMS  *ncm_GUIBinding;   /* Window of binding that's open */

    Object             *ncm_App;
    Object             *ncm_MainWindow;
    Object             *ncm_NakTimeoutObj;
    Object             *ncm_SingleLunObj;
    Object             *ncm_FixInquiryObj;
    Object             *ncm_FakeInquiryObj;
    Object             *ncm_SimpleSCSIObj;
    Object             *ncm_XLate610Obj;
    Object             *ncm_CSSBrokenObj;
    Object             *ncm_EmulLargeBlkObj;
    Object             *ncm_FixCapacityObj;
    Object             *ncm_DebugObj;
    Object             *ncm_RemSupportObj;
    Object             *ncm_NoFallbackObj;
    Object             *ncm_MaxTransferObj;
    Object             *ncm_AutoDtxMaxTransObj;
    Object             *ncm_FatFSObj;
    Object             *ncm_FatDosTypeObj;
    Object             *ncm_FatControlObj;
    Object             *ncm_NTFSObj;
    Object             *ncm_NTFSDosTypeObj;
    Object             *ncm_NTFSControlObj;
    Object             *ncm_CDFSObj;
    Object             *ncm_CDDosTypeObj;
    Object             *ncm_CDControlObj;
    Object             *ncm_StartupDelayObj;
    Object             *ncm_InitialResetObj;

    Object             *ncm_LunGroupObj;
    Object             *ncm_LunLVObj;
    Object             *ncm_UnitObj;
    Object             *ncm_AutoMountFATObj;
    Object             *ncm_AutoMountCDObj;
    Object             *ncm_FatDOSNameObj;
    Object             *ncm_FatBuffersObj;
    Object             *ncm_MountAllFATObj;
    Object             *ncm_AutoMountRDBObj;
    Object             *ncm_BootRDBObj;
    Object             *ncm_UnmountObj;

    Object             *ncm_UseObj;
    Object             *ncm_SetDefaultObj;
    Object             *ncm_CloseObj;

    Object             *ncm_AboutMI;
    Object             *ncm_UseMI;
    Object             *ncm_SetDefaultMI;
    Object             *ncm_MUIPrefsMI;

    struct Hook         ncm_LUNListDisplayHook;
};

struct NepMSBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* Utility base */

    struct NepMSDevBase *nh_DevBase;      /* base of device created */
    struct List         nh_Units;         /* List of units available */
    struct NepClassMS   nh_DummyNCM;      /* Dummy NCM for default config */

    struct SignalSemaphore nh_TaskLock;   /* single task monitor */
    struct Task        *nh_ReadySigTask;  /* Task to send ready signal to */
    LONG                nh_ReadySignal;   /* Signal to send when ready */
    struct Task        *nh_RemovableTask; /* Task for removable control */
    struct MsgPort     *nh_IOMsgPort;     /* Port for local IO */
    struct IOStdReq     nh_IOReq;         /* Fake IOReq */
    struct Library     *nh_ExpansionBase; /* ExpansionBase */
    struct Library     *nh_PsdBase;       /* PsdBase */
    struct Library     *nh_DOSBase;       /* DOS base */
    struct MsgPort     *nh_TimerMsgPort;  /* Port for timer.device */
    struct timerequest *nh_TimerIOReq;    /* Timer IO Request */
    BOOL                nh_RestartIt;     /* Restart removable task? */
    //struct RigidDisk    nh_RDsk;          /* RigidDisk */
    UBYTE              *nh_OneBlock;      /* buffer for one block */
    ULONG               nh_OneBlockSize;  /* size of one block buffer */

};

struct NepMSDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepMSBase   *np_ClsBase;       /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

#endif /* MASSSTORAGE_H */
