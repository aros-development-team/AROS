#include <exec/types.h>

#ifdef __SASC
#include <dos.h>
#endif

#include "defs.h"

static char rcsid [] = "$Id: prepager.c,v 3.5 95/12/16 18:36:46 Martin_Apel Exp $";

/* The PrePager has two different tasks:
 * 
 * 1. Free  memory which should be freed by other tasks from inside a Forbid
 *    / Disable.  A descriptor of the memory to be freed is put into a queue
 *    and freed by the prepager the time Permit / Enable is called.
 * 
 * 2. Prevent  IO from or to the paging device using virtual memory which is
 *    possibly not locked into memory.  Otherwise a deadlock would result.
 */

PRIVATE struct IOStdReq       *CopiedReq;
PRIVATE struct DosPacket      *CopiedPacket;
PRIVATE struct MsgPort        *FileHandlerPort;
PRIVATE struct Message*      (*OrigPktWait) (void);
PRIVATE struct VMMsg          *PrePageMsg;
PRIVATE UWORD                  ResetSignal;
PRIVATE struct Message         QuitMsg;
PRIVATE void                  *ResetHandlerData;

#define MIN(a,b) ((a)<(b)?(a):(b))

/********************************************************************/

PRIVATE void FreeForbiddenMem (void)

{
struct ForbiddenFreeStruct *tmp_ffs;
void *free_start;
ULONG free_size;

while (VMToBeFreed != NULL)
  {
  Forbid ();
  tmp_ffs = VMToBeFreed;
  free_start = tmp_ffs->address;
  free_size = tmp_ffs->size;
  VMToBeFreed = tmp_ffs->NextFree;
  tmp_ffs->NextFree = VMFreeRecycling;
  VMFreeRecycling = tmp_ffs;
  Permit ();
  PRINT_DEB ("Freeing memory for other task", 0L);
  FreeMem (free_start, free_size);
  }
}

/********************************************************************/

PRIVATE void PrepageIOStdReq (struct IOStdReq *req)

{
ULONG Offset;
BYTE Error;
ULONG Actual;
IPTR From;
void *buffer;
ULONG size;

PRINT_DEB ("PrepageIOStdReq called", 0L);

From = (IPTR)req->io_Data;

PRINT_DEB ("Current number of pageframes: %ld", NumPageFrames);
PRINT_DEB ("Length of current request: %lx", req->io_Length);

Offset = req->io_Offset;
Actual = 0L;
Error = FALSE;

while (From < (IPTR)req->io_Data + req->io_Length)
  {
  /* Bug fix: If there are multiple units in the paging device, the
   * Unit pointer must not be taken from OpenDevice, but be copied from
   * the received request.
   */
  CopiedReq->io_Device  = req->io_Device;
  CopiedReq->io_Unit    = req->io_Unit;

  size = (ULONG)((IPTR)req->io_Data + req->io_Length - (IPTR)From);
  while ((buffer = AllocVec (size, MEMF_PUBLIC)) == NULL)
    {
    PRINT_DEB ("Request has to be split", 0L);
    size = ALIGN_DOWN (size / 2, PagingDevParams.block_size);
    if (size == 0)
      {
      PRINT_DEB ("Not enough memory to copy one block of data", 0L);
      req->io_Error = IOERR_BADADDRESS;      /* This is not what it should */
      req->io_Actual = 0;                    /* be, but there's no better */
                                             /* error code available */
      ReplyMsg ((struct Message*)req);
      return;
      }
    }
      
  CopiedReq->io_Command = req->io_Command;
  /* the quick bit has been cleared by the parthandler */
  CopiedReq->io_Flags   = req->io_Flags | IOF_QUICK;
  CopiedReq->io_Data    = buffer;
  CopiedReq->io_Offset  = Offset;
  CopiedReq->io_Length  = size;

  if (req->io_Command == CMD_WRITE)
    {
    if ((From & 0x3) == 0)         /* longword-aligned */
      CopyMemQuick ((APTR)From, buffer, size);
    else
      CopyMem ((APTR)From, buffer, size);
    }

  PRINT_DEB ("Sending request to device", 0L);
  SendIO ((struct IORequest*)CopiedReq);
  WaitIO ((struct IORequest*)CopiedReq);
  PRINT_DEB ("Request finished", 0L);

  if (req->io_Command == CMD_READ)
    {
    if ((From & 0x3) == 0)         /* longword-aligned */
      CopyMemQuick (buffer, (APTR)From, size);
    else
      CopyMem (buffer, (APTR)From, size);
    }

  FreeVec (buffer);
  Offset += size;
  From += size;
  Actual += CopiedReq->io_Actual;

  if (CopiedReq->io_Error)
    {
    Error = CopiedReq->io_Error;
    PRINT_DEB ("IOStdReq returned with error %ld", (ULONG)Error);
    PRINT_DEB ("Command was %ld", (ULONG)CopiedReq->io_Command);
    break;
    }
  }

PRINT_DEB ("Done IO for requesting task", 0L);

req->io_Error = Error;
req->io_Actual= Actual;

ReplyMsg ((struct Message*)req);
}

