#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: cache.c,v 3.5 95/12/16 18:37:02 Martin_Apel Exp $";

PRIVATE ULONG LastAccessed = 0;
PRIVATE char *WriteCache;
PRIVATE BOOL *EntryValid;
PRIVATE ULONG FirstSlot,
              NumSlotsInCache;
PRIVATE ULONG NumSlotsUsedInCache;
PRIVATE ULONG NumSlotsUsed;         /* Number of overall used slots */
PRIVATE ULONG BufSize;

PRIVATE void FlushCache (void);

/* This module assumes that there is no allocation of a page on disk
 * between the allocation of another one and its writing to disk.
 * If this is ever changed 'fault.c' will have to be changed accordingly.
 */

/************************************************************************/

int InitCache (void)

{
int rc;
int i;

if ((rc = InitMap (PartSize / PAGESIZE)) != SUCCESS)
  {
  PRINT_DEB ("Couldn't init bitmap", 0L);
  return (rc);
  }

NumSlotsInCache = CurrentConfig.WriteBuffer / PAGESIZE;
BufSize = NumSlotsInCache * (PAGESIZE + sizeof (BOOL));

if (BufSize != NULL)
  {
  if ((WriteCache = AllocMem (BufSize, MEMF_PUBLIC)) == NULL)
    {
    PRINT_DEB ("Not enough memory for write cache", 0L);
    return (ERR_NOT_ENOUGH_MEM);
    }

  EntryValid = (BOOL*) (WriteCache + NumSlotsInCache * PAGESIZE);
  for (i = 0; i < NumSlotsInCache; i++)
    *(EntryValid + i) = FALSE;

  FirstSlot = AllocMultipleSlots (&NumSlotsInCache);
  NumSlotsUsedInCache = 0;
  }

NumSlotsUsed = 0;
return (SUCCESS);
}

/************************************************************************/

void KillCache (void)

{
if (BufSize != NULL)
  FreeMem (WriteCache, BufSize);

KillMap ();
}

/************************************************************************/

BOOL ReadPage (ULONG slot, struct TrapStruct *ThisFault)

{
if (BufSize != NULL)
  {
  /* Check if this page is in the cache */
  if (slot >= FirstSlot && slot < FirstSlot + NumSlotsInCache &&
      EntryValid [slot - FirstSlot])
    {
    /* PRINT_DEB ("Reading page from write cache", 0L); */
    CopyMemQuick (WriteCache + (slot - FirstSlot) * PAGESIZE,
                  (APTR)ThisFault->PhysAddr, PAGESIZE);
    return (READY);
    }
  }

ReadSinglePage (slot, ThisFault);
LastAccessed = slot;
return (IN_PROGRESS);
}

/************************************************************************/

BOOL WritePage (ULONG slot, struct TrapStruct *ThisFault)

{
if (BufSize != NULL)
  {
  if (slot >= FirstSlot && slot < FirstSlot + NumSlotsInCache)
    {
    /* PRINT_DEB ("Putting page in write cache", 0L); */
    CopyMemQuick ((APTR)ThisFault->PhysAddr, 
                  WriteCache + (slot - FirstSlot) * PAGESIZE, PAGESIZE);
    if (!EntryValid [slot - FirstSlot])
      NumSlotsUsedInCache++;
    EntryValid [slot - FirstSlot] = TRUE;
    return (READY);
    }

  PRINT_DEB ("WritePage: Page outside cache bounds (slot %ld)?!?", slot);
#ifdef DEBUG
  ColdReboot ();
#endif
  }

if (!ResetInProgress)
  WriteSinglePage (slot, ThisFault);
LastAccessed = slot;
return (IN_PROGRESS);
}

/************************************************************************/

ULONG AllocPageOnDisk (void)

{
NumSlotsUsed++;

if (BufSize != NULL)
  {
  ULONG i;

  for (i = 0; i < NumSlotsInCache; i++)
    {
    if (!EntryValid [i])
      return (i + FirstSlot);
    }

  /* If we have arrived here the cache is currently full, so it has
   * to be flushed.
   */
  FlushCache ();
  for (i = 0; i < NumSlotsInCache; i++)
    {
    if (!EntryValid [i])
      return (i + FirstSlot);
    }

  PRINT_DEB ("AllocPageOnDisk: Didn't find place in write buffer", 0L);
#ifdef DEBUG
  ColdReboot ();
#endif  
  }

return (AllocSlotNextTo (LastAccessed));
}

