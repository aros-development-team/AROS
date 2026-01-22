#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: fault.c,v 3.6 95/12/16 18:36:51 Martin_Apel Exp $";

PRIVATE int PagesWaiting = 0;

/*******************************************************************/

void AddFree (struct TrapStruct *ThisFault)

{
Forbid ();
AddTail (&Free, (struct Node*) ThisFault);
Permit ();
}

/*******************************************************************/

void AddPageReq (struct TrapStruct *ThisFault)

{
Forbid ();
AddTail (&PageReq, (struct Node*) ThisFault);
Permit ();
}

/*******************************************************************/

PRIVATE void InformTask (struct TrapStruct *ThisFault)

{
/* The page is back in memory. Signal the faulting task.
 */

PageFaultsInProgress--;
switch ((IPTR)ThisFault->FaultTask)
  {
  case REDUCE_TASK:  
       /* The pagehandler is trying to reduce the paging memory.
        * Check if it's enough, otherwise free another frame.
        */
       PRINT_DEB ("InformTask: Reduce", 0L);
       if ((NumPageFrames * PAGESIZE > CurrentConfig.MaxMem) && 
           (ThisFault->FaultAddress != 0))
         {
         /* Do it again */
         ThisFault->FaultAddress = 0;
         AddPageReq (ThisFault);
         SetSignal (1L << PageFaultSignal, 1L << PageFaultSignal);
         }
       else
         {
         PRINT_DEB ("Finished reducing memory usage", 0L);
         AddFree (ThisFault);
         LowMem--;
         }
       break;  

  /*********************************/
  case LOCK_TASK:
       PRINT_DEB ("InformTask: Lock", 0L);
       if (ThisFault->PageDescrAddr == NULL)
         {
         /* Just fetched the table. Do it again */
         AddPageReq (ThisFault);
         SetSignal (1L << PageFaultSignal, 1L << PageFaultSignal);
         }
       else
         {
         ThisPageLocked = (NumLocked + NumTables + 5 <= NumPageFrames) ||
                          AddFrame (0L);

         if (ThisPageLocked)
           {
           NumLocked++;
           Forbid ();
           *(ThisFault->PageDescrAddr) |= LOCKED;
           (*PFlushP) (ThisFault->FaultAddress);
           Permit ();
           }
         else
           PRINT_DEB ("Couldn't lock page. NumLocked = %ld", NumLocked);

         AddFree (ThisFault);
         Signal ((struct Task*)VM_ManagerProcess, 1L << LockAckSignal);
         }
       break;

  /*********************************/
  default:
/*     PRINT_DEB ("Signalling waiting task " str_Addr, (ULONG)ThisFault->FaultTask); */
       CHECK_CONSISTENCY;
       Signal (ThisFault->FaultTask, 1L << ThisFault->WakeupSignal);
  }
}
/***********************************************************************/

void CheckWaitingFaults (void)

{
struct TrapStruct *ThisFault,
                  *Successor;

if (PagesWaiting == 0)
  return;

/* Because ThisFault might be removed at the end of the loop,
 * its successor is stored at the start of each looping
 */

PRINT_DEB ("Checking for waiting pages", 0L);

for (ThisFault = (struct TrapStruct*)InTransit.lh_Head,
     Successor = (struct TrapStruct*)ThisFault->TS_Node.mln_Succ;
     ThisFault->TS_Node.mln_Succ != NULL;
     ThisFault = Successor,
     Successor = (struct TrapStruct*)ThisFault->TS_Node.mln_Succ)
  {
  if (((ThisFault->PageDescrAddr == NULL) &&        /* Table has come in */
       (TABLE_RESIDENT_P (*(ThisFault->TableDescrAddr))))
                          ||
      ((ThisFault->PageDescrAddr != NULL) &&
        PAGE_RESIDENT_P (*(ThisFault->PageDescrAddr))))  /* Page has come in */
    {
    PRINT_DEB ("Signalling waiting task", 0L);
    Remove ((struct Node*)ThisFault);
    PagesWaiting--;
    InformTask (ThisFault);
    }
  }
}

/***********************************************************************/

PRIVATE void MakePageResident (struct TrapStruct *ThisFault)