/********************************************************************/

PRIVATE struct Message *GetPacket (void)

{
struct Message *msg;

if (OrigPktWait != NULL)
  msg = (*OrigPktWait) ();
else
  {
  while ((msg = GetMsg (FileHandlerPort)) == NULL)
    Wait (SIGF_DOS);
  }

return (msg);
}

/********************************************************************/

PRIVATE void PrepagePacket (struct DosPacket *packet);

PRIVATE struct Message *PktWaitPatch (void)

{
struct Message *received_msg;
struct DosPacket *received_packet;

for (;;)
  {
  received_msg = GetPacket ();
  if (received_msg == &QuitMsg)
    {
    /* This is the quit msg. Deinstall myself */
    PRINT_DEB ("Deinstalling PktWaitPatch", 0L);
    FileHandlerProcess->pr_PktWait = (APTR)OrigPktWait;
    return (NULL);
    }

  received_packet = (struct DosPacket*)received_msg->mn_Node.ln_Name;

  switch (received_packet->dp_Action)
    {
    case ACTION_READ:
    case ACTION_WRITE:
      if ((received_packet->dp_Arg2 >= (IPTR)VirtAddrStart) &&
          (received_packet->dp_Arg2 <  (IPTR)VirtAddrEnd))
        {
        PRINT_DEB ("Received R/W packet to VM", 0L);
        PutMsg (PrePagerPort, received_msg);
        break;
        }
    default:
      return (received_msg);      
    }
  }
}

/********************************************************************/

PRIVATE void PrepagePacket (struct DosPacket *packet)

{
long Actual;
long result;
struct MsgPort *ReturnPort;
IPTR From,
      size;
void *buffer;

/* dp_Arg1 contains pointer to Object
 * dp_Arg2 is start of transfer
 * dp_Arg3 is length of transfer
 */
 
PRINT_DEB ("PrepagePacket called.", 0L);
From = packet->dp_Arg2;
Actual = 0;

while (From < packet->dp_Arg2 + packet->dp_Arg3)
  {
  size = (ULONG)packet->dp_Arg2 + packet->dp_Arg3 - From;
  PRINT_DEB ("Allocating %ld bytes for tmp buffer", size);
  while ((buffer = AllocVec (size, MEMF_PUBLIC)) == NULL)
    {
    PRINT_DEB ("Packet has to be split", 0L);
    size = size / 2;
    if (size == 0)
      {
      PRINT_DEB ("Not enough memory to copy data", 0L);
      packet->dp_Res1 = -1;        /* Error marker */
      packet->dp_Res2 = ERROR_NO_FREE_STORE;
      ReturnPort = packet->dp_Port;
      packet->dp_Port = FileHandlerPort;
      PutMsg (ReturnPort, packet->dp_Link);
      return;
      }
    }

  if (packet->dp_Action == ACTION_WRITE)
    CopyMem ((APTR)From, buffer, size);

  CopiedPacket->dp_Action = packet->dp_Action;
  CopiedPacket->dp_Arg1 = packet->dp_Arg1;
  CopiedPacket->dp_Arg2 = (IPTR)buffer;
  CopiedPacket->dp_Arg3 = size;

  SendPkt (CopiedPacket, FileHandlerPort, &(PrePagerProcess->pr_MsgPort));
  WaitPkt ();
  result = CopiedPacket->dp_Res1;

  if (packet->dp_Action == ACTION_READ)
    CopyMem (buffer, (APTR)From, result);    /* result contains the number */
                                             /* of bytes read */

  FreeVec (buffer);

  From += size;
  Actual += result;

  if (result <= 0)
    break;
  }

PRINT_DEB ("Done IO for requesting task", 0L);

packet->dp_Res1 = Actual;
packet->dp_Res2 = CopiedPacket->dp_Res2;
ReturnPort = packet->dp_Port;
packet->dp_Port = FileHandlerPort;
PutMsg (ReturnPort, packet->dp_Link);
}

