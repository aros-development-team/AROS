    include "exec/types.i"
    include "exec/tasks.i"
    include "exec/alerts.i"

ARRAY        macro
\1           equ   SOFFSET
SOFFSET      set   SOFFSET+(\2*\3)
             endm

TMP_STACKSIZE                   equ   700

 STRUCTURE TrapStruct,0
    STRUCT   TS_Node,MLN_SIZE
    APTR     TS_FaultTask
    ULONG    TS_FaultAddress
    LABEL    TS_RemFrameSize
    APTR     TS_TopOfStackFrame
    ARRAY    TS_TmpStack,1,TMP_STACKSIZE
    STRUCT   TS_TmpStackSwap,StackSwapStruct_SIZEOF
    APTR     TS_IOPacket
    APTR     TS_SeekPacket
    APTR     TS_FD
    APTR     TS_PageDescrAddr
    APTR     TS_TableDescrAddr
    ULONG    TS_PhysAddr
    UWORD    TS_WakeupSignal
    UWORD    TS_FramesRemoved
    ULONG    TS_RemFrameFlags
    LABEL    TS_SIZE



* Following are the only constants which should be changed for another
* configuration:
*

                                IFD   PAGE4K
PAGESIZE                        equ   4096
                                ELSE
                                IFD   PAGE8K
PAGESIZE                        equ   8192
                                ELSE
PAGESIZE                        equ   4096
                                ENDC
                                ENDC

                                IFND  NUM_PTR_TABLES
NUM_PTR_TABLES                  equ   16
                                ENDC

                                IFND  MAX_FAULTS
MAX_FAULTS                      equ   20
                                ENDC

*************************** Up to here ************************

POINTERS_PER_TABLE              equ   128

                                IFEQ  PAGESIZE-4096
PAGESIZESHIFT                   equ   12
PAGES_PER_TABLE                 equ   64
                                ELSE
PAGESIZESHIFT                   equ   13
PAGES_PER_TABLE                 equ   32
                                ENDC

MAX_TABLES                      equ   (POINTERS_PER_TABLE*NUM_PTR_TABLES)
MAX_TABLE_PAGES                 equ   (PAGES_PER_TABLE*MAX_TABLES/PAGESIZE)

* These are the codes set by VMM when building (parts of) an MMU table
TABLE_RESIDENT                  equ   2
PAGE_RESIDENT                   equ   1
INVALID                         equ   0

WRITEPROTECT                    equ   $04
USED                            equ   $08
MODIFIED                        equ   $10

* Bits for page descriptor
CM_MASK                         equ   $60

ROOTINDEX      MACRO     * d0 (modified), d1 (scratch)
               moveq     #25,d1
               lsr.l     d1,d0
               ENDM

POINTERINDEX   MACRO     * d0 (modified), d1 (scratch)
               moveq     #18,d1
               lsr.l     d1,d0
               and.w     #$7f,d0
               ENDM

PAGEINDEX      MACRO     * d0 (modified), d1 (scratch)
               moveq     #PAGESIZESHIFT,d1
               lsr.l     d1,d0
               and.l     #(PAGES_PER_TABLE-1),d0
               ENDM

PAGEADDR       MACRO     * reg (modified)
               and.w     #~(PAGESIZE-1),\1
               ENDM

PAGEOFFS       MACRO     * reg (modified)
               and.l     #(PAGESIZE-1),\1
               ENDM

* Structure for accessing the MMU registers

 STRUCTURE MMUState40,0
    ULONG    MS40_URP
    ULONG    MS40_SRP
    ULONG    MS40_ITT0
    ULONG    MS40_ITT1
    ULONG    MS40_DTT0
    ULONG    MS40_DTT1
    ULONG    MS40_TC
    LABEL    MS40_SIZE

 STRUCTURE MMUState30,0
    ULONG    MS30_TC
    ULONG    MS30_CRP_HI
    ULONG    MS30_CRP_LO
    ULONG    MS30_SRP_HI
    ULONG    MS30_SRP_LO
    ULONG    MS30_TT0
    ULONG    MS30_TT1
    LABEL    MS30_SIZE

 STRUCTURE HashEntry,0
    APTR     HE_Name
    APTR     HE_NextEntry
    ULONG    HE_MinPublic
    ULONG    HE_MinNonPublic
    UBYTE    HE_Referenced
    UBYTE    HE_HashIndex
    UWORD    HE_NumLongsM1
    LABEL    HE_SIZE


HASHBITS       equ       6
HASHTABSIZE    equ       (1<<HASHBITS)
HASH_VAL       MACRO     * \1 register to compute hash value in
               and.w     #$1f8,\1
               lsr.w     #3,\1
               ENDM

 STRUCTURE CxParams,0
    APTR     CP_CxPort
    APTR     CP_Broker
    APTR     CP_GUIFilter
    APTR     CP_EnableFilter
    APTR     CP_DisableFilter
    APTR     CP_PrefsPath
    APTR     CP_ConfigPath
    BOOL     CP_VMEnable
    BOOL     CP_ForceOverwrite
    LABEL    CP_SIZE

APPEAR_ID    EQU    0
ENABLE_ID    EQU    1
DISABLE_ID   EQU    2


 STRUCTURE VMMsg,0
    STRUCT   VMM_VMMessage,MN_SIZE
    APTR     VMM_VMSender
    UWORD    VMM_VMCommand
    * If the reply signal is zero, it means the receiver should free this
    * message.
    UWORD    VMM_ReplySignal

    LABEL    VMM_VMHeader
    LABEL    VMM_FreeAddress
    LABEL    VMM_PageAddress
    LABEL    VMM_StartupParams
    LABEL    VMM_UsageList
    LABEL    VMM_Config
    LABEL    VMM_NewWriteBuffer
    ULONG    VMM_VMSize

    LABEL    VMM_VMSema
    LABEL    VMM_FreeSize
    LABEL    VMM_NewWriteBufferSize
    ULONG    VMM_VMFree

    LABEL    VMM_MLName
    ULONG    VMM_Faults
    ULONG    VMM_PagesWritten
    ULONG    VMM_PagesRead
    ULONG    VMM_Frames
    ULONG    VMM_PagesUsed
    ULONG    VMM_PageSize
    ULONG    VMM_TrapStructsFree

    LABEL    VMM_SIZE


