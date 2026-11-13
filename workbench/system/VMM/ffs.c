#include <exec/types.h>
#include "defs.h"
#include "ffs.h"

static char rcsid [] = "$Id: ffs.c,v 3.6 95/12/16 18:37:01 Martin_Apel Exp $";

#define MAX_RETRIES  20
#define WAITING_TIME 5L    /* in ticks, i.e. 1/50 s */
#define MIN(a,b)  (((a)<(b))?(a):(b))

#define BLOCKSIZE 512L       /* Only this blocksize supported */
#define BITS_PER_BMBLOCK     ((BLOCKSIZE / 4 - 1) * 32)
#define DO_READ   TRUE
#define DO_WRITE  FALSE

PRIVATE ULONG               *Bitmap;
PRIVATE struct FFSRootBlock *RootBlock;
PRIVATE ULONG               *MiscBlock;
PRIVATE ULONG                NumBlocks;
PRIVATE ULONG                DiskType;

/*************************************************************************/

PRIVATE BOOL MyInhibit (BOOL DoInhibit)

{
struct MsgPort *HandlerPort;

HandlerPort = (struct MsgPort*)(PagingDevParams.SysTask + 1);
return (BOOL)(DoPkt (HandlerPort, ACTION_INHIBIT, (LONG)DoInhibit, 0L, 0L, 0L, 0L));
}

/*************************************************************************/

/* The following routine relies on that the paging device has already 
 * been opened and the trap-structs initialized in pageio.c
 */

PRIVATE void RWBlock (ULONG block_num, void *buffer, BOOL DoRead)

