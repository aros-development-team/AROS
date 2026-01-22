#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <utility/tagitem.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <libraries/configvars.h>
#include <libraries/commodities.h>
#include <rexx/storage.h>
#include <string.h>
#include <clib/alib_protos.h>
#if !defined(__AROS__)
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/utility_protos.h>
#include <clib/expansion_protos.h>
#include <clib/commodities_protos.h>
#include <clib/rexxsyslib_protos.h>
#ifndef __GNUC__
#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/expansion_pragmas.h>
#endif
#else
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/expansion.h>
#include <proto/commodities.h>
#include <proto/rexxsyslib.h>
#if defined(AROS_USE_LOGRES)
#include <proto/log.h>
#endif
#endif

#include "shared_defs.h"
#if !defined(__AROS__)
#include "cat/VMM_Cat.h"

#define str_Addr    "%lx%"
#else
#include "locale.h"

#if (__WORDSIZE==64)
#define str_Addr    "%llx%"
#else
#define str_Addr    "%lx%"
#endif
#endif

/* $Id: defs.h,v 3.8 95/12/16 18:37:07 Martin_Apel Exp $ */

#define PRIVATE static

#define POINTERTABALIGN    (POINTERS_PER_TABLE*sizeof(long))
#define PAGETABALIGN       (PAGES_PER_TABLE*sizeof(long))
#define PAGEALIGN          (PAGESIZE)

#define ALIGN_UP(addr,alignment) (((addr) + (alignment) - 1) & ~((alignment) - 1))
#define ALIGN_DOWN(addr,alignment) ((addr) & ~((alignment) - 1))

#define ROOTOFFSET(addr)   (ROOTINDEX(addr) - ROOTINDEX(VirtAddrStart))

/* Returncodes of Evict, GetNewPage: */
#define READY              TRUE
#define IN_PROGRESS        FALSE

struct VMMHBase
{
    struct Library              vmmhb_LibNode;
    APTR                        vmmhb_KernelBase;
#if defined(AROS_USE_LOGRES)
    APTR                        vmmhb_LogResBase;
    APTR                        vmmhb_LogRHandle;
#endif
	/**/
    APTR                        vmmhb_AllocMem;
    APTR                        vmmhb_FreeMem;
};

struct ExtIOReq
  {
  struct IOStdReq ioreq;
  struct TrapStruct *TrapInfo;
  };


/* For finding partition data */

struct DOSDevParams
  {
  struct Device *device;
  ULONG unit;
  ULONG flags;
  char *logical_dev_name;
  struct Task *SysTask;
  int heads,
      secs_per_track,
      low_cyl,
      high_cyl,
      res_start,
      res_end,
      block_size,
      secs_per_block;
  ULONG MaxTransfer;
  };

struct FrameDescr
  {
  struct MinNode FDNode;
  struct MinNode OrderNode;
  ULONG PageNumOnDisk;
  UWORD PageType;
  BOOL   Modified;
  BOOL   InFrameList;
  IPTR  PhysAddr;
  union
    {
    struct
      {
      ULONG *pp_PageDescr;
      ULONG pp_LogAddr;
      } page_params;
    ULONG tp_LogTableStart;
    } spec_params;
  };

#define PageDescr      spec_params.page_params.pp_PageDescr
#define LogAddr        spec_params.page_params.pp_LogAddr
#define LogTableStart  spec_params.tp_LogTableStart

/* Defines for PageType */
#define PT_PAGE  0
#define PT_TABLE 1
#define PT_EMPTY 2            /* only for frame descriptors */

/* Defines for state */
#define PAGED_OUT          1
#define COMING_IN          0

/* Defines for PageNumOnDisk */
#define LOCUNUSED          0x000fffff

/* There are three special actions the pagehandler has to provide.
 * These are carried out asynchroneously, i.e. they require a page
 * to be read or written.
 *
 *                                 FaultTask       FaultAddress
 * 1. Remove one frame             ReqTask              0
 * 2. Reduce number of frames      REDUCE_TASK          0
 * 3. Lock page                    LOCK_TASK     Page to be locked
 */

#define REDUCE_TASK   0xffffffff
#define LOCK_TASK     0xfffffffe

/* For EnterTask */
#define INSERT_FRONT TRUE
#define INSERT_BACK  FALSE

GLOBAL struct TrapStruct TrapInfo [MAX_FAULTS];

GLOBAL ULONG *RootTable;
GLOBAL ULONG NumPageFrames;

GLOBAL APTR VirtAddrStart,
             VirtAddrEnd;

GLOBAL struct Task *PageHandlerTask;
GLOBAL struct Process *PageHandlerProcess;        /* when paging to file */
GLOBAL struct Task *PrePagerTask;
GLOBAL struct Process *PrePagerProcess;           /* when paging to file */
GLOBAL struct Task *StatTask;
GLOBAL struct Process *VM_ManagerProcess;
GLOBAL struct Process *FileHandlerProcess;        /* when paging to file */

