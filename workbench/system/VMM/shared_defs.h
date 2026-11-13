/* $Id: shared_defs.h,v 3.8 95/12/16 18:37:08 Martin_Apel Exp $ */

/* IOPacket can either be a DosPacket (if PageDev == PD_FILE) or a 
 * ExtIOReq structure (if PageDev == PD_PART or PD_PSEUDOPART). 
 * It would have better been a union, but H2I doesn't support unions
 */

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <libraries/commodities.h>

#define TMP_STACKSIZE 700

#if !defined(__AROS__) && defined(__AMIGA__)
#define PLATFORM_HASZ2
#define PLATFORM_HASFASTROM
#else
#endif

struct TrapStruct
  {
  struct MinNode                         TS_Node;
  struct Task                           *FaultTask;
  /* If FaultAddress == NULL, this is used for removing a frame. */
  IPTR                                  FaultAddress;
  union
    {
    UWORD                               *u_TopOfStackFrame;
    ULONG                                u_RemFrameSize;
    } u_struct;
  UBYTE                                  TmpStack[TMP_STACKSIZE];
  struct StackSwapStruct                 TmpStackSwap;
  void                                  *IOPacket;
  void                                  *SeekPacket;   /* Only used for 
                                                        * paging to file
                                                        */
  struct FrameDescr                     *FD;
  /* contains the address of the page descriptor If this is NULL,
   * the table for this page is missing
   */
  ULONG                                 *PageDescrAddr;     
  /* contains the address of the table descriptor. 
   */
  ULONG                                 *TableDescrAddr;
  IPTR                                  PhysAddr;
  UWORD                                  WakeupSignal;
  UWORD                                  FramesRemoved;
  ULONG                                  RemFrameFlags;
  };

#define RemFrameSize u_struct.u_RemFrameSize
#define TopOfStackFrame u_struct.u_TopOfStackFrame

/* Following are the only constants which should be changed for another
 * configuration:
 */

#ifdef PAGE4K
#define PAGESIZE        4096
#else
#ifdef PAGE8K
#define PAGESIZE        8192
#else
#define PAGESIZE        4096            /* default */
#endif
#endif



#ifndef NUM_PTR_TABLES
#define NUM_PTR_TABLES       16          /* This gives 512 MB maximum */
#endif

#ifndef MAX_FAULTS
#define MAX_FAULTS          20
#endif

/*************************** Up to here ************************/

#define POINTERS_PER_TABLE 128

#if PAGESIZE==4096
#define PAGESIZESHIFT       12
#define PAGES_PER_TABLE     64
#else
#define PAGESIZESHIFT       13
#define PAGES_PER_TABLE     32
#endif

#define MAX_TABLES         (POINTERS_PER_TABLE*NUM_PTR_TABLES)
#define MAX_TABLE_PAGES    (PAGES_PER_TABLE*MAX_TABLES/PAGESIZE)

/* These are the codes set by VMM when building (parts of) an MMU table */
#define TABLE_RESIDENT     2
#define PAGE_RESIDENT      1
#define INVALID            0

#define WRITEPROTECT       0x04
#define USED               0x08
#define MODIFIED           0x10

/* Bits for page descriptor */
#define CM_MASK               0x60

#define ROOTINDEX(addr)    ((addr) >> 25)
#define POINTERINDEX(addr) (((addr) >> 18) & 0x7f)
#define PAGEINDEX(addr)    (((addr) >> PAGESIZESHIFT) & (PAGES_PER_TABLE-1))
#define PAGEADDR(descr)    ((descr) & ~(PAGESIZE-1))
#define PAGEOFFS(descr)    ((descr) &  (PAGESIZE-1))

/* This structure is also used for the 68060. */
struct MMUState40
  {
  ULONG URP,
        SRP,
        ITT0,
        ITT1,
        DTT0,
        DTT1;
  ULONG TC;
  };

/* This structure is also used for the 68851. The TT registers are
 * not used in this case.
 */
struct MMUState30
  {
  ULONG TC,
        CRP_Hi,
        CRP_Lo,
        SRP_Hi,
        SRP_Lo,
        TT0,
        TT1;
  };

/* Structures for tasks allocating memory */

struct HashEntry
  {
  ULONG                                 *Name;
  struct HashEntry                      *NextEntry;
  ULONG                                  MinPublic;
  ULONG                                  MinNonPublic;
  UBYTE                                  Referenced;
  UBYTE                                  HashIndex;
  UWORD                                  NumLongsM1;
  };

#define HASHBITS 6
#define HASHTABSIZE (1L << HASHBITS)
#define HASH_VAL(x) (((x)&0x1f8)>>3)

struct ExtPort
  {
  struct MsgPort  CxPort;
  struct Task    *PrefsTask;
  LONG            ShowSignal;
  };

struct CxParams
  {
  struct ExtPort                        *ExtCxPort;
  CxObj                                 *Broker;
  CxObj                                 *GUIFilter,
                                        *EnableFilter,
                                        *DisableFilter;
  char                                  *PrefsPath;
  char                                  *ConfigPath;        /* name of the initial config file */
  BOOL                                   VMEnable;
  BOOL                                   ForceOverwrite;
  };