{
struct FrameDescr *NewPage;

NewPage = ThisFault->FD;
NewPage->PageType = PT_PAGE;
NewPage->PageDescr = ThisFault->PageDescrAddr;
NewPage->LogAddr = PAGEADDR(ThisFault->FaultAddress);
NewPage->PageNumOnDisk = PGNUM_FROM_DESCR (*(ThisFault->PageDescrAddr));
NewPage->Modified = FALSE;

Forbid ();
*(ThisFault->PageDescrAddr) = ThisFault->PhysAddr | CACHE_MODE | PAGE_RESIDENT |
           ((ThisFault->FaultTask == (struct Task*)LOCK_TASK) ? LOCKED : 0);
(*PFlushP) (ThisFault->FaultAddress);
(*FlushVirt) (NewPage->LogAddr);
(*CPushP) (NewPage->PhysAddr);
Permit ();

#ifdef NEW_PRA
ReuseLate (ThisFault->FD);
#else
AddTail (&FrameList, (struct Node*)ThisFault->FD);
ThisFault->FD->InFrameList = TRUE;
#endif
}

/*****************************************************************/

PRIVATE void MakeTableResident (struct TrapStruct *ThisFault)

{
struct FrameDescr *NewTable;
ULONG *TableStart;
int i;
#ifdef DEBUG
ULONG *cur_descr;
#endif

PRINT_DEB ("MakeTableResident: for address " str_Addr, ThisFault->FaultAddress);

NewTable = ThisFault->FD;
NewTable->PageType = PT_TABLE;
NewTable->LogTableStart = ALIGN_DOWN (ThisFault->FaultAddress,
                                      PAGESIZE * PAGESIZE / sizeof (ULONG));
NewTable->PageNumOnDisk = PGNUM_FROM_DESCR (*(ThisFault->TableDescrAddr));
PRINT_DEB ("MakeTableResident: PageNumOnDisk = " str_Addr, NewTable->PageNumOnDisk);
NewTable->Modified = FALSE;

/* One Page of page descriptors contains 16 page tables. So the
 * pointers in the pointer table for those 16 page tables have to be
 * marked as valid
 */

#ifdef DEBUG
cur_descr = (ULONG*)ThisFault->PhysAddr;
for (i = 0; i < PAGESIZE / sizeof (ULONG); i++)
  {
  if (STATE_FROM_DESCR (*cur_descr++) == COMING_IN)
    {
    PRINT_DEB ("ERROR: Reading table with incoming pages", 0L);
    ColdReboot ();
    break;
    }
  }
#endif

TableStart = (ULONG*)ALIGN_DOWN (*(RootTable + ROOTINDEX (ThisFault->FaultAddress)),
                                 POINTERTABALIGN);
TableStart += ALIGN_DOWN (POINTERINDEX (ThisFault->FaultAddress),
                          PAGESIZE / sizeof (ULONG) / PAGES_PER_TABLE);

MarkPage ((IPTR)TableStart, NONCACHEABLE);

PRINT_DEB ("TableStart = " str_Addr, (IPTR)TableStart);

#if !defined(__AROS__)
Forbid ();
for (i = 0; i < PAGESIZE / sizeof (ULONG) / PAGES_PER_TABLE; i++)
  {
  *(TableStart + i) =
    (ThisFault->PhysAddr + i * PAGES_PER_TABLE * sizeof (ULONG)) | TABLE_RESIDENT;
  }

(*PFlushA)();
Permit ();
#endif
#ifdef NEW_PRA
ReuseLate (ThisFault->FD);
#else
AddTail (&FrameList, (struct Node*)ThisFault->FD);
ThisFault->FD->InFrameList = TRUE;
#endif
NumTables++;
}


/*****************************************************************/

PRIVATE void MakePageInvalid (struct FrameDescr *fd)