GLOBAL void (*OrigSwitch) ();
GLOBAL void (*OrigAddTask) ();
GLOBAL void (*OrigAllocMem) ();
GLOBAL void (*OrigFreeMem) ();
GLOBAL void (*OrigAvailMem) ();
GLOBAL void (*OrigWait) ();
GLOBAL void (*OrigCachePreDMA) ();
GLOBAL void (*OrigCachePostDMA) ();
GLOBAL void (*OrigLoadSeg) ();
GLOBAL void (*OrigNewLoadSeg) ();
GLOBAL void (*OrigBeginIO) ();
GLOBAL void (*OrigTrapHandler) ();
GLOBAL void (*OrigDynMMUTrap) ();
#ifdef DEBUG
GLOBAL void (*OrigRemTask) ();
GLOBAL void (*OrigOpen) ();
GLOBAL void (*OrigStackSwap) ();
GLOBAL void (*OrigAlert) ();
#endif
GLOBAL void (*OrigColdReboot) ();
GLOBAL void (*OrigSetWindowTitles) ();

/* A few function pointers for the MMU functions */
GLOBAL void (*FlushVirt) (ULONG log_addr);
GLOBAL void (*CPushP) (ULONG addr);
GLOBAL void (*CPushL) (ULONG addr);
GLOBAL void (*PFlushP) (ULONG addr);
GLOBAL void (*PFlushA) (void);
GLOBAL ULONG (*GenDescr) (ULONG addr);

GLOBAL struct ExecBase   *SysBase;
GLOBAL struct DosLibrary *DOSBase;
GLOBAL struct Library    *IntuitionBase;
GLOBAL struct GfxBase    *GfxBase;
GLOBAL struct Library    *UtilityBase;
GLOBAL struct Library    *ExpansionBase;
GLOBAL struct Library    *CxBase;
GLOBAL struct Library    *RexxSysBase;
GLOBAL struct Library    *LocaleBase;

GLOBAL struct MemHeader  *VirtMem;

GLOBAL struct MsgPort *VMPort;
GLOBAL struct MsgPort *RexxPort;
GLOBAL struct MsgPort *InitPort;
GLOBAL struct ForbiddenFreeStruct *VMToBeFreed,
                                  *VMFreeRecycling;

/* A task which wants to do IO to VM puts the IORequest here */
GLOBAL struct MsgPort *PrePagerPort;

/* New for 1.3: A task, which frees a substantial amount of VM, puts a
 * message here to tell the pagehandler to free the corresponding
 * frames
 */
GLOBAL struct MsgPort *PageHandlerPort;

/* Signals allocated by the VM_Manager */
GLOBAL UWORD LockAckSignal;

/* Signals allocated by the PageHandler */
GLOBAL UWORD PageFaultSignal,           /* Page-Fault occurred */
             ReplySignal,               /* Reply from paging device */
             PageHandlerQuitSignal;

/* Signals allocated by the PrePager */
GLOBAL UWORD PrePagerQuitSignal,
             FreeVMSignal;              /* The prepager should free some VM */


GLOBAL UWORD StatQuitSignal;


GLOBAL BOOL  ThisPageLocked;            /* Set by the pagehandler and read
                                         * by VM_Manager when it receives
                                         * the LockAckSignal
                                         */
GLOBAL ULONG AddedMemSize;

GLOBAL struct HashEntry *HashTab [HASHTABSIZE];
GLOBAL struct HashEntry HashEntries [HASHTABSIZE];

GLOBAL ULONG NumPageFaults;
GLOBAL ULONG PagesWritten;
GLOBAL ULONG PagesRead;
GLOBAL UWORD LowMem;
GLOBAL UWORD AllocMemUsers;

GLOBAL ULONG NumLocked;
GLOBAL ULONG NumTables;

/* The following variable is written by the VM_Manager before a 
 * task is created which should allocate memory for a new writebuffer
 * and read by that task.
 */
GLOBAL ULONG DesiredWriteBufferSize;

/* The following is a copy of the equally named variable in the 
 * VMMConfig structure. This is done for performance reasons only
 */
GLOBAL ULONG MinVMAlloc;

/* The following is decremented on every FreeMem of virtual memory.
 * When it decrements to zero task for collecting empty pages is 
 * created.
 */
GLOBAL ULONG VMFreeCounter;

#define COLLECT_INTERVAL 200       /* For the above */

/* Parameters of device paging goes to (in either case) */
GLOBAL struct DOSDevParams PagingDevParams;

/* Offset in bytes if !UseFile, otherwise zero */
GLOBAL ULONG PartStart;

/* Partition size if !UseFile, otherwise size of paging file */
GLOBAL LONG PartSize;

/* For some actions it is necessary to be sure they don't use virtual
 * memory, such as error reporting. Therefore a forbid counter for
 * virtual memory is established.
 */
GLOBAL UWORD VMD_NestCnt;  /* VMD stands for "virtual memory disable */

