#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: pagehandler.c,v 3.9 95/12/16 18:36:40 Martin_Apel Exp $";

/***********************************************************************/

PRIVATE ULONG OrigRootTableContents [NUM_PTR_TABLES];
extern ULONG RootTableContents [NUM_PTR_TABLES];
PRIVATE void *ResetHandlerData;
PRIVATE UWORD ResetSignal;
PRIVATE BOOL RootTableModified;

#define MIN(a,b) ((a)<(b)?(a):(b))


PRIVATE int InitPagetables (void)

/* Physical addresses have to be used. Later the physical address of
 * the allocated chunk is used. For now (68040.library does an identical
 * mapping) we use logical addresses.
 */

{
ULONG *FirstPage;
int i;
int rc;

#if !defined(__AROS__)
NewList (&FrameList);
NewList (&FrameAllocList);
#endif

for (i = 0; i < NUM_PTR_TABLES; i++)
  {
  RootTableContents [i] =
  OrigRootTableContents [i] = *(RootTable + ROOTINDEX ((IPTR)VirtAddrStart) + i);
  *(RootTable + ROOTINDEX ((IPTR)VirtAddrStart) + i) = BUILD_DESCR (LOCUNUSED,
                                                 PAGED_OUT, PT_TABLE);
  }

RootTableModified = TRUE;

if ((FirstPage = AllocAligned (PAGESIZE, MEMF_PUBLIC | MEMF_REVERSE,
                               PAGEALIGN, 0L)) == NULL)
  {
  PRINT_DEB ("Couldn't allocate first frame", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

if ((rc = MarkAddress ((IPTR)VirtAddrStart, PAGESIZE, CACHE_MODE | PAGE_RESIDENT,
                       (IPTR)FirstPage, FALSE)) != SUCCESS)
  {
  FreeMem (FirstPage, PAGESIZE);
  return (rc);
  }

if ((rc = MarkAddress ((IPTR)VirtAddrStart + PAGESIZE,
                  (ULONG)MIN ((IPTR)VirtAddrEnd - ((IPTR)VirtAddrStart + PAGESIZE), PartSize),
                  BUILD_DESCR (LOCUNUSED, PAGED_OUT, 0), 0L, FALSE)) != SUCCESS)
  return (rc);

PRINT_DEB ("Pointer and page table initialized", 0L);

/* Allocate misc page frames */

for (i = 0; i < CurrentConfig.MinMem / PAGESIZE; i++)
  {
  if (!AddFrame (0L))
    {
    PRINT_DEB ("Couldn't allocate %ld bytes", CurrentConfig.MinMem);
    return (ERR_NOT_ENOUGH_MEM);
    }
  }

PRINT_DEB ("Frames allocated", 0L);

CacheClearU ();
#if !defined(__AROS__)
  (*PFlushA) ();
#endif

return (SUCCESS);
}

/*********************************************************************/

PRIVATE void ResetHandler (void)

{
PRINT_DEB ("Sending reset signal to pagehandler", 0L);
Signal (PageHandlerTask, 1L << ResetSignal);

#if !defined(__AROS__)
if (IsA3000)
  {
  UBYTE *ForceReset = (UBYTE*)0xde0002;

  *ForceReset &= ~0x80;
  }

/* The following seems to cause a GURU 80000038 upon reset on A3000's and 
 * sometimes a GURU 8000000b on A4000's. 
 * On a A3000 without this the machine hung upon reset. Now enabled only for A3000
 */

if (IsA3000)
  RestoreMMUState30 ();
#endif
}

/*********************************************************************/

PRIVATE int InitPageHandler (void)

{
int rc;

#if defined(__AROS__)
// (Kalamatee) We need to init these here since they are accessed in the cleanup code if we fail...
NewList (&FrameList);
NewList (&FrameAllocList);
#endif

/* 16 signals per task are guaranteed, so the following three lines
 * can't fail
 */

PageFaultSignal       = AllocSignal (-1L);
PageHandlerQuitSignal = AllocSignal (-1L);
ResetSignal           = AllocSignal (-1L);

ResetInProgress = FALSE;

if ((PageHandlerPort = CreateMsgPort ()) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

bug("[VMM:PageHandler] %s: msg port @ 0x%p\n", __func__, PageHandlerPort);

NumTables = 0;

if ((rc = OpenPageFile ()) != SUCCESS)
  {
  PRINT_DEB ("InitPageHandler: Couldn't open paging file", 0L);
  return (rc);
  }

bug("[VMM:PageHandler] %s: page file opened\n", __func__);
PRINT_DEB ("Paging file opened", 0L);

PartSize = ALIGN_DOWN (PartSize, PAGESIZE);

if ((rc = InitPagetables ()) != SUCCESS)
  return (rc);

bug("[VMM:PageHandler] %s: page tables initialized\n", __func__);
PRINT_DEB ("Page tables initialized", 0L);

if ((rc = InitCache ()) != SUCCESS)
  {
  PRINT_DEB ("Couldn't init cache", 0L);
  return (rc);
  }

bug("[VMM:PageHandler] %s: cache initialized\n", __func__);
  
if ((ResetHandlerData = InstallResetHandler (ResetHandler, -32L)) == NULL)
  PRINT_DEB ("Couldn't install reset handler", 0L);

if (!VMMInstallTrapHandler())
  {
  PRINT_DEB ("InitPageHandler: Failed to install trap handler", 0L);
  return (ERR_INTERNAL);
  }

return (SUCCESS);
}

/***********************************************************************/

PRIVATE void Cleanup_PageHandler (void)

{
int i;

VMMRemoveTrapHandler();

PRINT_DEB ("Before killing pointer tables", 0L);

if (RootTableModified)
  {
  for (i = ROOTINDEX ((IPTR)VirtAddrStart);
       i < ROOTINDEX ((IPTR)VirtAddrStart) + NUM_PTR_TABLES; i++)
    {
    if (TABLE_RESIDENT_P (*(RootTable + i)))
      {
      if (KillPointerTable ((ULONG*)ALIGN_DOWN (*(RootTable + i), POINTERTABALIGN),
                            TRUE))
        {
        MarkPage (*(RootTable + i), CACHEABLE);
        *(RootTable + i) = OrigRootTableContents [i - ROOTINDEX ((IPTR)VirtAddrStart)];
        }
      }
    }

  CacheClearU ();
#if !defined(__AROS__)
  (*PFlushA) ();
#endif
  }

bug("[VMM:PageHandler] %s: removing ResetHandler...\n", __func__);
PRINT_DEB ("Removing reset handler", 0L);
RemoveResetHandler (ResetHandlerData);

PRINT_DEB ("Killing map", 0L);
bug("[VMM:PageHandler] %s: killing Cache map...\n", __func__);
KillCache ();

if (PartSize != 0)
{
  bug("[VMM:PageHandler] %s: closing Pagefile..\n", __func__);
  ClosePageFile ();
}

bug("[VMM:PageHandler] %s: Pagefile closed\n", __func__);
PRINT_DEB ("Page file closed", 0L);

bug("[VMM:PageHandler] %s: freeing Frames...\n", __func__);
RemAllFrames ();

bug("[VMM:PageHandler] %s: Frames freed\n", __func__);
PRINT_DEB ("All frames freed", 0L);

if (PageHandlerPort != NULL)
  DeleteMsgPort (PageHandlerPort);

bug("[VMM:PageHandler] %s: MsgPort deleted\n", __func__);

FreeSignal ((ULONG)PageFaultSignal);
FreeSignal ((ULONG)PageHandlerQuitSignal);
FreeSignal ((ULONG)ResetSignal);
bug("[VMM:PageHandler] %s: Signals freed\n", __func__);
}

/***********************************************************************/

PRIVATE BOOL LockResidentPage (ULONG Page)

/* Tries to lock a page in memory, if it is already resident.
 * It can only lock it if there are still unlocked pages in memory.
 * This has to be handled asynchroneously.
 */
{
ULONG *root_entry,
      *pointer_entry,
      *page_entry;

PRINT_DEB ("Received lock msg for address " str_Addr, Page);

root_entry = RootTable + ROOTINDEX (Page);
pointer_entry = (ULONG*)ALIGN_DOWN (*root_entry, POINTERTABALIGN) +
                POINTERINDEX (Page);
if (TABLE_INVALID_P (*pointer_entry))
  {
  PRINT_DEB ("Couldn't lock page", 0L);
  return (FALSE);  
  }

page_entry = (ULONG*)ALIGN_DOWN (*pointer_entry, PAGETABALIGN) +
             PAGEINDEX (Page);

#if !defined(__AROS__) && defined(__mc68000__)
Forbid ();

if (PAGE_RESIDENT_P (*page_entry))
  {
  if (!LOCKED_P (*page_entry))
    {
    if ((NumLocked + NumTables + 5 > NumPageFrames) && !AddFrame (0L))
      {
      Permit ();
      PRINT_DEB ("Couldn't lock page. NumLocked = %ld", NumLocked);
      return (FALSE);
      }

    PRINT_DEB ("Locking resident page " str_Addr, Page);
    NumLocked++;
    *page_entry |= LOCKED;
    (*PFlushP) (Page);
    }
  Permit ();
  return (TRUE);
  }

Permit ();
#endif
return (FALSE);
}

/***********************************************************************/

PRIVATE void LockPage (ULONG Page)

{
if (LockResidentPage (Page))
  {
  ThisPageLocked = TRUE;
  Signal ((struct Task*)VM_ManagerProcess, 1L << LockAckSignal);
  }
else
  {
  struct TrapStruct *LockFault;

  Forbid ();
  if ((LockFault = (struct TrapStruct*)RemHead (&Free)) == NULL)
    {
    Permit ();
    ThisPageLocked = FALSE;
    Signal ((struct Task*)VM_ManagerProcess, 1L << LockAckSignal);
    return;
    }

  Permit ();
  LockFault->FaultTask = (struct Task*)LOCK_TASK;
  LockFault->FaultAddress = Page;
  AddPageReq (LockFault);
  SetSignal (1L << PageFaultSignal, 1L << PageFaultSignal);
  }
}

/***********************************************************************/

PRIVATE void UnlockAllPages (void)

{
struct FrameDescr *tmp_fd;

for (tmp_fd = (struct FrameDescr*)FrameList.lh_Head;
     tmp_fd->FDNode.mln_Succ != NULL;
     tmp_fd = (struct FrameDescr*) tmp_fd->FDNode.mln_Succ)
  {
#if !defined(__AROS__) && defined(__mc68000__)
  if ((tmp_fd->PageType == PT_PAGE) && LOCKED_P (*(tmp_fd->PageDescr)))
    {
    Forbid ();
    *(tmp_fd->PageDescr) &= ~LOCKED;
    (*PFlushP) (tmp_fd->LogAddr);
    Permit ();
    }
#endif
  }

NumLocked = 0;
}

/***********************************************************************/

PRIVATE void FreePages (ULONG Start, ULONG Size)

{
ULONG start_page,
      end_page,
      tmp,
      *desc;

struct FrameDescr *cur,
                  *succ;

/*
PRINT_DEB ("Received FreePageReq for address " str_Addr, Start);
PRINT_DEB ("                         size    %ld", Size);
*/
start_page = ALIGN_UP(Start, PAGESIZE);
end_page   = ALIGN_DOWN (Start + Size, PAGESIZE);

/* Two passes have to be made: First for all the non-resident pages.
 * For them the corresponding swap page is freed.
 * The second pass loops through all pages frames, marking those as
 * unused which lie within the given address range.
 */

/* First pass */

tmp = start_page;
while (tmp < end_page)
  {
#if !defined(__AROS__) && defined(__mc68000__)
  desc = (ULONG*)ALIGN_DOWN (*(RootTable + ROOTINDEX (tmp)), POINTERTABALIGN);
  desc = desc + POINTERINDEX (tmp);

  if (TABLE_INVALID_P (*desc))
    {
    tmp += PAGES_PER_TABLE * PAGESIZE;
    continue;
    }

  desc = (ULONG*)ALIGN_DOWN (*desc, PAGETABALIGN) + PAGEINDEX (tmp);
  tmp += PAGESIZE;
  if (PAGE_RESIDENT_P (*desc))
    continue;          /* Not interested in resident pages in this pass */

  if (STATE_FROM_DESCR(*desc) == COMING_IN)
    continue;

  Forbid ();
  if (PGNUM_FROM_DESCR (*desc) != LOCUNUSED)
    {
    FreePageOnDisk (PGNUM_FROM_DESCR (*desc));
    *desc = BUILD_DESCR (LOCUNUSED, PAGED_OUT, PT_PAGE);
    (*PFlushP) (tmp-PAGESIZE);
    }
  Permit ();
#endif
  }

/* Second pass */

/* Because cur might be removed at the end of the loop,
 * its successor is stored at the start of each looping
 */

end_page -= PAGESIZE;
for (cur = (struct FrameDescr*)FrameList.lh_Head,
     succ = (struct FrameDescr*)cur->FDNode.mln_Succ;
     cur->FDNode.mln_Succ != NULL;
     cur = succ, succ = (struct FrameDescr*)cur->FDNode.mln_Succ)
  {
  if ((cur->PageType == PT_PAGE) &&
      (cur->LogAddr >= start_page) && (cur->LogAddr <= end_page))
    {
#if !defined(__AROS__) && defined(__mc68000__)
    Forbid ();
    *(cur->PageDescr) &= ~(USED | MODIFIED);
    (*PFlushP) (cur->LogAddr);
    Permit ();
#endif
    cur->Modified = FALSE;
    if (cur->PageNumOnDisk != LOCUNUSED)
      {
      FreePageOnDisk (cur->PageNumOnDisk);
      cur->PageNumOnDisk = LOCUNUSED;
      }
    Remove ((struct Node*)cur);
    AddHead (&FrameList, (struct Node*)cur);
    }
  }
}

/***********************************************************************/

PRIVATE void NewConfigReceived (void)

{
PRINT_DEB ("New config received", 0L);

if (NumPageFrames * PAGESIZE < CurrentConfig.MinMem)
  {
  /* There's memory missing */
  int total_frames;

  PRINT_DEB ("Adding mem until %ld bytes", CurrentConfig.MinMem);
  total_frames = CurrentConfig.MinMem / PAGESIZE;
  while ((NumPageFrames < total_frames) && AddFrame (0L));
  }
else if (NumPageFrames * PAGESIZE > CurrentConfig.MaxMem)
  {
  struct TrapStruct *DummyTrap;

  PRINT_DEB ("Removing mem until %ld bytes", CurrentConfig.MaxMem);
  /* Fool the pagehandler by simulating a low memory condition.
   * InformTask has to handle this as a special case.
   */
  Forbid ();
  DummyTrap = (struct TrapStruct*)RemHead (&Free);
  Permit ();
  if (DummyTrap == NULL)
    return;
  DummyTrap->FaultAddress = 0;     /* Marker for remove */
  DummyTrap->RemFrameFlags = MEMF_ANY;
  DummyTrap->FaultTask = (struct Task*)REDUCE_TASK;
  AddPageReq (DummyTrap);
  LowMem++;
  SetSignal (1L << PageFaultSignal, 1L << PageFaultSignal);
  }
}

/***********************************************************************/

PRIVATE void HandleVMMsg (void)

{
struct VMMsg *VMMsg;

while ((VMMsg = (struct VMMsg*)GetMsg (PageHandlerPort)) != NULL)
  {
  switch (VMMsg->VMCommand)
    {
    case VMCMD_LockPage: 
               LockPage (VMMsg->PageAddress);
               break;

    case VMCMD_UnlockAllPages:
               UnlockAllPages ();
               break;

    case VMCMD_FreePages:
/*             PRINT_DEB ("FreeRequest from task", 0L);
               PRINT_DEB (VMMsg->VMSender->tc_Node.ln_Name, 0L);
*/
               FreePages (VMMsg->FreeAddress, VMMsg->FreeSize);
               break;

    case VMCMD_NewConfig:
               NewConfigReceived ();
               break;

    case VMCMD_NewWriteBuffer:
               PRINT_DEB ("Received new write buffer. Size: %ld", VMMsg->NewWriteBufferSize);
               NewCache (VMMsg->NewWriteBuffer, VMMsg->NewWriteBufferSize);
               break;

    default: PRINT_DEB ("Received unknown msg type", (ULONG)VMMsg->VMCommand);
    }
  if (VMMsg->ReplySignal != 0)
    Signal (VMMsg->VMSender, 1L << VMMsg->ReplySignal);
  else
    FreeMem (VMMsg, sizeof (struct VMMsg));
  }
}

/***********************************************************************/

void PageHandler (void)

{
ULONG ReceivedSignals;
ULONG WaitMask;
struct VMMsg *InitMsg;
int rc;
BOOL quit = FALSE;

  bug("[VMM:PageHandler] %s: initializing...\n", __func__);

if ((rc = InitPageHandler ()) != SUCCESS)
  {
  bug("[VMM:PageHandler] %s: initialization failed\n", __func__);

  PRINT_DEB ("Init failed", 0L);
  InitError (rc);
  bug("[VMM:PageHandler] %s: cleaning up..\n", __func__);
  Cleanup_PageHandler ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

  bug("[VMM:PageHandler] %s: initialized\n", __func__);

/* Tell VM_Manager that the page handler has been initialized correctly */
if ((InitMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  {
  bug("[VMM:PageHandler] %s: failed to alloc mem for init msg\n", __func__);
  PRINT_DEB ("Not enough memory for init msg", 0L);
  Cleanup_PageHandler ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

InitMsg->VMSender = FindTask (NULL);
InitMsg->VMCommand = VMCMD_InitReady;
InitMsg->ReplySignal = 0;          /* VM_Manager should free this */
PutMsg (InitPort, (struct Message*)InitMsg);

WaitMask = (1L << PageFaultSignal) |
           (1L << PageHandlerQuitSignal) |
           (1L << ReplySignal) |
           (1L << PageHandlerPort->mp_SigBit) |
           (1L << ResetSignal);

  bug("[VMM:PageHandler] %s: waiting for work...\n", __func__);

while (!quit)
  {
  /* PRINT_DEB ("Waiting for signals", 0L); */
  ReceivedSignals = Wait (WaitMask);
  /* PRINT_DEB ("Received signals " str_Addr, ReceivedSignals); */

  if (ReceivedSignals & (1L << PageFaultSignal))
    {
    /* PRINT_DEB ("Received page fault signal", 0L); */
    HandlePageFault ();
    }

  if (ReceivedSignals & (1L << ReplySignal))
    {
    /* handle return from paging device.
     * Page has been either written out or read in.
     */
    /* PRINT_DEB ("Received return signal", 0L); */
    HandleReturn ();
    CheckWaitingFaults ();
    }

  if (ReceivedSignals & (1L << PageHandlerPort->mp_SigBit))
    {
    /* PRINT_DEB ("VMMsg received", 0L); */
    HandleVMMsg ();
    }

  if (ReceivedSignals & (1L << PageHandlerQuitSignal))
    {
    PRINT_DEB ("Received quit signal", 0L);
    quit = TRUE;
    }

  if (ReceivedSignals & (1L << ResetSignal))
    {
    PRINT_DEB ("Received ResetSignal", 0L);
    PRINT_DEB ("PagefaultsInProgress = %ld", (LONG)PageFaultsInProgress);
    ResetInProgress = TRUE;
    }

  if (ResetInProgress && PageFaultsInProgress == 0)
    {
    if (CurrentConfig.PageDev == PD_FILE)
      {
      PRINT_DEB ("Waiting 2 seconds", 0L);
      Delay (100L);  /* Wait 2 seconds for the fs to calm down */
      }
    PRINT_DEB ("No pagefaults in progress. Ready for reset", 0L);
    PRINT_DEB ("Allowing reset", 0L);
    ResetHandlerDone (ResetHandlerData);
    }
  }

Cleanup_PageHandler ();
PRINT_DEB ("Exiting", 0L);
}