{
#ifdef DEBUG
if (!fd->InFrameList)
  {
  PRINT_DEB ("MakePageInvalid: Frame not in frame list", 0L);
  ColdReboot ();
  }
#endif

#ifdef NEW_PRA
RemoveFromAvail (fd);
#else
Remove ((struct Node*)fd);
fd->InFrameList = FALSE;
#endif

#if !defined(__AROS__)
Forbid ();
*(fd->PageDescr) = BUILD_DESCR (fd->PageNumOnDisk, PAGED_OUT, PT_PAGE);
(*PFlushP) (fd->LogAddr);
(*CPushP) (fd->PhysAddr);
(*FlushVirt) (fd->LogAddr);
Permit ();
#endif
}

/*****************************************************************/

PRIVATE void MakeTableInvalid (struct FrameDescr *fd)

{
ULONG *TableStart;
int i;

PRINT_DEB ("MakeTableInvalid: PageNumOnDisk = " str_Addr, fd->PageNumOnDisk);

#ifdef DEBUG
if (!fd->InFrameList)
  {
  PRINT_DEB ("MakeTableInvalid: Frame not in frame list", 0L);
  ColdReboot ();
  }
#endif

#ifdef NEW_PRA
RemoveFromAvail (fd);
#else
Remove ((struct Node*)fd);
fd->InFrameList = FALSE;
#endif

#if !defined(__AROS__)
TableStart = (ULONG*)ALIGN_DOWN (*(RootTable + ROOTINDEX (fd->LogTableStart)),
                                 POINTERTABALIGN);
TableStart += POINTERINDEX (fd->LogTableStart);

Forbid ();
for (i = 0; i < PAGESIZE / sizeof (ULONG) / PAGES_PER_TABLE; i++)
  {
  *(TableStart + i) = BUILD_DESCR (fd->PageNumOnDisk, PAGED_OUT, PT_TABLE);
  }

(*PFlushA) ();
Permit ();
#endif
MarkPage ((IPTR)TableStart, CACHEABLE);
NumTables--;
}

/*****************************************************************/

PRIVATE void InstallNewTable (struct TrapStruct *ThisFault)

{
ULONG *TableStart;
int i;

PRINT_DEB ("InstallNewTable called", 0L);

#if !defined(__AROS__)
/* Make all page descriptors in this page invalid */
TableStart = (ULONG*)ThisFault->PhysAddr;    /* Start of page table */
for (i = 0; i < PAGESIZE / sizeof (ULONG); i++)
  {
  *(TableStart + i) = BUILD_DESCR (LOCUNUSED, PAGED_OUT, PT_PAGE);
  }
#endif

MakeTableResident (ThisFault);
}

/*****************************************************************/

#ifndef NEW_PRA
PRIVATE BOOL TablePagingAllowed (struct FrameDescr *table_frame)

{
ULONG *cur_descr;
int i;

#if !defined(__AROS__)
cur_descr = (ULONG*) table_frame->PhysAddr;
for (i = 0; i < PAGESIZE / sizeof (ULONG); i++, cur_descr++)
  {
  if (PAGE_RESIDENT_P (*cur_descr) || (STATE_FROM_DESCR (*cur_descr) == COMING_IN))
    return (FALSE);
  }
#endif
return (TRUE);
}
#endif

/*****************************************************************/

#define PAGE_NOT_IN_USE(fd) (((fd)->PageType == PT_PAGE) &&\
                             ((!PAGE_RESIDENT_P(*((fd)->PageDescr))) &&\
                              !MODIFIED_P(*fd)))

PRIVATE short Evict (struct TrapStruct *ThisFault)