#define APPEAR_ID  0
#define ENABLE_ID  1
#define DISABLE_ID 2                                         

struct VMMsg
  {
  struct Message                         VMMessage;
  struct Task                           *VMSender;
  UWORD                                  VMCommand;
  /* If the reply signal is zero, it means the receiver should free this
   * message.
   */
  UWORD                                  ReplySignal;
  /* The following params are for asking statistics */
  union
    {
    struct
      {
      ULONG                                  st_VMSize;
      ULONG                                  st_VMFree;
      ULONG                                  st_Faults;
      ULONG                                  st_PagesWritten;
      ULONG                                  st_PagesRead;
      ULONG                                  st_Frames;
      ULONG                                  st_PagesUsed;
      ULONG                                  st_PageSize;
      ULONG                                  st_TrapStructsFree;
      } stat_params;

    struct 
      {
      struct MemHeader                      *ip_VMHeader;
      struct SignalSemaphore                *ip_VMSema;
             /* A pointer to the memlist name */
      char                                  *ip_MLName;
      } init_params;

    struct
      {
      ULONG                                  fp_Address;
      ULONG                                  fp_Size;
      } free_params;

    struct
      {
      char                                  *bp_NewWriteBuffer;
      ULONG                                  bp_NewWriteBufferSize;
      } buffer_params;

    struct List                             *up_TaskList;
    ULONG                                    lp_PageAddress;
    struct CxParams                         *cp_StartupParams;
    struct VMMConfig                        *vc_CurrentConfig;
    } spec_params;
  };

/* Some shortcuts for the above union definitions */

#define st_VMSize          spec_params.stat_params.st_VMSize
#define st_VMFree          spec_params.stat_params.st_VMFree
#define st_Faults          spec_params.stat_params.st_Faults
#define st_PagesWritten    spec_params.stat_params.st_PagesWritten
#define st_PagesRead       spec_params.stat_params.st_PagesRead
#define st_Frames          spec_params.stat_params.st_Frames
#define st_PagesUsed       spec_params.stat_params.st_PagesUsed
#define st_PageSize        spec_params.stat_params.st_PageSize
#define st_TrapStructsFree spec_params.stat_params.st_TrapStructsFree

#define VMHeader           spec_params.init_params.ip_VMHeader
#define VMSema             spec_params.init_params.ip_VMSema
#define MLName             spec_params.init_params.ip_MLName

#define FreeAddress        spec_params.free_params.fp_Address
#define FreeSize           spec_params.free_params.fp_Size

#define PageAddress        spec_params.lp_PageAddress

#define StartupParams      spec_params.cp_StartupParams

#define UsageList          spec_params.up_TaskList

#define Config             spec_params.vc_CurrentConfig

#define NewWriteBuffer     spec_params.buffer_params.bp_NewWriteBuffer
#define NewWriteBufferSize spec_params.buffer_params.bp_NewWriteBufferSize

#define VMCMD_FirstCommand              1468
#define VMCMD_AskAllocMem               (VMCMD_FirstCommand+0)
#define VMCMD_QuitAll                   (VMCMD_FirstCommand+1)
#define VMCMD_InitReady                 (VMCMD_FirstCommand+2)
#define VMCMD_InitFailed                (VMCMD_FirstCommand+3)
#define VMCMD_AskStat                   (VMCMD_FirstCommand+4)   
#define VMCMD_ReqMemHeader              (VMCMD_FirstCommand+5)
#define VMCMD_FreePages                 (VMCMD_FirstCommand+6)
#define VMCMD_LockPage                  (VMCMD_FirstCommand+7)
#define VMCMD_UnlockAllPages            (VMCMD_FirstCommand+8)
#define VMCMD_NewConfig                 (VMCMD_FirstCommand+9)
#define VMCMD_Startup                   (VMCMD_FirstCommand+10)

/* Used by the preferences program to work around a bug in the
 * RAM filesystem. When opening a file it allocates memory in VM
 * it doesn't free, when the file is closed.
 */
#define VMCMD_EnableVM                  (VMCMD_FirstCommand+11)
#define VMCMD_DisableVM                 (VMCMD_FirstCommand+12)

#define VMCMD_AskVMUsage                (VMCMD_FirstCommand+13)
#define VMCMD_AskConfig                 (VMCMD_FirstCommand+14)
#define VMCMD_NewWriteBuffer            (VMCMD_FirstCommand+15)

#define VMCMD_LastCommand               (VMCMD_FirstCommand+15)

/*******************************************************************
 * The following alert will occur, if there are no more TrapStructs*
 * during either a page-fault or a context-switch.                 *
 *******************************************************************/

#define NoTrapStructsAlertNum (AT_DeadEnd|AO_Unknown|AN_Unknown)

/* Error levels */
#define ERR_NOERROR  0        /* Simple message */
#define ERR_CONTINUE 1        /* VM can be continued */
#define ERR_FATAL    2        /* Have to reboot */


