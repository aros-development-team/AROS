#include <exec/types.h>
#include "defs.h"
#include "Manager_Priv.h"

static char rcsid [] = "$Id: VM_Manager.c,v 3.6 95/12/16 18:36:53 Martin_Apel Exp $";

/*********************************************************************/

PRIVATE BOOL LockPage (ULONG Page)

{
struct VMMsg *LockMsg;

if ((LockMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  return (FALSE);

LockMsg->VMCommand = VMCMD_LockPage;
LockMsg->PageAddress  = Page;
LockMsg->ReplySignal = 0;
PutMsg (PageHandlerPort, (struct Message*)LockMsg);

Wait (1L << LockAckSignal);
return (ThisPageLocked);
}
  
/*********************************************************************/

PRIVATE BOOL LockPages (ULONG From, ULONG To)

{
ULONG Page;

for (Page = PAGEADDR (From); Page <= PAGEADDR (To-1); Page += PAGESIZE)
  {
  if (!LockPage (Page))
    return (FALSE);
  }

return (TRUE);
}

/*********************************************************************/

PRIVATE BOOL LoadRemainingPages (void)

{
/* Walk the memory list to find out how many page frames are still needed.
 */

struct MemChunk *mc;
IPTR start_of_alloc,
      end_of_alloc,
      end_of_last_alloc;

start_of_alloc = (IPTR)(VirtMem + 1);  /* points behind MemHeader */
end_of_alloc = (IPTR)VirtMem;

OBTAIN_VM_SEMA;
for (mc = VirtMem->mh_First; mc != NULL; mc = mc->mc_Next)
  {
  end_of_last_alloc = end_of_alloc;
  end_of_alloc = (IPTR)mc;
  if (start_of_alloc < end_of_alloc)
    {
    if (!LockPages (start_of_alloc, end_of_alloc))
      {
      RELEASE_VM_SEMA;
      return (FALSE);
      }
    }
    
  start_of_alloc = (IPTR)mc + mc->mc_Bytes;
  }

if (start_of_alloc < (IPTR)VirtMem->mh_Upper)
  {
  if (!LockPages (start_of_alloc, (IPTR)VirtMem->mh_Upper))
    {
    RELEASE_VM_SEMA;
    return (FALSE);
    }
  }

RELEASE_VM_SEMA;
return (TRUE);
}

/*********************************************************************/

PRIVATE BOOL ExitPossible (void)

{
/* This routine checks if it is currently possible to exit VMM.
 * 1. It tries to remove library if it is installed.
 * 2. It checks the remaining allocated virtual memory. If the still 
 *    allocated memory fits into the page frames, quit is allowed.
 *    CAUTION: This may cause the VM_Manager to block for a page-fault.
 */

struct Library *VMLib;
struct MemHeader *mh;
ULONG size = 0;

Forbid ();
VMLib = (struct Library*)FindName (&(SysBase->LibList), LIBNAME);
if (VMLib != NULL)
  {
  /* Try to remove library. */
  RemLibrary (VMLib);
  if (FindName (&(SysBase->LibList), LIBNAME) != NULL)
    {
    /* Library couldn't be expunged */
    Permit ();
    PRINT_DEB ("Library is still running", 0L);
    ReportError (_(msgVMMLibStillOpen), ERR_CONTINUE);
    return (FALSE);
    }
  }
Permit ();

/* It's a bit troublesome to find out, if there are enough page frames
 * to hold the rest of the allocated memory. To avoid this, simple
 * tests are done first.
 */

if (VirtMem->mh_Free == AddedMemSize - sizeof (struct MemHeader))
  return (TRUE);

size = 0;
Forbid ();
for (mh = (struct MemHeader*)SysBase->MemList.lh_Head;
     mh->mh_Node.ln_Succ != NULL; mh = (struct MemHeader*)mh->mh_Node.ln_Succ)
  {
  if ((mh->mh_Attributes & CurrentConfig.MemFlags) == CurrentConfig.MemFlags)
    size += (IPTR)mh->mh_Upper - (IPTR)mh->mh_Lower;
  }
Permit ();

PRINT_DEB ("Maximum size of available memory: %ld", size);
PRINT_DEB ("%ld bytes of VM are allocated", AddedMemSize - VirtMem->mh_Free);
if (AddedMemSize - VirtMem->mh_Free > size)
  {
  ReportError (_(msgNotEnoughFrames), ERR_CONTINUE);
  return (FALSE);
  }

if (!LoadRemainingPages ())
  {
  struct VMMsg UnlockMsg;

  PRINT_DEB ("Not enough frames to exit", 0L);
  UnlockMsg.VMCommand = VMCMD_UnlockAllPages;
  UnlockMsg.VMSender = (struct Task*)VM_ManagerProcess;
  UnlockMsg.ReplySignal = LockAckSignal;
  PutMsg (PageHandlerPort, (struct Message*) &UnlockMsg);
  Wait (1L << LockAckSignal);

  ReportError (_(msgNotEnoughFrames), ERR_CONTINUE);
  return (FALSE);
  }

EarlyExit = TRUE;
ReportError (_(msgStillVMAllocated), ERR_NOERROR);
return (TRUE);   
}

/*********************************************************************/

BOOL TryToQuit (void)

{
PRINT_DEB ("Trying to quit", 0L);
Delay (50L);            /* Leave a little time for the program requesting
                         * to quit to free its memory
                         */
DISABLE_VM;
if (ExitPossible ())
    {
    PRINT_DEB ("Quit possible", 0L);
    NoMoreVM ();

    if (VMPort->mp_Node.ln_Name != NULL)
      {
      RemPort (VMPort);
      VMPort->mp_Node.ln_Name = NULL;
      }

    if (RexxPort->mp_Node.ln_Name != NULL)
      {
      RemPort (RexxPort);
      RexxPort->mp_Node.ln_Name = NULL;
      }

    return (TRUE);
    }

ENABLE_VM;
return (FALSE);
}

/*********************************************************************/

void FillStat (struct VMMsg *StatMsg)

{
struct TrapStruct *tmp;
int i;

StatMsg->st_VMSize = AddedMemSize - sizeof (struct MemHeader);
StatMsg->st_VMFree = VirtMem->mh_Free;
StatMsg->st_Faults = NumPageFaults;
StatMsg->st_PagesWritten = PagesWritten;
StatMsg->st_PagesRead = PagesRead;
StatMsg->st_Frames = NumPageFrames;
StatMsg->st_PagesUsed = SlotsUsed ();
StatMsg->st_PageSize = PAGESIZE;

i = 0;
Forbid ();
for (tmp = (struct TrapStruct*)Free.lh_Head;
     tmp->TS_Node.mln_Succ != NULL;
     tmp = (struct TrapStruct*)tmp->TS_Node.mln_Succ)
  i++;
Permit ();
StatMsg->st_TrapStructsFree = i;
}

/*********************************************************************/

PRIVATE void FillMemHeader (struct VMMsg *ReqMsg)

{
ReqMsg->VMCommand = VMCMD_InitReady;
ReqMsg->VMHeader  = VirtMem;
ReqMsg->VMSema    = &VirtMemSema;
ReqMsg->MLName    = MEMLISTNAME;
}

/*********************************************************************/

int ShowGUI (void)

{
BPTR InOutHandle;
char command_buffer [200];

if (ExtCxPort->PrefsTask != NULL)
  {
  PRINT_DEB ("Prefs already running", 0L);
  Signal (ExtCxPort->PrefsTask, 1L << ExtCxPort->ShowSignal);
  return (SUCCESS);
  }

DISABLE_VM;

if ((InOutHandle = Open ("CON:0/0/640/200/VMM Window/CLOSE/AUTO/WAIT", 
                         MODE_NEWFILE)) == BNULL)
  {
  PRINT_DEB ("Couldn't open shell window", 0L);
  ENABLE_VM;
  return (ERR_NOT_ENOUGH_MEM);
  }

PRINT_DEB ("Trying to create process", 0L);

sprintf (command_buffer, "\"%s\"", CxParams->PrefsPath);
if (SystemTags (command_buffer,
                SYS_Input, (IPTR)InOutHandle,
                SYS_Output, NULL,
                SYS_Asynch, (ULONG) TRUE,
                NP_Priority, 0L,
                TAG_DONE, 0L) != 0)
  {
  PRINT_DEB ("Couldn't create preferences process", 0L);
  ENABLE_VM;
  return (ERR_NO_PREFS);
  }

PRINT_DEB ("Successfully created preferences process", 0L);
ENABLE_VM;
return (SUCCESS);
}

/*********************************************************************/

void HideGUI (void)

{
Forbid ();
if (ExtCxPort->PrefsTask != NULL)
  Signal (ExtCxPort->PrefsTask, SIGBREAKF_CTRL_C);
Permit ();
}

/*********************************************************************/

PRIVATE BOOL HandleVMMsg (void)

{
struct VMMsg *VMMsg;
BOOL Quit = FALSE;
BOOL NoReply;

UWORD Command;

while ((VMMsg = (struct VMMsg*)GetMsg (VMPort)))
  {
  PRINT_DEB ("Received VMMsg", 0L);
  Command = VMMsg->VMCommand;
  NoReply = FALSE;
  switch (Command)
    {
    /**********************/
    case VMCMD_AskAllocMem:
      ExtCheckVirtMem (VMMsg->VMSender);
      break;

    /**********************/
    case VMCMD_QuitAll:
      if (TryToQuit ())
        Quit = TRUE;
      break;

    /**********************/
    case VMCMD_AskStat:
      PRINT_DEB ("Received AskStat msg", 0L);
      FillStat (VMMsg);
      break;

    /**********************/
    case VMCMD_ReqMemHeader:
      PRINT_DEB ("Received ReqMemHeader msg", 0L);
      FillMemHeader (VMMsg);
      break;

    /**********************/
    case VMCMD_DisableVM:
      PRINT_DEB ("Received disable msg from Prefs program", 0L);
      DISABLE_VM;
      break;

    /**********************/
    case VMCMD_EnableVM:
      PRINT_DEB ("Received enable msg from Prefs program", 0L);
      ENABLE_VM;
      break;

    /**********************/
    case VMCMD_AskVMUsage:
      PRINT_DEB ("Received AskVMUsage message", 0L);
      /* A separate task will reply this message when finished */
      VMUsageInfo (VMMsg);
      NoReply = TRUE;
      break;

    /**********************/
    case VMCMD_AskConfig:
      PRINT_DEB ("Received AskConfig message", 0L);
      *(VMMsg->Config) = CurrentConfig;
      break;

    /**********************/
    default:
      PRINT_DEB ("Internal error: Unknown msg type. Type is %ld", 
                 (ULONG)Command);
      PRINT_DEB ("Sender is", 0L);
      PRINT_DEB (VMMsg->VMSender->tc_Node.ln_Name, 0L);
#ifdef DEBUG
      ColdReboot ();
#endif
    }

  if (!NoReply)
    {
    if (VMMsg->ReplySignal == 0)
      {
      PRINT_DEB ("Freeing VMMsg", 0L);
      FreeMem (VMMsg, sizeof (struct VMMsg));
      }
    else
      {
      Signal (VMMsg->VMSender, 1L << VMMsg->ReplySignal);
      PRINT_DEB ("Returning VMMsg to sender", 0L);
      }
    }
  }
return (Quit);
}

/*********************************************************************/

PRIVATE BOOL HandleCxMsg (void)

{
CxMsg *MyMsg;
BOOL Quit = FALSE;
int rc;

while ((MyMsg = (CxMsg*)GetMsg (&(ExtCxPort->CxPort))) != NULL)
  {
  switch (CxMsgType (MyMsg))
    {
    case CXM_IEVENT: 
      switch (CxMsgID (MyMsg))
        {
        case APPEAR_ID: if ((rc = ShowGUI ()) != SUCCESS)
                          RunTimeError (rc);
                        break;

        case ENABLE_ID: if (!VMEnabled)
                          {
                          VMEnabled = TRUE;
                          ENABLE_VM;
                          DisplayBeep (NULL);
                          }
                        break;

        case DISABLE_ID:if (VMEnabled)
                          {
                          VMEnabled = FALSE;
                          DISABLE_VM;
                          DisplayBeep (NULL);
                          }
                        break;
        }
      break;

    case CXM_COMMAND:
      switch (CxMsgID (MyMsg))
        {
        case CXCMD_DISABLE: 
               PRINT_DEB ("Received CXCMD_DISABLE", 0L);
               if (VMEnabled)
                 {
                 VMEnabled = FALSE;
                 DISABLE_VM;
                 }
               PRINT_DEB ("Deactivating broker", 0L);
               ActivateCxObj (CxParams->Broker, FALSE);
               PRINT_DEB ("Returned from deactivate", 0L);
               break;

        case CXCMD_ENABLE: 
               PRINT_DEB ("Received CXCMD_ENABLE", 0L);
               ActivateCxObj (CxParams->Broker, TRUE);
               if (!VMEnabled)
                 {
                 VMEnabled = TRUE;
                 ENABLE_VM;
                 }
               break;

        case CXCMD_APPEAR: 
               PRINT_DEB ("Received CXCMD_APPEAR", 0L);
               if ((rc = ShowGUI ()) != SUCCESS)
                 RunTimeError (rc);
               break;

        case CXCMD_DISAPPEAR: 
               PRINT_DEB ("Received CXCMD_DISAPPEAR", 0L);
               HideGUI ();
               break;

        case CXCMD_KILL: 
               PRINT_DEB ("Received CXCMD_KILL", 0L);
               if (TryToQuit ())
                 {
                 HideGUI ();
                 Quit = TRUE;
                 }
               break;
        }
      break;
    }

  ReplyMsg ((struct Message*) MyMsg);
  }

return (Quit);
}

/*********************************************************************/

PRIVATE void NewConfigReceived (void)

{
struct VMMsg *CfgMsg;
int rc;
ULONG OldWBufSize = CurrentConfig.WriteBuffer;

NoMoreVM ();
if ((rc = ReadConfigFile (CFG_FILENAME)) != SUCCESS)
  {
  RunTimeError (rc);
  return;
  }

/* try to accommodate to the new values. The values which have to 
 * be taken into account are:
 *   MinMem
 *   MaxMem
 *   PrintStat
 *   VMPriority
 *   WriteBuffer
 *   Enable and DisableVM hotkeys
 */

/* New VMPriority value */
Forbid ();
if (VirtMem->mh_Node.ln_Name != NULL)      
  {
  Remove ((struct Node*)VirtMem);
  VirtMem->mh_Node.ln_Pri = CurrentConfig.VMPriority;
  Enqueue (&(SysBase->MemList), (struct Node*)VirtMem);
  }
Permit ();

/* New PrintStat value */
if (CurrentConfig.StatEnabled && (FindTask (STAT_NAME) == NULL))
  {
  if ((rc = LaunchStat ()) != SUCCESS)
    RunTimeError (rc);
  }
else if (!CurrentConfig.StatEnabled && (FindTask (STAT_NAME) != NULL))
  Signal (StatTask, 1L << StatQuitSignal);

/* New MinMem, MaxMem values */
if ((CfgMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  {
  PRINT_DEB ("No mem for Config message to pagehandler", 0L);
  return;
  }

CfgMsg->VMSender = FindTask (NULL);
CfgMsg->ReplySignal = 0;
CfgMsg->VMCommand = VMCMD_NewConfig;
PutMsg (PageHandlerPort, (struct Message*)CfgMsg);

#if defined(PLATFORM_HASZ2)
/* New DontCacheZ2RAM value */
AllowZorroIICaching (CurrentConfig.CacheZ2RAM);
#endif
#if defined(PLATFORM_HASFASTROM)
if ((rc = SwitchFastROM (CurrentConfig.FastROM)) != SUCCESS)
  {
  RunTimeError (rc);
  CurrentConfig.FastROM = FALSE;
  }
#else
  CurrentConfig.FastROM = FALSE;
#endif

/* New PatchWB value */
if (CurrentConfig.PatchWB && (OrigSetWindowTitles == NULL))
  {
  PRINT_DEB ("Patching SetWindowTitles ()", 0L);
  OrigSetWindowTitles = (void (*) ()) SetFunction ((struct Library*)IntuitionBase,
                                     -0x114, (ULONG (*) ()) SetWindowTitlesPatch);
  }
else if (!CurrentConfig.PatchWB && (OrigSetWindowTitles != NULL))
  {
  SetFunction ((struct Library*)IntuitionBase, -0x114, 
               (ULONG (*) ()) OrigSetWindowTitles);
  OrigSetWindowTitles = NULL;
  }

/* New value for write-buffer */
if (OldWBufSize != CurrentConfig.WriteBuffer)
  {
  PRINT_DEB ("WriteBuffer size has changed", 0L);
  DesiredWriteBufferSize = CurrentConfig.WriteBuffer;
#if !defined(__AROS__)
  CreateTask (WBUF_ALLOC_NAME, 5L, AllocNewCache, 1000L);
#else
    NewCreateTask(TASKTAG_PC,
                                        AllocNewCache,
                                        TASKTAG_NAME,   WBUF_ALLOC_NAME,
                                        TASKTAG_PRI,    5,
                                        TAG_DONE);
#endif
  }

SetFilter (CxParams->EnableFilter, CurrentConfig.EnableHotkey);
SetFilter (CxParams->DisableFilter, CurrentConfig.DisableHotkey);
}

/*********************************************************************/

void VM_Manager (void)

{
ULONG ReceivedSignals;
ULONG WaitMask;
BOOL DoQuit = FALSE;
struct MsgPort *StarterPort;
struct VMMsg *InitMsg;
int rc;

	bug("[VMM-Handler] %s()\n", __func__);

rc = Init_VM_Manager ();

	bug("[VMM-Handler] %s: Init returned %0x\n", __func__, rc);

if (((StarterPort = FindPort (STARTER_PORT_STD)) == NULL) &&
    ((StarterPort = FindPort (STARTER_PORT_LIB)) == NULL))
  {
  PRINT_DEB ("VERY, VERY STRANGE: COULDN'T FIND STARTER PORT", 0L);
  return;
  }
else if ((InitMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  {
  PRINT_DEB ("Not enough memory for init msg", 0L);
  Signal (StarterPort->mp_SigTask, 1L << StarterPort->mp_SigBit);
  InitError (ERR_NOT_ENOUGH_MEM);
  Cleanup_VM_Manager ();
  return;
  }

InitMsg->VMSender  = (struct Task*)VM_ManagerProcess;
InitMsg->ReplySignal = 0;

if (rc != SUCCESS)
  {
	bug("[VMM-Handler] %s: Sending VMCMD_InitFailed\n", __func__);

  InitError (rc);
  InitMsg->VMCommand = VMCMD_InitFailed;
  PutMsg (StarterPort, (struct Message*)InitMsg);
  Cleanup_VM_Manager ();
  return;
  }

	bug("[VMM-Handler] %s: Sending VMCMD_InitReady\n", __func__);

InitMsg->VMCommand = VMCMD_InitReady;
InitMsg->VMHeader  = VirtMem;
InitMsg->VMSema    = &VirtMemSema;
PutMsg (StarterPort, (struct Message*)InitMsg);

WaitMask = (1L << VMPort->mp_SigBit) | (1L << CfgChangeSignal) |
           (1L << RexxPort->mp_SigBit);

if (ExtCxPort != NULL)
  {
  PRINT_DEB ("Including CXSigBit in WaitMask", 0L);
  WaitMask |= (1L << ExtCxPort->CxPort.mp_SigBit);
  }

while (!DoQuit)
  {
  PRINT_DEB ("Waiting for signals", 0L);
  ReceivedSignals = Wait (WaitMask);
  PRINT_DEB ("Received signals", 0L);

  if (ReceivedSignals & (1L << CfgChangeSignal))
    {
    PRINT_DEB ("Configuration changed", 0L);
    NewConfigReceived ();
    }

  if ((ExtCxPort != NULL) && 
      (ReceivedSignals & (1L << ExtCxPort->CxPort.mp_SigBit)))
    {
    PRINT_DEB ("Received commodities message", 0L);
    DoQuit = HandleCxMsg () || DoQuit;
    }

  if (ReceivedSignals & (1L << RexxPort->mp_SigBit))
    DoQuit = HandleRexxMsg () || DoQuit;

  if (ReceivedSignals & (1L << VMPort->mp_SigBit))
    DoQuit = HandleVMMsg () || DoQuit;
  }

Cleanup_VM_Manager ();
}