#define DISABLE_VM VMD_NestCnt++
#define ENABLE_VM  VMD_NestCnt--

GLOBAL struct CxParams *CxParams;

GLOBAL struct List Free,
                   PageReq,
                   InTransit;

GLOBAL struct List FrameList;

/* There's a list of frame descriptors ordered by the time they were
 * allocated. When a free is requested, a frame from the front of the list
 * which is not currently involved in paging (PageDescr == RESIDENT)
 * is freed. This way fragmentation of memory is reduced.
 */
GLOBAL struct List FrameAllocList;
GLOBAL struct List LoadingTasksList;

GLOBAL struct SignalSemaphore VirtMemSema;

#ifdef USE_OWN_SEMAPHORES
#define OBTAIN_VM_SEMA   VMMObtainSemaphore (&VirtMemSema)
#define RELEASE_VM_SEMA  VMMReleaseSemaphore (&VirtMemSema)
#define INIT_SEMA(sema)  VMMInitSemaphore (sema)
#else
#define OBTAIN_VM_SEMA   ObtainSemaphore (&VirtMemSema)
#define RELEASE_VM_SEMA  ReleaseSemaphore (&VirtMemSema)
#define INIT_SEMA(sema)  InitSemaphore (sema)
#endif

GLOBAL short PageFaultsInProgress;

GLOBAL struct VMMConfig CurrentConfig;
GLOBAL char PartitionName [40];
GLOBAL char PartWithColon [40];
/* Copy of the equally named variable in VMMConfig. For performance
 * reasons only.
 */
GLOBAL BOOL MemTracking;

GLOBAL BOOL IsA3000;

GLOBAL BOOL ResetInProgress;

GLOBAL UWORD ProcessorType;

#define PROC_68851       0
#define PROC_68030       1
#define PROC_68040       2
#define PROC_68060       3

#ifdef DEBUG
GLOBAL ULONG InstructionFaults;
#endif

#include "mmu_bits30.h"
#include "protos.h"

#ifdef DEBUG
#define PRINT_DEB(a,b) PrintDebugMsg(a,b)
#define CHECK_CONSISTENCY CheckMemList ()
#define DEBUG_MAGIC 0xac
#else
#define PRINT_DEB(a,b)
#define CHECK_CONSISTENCY
#endif

#ifdef SCHED_STAT
GLOBAL ULONG FramesExamined;    /* How many frames have to be examined */
                                /* to find one to evict ? */
#endif

#ifdef DEBUG
GLOBAL ULONG EnforcerHits;
#endif

#define USED_P(descr)      ((descr) & USED)
#define MODIFIED_P(FD)     ((*((FD).PageDescr) & MODIFIED) ||\
                              ((FD).Modified))

#define PGNUM_FROM_DESCR(descr) (((descr) >> 4) & 0x000fffff)
#define STATE_FROM_DESCR(descr) (((descr) >> 3) & PAGED_OUT)
#define TYPE_FROM_DESCR(descr)  (((descr) & 0x4) ? PT_TABLE : PT_PAGE)
/* build descriptor for a swapped page */
#define BUILD_DESCR(pgnum,state,type) (((pgnum) << 4) | ((state) << 3) |\
                                       ((type) << 2) | INVALID)

/* A page descriptor looks like the following when the page is resident:
 * 31 ... 12  11  10  9  8  7  6  5  4  3  2  1  0
 * Phys.Addr  UR   G U1 U0  S   CM   M  U  W  1  1
 * The UR bit is used for the locked state.
 *
 * ************************************************
 * When the page is swapped out it looks like this:
 * 31..24  23..4    3     2  1  0
 * unused PageNum State Type 0  0
 * This gives 20 bits for the slot number, address space is
 * 2 ** 20 * 2 ** 12 = 4 GB
 */

/* For 68040 this means copyback */
#define CACHEABLE             ((ULONG)((ProcessorType >= PROC_68040) ?\
                                         (0x20) : (0x00)))
/* For 68040 this means serialized */
#define NONCACHEABLE          0x40

#define CACHE_MODE            CACHEABLE

/* Uses unused bits of page descriptor */
#define LOCKED                ((ULONG)((ProcessorType >= PROC_68040) ? \
                                         (0x800) : (0x20)))

/* These have to be defined separately, because they are used on MMU tables
 * the system or Enforcer have built.
 */

#define TABLE_RESIDENT_P(descr) ((ProcessorType >= PROC_68040) ? ((descr) & 0x2) : \
                                 (((descr) & 0x3) == TABLE_RESIDENT))

#define PAGE_RESIDENT_P(descr)  ((ProcessorType >= PROC_68040) ? ((descr) & 0x1) : \
                                 (((descr) & 0x3) == PAGE_RESIDENT))

#define TABLE_INVALID_P(descr)  (!TABLE_RESIDENT_P(descr))
#define PAGE_INVALID_P(descr)   (!PAGE_RESIDENT_P(descr))
#define LOCKED_P(descr)         ((descr) & LOCKED)

#include "errors.h"
