
#define DEBUG 1
#include <exec/types.h>
#include "defs.h"
#define MANAGER_PRIV
#include "Manager_Priv.h"

static char rcsid [] = "$Id: VM_Manager_Init.c,v 1.8 95/12/16 18:37:04 Martin_Apel Exp $";

/*********************************************************************/

PRIVATE int OpenLibs (void)

{
#if defined(__AROS__)
  /* SysBase is set in patches_aros.c */
	bug("[VMM-Handler] %s()\n", __func__);
#else
SysBase = *((struct ExecBase**)4L);
#endif
if (SysBase->LibNode.lib_Version < 37)
  return (ERR_LOW_VERSION);

if ((DOSBase = (struct DosLibrary*)OpenLibrary ("dos.library", 37L)) == NULL)
  return (ERR_NO_DOS);

if ((IntuitionBase = OpenLibrary ("intuition.library", 37L)) == NULL)
  return (ERR_NO_INTUITION);

if ((GfxBase = (struct GfxBase*)OpenLibrary ("graphics.library", 37L)) == NULL)
  return (ERR_NO_GFX);

if ((UtilityBase = OpenLibrary ("utility.library", 37L)) == NULL)
  return (ERR_NO_UTILITY);

if ((ExpansionBase = OpenLibrary ("expansion.library", 37L)) == NULL)
  return (ERR_NO_EXPANSION);

if ((CxBase = OpenLibrary ("commodities.library", 37L)) == NULL)
  return (ERR_NO_COMMODITIES);

if ((RexxSysBase = OpenLibrary ("rexxsyslib.library", 36L)) == NULL)
  return (ERR_NO_REXX);

LocaleBase = OpenLibrary ("locale.library", 38L);

	bug("[VMM-Handler] %s: Libraries Opened\n", __func__);

return (SUCCESS);
}

/*********************************************************************/

PRIVATE BOOL ReceiveInitMsg (void)

{
struct VMMsg *InitMsg = NULL;
BOOL Success;

Wait (1L << InitPort->mp_SigBit);

if ((InitMsg = (struct VMMsg*)GetMsg (InitPort)) == NULL)
  {
  PRINT_DEB ("VM_Manager: Init failure", 0L);
  return (FALSE);
  }

if (InitMsg->VMCommand == VMCMD_InitReady)
  Success = TRUE;
else
  Success = FALSE;

if (InitMsg->ReplySignal == 0)
  {
  PRINT_DEB ("Freeing InitMsg", 0L);
  FreeMem (InitMsg, sizeof (struct VMMsg));
  }
else
  {
  PRINT_DEB ("Returning init msg to sender", 0L);
  Signal (InitMsg->VMSender, 1L << InitMsg->ReplySignal);
  }
return (Success);
}

/*********************************************************************/

PRIVATE int LaunchPageHandler (void)

{
struct TagItem PH_Tags [] =
{ { NP_Entry, (IPTR)PageHandler },
  { NP_Name, (IPTR)PAGEHANDLER_NAME },
  { NP_Priority, 19L },
  { NP_StackSize, 12000L },
  { TAG_DONE, 0L } };

switch (CurrentConfig.PageDev)
  {
  case PD_FILE:
  case PD_PSEUDOPART:
    if ((PageHandlerProcess = CreateNewProc (PH_Tags)) == NULL)
      return (ERR_NOT_ENOUGH_MEM);
    PageHandlerTask = (struct Task*)PageHandlerProcess;
    break;

  case PD_PART:
#if !defined(_AROS__)
  PageHandlerTask = CreateTask (PAGEHANDLER_NAME, 19L, PageHandler, 4000L);
#else
  PageHandlerTask = NewCreateTask(TASKTAG_PC,
                                        PageHandler,
                                        TASKTAG_NAME,   PAGEHANDLER_NAME,
                                        TASKTAG_PRI,    19,
                                        TAG_DONE);
#endif
    if (!PageHandlerTask)
      return (ERR_NOT_ENOUGH_MEM);
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("LaunchPageHandler: Unknown paging device", 0L);
    ColdReboot ();
#endif
  }

if (!ReceiveInitMsg ())
  return (ERR_MSG_POSTED);
return (SUCCESS);
}

/*********************************************************************/

PRIVATE int LaunchPrePager (void)

{
struct TagItem PP_Tags [] =
{ { NP_Entry, (IPTR)PrePager },
  { NP_Name, (IPTR)PREPAGER_NAME },
  { NP_Priority, 20L },
  { NP_StackSize, 4000L },
  { TAG_DONE, 0L } };

switch (CurrentConfig.PageDev)
  {
  case PD_FILE:
    if ((PrePagerProcess = CreateNewProc (PP_Tags)) == NULL)
      return (ERR_NOT_ENOUGH_MEM);
    PrePagerTask = (struct Task*)PrePagerProcess;
    break;

  case PD_PART:
  case PD_PSEUDOPART:
#if !defined(_AROS__)
    PrePagerTask = CreateTask (PREPAGER_NAME, 20L, PrePager, 4000L);
#else
    PrePagerTask = NewCreateTask(TASKTAG_PC,
                                        PrePager,
                                        TASKTAG_NAME,   PREPAGER_NAME,
                                        TASKTAG_PRI,    20,
                                        TAG_DONE);
#endif
    if (!PrePagerTask)
      return (ERR_NOT_ENOUGH_MEM);
    break;

#ifdef DEBUG
  default:
    PRINT_DEB ("LaunchPrePager: Unknown paging device", 0L);
    ColdReboot ();
#endif
  }

if (!ReceiveInitMsg ())
  return (ERR_MSG_POSTED);
return (SUCCESS);
}

