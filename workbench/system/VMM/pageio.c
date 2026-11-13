#include <exec/types.h>
#include "defs.h"

/* Handles all communication with DOS or the paging device */

static char rcsid [] = "$Id: pageio.c,v 3.6 95/12/16 18:36:42 Martin_Apel Exp $";

/* Params if PageDev == PD_FILE or PD_PSEUDO_PART */
PRIVATE BPTR PageFile;
PRIVATE LONG PageFileArg1;              /* Often used by packets */
PRIVATE struct MsgPort *ReplyPort;
PRIVATE struct MsgPort *PageFilePort;
PRIVATE struct FileInfoBlock *PagingInfo;
PRIVATE LONG BuffersAdded;

/* Data used for multi-page writing */
PRIVATE struct IOStdReq *MultiPageReq;
PRIVATE struct DosPacket *MultiPagePacket;
PRIVATE struct MsgPort *MultiPagePort;

#define CHUNKSIZE (512L * 1024L)        /* write 512 K in one go */

#if !defined(MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/*****************************************************************/

PRIVATE void AddBuffersForFile (void)

{
int rc;
LONG CurrentBuffers;
LONG DesiredBuffers;
ULONG DiskType;

if (GetDiskType (PartWithColon, &DiskType) != SUCCESS)
  return;

if (DiskType != ID_FFS_DISK && DiskType != ID_INTER_FFS_DISK)
  return;                     /* no additional buffers for other filesystems */

rc = AddBuffers (PartWithColon, 0L);

if (rc == FALSE)
  {
  PRINT_DEB ("AddBuffersForFile failed", 0L);
  return;
  }

if (rc == DOSTRUE)                 /* see doc for AddBuffers */
  CurrentBuffers = IoErr ();
else
  CurrentBuffers = rc;

DesiredBuffers = PartSize / 40 / 1024;    /* empirically determined */

PRINT_DEB ("CurrentBuffers = %ld", CurrentBuffers);
PRINT_DEB ("DesiredBuffers = %ld", DesiredBuffers);

if (DesiredBuffers > CurrentBuffers)
  {
  rc = AddBuffers (PartWithColon, DesiredBuffers - CurrentBuffers);
  if (rc == FALSE)
    return;

  if (rc == DOSTRUE)
    BuffersAdded = IoErr () - CurrentBuffers;
  else 
    BuffersAdded = rc - CurrentBuffers;
  
  PRINT_DEB ("Added %ld buffers", BuffersAdded);
  }
}

/*****************************************************************/

PRIVATE int OpenPageFileAsFile (void)