{
/* A page fault has occurred. Find a page to put on disk.
 * Writes the physical address of the evicted page frame into
 * ThisFault->PhysAddr.
 * This routine returns after sending the write request to the handler
 * The chosen frame is removed from FrameList until it is used again
 */

struct FrameDescr *chosen,
                  *cur_frame;
BOOL found = FALSE;
static ULONG AvailableForFrames = 100000L;

/* Add another frame only when all of the following conditions apply:
 *   Dynamic policy
 *   Not currently evicting page-frames to make room for some physical request
 *   First frame is not empty
 *   First frame is not unused and unmodified (probably generated by a 
 *   FreeMem which marked this frame in this way.
 */
if ((NumPageFrames * PAGESIZE < CurrentConfig.MaxMem) && (!LowMem) &&
   (((struct FrameDescr*)(FrameList.lh_Head))->PageType != PT_EMPTY) &&
   !PAGE_NOT_IN_USE(((struct FrameDescr*)(FrameList.lh_Head))))

  {
  /* This way AddFrame will only retry allocation of a frame if some
   * other memory has been freed in the meantime (since the last
   * allocation failed).
   */
  if (AddFrame (AvailableForFrames))
    AvailableForFrames = 1L;
  else
    AvailableForFrames = DoOrigAvailMem (CurrentConfig.MemFlags);
  }

#ifdef NEW_PRA
chosen = FindRemoveablePage ();
#else

do
  {
  cur_frame = (struct FrameDescr*)RemHead (&FrameList);
#ifdef SCHED_STAT
  FramesExamined++;
#endif
#ifdef DEBUG
  if (cur_frame == NULL)
    {
    PRINT_DEB ("Internal error: FrameList empty", 0L);
    ColdReboot ();
    }
#endif
  AddTail (&FrameList, (struct Node*)cur_frame);

  if (cur_frame->PageType == PT_EMPTY)
    {
    ThisFault->PhysAddr = (ULONG)cur_frame->PhysAddr;
    ThisFault->FD = cur_frame;
    Remove ((struct Node*)cur_frame);
    cur_frame->InFrameList = FALSE;
    return (READY);
    }

  if ((cur_frame->PageType == PT_PAGE) && LOCKED_P (*(cur_frame->PageDescr)))
    ;
  else if ((cur_frame->PageType == PT_PAGE) &&
           (USED_P (*cur_frame->PageDescr)))
    {
    Forbid ();
    if (MODIFIED_P (*cur_frame))
      cur_frame->Modified = TRUE;
    *cur_frame->PageDescr &= ~(USED | MODIFIED);
    (*PFlushP) (cur_frame->LogAddr);
    Permit ();
    }
  else if (cur_frame->PageType == PT_TABLE)
    {
    /* Check if any of the descriptors in this page are still valid.
     * Otherwise this page table can be written to disk.
     */
    if (TablePagingAllowed (cur_frame))
      found = TRUE;
    }
  else
    found = TRUE;             /* Found an unused page */
  }
while (!found);

chosen = cur_frame;
#endif

/* Empty pages cannot occur here */
ThisFault->PhysAddr = chosen->PhysAddr;

ThisFault->FD = chosen;

#ifdef DEBUG

if (chosen->PageType == PT_PAGE)
  /* PRINT_DEB ("Evicting page frame for log. address " str_Addr, chosen->LogAddr) */;
else if (chosen->PageType == PT_TABLE)
  PRINT_DEB ("Evicting table frame for address range " str_Addr, chosen->LogTableStart);
else
  {
  PRINT_DEB ("Internal error: Empty page in Evict. Type %ld", (LONG)chosen->PageType);
  ColdReboot ();
  }
#endif
/* PRINT_DEB ("Physical address is " str_Addr, ThisFault->PhysAddr); */

Forbid ();          /* Have to forbid here, otherwise this page could
                     * be modified after it has been tested for modification.
                     */
if ((chosen->PageType == PT_PAGE) && !MODIFIED_P (*chosen))
  {
  /* Don't need to write this one back.
   * Mark this address as invalid
   */
  MakePageInvalid (chosen);
  Permit ();
  return (READY);
  }
else
  {
  /* Write out modified page first */

  if (chosen->PageNumOnDisk != LOCUNUSED)
    FreePageOnDisk (chosen->PageNumOnDisk);
  chosen->PageNumOnDisk = AllocPageOnDisk ();

  if (chosen->PageType == PT_TABLE)
    MakeTableInvalid (chosen);
  else
    MakePageInvalid (chosen);

  Permit ();
  if (chosen->PageType == PT_TABLE)
    {
    PRINT_DEB ("PhysTableStart is " str_Addr, (ULONG)chosen->PhysAddr);
    PRINT_DEB ("LogTableStart is " str_Addr, chosen->LogTableStart);
    PRINT_DEB ("PageNumOnDisk = " str_Addr, (ULONG)chosen->PageNumOnDisk);
    (*PFlushP) (ThisFault->FaultAddress);
    }
  return (WritePage (chosen->PageNumOnDisk, ThisFault));
  }
}