/*********************************************************************/

int LaunchStat (void)

{
DISABLE_VM;
#if !defined(_AROS__)
StatTask = CreateTask (STAT_NAME, 5L, Statistics, 4000L);
#else
StatTask = NewCreateTask(TASKTAG_PC,
                                        Statistics,
                                        TASKTAG_NAME,   STAT_NAME,
                                        TASKTAG_PRI,    5,
                                        TAG_DONE);
#endif
if (!StatTask)
  {
  PRINT_DEB ("Could not create statistics task", 0L);
  ENABLE_VM;
  return (ERR_NOT_ENOUGH_MEM);
  }

if (!ReceiveInitMsg ())
  {
  PRINT_DEB ("ReceiveInitMsg failed", 0L);
  ENABLE_VM;
  return (ERR_MSG_POSTED);
  }

ENABLE_VM;
return (SUCCESS);
}

/*********************************************************************/

PRIVATE void adjust_priority (void)

{
char *PagingDevName;
struct Task *PagingDevTask;
LONG PagingDevPri;

/* If the paging device has its own task, the priority of the page handler
 * is one below that task, otherwise priority 19 is taken
 * It is assumed that the task handling the IO for the device is named
 * the same as the device itself
 */

PagingDevName = PagingDevParams.device->dd_Library.lib_Node.ln_Name;
if ((PagingDevTask = FindTask (PagingDevName)) != NULL)
  {
  PagingDevPri = PagingDevTask->tc_Node.ln_Pri;
  SetTaskPri (PageHandlerTask, PagingDevPri - 1);
  SetTaskPri (PrePagerTask, PagingDevPri - 2);
  }
else
  {
  SetTaskPri (PageHandlerTask, 19L);
  SetTaskPri (PrePagerTask, 18L);
  }
}  

/*********************************************************************/

PRIVATE int InitTrapstructs (void)

{
int i;

NewList (&Free);
NewList (&PageReq);
NewList (&InTransit);

for (i = 0; i < MAX_FAULTS; i++)
  {
  TrapInfo [i].TmpStackSwap.stk_Lower = &(TrapInfo [i].TmpStack [0]);
  TrapInfo [i].TmpStackSwap.stk_Upper = &(TrapInfo [i].TmpStack [TMP_STACKSIZE]);
  TrapInfo [i].TmpStackSwap.stk_Pointer = &(TrapInfo [i].TmpStack [TMP_STACKSIZE]);

  AddHead (&Free, (struct Node*)&(TrapInfo [i]));
  }

return (SUCCESS);
}


/*********************************************************************/

PRIVATE void patch (void)

{
Forbid ();
PRINT_DEB ("Patching LoadSeg ()", 0L);
OrigLoadSeg = (void (*) ()) SetFunction ((struct Library*)DOSBase, -0x96, 
                              (ULONG (*) ()) LoadSegPatch);

PRINT_DEB ("Patching NewLoadSeg ()", 0L);
OrigNewLoadSeg = (void (*) ()) SetFunction ((struct Library*)DOSBase, -0x300, 
                              (ULONG (*) ()) NewLoadSegPatch);

#ifdef DEBUG
PRINT_DEB ("Patching RemTask ()", 0L);
OrigRemTask = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                   -0x120, (ULONG (*) ()) RemTaskPatch);

/*
PRINT_DEB ("Patching StackSwap ()", 0L);
OrigStackSwap = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                   -0x2dc, (ULONG (*) ()) StackSwapPatch);
*/

PRINT_DEB ("Patching Open ()", 0L);
OrigOpen = (void (*) ()) SetFunction ((struct Library*)DOSBase,
                                   -0x1e, (ULONG (*) ()) OpenPatch);

#endif

if (CurrentConfig.PatchWB)
  {
  PRINT_DEB ("Patching SetWindowTitles ()", 0L);
  OrigSetWindowTitles = (void (*) ()) SetFunction ((struct Library*)IntuitionBase,
                                     -0x114, (ULONG (*) ()) SetWindowTitlesPatch);
  }
 
#if !defined(__AROS__)
if (ProcessorType == PROC_68030)
  {
  PRINT_DEB ("Patching ColdReboot ()", 0L);
  OrigColdReboot = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                     -0x2d6, (ULONG (*) ()) ColdRebootPatch);
  }
#endif

PRINT_DEB ("Patching CachePreDMA ()", 0L);
OrigCachePreDMA = (void (*) ()) SetFunction ((struct Library*)SysBase, 
                                   -0x2fa, (ULONG (*) ()) CachePreDMAPatch);