{
char *tmp;
int i;
ULONG size;

/* First try to open existing file, if none exists, open a new one.
 * C evaluation rules used.
 */
if (((PageFile = Open (CurrentConfig.PartOrFileName, MODE_OLDFILE)) == NULL) &&
    ((PageFile = Open (CurrentConfig.PartOrFileName, MODE_NEWFILE)) == NULL))
  {
  PRINT_DEB ("OpenPageFileAsFile: Couldn't open paging file", 0L);
  return (ERR_NO_PAGING_FILE);
  }

PRINT_DEB ("OpenPageFileAsFile: File opened successfully", 0L);

PageFilePort = DeviceProc (CurrentConfig.PartOrFileName);

PageFileArg1 = ((struct FileHandle*)BADDR (PageFile))->fh_Arg1;
PartSize = SetFileSize (PageFile, CurrentConfig.FileSize * 1024L * 1024L, 
                        OFFSET_BEGINNING);

PRINT_DEB ("OpenPageFileAsFile: After SetFileSize", 0L);

/* Not all handlers support SetFileSize */
if (PartSize != CurrentConfig.FileSize * 1024L * 1024L)
  {
  PRINT_DEB ("Couldn't make page file large enough", 0L);
  PRINT_DEB ("Trying to write desired file size", 0L);

  if (IoErr () == ERROR_DISK_FULL)
    return (ERR_NO_SPACE);

  size = Seek (PageFile, 0L, OFFSET_END);
  PRINT_DEB ("Current file size is %ld bytes", size);
  tmp = ((struct MemHeader*)(SysBase->MemList.lh_Head))->mh_Lower;

  /* Do it the slow, but secure way */
  while (size < CurrentConfig.FileSize * 1024L * 1024L)
    {
    if (Write (PageFile, tmp, CHUNKSIZE) != CHUNKSIZE)
      {
      PRINT_DEB ("Not enough space on volume", 0L);
      return (ERR_NO_SPACE);
      }
    size += CHUNKSIZE;
    }

  PRINT_DEB ("Managed to create page file", 0L);
  PartSize = CurrentConfig.FileSize * 1024L * 1024L;
  }

if ((ReplyPort = CreateMsgPort ()) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

ReplySignal = ReplyPort->mp_SigBit;

if ((MultiPagePort = CreateMsgPort ()) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

PRINT_DEB ("OpenPageFileAsFile: Created Ports", 0L);

for (i = 0; i < MAX_FAULTS; i++)
  {
  /* dp_Arg7 is used as a pointer from the packet to the corresponding
   * TrapStruct.
   */
  if ((TrapInfo[i].IOPacket = AllocDosObject (DOS_STDPKT, NULL)) == NULL)
    return (ERR_NOT_ENOUGH_MEM);

  ((struct DosPacket*)TrapInfo[i].IOPacket)->dp_Arg7 = (IPTR)&(TrapInfo [i]);

  /* Another packet is needed for the seek operation, which has to precede
   * each read or write operation.
   */
  if ((TrapInfo[i].SeekPacket = AllocDosObject (DOS_STDPKT, NULL)) == NULL)
    return (ERR_NOT_ENOUGH_MEM);

  ((struct DosPacket*)TrapInfo[i].SeekPacket)->dp_Arg7 = (IPTR)&(TrapInfo [i]);
  }

if ((MultiPagePacket = AllocDosObject (DOS_STDPKT, NULL)) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

PRINT_DEB ("OpenPageFileAsFile: Initialized Packets", 0L);

AddBuffersForFile ();

PRINT_DEB ("OpenPageFileAsFile: Added buffers", 0L);

return (SUCCESS);
}

/*****************************************************************/

PRIVATE int OpenPageFileAsPartition (BOOL WritePermit)

{
int i;
char *buffer,
     *tmp;
struct EasyStruct MyRequest =
  {
  sizeof (struct EasyStruct),
  0,
  PROGNAME,
  NULL,
  NULL
  };

/* Reserve first 512 bytes for ID */
PartStart = PagingDevParams.low_cyl * PagingDevParams.heads *
            PagingDevParams.secs_per_track * 512L + 512;
PartSize = (PagingDevParams.high_cyl - PagingDevParams.low_cyl + 1) *
            PagingDevParams.heads * PagingDevParams.secs_per_track * 512L - 512;

if (((ReplyPort = CreateMsgPort ()) == NULL) || 
    ((MultiPagePort = CreateMsgPort ()) == NULL))
  return (ERR_NOT_ENOUGH_MEM);

for (i = 0; i < MAX_FAULTS; i++)
  {
  if ((TrapInfo[i].IOPacket = CreateIORequest (ReplyPort,
               sizeof (struct ExtIOReq))) == NULL)
    return (ERR_NOT_ENOUGH_MEM);

  /* Install back pointer from IORequest to TrapInfo */
  ((struct ExtIOReq*)TrapInfo[i].IOPacket)->TrapInfo = &(TrapInfo[i]);
  }

if ((MultiPageReq = CreateIORequest (MultiPagePort, sizeof (struct IOStdReq))) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

if (OpenDevice (PagingDevParams.device->dd_Library.lib_Node.ln_Name,
                PagingDevParams.unit,
                (struct IORequest*)TrapInfo[0].IOPacket,
                PagingDevParams.flags))
  {
  char ErrorBuffer [80];
  sprintf (ErrorBuffer, (char *)_(msgNoDevice),
           PagingDevParams.device->dd_Library.lib_Node.ln_Name);
  ReportError (ErrorBuffer, ERR_CONTINUE);
  return (ERR_MSG_POSTED);
  }

for (i = 1; i < MAX_FAULTS; i++)
  {
  ((struct ExtIOReq*)TrapInfo [i].IOPacket)->ioreq.io_Device =
                ((struct ExtIOReq*)TrapInfo [0].IOPacket)->ioreq.io_Device;
  ((struct ExtIOReq*)TrapInfo [i].IOPacket)->ioreq.io_Unit =
                    ((struct ExtIOReq*)TrapInfo [0].IOPacket)->ioreq.io_Unit;
  }

MultiPageReq->io_Device = ((struct ExtIOReq*)TrapInfo [0].IOPacket)->ioreq.io_Device;
MultiPageReq->io_Unit   = ((struct ExtIOReq*)TrapInfo [0].IOPacket)->ioreq.io_Unit;

if (WritePermit)
  {
  /* Allocate enough memory for a LINUX page. LINUX marks its swap-space
   * by a signature in the last bytes of the first page.
   */
  if ((buffer = DoOrigAllocMem (4096L, MEMF_PUBLIC)) == NULL)
    return (ERR_NOT_ENOUGH_MEM);

  ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Command = CMD_READ;
  ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Flags = 0;
  ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Length = 4096;
  ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Data = buffer;
  ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Offset = PartStart - 512;
  SendIO ((struct IORequest*)TrapInfo[0].IOPacket);
  WaitIO ((struct IORequest*)TrapInfo[0].IOPacket);
  if (((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Error != 0)
    {
    FreeMem (buffer, 4096L);
    return (ERR_FAILED_IO);
    }

  if (strcmp (PROGNAME, buffer) != 0)
    {
    tmp = PartWithColon;
    MyRequest.es_TextFormat = _(msgUnusedPartition);
    MyRequest.es_GadgetFormat = _(msgUseCancel);
    if (strncmp (buffer + 4096 - 10, "SWAP-SPACE", 10) != 0 &&
        !CxParams->ForceOverwrite && 
        !EasyRequestArgs (NULL, &MyRequest, NULL, (RAWARG)&tmp))
      {
      FreeMem (buffer, 4096L);
      return (ERR_MSG_POSTED);
      }
    strcpy (buffer, PROGNAME);
    ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Command = CMD_WRITE;
    ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Flags = 0;
    ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Length = 512;
    ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Data = buffer;
    ((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Offset = PartStart - 512;
    SendIO ((struct IORequest*)TrapInfo[0].IOPacket);
    WaitIO ((struct IORequest*)TrapInfo[0].IOPacket);
    if (((struct IOStdReq*)TrapInfo[0].IOPacket)->io_Error != 0)
      {
      FreeMem (buffer, 4096L);
      return (ERR_FAILED_IO);
      }
    }

  FreeMem (buffer, 4096L);
  }
ReplySignal = ReplyPort->mp_SigBit;
return (SUCCESS);
}

/*****************************************************************/

PRIVATE int OpenPageFileAsPseudoPart (void)

{
ULONG first_block,
      last_block;
char *tmp;
int rc;

struct EasyStruct DeleteRequest =
  {
  sizeof (struct EasyStruct),
  0,
  PROGNAME,
  NULL,
  NULL
  };

if ((rc = IsValidFFSPartition ()) != SUCCESS)
  return (rc);

if ((PagingInfo = AllocDosObject (DOS_FIB, NULL)) == NULL)
  {
  PRINT_DEB ("Not enough memory for FileInfoBlock", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

/* First try to open existing file, if none exists, open a new one.
 */
if ((PageFile = Lock (CurrentConfig.PartOrFileName, EXCLUSIVE_LOCK)) != NULL)
  {
  /* File exists. Check if its a valid pseudo-part */
  if (!Examine (PageFile, PagingInfo))
    {
    PRINT_DEB ("Couldn't find information about paging file", 0L);
    return (ERR_NOT_ENOUGH_MEM);   /* Error to complicated to tell the user */
    }

  if (PagingInfo->fib_DirEntryType > 0)
    {
    PRINT_DEB ("OpenPageFileAsPseudoPart: PageFile is a directory", 0L);
    return (ERR_FILE_IS_DIR);
    }

  if (PagingInfo->fib_Size == CurrentConfig.FileSize * 1024L * 1024L)
    {
    if (IsPseudoPart (PagingInfo->fib_DiskKey, &first_block, &last_block))
      {
      PRINT_DEB ("First block = %ld", first_block);
      PRINT_DEB ("Last block  = %ld", last_block);

      PartStart = (PagingDevParams.low_cyl * PagingDevParams.heads *
                   PagingDevParams.secs_per_track + first_block) * 512L;
      PartSize = (last_block - first_block + 1) * 512L;
      PRINT_DEB ("Found existing pseudopart. Start at %ld", PartStart);
      PRINT_DEB ("                           Size  is %ld", PartSize);
      return (SUCCESS);
      }
    }

  PRINT_DEB ("Found mismatching file. Deleting it", 0L);
  UnLock (PageFile);
  PageFile = NULL;       /* In case of an error not to be unlocked twice */

  tmp = CurrentConfig.PartOrFileName;
  DeleteRequest.es_TextFormat = _(msgDeleteFile);
  DeleteRequest.es_GadgetFormat = _(msgDeleteCancel);
  if (!CxParams->ForceOverwrite && !EasyRequestArgs (NULL, &DeleteRequest, NULL, (RAWARG)&tmp))
    return (ERR_MSG_POSTED);

  /* No need to actually delete the file. It will be deleted as part
   * of the opening with MODE_NEWFILE.
   */
  }


/* Pagefile did not exist. Create a new one */
if ((PageFile = Open (CurrentConfig.PartOrFileName, MODE_NEWFILE)) == NULL)
  {
  PRINT_DEB ("OpenPageFileAsPseudoPart: Couldn't open paging file", 0L);
  return (ERR_NO_PAGING_FILE);
  }

Close (PageFile);
if ((PageFile = Lock (CurrentConfig.PartOrFileName, EXCLUSIVE_LOCK)) == NULL)
  {
  PRINT_DEB ("OpenPageFileAsPseudoPart: Internal error", 0L);
  return (ERR_INTERNAL);
  }

if (!Examine (PageFile, PagingInfo))
  {
  PRINT_DEB ("OpenPageFileAsPseudoPart: Can't examine new file", 0L);
  return (ERR_NOT_ENOUGH_MEM);     /* Error too complicated to tell the user */
  }

rc = CreatePseudoPart (PagingInfo->fib_DiskKey, &first_block, &last_block);
if (rc != SUCCESS)
  {
  PRINT_DEB ("OpenPageFileAsPseudoPart: Couldn't create pseudo partition", 0L);
  return (rc);
  }

PRINT_DEB ("First block = %ld", first_block);
PRINT_DEB ("Last block  = %ld", last_block);

PartStart = (PagingDevParams.low_cyl * PagingDevParams.heads *
             PagingDevParams.secs_per_track + first_block) * 512L;
PartSize = (last_block - first_block + 1) * 512L;
PRINT_DEB ("Created new pseudopart. Start at %ld", PartStart);
PRINT_DEB ("                        Size  is %ld", PartSize);
return (SUCCESS);
}

/*****************************************************************/

int OpenPageFile (void)

{
int rc;

switch (CurrentConfig.PageDev)
  {
  case PD_FILE: PRINT_DEB ("Opening pagefile as file", 0L);
                return (OpenPageFileAsFile ());

  case PD_PART: PRINT_DEB ("Opening pagefile as partition", 0L);
                return (OpenPageFileAsPartition (TRUE));

  case PD_PSEUDOPART: PRINT_DEB ("Opening pagefile as pseudopart", 0L);
                      if ((rc = OpenPageFileAsPartition (FALSE)) != SUCCESS)
                        return (rc );
                      return (OpenPageFileAsPseudoPart ());
#ifdef DEBUG
  default:      PRINT_DEB ("OpenPageFile: Unknown paging device", 0L);
                ColdReboot ();
                return (SUCCESS);       /* dummy */
#endif
  }
}

/*****************************************************************/

void ClosePageFile ()

{
int i;

switch (CurrentConfig.PageDev)
  {
  case PD_FILE: AddBuffers (PartWithColon, -BuffersAdded);
                for (i = 0; i < MAX_FAULTS; i++)
                  {
                  if (TrapInfo[i].IOPacket != NULL)
                    FreeDosObject (DOS_STDPKT, TrapInfo[i].IOPacket);
                  if (TrapInfo[i].SeekPacket != NULL)
                    FreeDosObject (DOS_STDPKT, TrapInfo[i].SeekPacket);
                  }

                if (MultiPagePacket != NULL)
                  FreeDosObject (DOS_STDPKT, MultiPagePacket);
              
                if (ReplyPort != NULL)
                  DeleteMsgPort (ReplyPort);

                if (MultiPagePort != NULL)
                  DeleteMsgPort (MultiPagePort);

                if (PageFile != NULL)
                  Close (PageFile);
                break;

  case PD_PSEUDOPART:
                if (PageFile != NULL)
                  UnLock (PageFile);

                if (PagingInfo != NULL)
                  FreeDosObject (DOS_FIB, PagingInfo);
                  
                /* fall through to device closing */

  case PD_PART: CloseDevice ((struct IORequest*)TrapInfo [0].IOPacket);
                for (i = 0; i < MAX_FAULTS; i++)
                  DeleteIORequest ((struct IORequest*)TrapInfo [i].IOPacket);

                DeleteIORequest ((struct IORequest*)MultiPageReq);

                DeleteMsgPort (ReplyPort);
                DeleteMsgPort (MultiPagePort);
                break;
#ifdef DEBUG
  default:      PRINT_DEB ("ClosePageFile: Unknown paging device", 0L);
                ColdReboot ();
#endif
  }
}

/*****************************************************************/

void WriteSinglePage (ULONG slot_num, struct TrapStruct *ThisTrap)

{
struct DosPacket *SeekPacket,
                 *WritePacket;

/* PRINT_DEB ("Writing page %ld", slot_num); */

PagesWritten++;

switch (CurrentConfig.PageDev)
  {
  case PD_FILE:
    WritePacket = (struct DosPacket*)ThisTrap->IOPacket;
    SeekPacket  = (struct DosPacket*)ThisTrap->SeekPacket;

    SeekPacket->dp_Port = ReplyPort;
    SeekPacket->dp_Action = ACTION_SEEK;
    SeekPacket->dp_Arg1 = PageFileArg1;
    SeekPacket->dp_Arg2 = slot_num << PAGESIZESHIFT;
    SeekPacket->dp_Arg3 = OFFSET_BEGINNING;
    SendPkt (SeekPacket, PageFilePort, ReplyPort);

    WritePacket->dp_Port = ReplyPort;
    WritePacket->dp_Action = ACTION_WRITE;
    WritePacket->dp_Arg1 = PageFileArg1;
    WritePacket->dp_Arg2 = ThisTrap->PhysAddr;
    WritePacket->dp_Arg3 = PAGESIZE;
    SendPkt (WritePacket, PageFilePort, ReplyPort);
    break;

  case PD_PART:
  case PD_PSEUDOPART:
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Command = CMD_WRITE;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Flags = 0;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Length = PAGESIZE;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Data = (APTR)ThisTrap->PhysAddr;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset = slot_num * PAGESIZE + PartStart;

    if (((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset < PartStart ||
        ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset > PartStart + PartSize)
      FatalError (ERR_OUT_OF_BOUNDS);

    SendIO ((struct IORequest*)ThisTrap->IOPacket);
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("WritePage: Unknown paging device", 0L);
    ColdReboot ();
#endif
  }
}

/*****************************************************************/

void ReadSinglePage (ULONG slot_num, struct TrapStruct *ThisTrap)

{
struct DosPacket *SeekPacket,
                 *ReadPacket;

/* PRINT_DEB ("Reading page %ld", slot_num); */
PagesRead++;

switch (CurrentConfig.PageDev)
  {
  case PD_FILE:
    ReadPacket = (struct DosPacket*)ThisTrap->IOPacket;
    SeekPacket = (struct DosPacket*)ThisTrap->SeekPacket;

    SeekPacket->dp_Port = ReplyPort;
    SeekPacket->dp_Action = ACTION_SEEK;
    SeekPacket->dp_Arg1 = PageFileArg1;
    SeekPacket->dp_Arg2 = slot_num << PAGESIZESHIFT;
    SeekPacket->dp_Arg3 = OFFSET_BEGINNING;
    SendPkt (SeekPacket, PageFilePort, ReplyPort);

    ReadPacket->dp_Port = ReplyPort;
    ReadPacket->dp_Action = ACTION_READ;
    ReadPacket->dp_Arg1 = PageFileArg1;
    ReadPacket->dp_Arg2 = ThisTrap->PhysAddr;
    ReadPacket->dp_Arg3 = PAGESIZE;
    SendPkt (ReadPacket, PageFilePort, ReplyPort);
    break;

  case PD_PART:
  case PD_PSEUDOPART:
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Command = CMD_READ;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Flags = 0;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Length = PAGESIZE;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Data = (APTR)ThisTrap->PhysAddr;
    ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset = slot_num * PAGESIZE + PartStart;

    if (((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset < PartStart ||
        ((struct ExtIOReq*)ThisTrap->IOPacket)->ioreq.io_Offset > PartStart + PartSize)
      FatalError (ERR_OUT_OF_BOUNDS);

    SendIO ((struct IORequest*)ThisTrap->IOPacket);
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("ReadPage: Unknown paging device", 0L);
    ColdReboot ();
#endif
  }
}

/*****************************************************************/

void WriteMultiplePages (ULONG slot_num, void *buffer, ULONG num_pages)

{
/* Writes multiple pages to disk SYNCHRONOUSLY */
BOOL ErrorOccurred;
ULONG BytesToWrite;
ULONG Offset;
IPTR Location;

/* PRINT_DEB ("WriteMultiplePages called. %ld pages", num_pages); */
PagesWritten += num_pages;

switch (CurrentConfig.PageDev)
  {
  case PD_FILE:
    MultiPagePacket->dp_Port = MultiPagePort;
    MultiPagePacket->dp_Action = ACTION_SEEK;
    MultiPagePacket->dp_Arg1 = PageFileArg1;
    MultiPagePacket->dp_Arg2 = slot_num << PAGESIZESHIFT;
    MultiPagePacket->dp_Arg3 = OFFSET_BEGINNING;
    SendPkt (MultiPagePacket, PageFilePort, MultiPagePort);
    WaitPort (MultiPagePort);
    GetMsg (MultiPagePort);

    MultiPagePacket->dp_Port = MultiPagePort;
    MultiPagePacket->dp_Action = ACTION_WRITE;
    MultiPagePacket->dp_Arg1 = PageFileArg1;
    MultiPagePacket->dp_Arg2 = (long)buffer;
    MultiPagePacket->dp_Arg3 = num_pages * PAGESIZE;
    SendPkt (MultiPagePacket, PageFilePort, MultiPagePort);
    WaitPort (MultiPagePort);
    GetMsg (MultiPagePort);
    ErrorOccurred = (MultiPagePacket->dp_Res1 != num_pages * PAGESIZE);
    break;

  case PD_PART:
  case PD_PSEUDOPART:
    BytesToWrite = num_pages * PAGESIZE;
    Location = (IPTR)buffer;
    Offset = slot_num * PAGESIZE + PartStart;

    while (BytesToWrite > 0)
      {
      ULONG BytesThisTime = MIN (BytesToWrite, 
                    ALIGN_DOWN (PagingDevParams.MaxTransfer + 1, PAGESIZE));

      PRINT_DEB ("Transferring %ld bytes", BytesThisTime);

      MultiPageReq->io_Command = CMD_WRITE;
      MultiPageReq->io_Flags = 0;
      MultiPageReq->io_Length = BytesThisTime;
      MultiPageReq->io_Data = (APTR)Location;
      MultiPageReq->io_Offset = Offset;

      if (MultiPageReq->io_Offset < PartStart ||
          MultiPageReq->io_Offset > PartStart + PartSize)
        FatalError (ERR_OUT_OF_BOUNDS);

      SendIO ((struct IORequest*)MultiPageReq);
      WaitIO ((struct IORequest*)MultiPageReq);
      ErrorOccurred = (MultiPageReq->io_Error != 0) || 
                      (MultiPageReq->io_Actual != BytesThisTime);
      BytesToWrite -= BytesThisTime;
      Location += BytesThisTime;
      Offset += BytesThisTime;
      }

    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("WriteMultiplePages: Unknown paging device", 0L);
    ColdReboot ();
#endif
  }

if (ErrorOccurred)
  {
  PRINT_DEB ("IO to paging device failed", 0L);
#ifdef DEBUG
  ColdReboot ();
#endif
  FatalError (ERR_FAILED_IO);
  }
}

/*****************************************************************/

void HandleReturn (void)

{
struct Message *RetMsg;
struct DosPacket *RetPacket;
struct TrapStruct *ThisFault;
struct ExtIOReq *IO;
BOOL WriteReady;
ULONG ErrorOccurred;

while ((RetMsg = GetMsg (ReplyPort)) != NULL)
  {
  switch (CurrentConfig.PageDev)
    {
    case PD_FILE:
      RetPacket = (struct DosPacket*)RetMsg->mn_Node.ln_Name;
      if (RetPacket->dp_Action == ACTION_SEEK)
        {
        /* Ignore the return of seek packets */
        continue;
        }

      ThisFault = (struct TrapStruct*)RetPacket->dp_Arg7;
      WriteReady = (RetPacket->dp_Action == ACTION_WRITE);
      ErrorOccurred = (RetPacket->dp_Res1 != PAGESIZE);
      break;

    case PD_PART:
    case PD_PSEUDOPART:
      IO = (struct ExtIOReq*)RetMsg;
      ThisFault = IO->TrapInfo;
      WriteReady = (IO->ioreq.io_Command == CMD_WRITE);
      ErrorOccurred = ((IO->ioreq.io_Error != 0) ||
                       (IO->ioreq.io_Actual != PAGESIZE));
      break;

#ifdef DEBUG
    default:
      PRINT_DEB ("HandleReturn: Unknown paging device", 0L);
      ColdReboot ();
#endif
    }

  if (ErrorOccurred)
    {
    PRINT_DEB ("IO to paging device failed", 0L);
#ifdef DEBUG
    ColdReboot ();
#endif
    FatalError (ERR_FAILED_IO);
    }

  if (WriteReady)
    WriteReturned (ThisFault);
  else
    ReadReturned (ThisFault);
  }
}