/************************************************************************/

void FreePageOnDisk (ULONG slot)

{
NumSlotsUsed--;

if (BufSize != NULL)
  {
  if (slot >= FirstSlot && slot < FirstSlot + NumSlotsInCache)
    {
    EntryValid [slot - FirstSlot] = FALSE;
    NumSlotsUsedInCache--;
    return;
    }
  }

FreeSlot (slot);
}

/************************************************************************/

PRIVATE void FlushCache (void)

{
LONG FirstPage,
     LastPage,
     i;

PRINT_DEB ("Flushing cache", 0L);

/* Search for the first and the last valid page and write all pages
 * in between.
 */

for (FirstPage = 0; FirstPage < NumSlotsInCache; FirstPage++)
  {
  if (EntryValid [FirstPage])
    break;
  }

for (LastPage = NumSlotsInCache - 1; LastPage >= 0; LastPage--)
  {
  if (EntryValid [LastPage])
    break;
  }

if (FirstPage > LastPage)     /* Cache empty */
  return;

if (!ResetInProgress)
  WriteMultiplePages (FirstSlot + FirstPage, WriteCache + FirstPage * PAGESIZE,
                      LastPage - FirstPage + 1);

/* Free unused slots */
for (i = 0; i < NumSlotsInCache; i++)
  {
  if (!EntryValid [i])
    FreeSlot (i + FirstSlot);
  else
    EntryValid [i] = FALSE;
  }

NumSlotsInCache = BufSize / (PAGESIZE + sizeof (BOOL));
FirstSlot = AllocMultipleSlots (&NumSlotsInCache);

NumSlotsUsedInCache = 0;
}

/************************************************************************/

ULONG SlotsUsed (void)

{
return (NumSlotsUsed);
}

/************************************************************************/

void AllocNewCache (void)

{
/* This function is executed by a separate task which is created only for 
 * this function. Because it can lead to difficulties, if the VM_Manager
 * or the pagehandler have to allocate memory during runtime this approach
 * has been chosen.
 * This function reads the desired buffer size from a global variable named
 * 'DesiredWriteBufferSize', tries to allocate enough memory, sends a message to
 * the pagehandler and exits.
 * The pagehandler then installs this buffer and frees the old one.
 */

ULONG MyBufSize;
struct VMMsg *NewWriteBufMsg;
char *NewWriteBuf;

if ((NewWriteBufMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  /* Cannot do anything about it. Just leave it the way it was */
  return;

MyBufSize = (DesiredWriteBufferSize / PAGESIZE) * (PAGESIZE + sizeof (BOOL));

if (MyBufSize != NULL)
  {
  if ((NewWriteBuf = AllocMem (MyBufSize, MEMF_PUBLIC)) == NULL)
    {
    FreeMem (NewWriteBufMsg, sizeof (struct VMMsg));
    return;
    }
  }
else
  NewWriteBuf = NULL;

NewWriteBufMsg->VMCommand = VMCMD_NewWriteBuffer;
NewWriteBufMsg->ReplySignal = 0;
NewWriteBufMsg->NewWriteBuffer = NewWriteBuf ;
NewWriteBufMsg->NewWriteBufferSize = MyBufSize;
PutMsg (PageHandlerPort, (struct Message*)NewWriteBufMsg);
}

/************************************************************************/

void NewCache (char *buffer, ULONG size)

{
int i;

if (BufSize != NULL)
  {
  FlushCache ();

  /* Free unused slots */
  for (i = 0; i < NumSlotsInCache; i++)
    FreeSlot (i + FirstSlot);

  FreeMem (WriteCache, BufSize);
  }

BufSize = size;
WriteCache = buffer;
if (BufSize != 0L)
  {
  NumSlotsInCache = BufSize / (PAGESIZE + sizeof (BOOL));
  EntryValid = (BOOL*) (WriteCache + NumSlotsInCache * PAGESIZE);
  for (i = 0; i < NumSlotsInCache; i++)
    *(EntryValid + i) = FALSE;
  
  FirstSlot = AllocMultipleSlots (&NumSlotsInCache);
  NumSlotsUsedInCache = 0;
  }
}
