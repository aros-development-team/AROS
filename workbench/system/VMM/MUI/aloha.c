#include "defs.h"

static char rcsid [] = "$Id: aloha.c,v 3.6 95/12/16 19:02:09 Martin_Apel Rel $";

/**********************************************************************/

struct NewBroker nb = 
  {
  NB_VERSION,
  PROGNAME,
  PROGNAME,
  NULL,             /* stuffed in later */  
  NBU_UNIQUE,
  COF_SHOW_HIDE,
  0,                /* stuffed in later */  
  NULL,             /* stuffed in later */
  0
  };

/**********************************************************************/

static BOOL VMEnabled = TRUE;

BOOL InstallAsCommodity (void)

{
char *portname;
char PrefsDir [200];
char PrefsName [80];
BOOL success;
struct Process *myself;
LONG CxSignal;

/* Get the full path for loading this program */
myself = (struct Process*) FindTask (NULL);
if (!NameFromLock (myself->pr_HomeDir, PrefsDir, 200L))
  return (FALSE);

if (GetProgramName (PrefsName, 80L))         /* only for CLI */
  success = AddPart (PrefsDir, FilePart (PrefsName), 200L);
else
  success = AddPart (PrefsDir, myself->pr_Task.tc_Node.ln_Name, 200L);

if (!success)
  return (FALSE);

/* PrefsDir now contains the complete path for loading the Prefs program */

if ((CxParams = AllocVec (sizeof (struct CxParams) + (ULONG)strlen (PrefsDir) + 3,
                          MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  return (FALSE);

strcpy ((char*)(CxParams + 1), PrefsDir);
CxParams->PrefsPath = (char*)(CxParams + 1);
CxParams->ForceOverwrite = ForceOverwrite;

/* Don't use CreateMsgPort for this, because the SigTask and SigBit fields
 * will be changed later.
 * Directly behind the MsgPort there's a pointer to the preferences 
 * process whenever the it is running.
 */

if ((CxParams->ExtCxPort = AllocMem (sizeof (struct ExtPort), MEMF_PUBLIC)) == NULL)
  {
  printf ("Couldn't create commodities port\n");
  return (FALSE);
  }

ExtCxPort = CxParams->ExtCxPort;
ExtCxPort->PrefsTask = NULL;
ExtCxPort->ShowSignal = -1L;

if ((CxSignal = AllocSignal (-1L)) == -1L)
  {
  printf ("Ran out of signals\n");
  return (FALSE);
  }

ExtCxPort->CxPort.mp_SigBit = (UBYTE)CxSignal;
ExtCxPort->CxPort.mp_SigTask = FindTask (NULL);
ExtCxPort->CxPort.mp_Flags = PA_SIGNAL;
ExtCxPort->CxPort.mp_Node.ln_Type = NT_MSGPORT;
ExtCxPort->CxPort.mp_Node.ln_Pri = 0;
NewList (&(ExtCxPort->CxPort.mp_MsgList));
if ((portname = AllocVec ((ULONG)strlen (CXPORTNAME) + 1, MEMF_PUBLIC)) == NULL)
  {
  printf ("Not enough memory for portname\n");
  return (FALSE);
  }

strcpy (portname, CXPORTNAME);

ExtCxPort->CxPort.mp_Node.ln_Name = portname;
AddPort (&(ExtCxPort->CxPort));

nb.nb_Pri = CXPri;
nb.nb_Port = &(ExtCxPort->CxPort);
nb.nb_Descr = _(msgVMMDescr);
if ((CxParams->Broker = CxBroker (&nb, NULL)) == NULL)
  {
  printf ("Couldn't create broker\n");
  return (FALSE);
  }


CxParams->GUIFilter = HotKey (CXPopKey, &(ExtCxPort->CxPort), APPEAR_ID);
if (CxParams->GUIFilter == NULL)
  {
  printf ("Couldn't create commodity hotkey\n");
  return (FALSE);
  }

AttachCxObj (CxParams->Broker, CxParams->GUIFilter);

CxParams->EnableFilter = HotKey (EnableVMHotkey, &(ExtCxPort->CxPort), ENABLE_ID);
if (CxParams->EnableFilter != NULL)
  AttachCxObj (CxParams->Broker, CxParams->EnableFilter);
#ifdef DEBUG
else if (EnableVMHotkey [0] != 0)
  printf ("Couldn't install Enable hotkey\n");
#endif

CxParams->DisableFilter = HotKey (DisableVMHotkey, &(ExtCxPort->CxPort), DISABLE_ID);
if (CxParams->DisableFilter != NULL)
  AttachCxObj (CxParams->Broker, CxParams->DisableFilter);
#ifdef DEBUG
else if (DisableVMHotkey [0] != 0)
  printf ("Couldn't install Disable hotkey\n");
#endif

ActivateCxObj (CxParams->Broker, TRUE);

return (TRUE);
}

/*********************************************************************/

void UninstallAsCommodity (void)

{
struct Message *Msg;

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
  FreeSignal (ExtCxPort->ShowSignal);

  FreeVec (ExtCxPort->CxPort.mp_Node.ln_Name);
  FreeMem (ExtCxPort, sizeof (struct ExtPort));
  }

FreeVec (CxParams);
}

/**********************************************************************/

int HandleCxMsg (void)

/* VMM is not running when this function is called. Otherwise VMM itself
 * would handle these messages.
 */
{
CxMsg *MyMsg;
int status = Q_DONTQUIT;

while ((CxParams != NULL) && 
       (MyMsg = (CxMsg*)GetMsg (&(ExtCxPort->CxPort))) != NULL)
  {
  switch (CxMsgType (MyMsg))
    {
    case CXM_COMMAND:
      switch (CxMsgID (MyMsg))
        {
        case CXCMD_DISABLE: 
               ActivateCxObj (CxParams->Broker, FALSE);
               break;

        case CXCMD_ENABLE: 
               ActivateCxObj (CxParams->Broker, TRUE);
               break;

        case CXCMD_APPEAR: 
               set (Application, MUIA_Application_Iconified, FALSE);
               break;

        case CXCMD_DISAPPEAR: 
               StartVMM ();
               status = Q_QUITGUI;
               break;

        case CXCMD_KILL: 
               status = Q_QUITBOTH;
               break;
        }
      break;

    case CXM_IEVENT:
      switch (CxMsgID (MyMsg))
        {
        case APPEAR_ID: set (Application, MUIA_Application_Iconified, FALSE);
                        break;

        case ENABLE_ID: if (!VMEnabled)
                          {
                          VMEnabled = TRUE;
                          DisplayBeep (NULL);
                          }
                        break;

        case DISABLE_ID:if (VMEnabled)
                          {
                          VMEnabled = FALSE;
                          DisplayBeep (NULL);
                          }
                        break;
        }
    break;
    }

  ReplyMsg ((struct Message*) MyMsg);
  }

return (status);
}

/**********************************************************************/

BOOL StartVMM (void)

{
struct MsgPort *StarterPort;
BPTR SegList;
struct Process *VM_Manager;
struct VMMsg *StartMsg;
BOOL success;
LONG CxSignal;

if (CxParams->EnableFilter == NULL)
  {
  CxParams->EnableFilter = HotKey (EnableVMHotkey, 
                                   &(ExtCxPort->CxPort), ENABLE_ID);
  if (CxParams->EnableFilter == NULL && EnableVMHotkey [0] != 0)
    {
    printf ("Invalid hotkey for VM enable\n");
    return (FALSE);
    }
  AttachCxObj (CxParams->Broker, CxParams->EnableFilter);
  }
else
  {
  SetFilter (CxParams->EnableFilter, EnableVMHotkey);
  if ((CxObjError (CxParams->EnableFilter) != 0) && 
      EnableVMHotkey [0] != 0)
    {
    printf ("Invalid hotkey for VM enable\n");
    return (FALSE);
    }
  }

if (CxParams->DisableFilter == NULL)
  {
  CxParams->DisableFilter = HotKey (DisableVMHotkey, 
                                   &(ExtCxPort->CxPort), DISABLE_ID);
  if (CxParams->DisableFilter == NULL && DisableVMHotkey [0] != 0)
    {
    printf ("Invalid hotkey for VM disable\n");
    return (FALSE);
    }
  AttachCxObj (CxParams->Broker, CxParams->DisableFilter);
  }
else
  {
  SetFilter (CxParams->DisableFilter, DisableVMHotkey);
  if ((CxObjError (CxParams->DisableFilter) != 0) &&
      DisableVMHotkey [0] != 0)
    {
    printf ("Invalid hotkey for VM disable\n");
    return (FALSE);
    }
  }

if ((StartMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  {
  printf (_(msgOutOfMem));
  return (FALSE);
  }

if ((StarterPort = CreateMsgPort ()) == NULL)
  {
  printf ("Couldn't create temporary port\n");
  FreeMem (StartMsg, sizeof (struct VMMsg));
  return (FALSE);
  }

StarterPort->mp_Node.ln_Name = STARTER_PORT_STD;
StarterPort->mp_Node.ln_Pri  = 0;
AddPort (StarterPort);

SegList = NewLoadSeg (PROGPATH, NULL);
if (SegList == NULL)
  {
  printf ("Couldn't load VMM executable\n");
  RemPort (StarterPort);
  DeleteMsgPort (StarterPort);
  FreeMem (StartMsg, sizeof (struct VMMsg));
  return (FALSE);
  }

if ((VM_Manager = CreateNewProcTags (NP_Seglist, (IPTR)SegList,
                                     NP_FreeSeglist, TRUE,
                                     NP_Name, (IPTR)VM_MANAGER_NAME,
                                     NP_StackSize, 4000L,
                                     TAG_DONE, 0L)) == NULL)
  {
  printf ("Couldn't create VMM_Manager process\n");
  UnLoadSeg (SegList);
  RemPort (StarterPort);
  DeleteMsgPort (StarterPort);
  FreeMem (StartMsg, sizeof (struct VMMsg));
  return (FALSE);
  }

/* Send startup message */
CxParams->VMEnable = VMEnabled;
CxParams->ConfigPath = CfgName;
//StartMsg->VMSender = FindTask(NULL);
StartMsg->StartupParams = CxParams;
StartMsg->VMCommand = VMCMD_Startup;
StartMsg->ReplySignal = 0;

CxSignal = ExtCxPort->CxPort.mp_SigBit;
CxParams = NULL;
FreeSignal (CxSignal);
PutMsg (&(VM_Manager->pr_MsgPort), (struct Message*) StartMsg);

/* Wait for initialization message */
 WaitPort (StarterPort);
 if ((StartMsg = (struct VMMsg*)GetMsg (StarterPort)) != NULL)
 {
    success = (StartMsg->VMCommand == VMCMD_InitReady);
    FreeMem (StartMsg, sizeof (struct VMMsg));
 }
 else 
   success = FALSE;

 RemPort (StarterPort);
 DeleteMsgPort (StarterPort);

 return (success);
}

/**********************************************************************/

BOOL StopVMM (void)

/* success only means the quit message could be posted. */
{
struct VMMsg *StopMsg;
struct MsgPort *VMPort;
BOOL success = FALSE;

if ((StopMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) != NULL)
  {
  StopMsg->VMCommand = VMCMD_QuitAll;
  StopMsg->ReplySignal = 0;

  Forbid ();
  if ((VMPort = FindPort (VMPORTNAME)) != NULL)
    {
    success = TRUE;
    PutMsg (VMPort, (struct Message*)StopMsg);
    }
  else
    FreeMem (StopMsg, sizeof (struct VMMsg));
  Permit ();
  }

return (success);
}