{
((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Command = DoRead ? CMD_READ : CMD_WRITE;
((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Flags = 0;
((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Length = BLOCKSIZE;
((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Data = buffer;
((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Offset = 
   (PagingDevParams.low_cyl * PagingDevParams.heads *
    PagingDevParams.secs_per_track + block_num) * BLOCKSIZE;

SendIO ((struct IORequest*)TrapInfo[0].IOPacket);
WaitIO ((struct IORequest*)TrapInfo[0].IOPacket);

if (((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Error != 0)
  {
  PRINT_DEB ("Error while transferring block %ld", block_num);
  InitError (ERR_FAILED_IO);
  }
}

#define ReadBlock(num, buffer)  RWBlock (num, buffer, DO_READ)
#define WriteBlock(num, buffer) RWBlock (num, buffer, DO_WRITE)

/*****************************************************************/

PRIVATE void Cleanup (void)

{
if (Bitmap != NULL)
  FreeVec (Bitmap);
if (RootBlock != NULL)
  FreeMem (RootBlock, BLOCKSIZE);
if (MiscBlock != NULL)
  FreeMem (MiscBlock, BLOCKSIZE);

PRINT_DEB ("Uninhibiting partition...", 0L);

if (!MyInhibit (FALSE))
  PRINT_DEB ("Could not uninhibit partition", 0L);
else
  PRINT_DEB ("Successfully uninhibited partition", 0L);
}

/*****************************************************************/

PRIVATE int InitFFS (ULONG BMBlocks)

{
int retries = 0;
int rc = SUCCESS;

while (!MyInhibit (DOSTRUE) && (retries++ < MAX_RETRIES))
  Delay (WAITING_TIME);

if (retries >= MAX_RETRIES)
  return (ERR_INHIBIT_FAILED);

PRINT_DEB ("Inhibited partition", 0L);
PRINT_DEB (PartWithColon, 0L);

if ((RootBlock = AllocMem (sizeof (struct FFSRootBlock), MEMF_PUBLIC)) == NULL)
  rc = ERR_NOT_ENOUGH_MEM;

if ((MiscBlock = AllocMem (BLOCKSIZE, MEMF_PUBLIC)) == NULL)
  rc = ERR_NOT_ENOUGH_MEM;

if (BMBlocks != 0L && 
    ((Bitmap = AllocVec (BMBlocks * BLOCKSIZE, MEMF_PUBLIC)) == NULL))
  rc = ERR_NOT_ENOUGH_MEM;

if (rc != SUCCESS)
  Cleanup ();

return (rc);
}

/*****************************************************************/

#define MODE_SET 0
#define MODE_CLR 1
#define MODE_TST 2

BOOL BitManipulation (ULONG bit_num, UWORD mode)

{
ULONG *LW,
      mask;
BOOL was_set;

if (bit_num < PagingDevParams.res_start)
  return (FALSE);

bit_num -= PagingDevParams.res_start;  /* Reserved blocks are not */
                                       /* in bitmap */
/* The checksum words are still part of the bitmap */
LW = Bitmap + (bit_num / 32) + (bit_num / 32 / (BLOCKSIZE/4 - 1)) + 1;
mask = (1L << (bit_num % 32));

was_set = (*LW & mask) != 0;

switch (mode)
  {
  case MODE_SET: *LW |= mask;
                 break;
  case MODE_CLR: *LW &= ~mask;
                 break;
  }

return (was_set);
}

#define BitSet(i) BitManipulation (i, MODE_SET)
#define BitClr(i) BitManipulation (i, MODE_CLR)
#define BitTst(i) BitManipulation (i, MODE_TST)

#define AllocThisBlock(i) BitClr(i)
#define BlockUsed(i)      (!BitTst(i))

/*****************************************************************/

PRIVATE void Checksum (void *block)

{
/* This routine does the checksumming for either a fileheader,
 * a file extension block or the root block. All of them have the 
 * checksum at the same offset.
 */
LONG sum = 0;
ULONG i;
struct FFSFileHeader *fh = (struct FFSFileHeader*) block;

fh->Checksum = 0L;

for (i = 0; i < BLOCKSIZE / sizeof (ULONG); i++)
  sum += *((ULONG*)fh + i);

fh->Checksum = -sum;
}

/*****************************************************************/

BOOL IsPseudoPart (ULONG header_block, ULONG *first_block, ULONG *last_block)

{
struct FFSFileHeader *fh;
struct FFSExtBlock *eb;
int i;
int ErrorCode;

if ((ErrorCode = InitFFS (0L)) != SUCCESS)
  {
  InitError (ErrorCode);
  return (FALSE);
  }

fh = (struct FFSFileHeader*) MiscBlock;
eb = (struct FFSExtBlock*) MiscBlock;

ReadBlock (header_block, fh);

#ifdef DEBUG
if (header_block != fh->OwnKey)
  {
  PRINT_DEB ("header_block != fh->OwnKey", 0L);
  ColdReboot ();
  }

if (fh->SecondaryType > 0)
  {
  PRINT_DEB ("Directory", 0L);
  ColdReboot ();
  }
#endif

*first_block = fh->FirstDataBlock;
*last_block = *first_block - 1;

for (;;)
  {  
  for (i = 0; i < eb->BlocksInTable; i++)
    {
    if (eb->DataBlocks [71-i] != *last_block + 1)
      {
      Cleanup ();
      return (FALSE);
      }
    (*last_block)++;
    }

  if (eb->ExtBlock == 0)
    break;

  ReadBlock (eb->ExtBlock, eb);
  }

Cleanup ();
return (TRUE);
}

/*****************************************************************/

int GetDiskType (char *name, ULONG *DiskType)

/* Determines the disk type of a given disk. Used by FFS code and
 * by pageio.c to determine the number of buffers necessary for
 * file paging.
 */
{
int rc;
struct InfoData *InfoData;
BPTR tmp_lock;

PRINT_DEB ("Getting information about the following partition", 0L);
PRINT_DEB (name, 0L);

if ((InfoData = AllocMem (sizeof (struct InfoData), MEMF_PUBLIC)) != NULL)
  {
  if ((tmp_lock = Lock (name, ACCESS_READ)) != NULL)
    {
    if (Info (tmp_lock, InfoData))
      {
      if (InfoData->id_DiskState != ID_VALIDATED)
        rc = ERR_NOT_VALIDATED;
      else
        {
        *DiskType = InfoData->id_DiskType;
        rc = SUCCESS;
        PRINT_DEB ("NumBlocks (Info) = %ld", InfoData->id_NumBlocks);
        PRINT_DEB ("NumBlocksUsed (Info) = %ld", InfoData->id_NumBlocksUsed);
        }
      }
    else
      {
      PRINT_DEB ("GetDiskType: Couldn't get info for partition", 0L);
      rc = ERR_NO_DISKINFO;
      }
    UnLock (tmp_lock);
    }
  else
    {
    PRINT_DEB ("GetDiskType: Couldn't lock partition", 0L);
    rc = ERR_NO_PAGING_FILE;
    }
  FreeMem (InfoData, sizeof (struct InfoData));
  }
else
  rc = ERR_NOT_ENOUGH_MEM;

return (rc);
}

/*****************************************************************/

int IsValidFFSPartition (void)

/* name is the name of a file on the disk to be checked. */
{
int rc;

if (PagingDevParams.block_size != BLOCKSIZE || PagingDevParams.secs_per_block != 1)
  return (ERR_WRONG_BLOCKSIZE);

if ((rc = GetDiskType (PartWithColon, &DiskType)) == SUCCESS)
  {
  if (DiskType != ID_FFS_DISK && DiskType != ID_INTER_FFS_DISK && 
      DiskType != ID_FASTDIR_FFS_DISK)
    rc = ERR_NO_FFS;
  }

return (rc);
}

/*****************************************************************/

PRIVATE int RWBitmap (ULONG RootNum, BOOL DoRead)

{
struct FFSBitmapExtBlock *BMExtBlock;
int i, j;
ULONG *current_data;
ULONG ExtBlock;
int retries = 0;
LONG sum;

PRINT_DEB ("RWBitmap called", 0L);

current_data = Bitmap;

BMExtBlock = (struct FFSBitmapExtBlock*) RootBlock;

PRINT_DEB ("RWBitmap: Reading rootblock from %ld", RootNum);

/* Wait for the bitmap to become valid */
while (ReadBlock (RootNum, RootBlock),
       !RootBlock->BitmapValid && retries++ < MAX_RETRIES)
  Delay (WAITING_TIME);

if (!RootBlock->BitmapValid)
  {
  PRINT_DEB ("RWBitmap: Bitmap is invalid", 0L);
  return (ERR_NOT_VALIDATED);
  }

for (i = 0; i < 25; i++)
  {
  if (RootBlock->BitmapBlocks [i] == 0)
    return (SUCCESS);
  else
    {
    PRINT_DEB ("RWBitmap: Transferring bitmapblock %ld", 
               RootBlock->BitmapBlocks [i]);
    if (DoRead)
      ReadBlock (RootBlock->BitmapBlocks [i], current_data);
    else
      {
      sum = 0;
      for (j = 1; j < BLOCKSIZE / 4; j++)
        sum += *(current_data + j);
      *current_data = -sum;
      WriteBlock (RootBlock->BitmapBlocks [i], current_data);
      }
    current_data += BLOCKSIZE / sizeof (ULONG);
    }
  }

ExtBlock = RootBlock->BitmapExtBlock;

while (ExtBlock != 0)
  {
  PRINT_DEB ("RWBitmap: Reading BitmapExtBlock %ld", ExtBlock);
  ReadBlock (ExtBlock, BMExtBlock);  
  for (i = 0; i < BLOCKSIZE/4 - 1; i++)
    {
    if (BMExtBlock->BitmapBlocks [i] == 0)
      return (SUCCESS);
    else
      {
      PRINT_DEB ("ReadBitmap: Transferring bitmapblock %ld", 
                 BMExtBlock->BitmapBlocks [i]);
      if (DoRead)
        ReadBlock (BMExtBlock->BitmapBlocks [i], current_data);
      else
        {
        sum = 0;
        for (j = 1; j < BLOCKSIZE / 4; j++)
          sum += *(current_data + j);
        *current_data = -sum;
        WriteBlock (BMExtBlock->BitmapBlocks [i], current_data);
        }
      current_data += BLOCKSIZE / sizeof (ULONG);
      }
    }
  ExtBlock = BMExtBlock->NextBitmapExtBlock;
  }

return (SUCCESS);
}

/*****************************************************************/

PRIVATE ULONG AllocBlock (void)

{
ULONG i;
static ULONG last_found = 0;

for (i = last_found + 1;
     i < PagingDevParams.res_start + NumBlocks; i++)
  {
  if (!BlockUsed (i))
    {
    AllocThisBlock (i);
    last_found = i;
    return (i);
    }
  }
return (0L);
}

/*****************************************************************/

PRIVATE ULONG FindContiguousChunk (ULONG FileBlocks, ULONG *Largest)

/* Tries to find a contiguous chunk of 'FileBlocks' blocks. If it
 * doesn't find such, it returns the size of the largest available
 * chunk.
 */
{
ULONG last_used = PagingDevParams.res_start - 1;
ULONG i;
ULONG start;

PRINT_DEB ("FindContiguousChunk (%ld)", FileBlocks);

*Largest = 0;

for (i = PagingDevParams.res_start; 
     i < PagingDevParams.res_start + NumBlocks; i++)
  {
  if (BlockUsed (i))
    last_used = i;
  else
    {
    if (i >= last_used + FileBlocks)
      {
      start = last_used + 1;
      for (i = start; i < start + FileBlocks; i++)
        AllocThisBlock (i);
      return (start);
      }
    else if (i - last_used > *Largest)
      *Largest = i - last_used;
    }
  }
return (0L);
}

/*****************************************************************/

PRIVATE void UpdateDCFFSCache (ULONG file_block, ULONG dir_block,
                               ULONG filesize)

{
struct FFSDirBlock *dir;
struct DCFFSCacheBlock *cache;
struct ListEntry *le;
int i;
ULONG cur_block;
char *tmp;

ReadBlock (dir_block, MiscBlock);
dir = (struct FFSDirBlock*) MiscBlock;
cache = (struct DCFFSCacheBlock*) MiscBlock;
cur_block = dir->DirList;

do
  {
  ReadBlock (cur_block, MiscBlock);
  PRINT_DEB ("Reading cache block. NumEntries = %ld", cache->NumEntries);
  le = (struct ListEntry*) (cache + 1);
  if (cache->Type != T_DIRLIST)
    {
    PRINT_DEB ("UpdateDCFFSCache: Unknown cache block format", 0L);
    return;
    }

  for (i = 0; i < cache->NumEntries; i++)
    {
    if (le->Key == file_block)
      {
      PRINT_DEB ("UpdateDCFFSCache: Found file", 0L);
      PRINT_DEB ("UpdateDCFFSCache: Entering size %ld in cache block", filesize);
      le->Size = filesize;
      Checksum (MiscBlock);
      WriteBlock (cur_block, MiscBlock);
      return;
      }

    tmp = &(le->FileNameLength);
    PRINT_DEB ("UpdateDCFFSCache: Current file name is", 0L);
    PRINT_DEB (tmp + 1, 0L);
    tmp += *tmp + 1;            /* Skip name (BCPL string) */
    tmp += *tmp + 1;            /* Skip comment (BCPL string) */
    if ((IPTR)tmp & 1)
      tmp++;                    /* word align */
    le = (struct ListEntry *) tmp;
    }
  } while ((cur_block = cache->NextBlock) != 0);
}

/*****************************************************************/

int CreatePseudoPart (ULONG header_block, ULONG *first_block, 
                      ULONG *last_block)

{
ULONG Root;
ULONG Used = 0;
#ifdef DEBUG
ULONG last_used,
      last_unused;
#endif
int i;
ULONG BlocksForFile;
ULONG NumDataBlocks;
ULONG FileStart;
struct FFSFileHeader *fh;
struct FFSExtBlock *eb;
ULONG ExtBlockNum;
ULONG CurrentBlock;
ULONG ParentBlock;
ULONG BitmapBlocks;
ULONG LargestChunk;
ULONG FreeBlocks;
ULONG FileSize;
int rc;

PRINT_DEB ("CreatePseudoPart called", 0L);

NumBlocks = (PagingDevParams.high_cyl - PagingDevParams.low_cyl + 1) * 
             PagingDevParams.heads * PagingDevParams.secs_per_track;
PRINT_DEB ("NumBlocks = %ld", NumBlocks);

Root = (NumBlocks + 1) / 2;

NumBlocks -= PagingDevParams.res_start + PagingDevParams.res_end;
BitmapBlocks = (NumBlocks + (BITS_PER_BMBLOCK - 1)) / BITS_PER_BMBLOCK;

if ((rc = InitFFS (BitmapBlocks)) != SUCCESS)
  return (rc);

if ((rc = RWBitmap (Root, DO_READ)) != SUCCESS)
  {
  PRINT_DEB ("CreatePseudoPart: Couldn't read bitmap", 0L);
  Cleanup ();
  return (rc);
  }

#ifdef DEBUG
last_used = PagingDevParams.res_start - 1;
last_unused = 0;
#endif

for (i = PagingDevParams.res_start; 
     i < PagingDevParams.res_start + NumBlocks; i++)
  {
  if (BlockUsed ((ULONG)i))
    {
#ifdef DEBUG
    if (last_unused == i - 1)
      PRINT_DEB ("Used block starts at %ld", (ULONG)i);
    last_used = i;
#endif
    Used++;
    }
#ifdef DEBUG
  else
    {
    if (last_used == i - 1)
      PRINT_DEB ("Unused block starts at %ld", (ULONG)i);
    last_unused = i;
    }
#endif
  }

PRINT_DEB ("NumUsed = %ld", Used);

NumDataBlocks = (CurrentConfig.FileSize * 1024L * 1024L + 
                 BLOCKSIZE - 1) / BLOCKSIZE;
/* 72 pointers to blocks are in the fileheader. In each file extension 
 * block there are 72 pointers to additional blocks.
 */
 
BlocksForFile = NumDataBlocks + (NumDataBlocks + 71) / 72;
FreeBlocks = NumBlocks - Used;

if (BlocksForFile > FreeBlocks)
  {
  char error_msg [200];

  PRINT_DEB ("CreatePseudoPart: Not enough space on device left", 0L);
  sprintf (error_msg, (char *)_(msgPPTooLarge),
           (FreeBlocks - (FreeBlocks + 71) / 72) / 2 / 1024);
  ReportError (error_msg, ERR_CONTINUE);
  Cleanup ();
  return (ERR_MSG_POSTED);
  }

if ((FileStart = FindContiguousChunk (NumDataBlocks, &LargestChunk)) == 0)
  {
  char error_msg [200];

  PRINT_DEB ("CreatePseudoPart: Couldn't find large enough contiguous chunk", 0L);
  sprintf (error_msg, (char *)_(msgTooFragmented), LargestChunk / 2048);
  ReportError (error_msg, ERR_CONTINUE);
  Cleanup ();
  return (ERR_MSG_POSTED);
  }

PRINT_DEB ("CreatePseudoPart: Found contiguous chunk starting at %ld", FileStart);

*first_block = FileStart;
*last_block  = FileStart + NumDataBlocks - 1;

fh = (struct FFSFileHeader*) MiscBlock;
eb = (struct FFSExtBlock*)MiscBlock;

ReadBlock (header_block, fh);
fh->FirstDataBlock = FileStart;
fh->Size = FileSize = NumDataBlocks * BLOCKSIZE;
fh->Type = T_SHORT;
fh->SecondaryType = ST_FILE;
fh->OwnKey = header_block;
CurrentBlock = header_block;
ParentBlock = fh->ParentKey;

do
  {
  eb->BlocksInTable = MIN (72, NumDataBlocks);
  for (i = 0; i < eb->BlocksInTable; i++)
    eb->DataBlocks [71-i] = FileStart++;

  NumDataBlocks -= eb->BlocksInTable;
  if (NumDataBlocks > 0)
    {
    ExtBlockNum = AllocBlock ();
    eb->ExtBlock = ExtBlockNum;
    }
  else
    {
    eb->ExtBlock = 0L;
    for (i = eb->BlocksInTable; i < 72; i++)
      eb->DataBlocks [71-i] = 0L;
    }

  Checksum (eb);
  WriteBlock (CurrentBlock, eb);

  eb->Type = T_LIST;
  eb->SecondaryType = ST_FILE;
  CurrentBlock = ExtBlockNum;
  eb->OwnKey = CurrentBlock;
  eb->HeaderKey = header_block;
  }
while (eb->ExtBlock != 0L);

if ((rc = RWBitmap (Root, DO_WRITE)) != SUCCESS)
  {
  PRINT_DEB ("Couldn't write modified bitmap", 0L);
  ReadBlock (Root, RootBlock);
  RootBlock->BitmapValid = FALSE;       /* Let filesystem structure validate */
  Checksum (RootBlock);
  WriteBlock (Root, RootBlock);
  }

if (DiskType == ID_FASTDIR_FFS_DISK)
  UpdateDCFFSCache (header_block, ParentBlock, FileSize);

Cleanup ();
return (SUCCESS);
}