/**********************************************************************/

PRIVATE short GetNewPage (struct TrapStruct *ThisFault)

{
/* There's a place free at physical address ThisFault->PhysAddr.
 * Either read the needed page from disk or if this is the first reference
 * install a new one
 */

if (PGNUM_FROM_DESCR (*(ThisFault->PageDescrAddr)) == LOCUNUSED)
  {
  /* PRINT_DEB ("Installing new page", 0L); */
  MakePageResident (ThisFault);
  return (READY);
  }
else
  {
  if (ReadPage (PGNUM_FROM_DESCR (*(ThisFault->PageDescrAddr)), ThisFault) == READY)
    {
    MakePageResident (ThisFault);
    return (READY);
    }
  return (IN_PROGRESS);
  }
}

/**********************************************************************/

PRIVATE short GetNewTable (struct TrapStruct *ThisFault)

{
if (PGNUM_FROM_DESCR (*(ThisFault->TableDescrAddr)) == LOCUNUSED)
  {
  PRINT_DEB ("Installing new page table", 0L);
  InstallNewTable (ThisFault);
  return (READY);
  }
else
  {
  if (ReadPage (PGNUM_FROM_DESCR (*(ThisFault->TableDescrAddr)), ThisFault) == READY)
    {
    MakeTableResident (ThisFault);
    return (READY);
    }
  return (IN_PROGRESS);
  }
}

/***********************************************************************/

PRIVATE void HandleMissingPage (struct TrapStruct *ThisFault)

{
if (PAGE_RESIDENT_P (*(ThisFault->PageDescrAddr)))
  {
  /* Page was just requested by another task and has already come in */
  PRINT_DEB ("Faulted page has already come in", 0L);
  InformTask (ThisFault);
  }
else if (STATE_FROM_DESCR (*(ThisFault->PageDescrAddr)) == COMING_IN)
  {
  /* Page was just requested by another task and is in transit */
  AddTail (&InTransit, (struct Node*)ThisFault);
  PagesWaiting++;
  PRINT_DEB ("Putting fault to sleep", 0L);
  PRINT_DEB ("TableDescr is at " str_Addr, (IPTR)ThisFault->TableDescrAddr);
  PRINT_DEB ("TableDescr contains " str_Addr, *(ThisFault->TableDescrAddr));
  PRINT_DEB ("PageDescr is at " str_Addr, (IPTR)ThisFault->PageDescrAddr);
  PRINT_DEB ("PageDescr contains " str_Addr, *(ThisFault->PageDescrAddr));
  }
else
  {
  /* Start handling */
#if !defined(__AROS__)
  *(ThisFault->PageDescrAddr) =
     BUILD_DESCR (PGNUM_FROM_DESCR (*(ThisFault->PageDescrAddr)),
                  COMING_IN, TYPE_FROM_DESCR (*(ThisFault->PageDescrAddr)));

  (*PFlushP) (ThisFault->FaultAddress);
#endif
  if (Evict (ThisFault) == READY)
    {
    if (GetNewPage (ThisFault) == READY)
      {
      InformTask (ThisFault);
      CheckWaitingFaults ();
      }
    }
  }
}

/***********************************************************************/

PRIVATE void HandleMissingTable (struct TrapStruct *ThisFault)

{
PRINT_DEB ("Table missing", 0L);

if (TABLE_RESIDENT_P (*(ThisFault->TableDescrAddr)))
  {
  /* Table was requested by another task and has already come in */
  PRINT_DEB ("Faulted table has already come in", 0L);
  InformTask (ThisFault);
  }
else if (STATE_FROM_DESCR (*(ThisFault->TableDescrAddr)) == COMING_IN)
  {
  PRINT_DEB ("Requested table is coming in", 0L);
  AddTail (&InTransit, (struct Node*)ThisFault);
  PagesWaiting++;
  }
else
  {
#if !defined(__AROS__)
  /* Get this table */
  *(ThisFault->TableDescrAddr) =
     BUILD_DESCR (PGNUM_FROM_DESCR (*(ThisFault->TableDescrAddr)),
                  COMING_IN, TYPE_FROM_DESCR (*(ThisFault->TableDescrAddr)));

  (*PFlushA) ();
#endif
  if (Evict (ThisFault) == READY)
    {
    if (GetNewTable (ThisFault) == READY)
      {
      InformTask (ThisFault);
      CheckWaitingFaults ();
      }
    }
  }
}