PRINT_DEB ("Patching CachePostDMA ()", 0L);
OrigCachePostDMA = (void (*) ()) SetFunction ((struct Library*)SysBase, 
                                   -0x300, (ULONG (*) ()) CachePostDMAPatch);

PRINT_DEB ("Patching Switch ()", 0L);
OrigSwitch = (void (*) ()) SetFunction ((struct Library*)SysBase, 
                                   -0x36, (ULONG (*) ()) SwitchPatch);

PRINT_DEB ("Patching AddTask ()", 0L);
OrigAddTask = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                   -0x11a, (ULONG (*) ()) AddTaskPatch);

PRINT_DEB ("Patching Wait ()", 0L);
OrigWait = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                   -0x13e, (ULONG (*) ()) WaitPatch);

PRINT_DEB ("Patching AvailMem ()", 0L);
OrigAvailMem = (void (*) ()) SetFunction ((struct Library*)SysBase, -0xd8, 
                              (ULONG (*) ()) AvailMemPatch);

PRINT_DEB ("Patching FreeMem ()", 0L);
OrigFreeMem = (void (*) ()) SetFunction ((struct Library*)SysBase, -0xd2, 
                              (ULONG (*) ()) FreeMemPatch);

PRINT_DEB ("Patching AllocMem ()", 0L);
OrigAllocMem = (void (*) ()) SetFunction ((struct Library*)SysBase, -0xc6, 
                              (ULONG (*) ()) AllocMemPatch);

PRINT_DEB ("Patching done", 0L);
Permit ();
}

/*********************************************************************/

PRIVATE BOOL PatchRamlib (void)

{
struct MsgPort *ramlib_port;
UWORD *ramlib_private;
struct Task *RamLib;

ramlib_private = (UWORD*) SysBase->ex_RamLibPrivate;

if ((RamLib = FindTask ("ramlib")) == NULL)
  {
  PRINT_DEB ("PatchRamlib: Didn't find ramlib task", 0L);
  return (FALSE);
  }

/* Search the next 200 bytes for something that looks like a port. */
while (ramlib_private < (UWORD*)SysBase->ex_RamLibPrivate + 100)
  {
  ramlib_port = (struct MsgPort*) ramlib_private;

  if (ramlib_port->mp_Flags == PA_SIGNAL &&
      ramlib_port->mp_SigTask == RamLib)
    {
    /* Found the port. Find a free signal bit, mark it as used and enter it
     * as the port bit into that port.
     */
    UBYTE signal_bit;

    if (ramlib_port->mp_SigBit != SIGB_SINGLE)
      {
      PRINT_DEB ("PatchRamlib: Patch has already been applied", 0L);
      return (TRUE);
      }

    signal_bit = 31;
    Forbid ();
    while (signal_bit > 15 && 
          ((1L << signal_bit) & RamLib->tc_SigAlloc))
      signal_bit--;

    if (signal_bit == 15)
      {
      Permit ();
      PRINT_DEB ("PatchRamlib: All signal bits of ramlib task in use", 0L);
      return (FALSE);
      }

    RamLib->tc_SigAlloc |= (1L << signal_bit);
    RamLib->tc_SigWait |= (1L << signal_bit);
    ramlib_port->mp_SigBit = signal_bit;
    Signal (RamLib, SIGF_SINGLE | (1L << signal_bit));
    Permit ();
    PRINT_DEB ("PatchRamlib: Successfully patched ramlib", 0L);
    return (TRUE);
    }
  else      
    ramlib_private++;
  }

PRINT_DEB ("PatchRamlib: Didn't find ramlib's port", 0L);
return (FALSE);
}

/*********************************************************************/

int Init_VM_Manager (void)