/* The following structure is used for VM, which is freed during a 
 * forbidden section. It's parameters are put into the structure, the struct
 * is chained in a list and the prepager is signalled to really free outside
 * the forbidden section.
 * In order not to allocate such a small structure each time a FreeMem is
 * called in the forbidden state, they are put into a recycling chain
 * after use.
 */

struct ForbiddenFreeStruct
  {
  struct ForbiddenFreeStruct            *NextFree;
  void                                  *address;
  ULONG                                  size;
  };

/*** Memory tracking ***/
#define TRACK_MAGIC     (('V'<<24)|('M'<<16)|('T'<<8)|'R')

/* For use with the MinPublic and MinNonPublic variables */
#define USE_ALWAYS  0L
#define USE_NEVER   0xffffffff

/* Defines for code paging */
#define CSWAP TRUE
#define CNOSWAP FALSE

/* Defines for data paging */
#define DP_FALSE    0
#define DP_TRUE     1
#define DP_ADVANCED 2

#define PD_FILE 0
#define PD_PART 1
#define PD_PSEUDOPART 2

#define DYN_MIN 0
#define DYN_MAX 0x40000000

#define CFG_MAGIC 0x342ac93a

struct VMMConfig
  {
  ULONG CfgMagic;
  UWORD Version,
        Revision;
  ULONG MinMem,
        MaxMem,
        WriteBuffer;
  LONG  VMPriority;
  ULONG MemFlags,
        FileSize,        
        MinVMAlloc,
        ZLeftEdge,
        ZTopEdge,
        UnZLeftEdge,
        UnZTopEdge;
  LONG  DefaultMinPublic,
        DefaultMinNonPublic;
  ULONG NumTaskEntries;
  UWORD PageDev;
  char  EnableHotkey [80],
        DisableHotkey [80],
        PartOrFileName [80];       /* If it's a partition name it includes
                                    * a trailing colon */
  BOOL  StatEnabled,
        StatZoomed,
        MemTracking,
        PatchWB,
        CacheZ2RAM,
        DefaultCodePaging;
  UWORD DefaultDataPaging;
  BOOL  FastROM;
  UWORD Reserved [9];
  };

struct OldTaskEntryInFile
  {
  struct MinNode te_Node;
  char  TaskName [40];
  ULONG MinPublic,
        MinNonPublic;
  UWORD DataPaging;
  BOOL  CodePaging;
  BOOL  IsDefault;
  };
  
struct TaskEntryInFile
  {
  ULONG MinPublic,
        MinNonPublic;
  UWORD DataPaging;
  BOOL  CodePaging;
  BOOL  IsDefault;
  UWORD NameLen;         /* Including trailing zero */
  };

struct LoadingTaskStruct
  {
  struct MinNode lt_Node;
  struct Task *LoadingTask;
  char        *LoadfileName;
  };

#ifdef DEBUG

/* Debugging information is held in the following structure. */
#define DEBUG_SIZE  (512L * 1024L)
#define DEBUG_MAGIC1 ((ULONG)(('V' << 16) | ('M' << 8) | 'M'))
#define DEBUG_MAGIC2 ((ULONG)(('D' << 16) | ('B' << 8) | 'G'))

struct DebugBuffer
  {
  ULONG Dummy [2];       /* The first two longwords of an AllocAbs cannot be
                          * trusted anyway */
  ULONG Magic1;
  struct DebugBuffer *ThisBuffer;
  char *NextFree;
  char DbgInfo [DEBUG_SIZE - 4 * sizeof (ULONG) 
                           - sizeof (struct DebugBuffer*)
                           - sizeof (char*) ];
  ULONG Magic2;
  };

#endif


#define PAGEHANDLER_NAME    "VMM_PageHandler"
#define PREPAGER_NAME       "VMM_PrePager"
#define VM_MANAGER_NAME     "VMM_Manager"
#define STAT_NAME           "VMM_Statistics"
#define ERR_NAME            "VMM_ErrorHandler"
#define WBUF_ALLOC_NAME     "VMM_WriteBufAllocator"
#define GARBAGE_COLL_NAME   "VMM_EmptyPageCollector"
#define VMPORTNAME          "VMM_Port"
#define CXPORTNAME          "VMM CX Port"
#define STARTER_PORT_STD    "VMM_Starter_Std"
#define STARTER_PORT_LIB    "VMM_Starter_Lib"
#define PROGNAME            "VMM"
#define PROGPATH            "L:VMM-Handler"
#define LIBNAME             "vmm.library"
#define MEMLISTNAME         "VMM Mem (paged)"
#define CFG_FILEBASE        "VMM.prefs"
#define CFG_FILENAME        "ENV:" CFG_FILEBASE
#define MMUCFG_FILENAME     "ENV:VMM_MMU.config"
#define DEFAULT_ENABLE_KEY  "ralt rshift y"
#define DEFAULT_DISABLE_KEY "ralt rshift n"
#define DEFAULT_MINVMSIZE   200L
#define AUTHOR              "Martin Apel"
#define COPYRIGHT           "Copyright 1993-95 Martin Apel"
#define DESCRIPTION         "Virtual memory manager"
#define VER_STRING          "VMM V3.3"

#define EMERGENCY_SIGNAL    SIGB_SINGLE