/***********************************************************************/

PRIVATE struct FrameDescr *FindFrame (ULONG RequiredFlags)

{
/* Finds an evictable frame with the desired memory flags and returns it
 * In order to free the frames the reverse way they were allocated,
 * the following is done:
 * One frame to be evicted is chosen in the standard way.
 * A frame from the front of the FrameOrderList is chosen, which
 * is not currently involved in paging (PageDescr != INVALID).
 * This frame is copied to the evicted frame. The memory of the chosen
 * frame is freed then.
 */

struct FrameDescr *tmp_fd;
struct MinNode    *tmp_alloc_node;
BOOL found = FALSE;

tmp_alloc_node = (struct MinNode*)FrameAllocList.lh_Head;

while (!found)
  {
  tmp_fd = (struct FrameDescr*)(tmp_alloc_node - 1);
  if (tmp_fd->InFrameList)
    {
    switch (tmp_fd->PageType)
      {
      case PT_PAGE : if (!LOCKED_P (*(tmp_fd->PageDescr)))
                       found = TRUE;
                     break;
      case PT_TABLE: if (TablePagingAllowed (tmp_fd))
                       found = TRUE;
                     break;
      case PT_EMPTY: found = TRUE;
                     break;
      }
    }

  /* Makes use of the evaluation order of boolean expressions */
  if (!found || ((RequiredFlags & TypeOfMem ((APTR)tmp_fd->PhysAddr))
                 != RequiredFlags))
    {
    found = FALSE;
    tmp_alloc_node = tmp_alloc_node->mln_Succ;
    if (tmp_alloc_node->mln_Succ == 0)
      return (NULL);
    }
  }

return (tmp_fd);
}

/***********************************************************************/

PRIVATE short RemFrameFinish (struct TrapStruct *ThisFault)