{
ULONG NeededForTables;
struct Process *myself;
struct VMMsg *StartupMsg;
int rc;

if ((rc = OpenLibs ()) != SUCCESS)
  return (rc);

#ifdef DEBUG
if (!OpenDebugWindow ())
  ReportError ("Debug buffer could not be installed\n"
               "Resuming without debug information", ERR_CONTINUE);
#endif

/* Wait for startup message */
myself = (struct Process*)FindTask (NULL);

PRINT_DEB ("Waiting for startup message", 0L);
WaitPort (&(myself->pr_MsgPort));
StartupMsg = (struct VMMsg*) GetMsg (&(myself->pr_MsgPort));
	bug("[VMM-Handler] %s: StartupMsg @ 0x%p\n", __func__, StartupMsg);
	bug("[VMM-Handler] %s:      Cmd = %08x\n", __func__, StartupMsg->VMCommand);
CxParams = StartupMsg->StartupParams;
	bug("[VMM-Handler] %s: CxParams @ 0x%p\n", __func__, CxParams);

/* CxParams == NULL can happen if started from vmm.library */
if (CxParams != NULL)
  {
  PRINT_DEB ("Received CxParams", 0L);
  if (!(VMEnabled = CxParams->VMEnable))
    DISABLE_VM;
  ExtCxPort = CxParams->ExtCxPort;
  CxSignal = AllocSignal (-1L);
  bug("[VMM-Handler] %s:     prefs file: %s\n", __func__, StartupMsg->StartupParams->PrefsPath);
  PRINT_DEB ("Path for preferences file:", 0L);
  PRINT_DEB (StartupMsg->StartupParams->PrefsPath, 0L);
  Forbid ();
  ExtCxPort->CxPort.mp_SigBit = CxSignal;
  ExtCxPort->CxPort.mp_SigTask = FindTask (NULL);
  Permit ();
  }

PRINT_DEB ("Received startup message", 0L);

bug("[VMM-Handler] %s: freeing StartupMsg\n", __func__);
FreeMem (StartupMsg, sizeof (struct VMMsg));

bug("[VMM-Handler] %s: patching ramlib\n", __func__);
if (!PatchRamlib ())
  {
  PRINT_DEB ("*** Could not install ramlib patch", 0L);
#ifdef DEBUG
  ReportError ("Ramlib patch could not be installed.\nPlease contact author "
               "with output of\n'print_debug' and Kickstart version number", ERR_CONTINUE);
#endif
  }

NewList (&LoadingTasksList);

INIT_SEMA (&VirtMemSema);

CfgChangeSignal = AllocSignal (-1L);
LockAckSignal = AllocSignal (-1L);

VM_ManagerProcess = (struct Process*)FindTask (NULL);

  bug("[VMM-Handler] %s: Manager Process @ 0x%p\n", __func__, VM_ManagerProcess);

#if !defined(__AROS__)
OpenVMMCatalog ();
  
if (SysBase->AttnFlags & AFF_68040)
  {
  PRINT_DEB ("Checking for 68060", 0L);
  if (Is68060 ())
    {
    PRINT_DEB ("68060 detected", 0L);
    ProcessorType = PROC_68060;
    FlushVirt = (void (*) (ULONG)) EmptyFunc;
    CPushP = CPushP60;
    CPushL = CPushL60;
    PFlushP = PFlushP60;
    PFlushA = PFlushA60;
    GenDescr = GenDescr60;
    }
  else
    {
    PRINT_DEB ("68040 detected", 0L);
    ProcessorType = PROC_68040;
    FlushVirt = (void (*) (ULONG)) EmptyFunc;
    CPushP = CPushP40;
    CPushL = CPushL40;
    PFlushP = PFlushP40;
    PFlushA = PFlushA40;
    GenDescr = GenDescr40;
    }
  }
else if (SysBase->AttnFlags & AFF_68030)
  {
  PRINT_DEB ("68030 detected", 0L);
  ProcessorType = PROC_68030;
  FlushVirt = (void (*) (ULONG)) CacheClearU;
  CPushP = (void (*) (ULONG)) EmptyFunc;
  CPushL = (void (*) (ULONG)) EmptyFunc;
  PFlushP = PFlushP30;
  PFlushA = PFlushA30;
  GenDescr = GenDescr30;
  }
else if ((SysBase->AttnFlags & AFF_68020) && MMU68851 ())
  {
  PRINT_DEB ("68020 + 68851 detected", 0L);
  ProcessorType = PROC_68851;
  FlushVirt = (void (*) (ULONG)) CacheClearU;
  CPushP = (void (*) (ULONG)) EmptyFunc;
  CPushL = (void (*) (ULONG)) EmptyFunc;
  PFlushP = PFlushP30;
  PFlushA = PFlushA30;
  GenDescr = GenDescr30;
  }
else
  {
  PRINT_DEB ("VM_Manager: Need 68060, 68040, 68030 or 68020+68851", 0L);
  return (ERR_WRONG_CPU);
  }

/* Figure out if we're running on an A3000. If yes we can force a hard
 * rekick upon reboot if necessary.
 */
IsA3000 = ((GfxBase->ChipRevBits0 & SETCHIPREV_AA) == SETCHIPREV_ECS) &&
           (ProcessorType == PROC_68030 ) && 
           (ALIGN_DOWN ((*GenDescr) (0xF80000), PAGEALIGN) == 0x7f80000);
#else
  Locale_Initialize();

#endif

PRINT_DEB ("Initializing trapstructs", 0L);

if ((rc = InitTrapstructs ()) != SUCCESS)
  return (rc);

PRINT_DEB ("Before MMU table", 0L);

if ((rc = SetupMMUTable ()) != SUCCESS)
  {
  PRINT_DEB ("Couldn't set up MMU table", 0L);
  return (rc);
  }

bug("[VMM-Handler] %s: SetupMMUTable ready. Pagesize is %ld\n", __func__, PAGESIZE);
PRINT_DEB ("SetupMMUTable ready. Pagesize is %ld", PAGESIZE);

if ((InitPort = CreateMsgPort ()) == NULL)
  {
  PRINT_DEB ("VM_Manager: Couldn't allocate InitPort", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }
  
if ((VMPort = CreateMsgPort ()) == NULL)
  {
  PRINT_DEB ("VM_Manager: Couldn't allocate VMPort", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

bug("[VMM-Handler] %s: Msg ports created\n", __func__);

VMPort->mp_Node.ln_Pri = 0L;
VMPort->mp_Node.ln_Name = VMPORTNAME;
AddPort (VMPort);

PRINT_DEB ("VM_Manager: Public port added successfully", 0L);

if ((RexxPort = CreateMsgPort ()) == NULL)
  {
  PRINT_DEB ("VM_Manager: Couldn't allocate RexxPort", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

RexxPort->mp_Node.ln_Pri = 0L;
RexxPort->mp_Node.ln_Name = PROGNAME;   /* Rexx port should have the same
                                         * name as the program. */

AddPort (RexxPort);

bug("[VMM-Handler] %s: Rexx port registered\n", __func__);
PRINT_DEB ("VM_Manager: Rexx port added successfully", 0L);

if ((rc = AllocAddressRange ()) != SUCCESS)
  return (rc);

bug("[VMM-Handler] %s: AddressRange allocated\n", __func__);

PRINT_DEB ("VM_Manager: Start of address space is " str_Addr, (IPTR)VirtAddrStart);
PRINT_DEB ("VM_Manager: End   of address space is " str_Addr, (IPTR)VirtAddrEnd);

if ((rc = InitTaskTable ()) != SUCCESS)
  return (rc);

bug("[VMM-Handler] %s: TaskTable initialised\n", __func__);

if ((rc = InitTrackInfo ()) != SUCCESS)
  return (rc);

bug("[VMM-Handler] %s: TrackInfo initialised\n", __func__);

if ((rc = ReadConfigFile ((CxParams != NULL) ? CxParams->ConfigPath : CFG_FILENAME)) != SUCCESS)
  {
  PRINT_DEB ("ReadConfigFile returned error %ld", (ULONG)rc);
  return (rc);
  }

bug("[VMM-Handler] %s: Config file read\n", __func__);

PRINT_DEB ("VM_Manager: Config file successfully read", 0L);

#if !defined(__AROS__)
AllowZorroIICaching ((BOOL)CurrentConfig.CacheZ2RAM);
if ((rc = SwitchFastROM (CurrentConfig.FastROM)) != SUCCESS)
  {
  PRINT_DEB ("Init_VM_Manager: Error during turning on FastROM", 0L);
  return (rc);
  }
#endif

if ((rc = EnterTask (ERR_NAME, USE_NEVER, USE_NEVER, CNOSWAP, INSERT_FRONT)) != SUCCESS)
  {
  PRINT_DEB ("Init_VM_Manager: Couldn't enter task", 0L);
  return (rc);
  }


bug("[VMM-Handler] %s: started '%s'\n", __func__, ERR_NAME);

if (CurrentConfig.PageDev == PD_FILE)
  {
  if ((rc = EnterTask (PartitionName, USE_NEVER, USE_NEVER, CNOSWAP, INSERT_FRONT)) != SUCCESS)
    {
    PRINT_DEB ("Init_VM_Manager: Couldn't enter task", 0L);
    return (rc);
    }
  }

bug("[VMM-Handler] %s: started '%s'\n", __func__, PartitionName);

if ((rc = EnterTask (PREPAGER_NAME, USE_NEVER, USE_NEVER, CNOSWAP, INSERT_FRONT)) != SUCCESS)
  {
  PRINT_DEB ("Init_VM_Manager: Couldn't enter prepager", 0L);
  return (rc);
  }

bug("[VMM-Handler] %s: started '%s'\n", __func__, PREPAGER_NAME);
  
if (CxParams != NULL)
  {
  if ((rc = EnterTask (FilePart (CxParams->PrefsPath), USE_NEVER, USE_NEVER, CNOSWAP, INSERT_FRONT)) != SUCCESS)
    {
    PRINT_DEB ("Init_VM_Manager: Couldn't enter preferences task", 0L);
    return (rc);
    }
  }

bug("[VMM-Handler] %s: started '%s'\n", __func__, FilePart (CxParams->PrefsPath));
  
VMFreeCounter = COLLECT_INTERVAL;

if (!FindDevParams (PartitionName, &PagingDevParams))
  {
  PRINT_DEB ("VM_Manager: Couldn't get partition information", 0L);
  return (ERR_VOLUME_NOT_FOUND);
  }

PRINT_DEB ("VM_Manager: Found Device params", 0L);

if ((CurrentConfig.PageDev == PD_PART) && (PagingDevParams.SysTask != NULL) && 
    (!Inhibit (PartWithColon, DOSTRUE)))
  {
  PRINT_DEB ("VM_Manager: Couldn't inhibit paging partition", 0L);
  return (ERR_INHIBIT_FAILED);
  }

if ((rc = LaunchPrePager ()) != SUCCESS)
  {
  PRINT_DEB ("VM_Manager: Couldn't create prepager", 0L);
  /* Error msg has already been posted */
  return (rc);
  }

bug("[VMM-Handler] %s: prepager launched\n", __func__);

if ((rc = LaunchPageHandler ()) != SUCCESS)
  {
  PRINT_DEB ("VM_Manager: Couldn't create page handler", 0L);
  return (rc);
  }

bug("[VMM-Handler] %s: pagehandler launched\n", __func__);

if (CurrentConfig.StatEnabled && (rc = LaunchStat ()) != SUCCESS)
  {
  PRINT_DEB ("VM_Manager: Couldn't init stat", 0L);
  /* Error msg has already been posted */
  return (rc);
  }

bug("[VMM-Handler] %s: all support tasks launched\n", __func__);
  
PRINT_DEB ("VM_Manager: All tasks launched", 0L);
  
SetTaskPri (FindTask (NULL), 5L);

if (CurrentConfig.PageDev != PD_FILE)
  adjust_priority ();

CfgChangeNotify.nr_Name = CFG_FILENAME;
CfgChangeNotify.nr_Flags = NRF_SEND_SIGNAL;
CfgChangeNotify.nr_stuff.nr_Signal.nr_Task = FindTask (NULL);
CfgChangeNotify.nr_stuff.nr_Signal.nr_SignalNum = CfgChangeSignal;
StartNotify (&CfgChangeNotify);

bug("[VMM-Handler] %s: FS Notifications enabled for '%s'\n", __func__, CfgChangeNotify.nr_Name);

patch ();

AddedMemSize = (NUM_PTR_TABLES * POINTERS_PER_TABLE * PAGES_PER_TABLE - 1)
                * PAGESIZE;

NeededForTables = ALIGN_UP (PartSize / (PAGESIZE / sizeof (ULONG)), PAGESIZE);

PRINT_DEB ("Needed for tables: " str_Addr, NeededForTables);

if (PartSize - NeededForTables < AddedMemSize)
  AddedMemSize = PartSize - NeededForTables;

VirtMem = (struct MemHeader*)VirtAddrStart;
VirtAddrEnd = VirtAddrStart + AddedMemSize;
PRINT_DEB ("Setting VirtAddrEnd to " str_Addr, (IPTR)VirtAddrEnd);

/* If the VM_Manager is started from StartVMM40, the memory list is 
 * installed in the system memory pool. Otherwise the manager was started
 * by the init routine of vmm.library. Then the memory header is simply
 * initialized but not added to the system
 */

if (FindPort (STARTER_PORT_STD))
  {
bug("[VMM-Handler] %s: Adding MemList %u bytes @ 0x%p '%s'\n", __func__, AddedMemSize, VirtAddrStart, MEMLISTNAME);
  AddMemList (AddedMemSize, MEMF_FAST, CurrentConfig.VMPriority, 
              (APTR)VirtAddrStart, MEMLISTNAME);
  }
else
  {
  VirtMem->mh_Node.ln_Type = NT_MEMORY;
  VirtMem->mh_Node.ln_Name = NULL;      /* Marker for not added to system pool */
  VirtMem->mh_Node.ln_Pri  = CurrentConfig.VMPriority;
  VirtMem->mh_Attributes   = MEMF_FAST;
  VirtMem->mh_First = (struct MemChunk*)(VirtAddrStart + sizeof (struct MemHeader));
  VirtMem->mh_Lower = (void*)(VirtAddrStart + sizeof (struct MemHeader));
  VirtMem->mh_Upper = (void*)(VirtAddrStart + AddedMemSize);
  VirtMem->mh_Free  = AddedMemSize - sizeof (struct MemHeader);
  VirtMem->mh_First->mc_Next = NULL;
  VirtMem->mh_First->mc_Bytes = AddedMemSize - sizeof (struct MemHeader);
  }

PRINT_DEB ("VM_Manager: AddedMemSize = %ld", AddedMemSize);

return (SUCCESS);
}

/*********************************************************************/

PRIVATE void CloseLibs (void)

{
if (DOSBase != NULL)
  CloseLibrary ((struct Library*)DOSBase);

if (IntuitionBase != NULL)
  CloseLibrary (IntuitionBase);

if (GfxBase != NULL)
  CloseLibrary ((struct Library*)GfxBase);

if (UtilityBase != NULL)
  CloseLibrary (UtilityBase);

if (ExpansionBase != NULL)
  CloseLibrary (ExpansionBase);

if (CxBase != NULL)
  CloseLibrary (CxBase);

if (RexxSysBase != NULL)
  CloseLibrary (RexxSysBase);

if (LocaleBase != NULL)
  {
  CloseLibrary (LocaleBase);
  LocaleBase = NULL;
  }
}

/*********************************************************************/

PRIVATE void InstallFreeMemAfterExit (void)

{
/* The following is very ugly, but I haven't found a better way to install
 * a routine that stays alive after exit.
 */

extern ULONG FreeMemAfterExit,
             FreeMemAfterExitEnd,
             PatchLoc1,
             PatchLoc2,
             PatchLoc3,
             ExecFreeMem,
             RemainingBytes;
ULONG NewFreeMem,
      NewPatchLoc1,
      NewPatchLoc2,
      NewPatchLoc3,
      NewExecFreeMem,
      NewRemainingBytes;

#if (0) // Kalamatee
NewFreeMem = (ULONG)DoOrigAllocMem ((ULONG)&FreeMemAfterExitEnd - (ULONG)&FreeMemAfterExit,
                       MEMF_PUBLIC);
if (NewFreeMem == NULL)
  {
  PRINT_DEB ("PANIC: Not enough memory for FreeMemAfterExit", 0L);
  return;
  }

CopyMem ((ULONG*)&FreeMemAfterExit, (ULONG*)NewFreeMem,
         (ULONG)&FreeMemAfterExitEnd - (ULONG)&FreeMemAfterExit);

NewPatchLoc1 = (ULONG)&PatchLoc1 - (ULONG)&FreeMemAfterExit + NewFreeMem + 2;
NewPatchLoc2 = (ULONG)&PatchLoc2 - (ULONG)&FreeMemAfterExit + NewFreeMem + 2;
NewPatchLoc3 = (ULONG)&PatchLoc3 - (ULONG)&FreeMemAfterExit + NewFreeMem + 2;
NewExecFreeMem = (ULONG)&ExecFreeMem - (ULONG)&FreeMemAfterExit + NewFreeMem;
NewRemainingBytes = (ULONG)&RemainingBytes - (ULONG)&FreeMemAfterExit + NewFreeMem;

*(ULONG*)NewExecFreeMem = (ULONG)OrigFreeMem;
*(ULONG*)NewPatchLoc1   = VirtAddrEnd;
*(ULONG*)NewPatchLoc2   = VirtAddrStart;
*(ULONG*)NewPatchLoc3   = (ULONG)(RootTable + ROOTINDEX ((IPTR)VirtAddrStart));
*(ULONG*)NewRemainingBytes = AddedMemSize - sizeof (struct MemHeader) - 
                             VirtMem->mh_Free;

OrigFreeMem = (void (*) ())NewFreeMem;

CacheClearU ();
#else
#warning TODO: WTF!?!?
#endif
}

/*********************************************************************/

PRIVATE void InstallColdRebootAfterExit (void)

{
/* The following is very ugly, but I haven't found a better way to install
 * a routine that stays alive after exit.
 */

#if !defined(__AROS__)
extern ULONG ColdRebootPatchStart,
             ColdRebootPatchEnd,
             PatchLoc4;
IPTR NewColdReboot,
      NewPatchLoc4;

PRINT_DEB ("Installing cold reboot patch after exit", 0L);
NewColdReboot = (IPTR)DoOrigAllocMem ((IPTR)&ColdRebootPatchEnd - 
                                       (IPTR)&ColdRebootPatchStart,
                                       MEMF_PUBLIC);
if (NewColdReboot == NULL)
  {
  PRINT_DEB ("PANIC: Not enough memory for ColdRebootAfterExit", 0L);
  return;
  }

CopyMem ((ULONG*)&ColdRebootPatchStart, (ULONG*)NewColdReboot,
         (IPTR)&ColdRebootPatchEnd - (IPTR)&ColdRebootPatchStart);

NewPatchLoc4 = (IPTR)&PatchLoc4 - (IPTR)&ColdRebootPatchStart + NewColdReboot + 2;

*(ULONG*)NewPatchLoc4   = (ULONG)OrigColdReboot;
*(UWORD*)(NewPatchLoc4 - 2) = 0x2f3c;        /* move.l      #dummy,-(sp) */

OrigColdReboot = (void (*) ())NewColdReboot;

CacheClearU ();
#endif
}

/*********************************************************************/

PRIVATE void unpatch (void)

{
if (OrigSwitch != NULL)
  {
  Forbid ();
  PRINT_DEB ("Undoing patches", 0L);
  SetFunction ((struct Library*)SysBase, -0x2fa, (ULONG (*) ()) OrigCachePreDMA);
  SetFunction ((struct Library*)SysBase, -0x300, (ULONG (*) ()) OrigCachePostDMA);
  SetFunction ((struct Library*)SysBase,  -0x36, (ULONG (*) ()) OrigSwitch);
  SetFunction ((struct Library*)SysBase, -0x11a, (ULONG (*) ()) OrigAddTask);
  SetFunction ((struct Library*)SysBase, -0x13e, (ULONG (*) ()) OrigWait);
  SetFunction ((struct Library*)SysBase,  -0xd8, (ULONG (*) ()) OrigAvailMem);
  SetFunction ((struct Library*)SysBase,  -0xd2, (ULONG (*) ()) OrigFreeMem);
  SetFunction ((struct Library*)SysBase,  -0xc6, (ULONG (*) ()) OrigAllocMem);
  SetFunction ((struct Library*)DOSBase,  -0x96, (ULONG (*) ()) OrigLoadSeg);
  SetFunction ((struct Library*)DOSBase, -0x300, (ULONG (*) ()) OrigNewLoadSeg);
#ifdef DEBUG
  SetFunction ((struct Library*)SysBase, -0x120, (ULONG (*) ()) OrigRemTask);
/*  SetFunction ((struct Library*)SysBase, -0x2dc, (ULONG (*) ()) OrigStackSwap); */
  SetFunction ((struct Library*)DOSBase, -0x1e,  (ULONG (*) ()) OrigOpen);
#endif
  if (ProcessorType == PROC_68030)
    SetFunction ((struct Library*)SysBase, -0x2d6, (ULONG (*) ()) OrigColdReboot);
  if (CurrentConfig.PatchWB)
    SetFunction ((struct Library*)IntuitionBase, -0x114, 
                 (ULONG (*) ()) OrigSetWindowTitles);
  Permit ();
  }
}

/*********************************************************************/

PRIVATE void UninstallAsCommodity (void)

{
struct Message *Msg;

PRINT_DEB ("Uninstalling commodity", 0L);

if (CxParams == NULL)
  return;

if (CxParams->Broker != NULL)
  DeleteCxObjAll (CxParams->Broker);

if (ExtCxPort != NULL)
  {
  RemPort (&(ExtCxPort->CxPort));
  while ((Msg = GetMsg (&(ExtCxPort->CxPort))) != NULL)
    ReplyMsg (Msg);

  FreeSignal ((LONG)ExtCxPort->CxPort.mp_SigBit);

  FreeVec (ExtCxPort->CxPort.mp_Node.ln_Name);
  FreeMem (ExtCxPort, sizeof (struct ExtPort));
  }

FreeVec (CxParams);
}

/**********************************************************************/

void Cleanup_VM_Manager ()

{
int rc;

	bug("[VMM-Handler] %s()\n", __func__);

if (VirtMem != NULL)
  EndNotify (&CfgChangeNotify);
FreeSignal ((LONG)CfgChangeSignal);
FreeSignal ((LONG)LockAckSignal);

if (VirtMem != NULL && VirtMem->mh_Node.ln_Name != NULL)
  {
  Forbid ();
  Remove ((struct Node*)VirtMem);
  Permit ();
  }

	bug("[VMM-Handler] %s: MemHeader removed\n", __func__);
  
if (EarlyExit)
  {
  InstallFreeMemAfterExit ();
  if (ProcessorType == PROC_68030)
    InstallColdRebootAfterExit ();
  }

	bug("[VMM-Handler] %s: removing patches ..\n", __func__);

unpatch ();

	bug("[VMM-Handler] %s: uninstalling commodity..\n", __func__);

UninstallAsCommodity ();

	bug("[VMM-Handler] %s: switching fastrom\n", __func__);

#if !defined(__AROS__)
if ((rc = SwitchFastROM (FALSE)) != SUCCESS)
  RunTimeError (rc);
#endif

while (FindTask (WBUF_ALLOC_NAME) || FindTask (GARBAGE_COLL_NAME))
  Delay (1L);

	bug("[VMM-Handler] %s: killing support tasks...\n", __func__);

if (CurrentConfig.StatEnabled && FindTask (STAT_NAME))
  {
  Signal (StatTask, 1L << StatQuitSignal);
  while (FindTask (STAT_NAME))
    Delay (1L);
  }

if (FindTask (PREPAGER_NAME))
  {
  Signal (PrePagerTask, 1L << PrePagerQuitSignal);
  while (FindTask (PREPAGER_NAME))
    Delay (1L);
  }

if (FindTask (PAGEHANDLER_NAME))
  {
  Signal (PageHandlerTask, 1L << PageHandlerQuitSignal);
  while (FindTask (PAGEHANDLER_NAME))
    Delay (1L);
  }

while (FindTask (ERR_NAME))
  Delay (1L);

  bug("[VMM-Handler] %s: support tasks killed\n", __func__);

PRINT_DEB ("VM_Manager: All subtasks terminated", 0L);

/* Wait until all users of the patched routines have finished */
while ((AllocMemUsers != 0) || (VirtMem != NULL && VirtMemSema.ss_Owner != NULL))
  Delay (1L);

PRINT_DEB ("All users of patched routines exited", 0L);

if (VirtAddrStart != NULL)
  {
/**********************************************************************
 *                     This doesn't work in OS3.0
 * PRINT_DEB ("VM_Manager: Freeing slot " str_Addr, VirtAddrStart >> E_SLOTSHIFT);
 * FreeExpansionMem (VirtAddrStart >> E_SLOTSHIFT, NUM_PTR_TABLES * 0x200L);
 **********************************************************************/
  }

if ((CurrentConfig.PageDev == PD_PART) && (PartitionName [0] != 0) && 
    (PagingDevParams.SysTask != NULL))
  {
  if (Inhibit (PartWithColon, FALSE))
    PRINT_DEB ("Successfully removed inhibit", 0L);
  else
    PRINT_DEB ("Couldn't remove inhibit", 0L);
  }

KillTrackInfo ();

PRINT_DEB ("Killing task table", 0L);
KillTaskTable ();

if (RexxPort)
  {
  if (RexxPort->mp_Node.ln_Name != NULL)
    RemPort (RexxPort);
  DeleteMsgPort (RexxPort);
  }

if (VMPort)
  {
  if (VMPort->mp_Node.ln_Name != NULL)
    RemPort (VMPort);
  DeleteMsgPort (VMPort);
  }

if (InitPort)
  DeleteMsgPort (InitPort);

if (!EarlyExit)
  {
  PRINT_DEB ("Killing MMU table", 0L);
  KillMMUTable ();
  }

#if !defined(__AROS__)
CloseVMMCatalog ();
#else
  Locale_Deinitialize();
#endif

PRINT_DEB ("VM_Manager: Exiting", 0L);

#ifdef DEBUG
CloseDebugWindow ();
#endif

  bug("[VMM-Handler] %s: closing libraries..\n", __func__);

CloseLibs ();

  bug("[VMM-Handler] %s: done\n", __func__);

}