VMCMD_FirstCommand              equ   1468
VMCMD_AskAllocMem               equ   (VMCMD_FirstCommand+0)
VMCMD_QuitAll                   equ   (VMCMD_FirstCommand+1)
VMCMD_InitReady                 equ   (VMCMD_FirstCommand+2)
VMCMD_InitFailed                equ   (VMCMD_FirstCommand+3)
VMCMD_AskStat                   equ   (VMCMD_FirstCommand+4)
VMCMD_ReqMemHeader              equ   (VMCMD_FirstCommand+5)
VMCMD_FreePages                 equ   (VMCMD_FirstCommand+6)
VMCMD_LockPage                  equ   (VMCMD_FirstCommand+7)
VMCMD_UnlockAllPages            equ   (VMCMD_FirstCommand+8)
VMCMD_NewConfig                 equ   (VMCMD_FirstCommand+9)
VMCMD_Startup                   equ   (VMCMD_FirstCommand+10)
VMCMD_EnableVM                  equ   (VMCMD_FirstCommand+11)
VMCMD_DisableVM                 equ   (VMCMD_FirstCommand+12)
VMCMD_AskVMUsage                equ   (VMCMD_FirstCommand+13)
VMCMD_AskConfig                 equ   (VMCMD_FirstCommand+14)
VMCMD_NewWriteBuffer            equ   (VMCMD_FirstCommand+15)

NoTrapStructsAlertNum           equ   (AT_DeadEnd+AO_Unknown+AN_Unknown)

ERR_NOERROR                     equ   0
ERR_CONTINUE                    equ   1
ERR_FATAL                       equ   2

 STRUCTURE ForbiddenFreeStruct,0
    APTR     FF_NextFree
    APTR     FF_address
    ULONG    FF_size
    LABEL    FF_SIZE

*** Memory tracking ***
TRACK_MAGIC EQU    'VMTR'

 STRUCTURE VMMConfig,0
    ULONG CfgMagic
    UWORD Version
    UWORD Revision
    ULONG MinMem
    ULONG MaxMem
    ULONG WriteBuffer
    LONG  VMPriority
    ULONG MemFlags
    ULONG FileSize
    ULONG MinVMAlloc
    ULONG ZLeftEdge
    ULONG ZTopEdge
    ULONG UnZLeftEdge
    ULONG UnZTopEdge
    LONG  DefaultMinPublic
    LONG  DefaultMinNonPublic
    ULONG NumTaskEntries
    UWORD PageDev
    ARRAY EnableHotkey,1,80
    ARRAY DisableHotkey,1,80
    ARRAY PartOrFileName,1,80
    BOOL  StatEnabled
    BOOL  StatZoomed
    BOOL  MemTracking
    BOOL  PatchWB
    BOOL  CacheZ2RAM
    BOOL  DefaultCodePaging
    UWORD DefaultDataPaging
    BOOL  FastROM
    ARRAY Reserved,9,2
    LABEL VC_SIZE



 STRUCTURE LoadingTaskStruct,0
    STRUCT   lt_Node,MLN_SIZE
    APTR     LoadingTask
    APTR     LoadfileName
    LABEL    LT_SIZE



PAGEHANDLER_NAME   MACRO
                   DC.B    "VMM_PageHandler",0
                   ENDM

PREPAGER_NAME      MACRO
                   DC.B    "VMM_PrePager",0
                   ENDM

VM_MANAGER_NAME    MACRO
                   DC.B    "VMM_Manager",0
                   ENDM

STAT_NAME          MACRO
                   DC.B    "VMM_Statistics",0
                   ENDM

ERR_NAME           MACRO
                   DC.B    "VMM_ErrorHandler",0
                   ENDM

WBUF_ALLOC_NAME    MACRO
                   DC.B    "VMM_WriteBufAllocator",0
                   ENDM

GARBAGE_COLL_NAME  MACRO
                   DC.B    "VMM_EmptyPageCollector",0
                   ENDM

VMPORTNAME         MACRO
                   DC.B    "VMM_Port",0
                   ENDM

CXPORTNAME         MACRO
                   DC.B    "VMM CX Port",0
                   ENDM

STARTER_PORT_STD   MACRO
                   DC.B    "VMM_Starter_Std",0
                   ENDM

STARTER_PORT_LIB   MACRO
                   DC.B    "VMM_Starter_Lib",0
                   ENDM

PROGNAME           MACRO
                   DC.B    "VMM",0
                   ENDM

PROGPATH           MACRO
                   DC.B    "L:VMM-Handler",0
                   ENDM

LIBNAME            MACRO
                   DC.B    "vmm.library",0
                   ENDM

MEMLISTNAME        MACRO
                   DC.B    "VMM Mem (paged)",0
                   ENDM

CFG_FILEBASE       MACRO
                   DC.B    "VMM.prefs",0
                   ENDM

CFG_FILENAME       MACRO
                   DC.B    "ENV:"
                   CFG_FILEBASE
                   ENDM

MMUCFG_FILENAME    MACRO
                   DC.B    "ENV:VMM_MMU.config",0
                   ENDM

EMERGENCY_SIGNAL   EQU     SIGB_SINGLE