/*********************************************************************/

PRIVATE void ResetHandler (void)

{
PRINT_DEB ("Sending reset signal to prepager", 0L);
Signal (PrePagerTask, 1L << ResetSignal);
}

/********************************************************************/

PRIVATE int InitPrePager (void)

{
/* 16 signals per task are guaranteed, so the following four lines
 * can't fail
 */

PrePagerQuitSignal = AllocSignal (-1L);
FreeVMSignal       = AllocSignal (-1L);
ResetSignal        = AllocSignal (-1L);

if ((PrePagerPort = CreateMsgPort ()) == NULL) 
  return (ERR_NOT_ENOUGH_MEM);

if ((CopiedReq = CreateIORequest (PrePagerPort, sizeof (struct IOStdReq))) 
                    == NULL)
  return (ERR_NOT_ENOUGH_MEM);

PRINT_DEB ("Patching BeginIO", 0L);

#if !defined(__AROS__)
OrigBeginIO = (void (*) ()) SetFunction ((struct Library*)PagingDevParams.device, 
                                 DEV_BEGINIO, (ULONG (*) ()) PartHandler);
#endif

PRINT_DEB ("BeginIO successfully patched", 0L);

VMToBeFreed = NULL;
VMFreeRecycling = NULL;

if ((PrePageMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

PRINT_DEB ("Allocated PrePageMsg", 0L);

if (CurrentConfig.PageDev == PD_FILE)
  {
  if ((CopiedPacket = AllocDosObject (DOS_STDPKT, NULL)) == NULL)
    {
    PRINT_DEB ("No mem for CopiedPacket", 0L);
    return (ERR_NOT_ENOUGH_MEM);
    }

  FileHandlerProcess = (struct Process*)PagingDevParams.SysTask;
  FileHandlerPort    = &(FileHandlerProcess->pr_MsgPort);

#if !defined(__AROS__)
  /* Install PktWaitPatch */
  OrigPktWait = (struct Message* (*) ())FileHandlerProcess->pr_PktWait;
  FileHandlerProcess->pr_PktWait = (APTR)PktWaitPatch;
#endif
  }

if ((ResetHandlerData = InstallResetHandler (ResetHandler, -32L)) == NULL)
  PRINT_DEB ("Couldn't install reset handler", 0L);

PRINT_DEB ("Installed reset handler", 0L);
return (SUCCESS);
}

/********************************************************************/

PRIVATE void Cleanup_PrePager (void)

{
struct ForbiddenFreeStruct *tmp_ffs;

bug("[VMM:PrePager] %s: removing reset handler...\n", __func__);

RemoveResetHandler (ResetHandlerData);

if ((CurrentConfig.PageDev == PD_FILE) && (OrigPktWait))
  {
  PRINT_DEB ("Requesting PktWaitPatch to quit", 0L);
  PutMsg (FileHandlerPort, &QuitMsg);
  while (FileHandlerProcess->pr_PktWait != (APTR)OrigPktWait)
    Delay (1L);

  FreeDosObject (DOS_STDPKT, CopiedPacket);
  }

bug("[VMM:PrePager] %s: freeing PrePageMsg\n", __func__);

if (PrePageMsg != NULL)
  FreeMem (PrePageMsg, sizeof (struct VMMsg));

while ((tmp_ffs = VMFreeRecycling) != NULL)
  {
  VMFreeRecycling = tmp_ffs->NextFree;
  FreeMem (tmp_ffs, sizeof (struct ForbiddenFreeStruct));
  }

#if !defined(__AROS__)
if (OrigBeginIO != NULL)
  SetFunction ((struct Library*)PagingDevParams.device, 
                                 DEV_BEGINIO, (ULONG (*) ()) OrigBeginIO);
#endif

bug("[VMM:PrePager] %s: deleting IO request\n", __func__);

if (CopiedReq != NULL)
  DeleteIORequest ((struct IORequest*)CopiedReq);

bug("[VMM:PrePager] %s: deleting MsgPort\n", __func__);

if (PrePagerPort != NULL)
  DeleteMsgPort (PrePagerPort);

bug("[VMM:PrePager] %s: freeing signals\n", __func__);

FreeSignal ((ULONG) PrePagerQuitSignal);
FreeSignal ((ULONG) FreeVMSignal);
FreeSignal ((ULONG) ResetSignal);

bug("[VMM:PrePager] %s: finished\n", __func__);

}

/********************************************************************/

void PrePager (void)

{
ULONG WaitMask;
ULONG ReceivedSignals;
BOOL quit = FALSE;
struct VMMsg *InitMsg;
struct Message *PrePageMsg;
int rc;

if ((rc = InitPrePager ()) != SUCCESS)
  {
bug("[VMM:PrePager] %s: failed to initialize\n", __func__);

  PRINT_DEB ("Init failed", 0L);
  InitError (rc);
  Cleanup_PrePager ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

PRINT_DEB ("Init successful", 0L);
/* Tell VM_Manager that the PrePager has been initialized correctly */
if ((InitMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  {
bug("[VMM:PrePager] %s: failed to alloc Init msg\n", __func__);

  PRINT_DEB ("PrePager: Not enough memory for init msg", 0L);
  InitError (ERR_NOT_ENOUGH_MEM);
  Cleanup_PrePager ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

InitMsg->VMSender = FindTask (NULL);
InitMsg->VMCommand = VMCMD_InitReady;
InitMsg->ReplySignal = 0;       /* Let VM_Manager free this */
PutMsg (InitPort, (struct Message*)InitMsg);

WaitMask = (1L << PrePagerPort->mp_SigBit) | 
           (1L << PrePagerQuitSignal)      |
           (1L << FreeVMSignal)            |
           (1L << ResetSignal);

while (!quit)
  {
  PRINT_DEB ("Waiting for signals", 0L);
  ReceivedSignals = Wait (WaitMask);
  PRINT_DEB ("Received signals: %lx", ReceivedSignals);

  if (ReceivedSignals & (1L << PrePagerPort->mp_SigBit))
    {
    while ((PrePageMsg = GetMsg (PrePagerPort)) != NULL)
      {
      if ((TypeOfMem (PrePageMsg->mn_Node.ln_Name) != 0) &&
          (((struct DosPacket*)(PrePageMsg->mn_Node.ln_Name))->dp_Link == 
                                                           PrePageMsg))
        PrepagePacket ((struct DosPacket*)PrePageMsg->mn_Node.ln_Name);
      else
        PrepageIOStdReq ((struct IOStdReq*)PrePageMsg);
      }
    }

  if (ReceivedSignals & (1L << FreeVMSignal))
    FreeForbiddenMem ();

  if (ReceivedSignals & (1L << PrePagerQuitSignal))
    quit = TRUE;

  if (ReceivedSignals & (1L << ResetSignal))
    {
    PRINT_DEB ("Received reset signal", 0L);
    ResetHandlerDone (ResetHandlerData);
    WaitMask = (1L << FreeVMSignal);
    }
  }

Cleanup_PrePager ();
PRINT_DEB ("Exiting", 0L);
}