{

struct FrameDescr *rem_frame;           /* The frame to be removed */
IPTR phys_addr;
BOOL modified;
ULONG DesiredFlags;

/* If more than 25 frames are requested, frames are not copied.
 * Apparently more frames are needed so this will only waste
 * time.
 */

if ((IPTR)ThisFault->RemFrameSize - 
    (IPTR)DoOrigAvailMem (ThisFault->RemFrameFlags | MEMF_PUBLIC) 
    > 25 * PAGESIZE)
  {
  PRINT_DEB ("Needing a lot of frames. Not copying", 0L);
  Remove ((struct Node*)&(ThisFault->FD->OrderNode));
  FreeMem ((APTR)ThisFault->FD->PhysAddr, PAGESIZE);
  FreeMem (ThisFault->FD, sizeof (struct FrameDescr));
  ThisFault->FramesRemoved++;
  ThisFault->RemFrameSize -= PAGESIZE;
  NumPageFrames--;

  /* Try again */
  Forbid ();
  AddHead (&PageReq, (struct Node*) ThisFault);
  Permit ();
  SetSignal (1L << PageFaultSignal, 1L << PageFaultSignal);
  PageFaultsInProgress--;
  PRINT_DEB ("Retrying RemFrame", 0L);

  PRINT_DEB ("Page frames left: %ld", NumPageFrames);
  return (IN_PROGRESS);
  }

DesiredFlags = ThisFault->RemFrameFlags &
               (MEMF_PUBLIC | MEMF_FAST | MEMF_CHIP);

/* If chip memory is not required explicitly, try to free a frame in
 * fast memory. This way program code is run much faster.
 */
if (!(DesiredFlags & MEMF_CHIP))
  {
  if ((rem_frame = FindFrame (DesiredFlags | MEMF_FAST)) == NULL)
    rem_frame = FindFrame (DesiredFlags);
  }
else
  rem_frame = FindFrame (DesiredFlags);

if (rem_frame == NULL)
  {
  PRINT_DEB ("Didn't find frame with desired flags " str_Addr, DesiredFlags);
  ThisFault->FD->PageType = PT_EMPTY;
  ThisFault->FD->Modified = FALSE;
  ThisFault->FD->PageNumOnDisk = LOCUNUSED;
  AddHead (&FrameList, (struct Node*)ThisFault->FD);
  ThisFault->FD->InFrameList = TRUE;
  return (READY);
  }

phys_addr = rem_frame->PhysAddr;
Forbid ();
switch (rem_frame->PageType)
  {
  case PT_PAGE:
#if !defined(__AROS__)
    (*FlushVirt) (phys_addr);        /* == logical address here */
#endif
    CopyMemQuick ((APTR)phys_addr,
                  (APTR)ThisFault->PhysAddr, PAGESIZE);
    modified = MODIFIED_P (*rem_frame);
    MakePageInvalid (rem_frame);
    ThisFault->FaultAddress = rem_frame->LogAddr;
    ThisFault->PageDescrAddr = rem_frame->PageDescr;
    MakePageResident (ThisFault);
    ThisFault->FD->Modified = modified;
    break;
  case PT_TABLE:
    PRINT_DEB ("RemFrameFinish: Removing table. Copying to " str_Addr, ThisFault->PhysAddr);
#if !defined(__AROS__)
    (*FlushVirt) (phys_addr);        /* == logical address here */
#endif
    CopyMemQuick ((APTR)phys_addr,
                  (APTR)ThisFault->PhysAddr, PAGESIZE);
    MakeTableInvalid (rem_frame);
    ThisFault->FaultAddress = rem_frame->LogTableStart;
    ThisFault->TableDescrAddr = (ULONG*)
               ALIGN_DOWN (*(RootTable + ROOTINDEX (ThisFault->FaultAddress)),
                           POINTERTABALIGN);
    ThisFault->TableDescrAddr += POINTERINDEX (ThisFault->FaultAddress);
    MakeTableResident (ThisFault);
    break;
  case PT_EMPTY:
    PRINT_DEB ("RemFrameFinish: Removing empty page", 0L);
    ThisFault->FD->PageType = PT_EMPTY;
    ThisFault->FD->Modified = FALSE;
    ThisFault->FD->PageNumOnDisk = LOCUNUSED;
#ifdef DEBUG
    if (!rem_frame->InFrameList)
      {
      PRINT_DEB ("RemFrameFinish: Removing frame not in frame list", 0L);
      ColdReboot ();
      }
#endif
    Remove ((struct Node*)rem_frame);
    rem_frame->InFrameList = FALSE;
    AddHead (&FrameList, (struct Node*)ThisFault->FD);
    ThisFault->FD->InFrameList = TRUE;
    break;
  }

Permit ();

Remove ((struct Node*)&(rem_frame->OrderNode));
FreeMem ((APTR)rem_frame->PhysAddr, PAGESIZE);
FreeMem (rem_frame, sizeof (struct FrameDescr));
ThisFault->FramesRemoved++;
NumPageFrames--;

PRINT_DEB ("Page frames left: %ld", NumPageFrames);
return (READY);
}

/***********************************************************************/

PRIVATE void RemFrame (struct TrapStruct *ThisFault)

{
PRINT_DEB ("RemFrame called with flags " str_Addr, ThisFault->RemFrameFlags);
PRINT_DEB ("                     size  " str_Addr, ThisFault->RemFrameSize);

if (((CurrentConfig.PageDev == PD_FILE) && 
     (ThisFault->FaultTask == (struct Task*)FileHandlerProcess)) ||
     (NumPageFrames * PAGESIZE <= CurrentConfig.MinMem) || 
     ((NumLocked + 3) >= NumPageFrames))
  {
  PRINT_DEB ("Not enough frames", 0L);
  InformTask (ThisFault);
  return;
  }

if (Evict (ThisFault) == READY)
  {
  if (RemFrameFinish (ThisFault) == READY)
    InformTask (ThisFault);
  }
}

/***********************************************************************/

void HandlePageFault (void)

{
struct TrapStruct *ThisFault;
ULONG *PageTable;

Forbid ();
ThisFault = (struct TrapStruct*)RemHead (&PageReq);
Permit ();

while (ThisFault != NULL)
  {
  /* PRINT_DEB ("FaultAddress is %08lx", ThisFault->FaultAddress); */
  PageFaultsInProgress++;
  CHECK_CONSISTENCY;

  if (ThisFault->FaultAddress == 0)
    RemFrame (ThisFault);          /* Free a frame */
  else 
    {
    NumPageFaults++;

    ThisFault->TableDescrAddr = (ULONG*)
               ALIGN_DOWN (*(RootTable + ROOTINDEX (ThisFault->FaultAddress)),
                           POINTERTABALIGN);
    ThisFault->TableDescrAddr += POINTERINDEX (ThisFault->FaultAddress);

    if (TABLE_RESIDENT_P (*(ThisFault->TableDescrAddr)))
      {
      PageTable = (ULONG*) ALIGN_DOWN (*(ThisFault->TableDescrAddr), PAGETABALIGN);
      ThisFault->PageDescrAddr = PageTable + PAGEINDEX(ThisFault->FaultAddress);
      HandleMissingPage (ThisFault);
      }
    else
      {
      ThisFault->PageDescrAddr = NULL;
      HandleMissingTable (ThisFault);
      }
    }
  Forbid ();
  ThisFault = (struct TrapStruct*)RemHead (&PageReq);
  Permit ();
  }
}

/*****************************************************************/

void ReadReturned (struct TrapStruct *ThisFault)

{
/* PRINT_DEB ("Returned from reading", 0L); */

if (ThisFault->PageDescrAddr == NULL)
  MakeTableResident (ThisFault);
else
  MakePageResident (ThisFault);
InformTask (ThisFault);
}

/*****************************************************************/

void WriteReturned (struct TrapStruct *ThisFault)

{
short state;

/* PRINT_DEB ("Returned from writing", 0L); */
if (ThisFault->FaultAddress == 0)
  state = RemFrameFinish (ThisFault);
else if (ThisFault->PageDescrAddr == NULL)
  state = GetNewTable (ThisFault);
else
  state = GetNewPage (ThisFault);

if (state == READY)
  InformTask (ThisFault);
}

/*****************************************************************/

BOOL AddFrame (ULONG priority)

/* This function allocates a pageframe and the accompanying frame
 * descriptor. If it is successfully allocated, it is put into
 * the FrameList. Otherwise everything will be freed and FALSE
 * will be returned.
 */

{
char *frame;
struct FrameDescr *FD;

/* PRINT_DEB ("AllocFrame called", 0L); */

if ((frame = AllocAligned (PAGESIZE, CurrentConfig.MemFlags | MEMF_REVERSE,
                           PAGEALIGN, priority)) == NULL)
  {
  /* PRINT_DEB ("Couldn't allocate mem for page-frame", 0L); */
  return (FALSE);
  }

if ((FD = DoOrigAllocMem (sizeof (struct FrameDescr), MEMF_PUBLIC)) == NULL)
  {
  PRINT_DEB ("No mem for frame descriptor", 0L);
  FreeMem (frame, PAGESIZE);
  return (FALSE);
  }

FD->PageType = PT_EMPTY;
FD->Modified = FALSE;
FD->PhysAddr = (IPTR)frame;

AddHead (&FrameList, (struct Node*)FD);
FD->InFrameList = TRUE;
AddHead (&FrameAllocList, (struct Node*)&(FD->OrderNode));

NumPageFrames++;
return (TRUE);
}

/***********************************************************************/

void RemAllFrames (void)

{
struct FrameDescr *tmp_fd;

while ((tmp_fd = (struct FrameDescr*)RemHead (&FrameList)) != NULL)
  {
  Remove ((struct Node*)&(tmp_fd->OrderNode));

  /* The pages themselves are separately freed unless they are not marked
   * as empty.
   */
  if (tmp_fd->PageType == PT_EMPTY)
    FreeMem ((APTR)tmp_fd->PhysAddr, PAGESIZE);
  FreeMem (tmp_fd, sizeof (struct FrameDescr));
  }

NumPageFrames = 0;
}
