#include <exec/types.h>
#include "defs.h"
#include "Manager_Priv.h"

static char rcsid [] = "$Id: rexx.c,v 3.6 95/12/16 18:37:06 Martin_Apel Exp $";

#define isspace(c) (((c)==' ') || ((c) == '\t') || ((c) == '\n'))

#define RETURN_NO_MEM      -2
#define RETURN_CMD_UNKNOWN -3
#define RETURN_ARG_ERR     -4

PRIVATE char *On  = "ON",
             *Off = "OFF";

/*********************************************************************/

PRIVATE UWORD RexxCommandNum (char *command)

{
int i;

i = 0;
while (RexxFuncs [i].FuncName != NULL)
  {
  if (Stricmp (command, RexxFuncs [i].FuncName) == 0)
    return (RexxFuncs [i].FuncNum);
  i++;
  }

return (REXX_INV_CMD);
}

/*********************************************************************/

PRIVATE BOOL ReadInt (struct RexxMsg *rmsg, int position, LONG *val)

{
char buffer [50];

GetNthString (ARG0 (rmsg), buffer, position);
if (StrToLong (buffer, val) == -1)
  return (FALSE);

return (TRUE);
}

/*********************************************************************/

PRIVATE BOOL CheckOnOff (char *buffer, BOOL *error)

{
*error = FALSE;

if (Stricmp (buffer, On) == 0)
  return (TRUE);
if (Stricmp (buffer, Off) == 0)
  return (FALSE);

*error = TRUE;
return (FALSE);
}

/*********************************************************************/

PRIVATE int RexxAddProg (struct RexxMsg *rmsg)

{
char task_name [50],
     buffer [30];
LONG MinPublic,
     MinNonPublic;
BOOL CodePaging;
BOOL ArgErr;

GetNthString (ARG0 (rmsg), task_name, 2);
GetNthString (ARG0 (rmsg), buffer, 5);
CodePaging = CheckOnOff (buffer, &ArgErr);

if ((task_name [0] == 0) || 
    !ReadInt (rmsg, 3, &MinPublic) ||
    !ReadInt (rmsg, 4, &MinNonPublic) ||
    ArgErr)
  return (RETURN_ARG_ERR);

EnterTask (task_name, MinPublic, MinNonPublic, CodePaging, INSERT_FRONT);

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxRemProg (struct RexxMsg *rmsg)

{
char task_name [50];

GetNthString (ARG0 (rmsg), task_name, 2);
if (task_name [0] == 0)
  return (RETURN_ARG_ERR);

RemoveTask (task_name);

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxPatchWB (struct RexxMsg *rmsg)

{
char param [30];
BOOL ArgErr;
BOOL PatchWB;

GetNthString (ARG0 (rmsg), param, 2);

PatchWB = CheckOnOff (param, &ArgErr);
if (ArgErr)
  return (RETURN_ARG_ERR);

CurrentConfig.PatchWB = PatchWB;  
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

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxMemTrack (struct RexxMsg *rmsg)

{
char param [30];
BOOL ArgErr;
BOOL TmpMemTracking;

GetNthString (ARG0 (rmsg), param, 2);

TmpMemTracking = CheckOnOff (param, &ArgErr);
if (ArgErr)
  return (RETURN_ARG_ERR);

CurrentConfig.MemTracking = MemTracking = TmpMemTracking;
return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxMinVMAlloc (struct RexxMsg *rmsg)

{
LONG val;

if (!ReadInt (rmsg, 2, &val) || (val < 0))
  return (RETURN_ARG_ERR);

CurrentConfig.MinVMAlloc = MinVMAlloc = val;

return (RETURN_OK);
}

/*********************************************************************/

struct
  {
  char *keyword,
       *answer;
  } InfoVals [] =
  { 
    { "title", PROGNAME },
    { "author", AUTHOR },
    { "copyright", COPYRIGHT },
    { "description", DESCRIPTION },
    { "version", VER_STRING },
    { "base", PROGNAME },
    { "screen", NULL },
    { NULL, NULL }
  };
  
/*********************************************************************/

PRIVATE int RexxInfo (struct RexxMsg *rmsg)

{
char param [30];
int i = 0;

GetNthString (ARG0 (rmsg), param, 2);

while (InfoVals [i].keyword != NULL)
  {
  if (Stricmp (param, InfoVals [i].keyword) == 0)
    {
    if (InfoVals [i].answer == NULL)
      rmsg->rm_Result2 = 0;
    else
      rmsg->rm_Result2 = (IPTR)CreateArgstring (InfoVals [i].answer,
                                    (ULONG)strlen (InfoVals [i].answer));
    return (RETURN_OK);
    }
  i++;
  }

return (RETURN_ARG_ERR);
}

/*********************************************************************/

PRIVATE int RexxStat (struct RexxMsg *rmsg)

{
char param [30];
int rc;
BOOL ArgErr;
BOOL enable;

GetNthString (ARG0 (rmsg), param, 2);
enable = CheckOnOff (param, &ArgErr);
if (ArgErr)
  return (RETURN_ARG_ERR);

CurrentConfig.StatEnabled = enable;
if (CurrentConfig.StatEnabled && (FindTask (STAT_NAME) == NULL))
  {
  if ((rc = LaunchStat ()) != SUCCESS)
    {
    RunTimeError (rc);
    return (RETURN_ERROR);
    }
  }
else if (!CurrentConfig.StatEnabled && (FindTask (STAT_NAME) != NULL))
  Signal (StatTask, 1L << StatQuitSignal);

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxZoom (struct RexxMsg *rmsg)

{
char param [30];
BOOL ArgErr;
BOOL zoom;

GetNthString (ARG0 (rmsg), param, 2);
zoom = CheckOnOff (param, &ArgErr);
if (ArgErr)
  return (RETURN_ARG_ERR);

CurrentConfig.StatZoomed = zoom;
return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxMem (struct RexxMsg *rmsg)

{
ULONG MinMem,
      MaxMem;
struct VMMsg *ConfigMsg;

if (!ReadInt (rmsg, 2, (LONG*)&MinMem) ||
    !ReadInt (rmsg, 3, (LONG*)&MaxMem))
  return (RETURN_ARG_ERR);

MinMem = ALIGN_DOWN (MinMem, PAGESIZE);
MaxMem = ALIGN_DOWN (MaxMem, PAGESIZE);
if (MinMem < MAX_FAULTS * PAGESIZE)
  MinMem = MAX_FAULTS * PAGESIZE;

if (MaxMem < MinMem)
  MaxMem = MinMem;

CurrentConfig.MinMem = MinMem;
CurrentConfig.MaxMem = MaxMem;

if ((ConfigMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  return (RETURN_NO_MEM);

ConfigMsg->VMCommand = VMCMD_NewConfig;
ConfigMsg->ReplySignal = 0;
ConfigMsg->VMSender = (struct Task*)VM_ManagerProcess;
PutMsg (PageHandlerPort, (struct Message*) ConfigMsg);

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxMemType (struct RexxMsg *rmsg)

{
ULONG flags;

if (!ReadInt (rmsg, 2, (LONG*)&flags))
  return (RETURN_ARG_ERR);

CurrentConfig.MemFlags = flags;

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxWriteBuffer (struct RexxMsg *rmsg)

{
if (!ReadInt (rmsg, 2, (LONG*)&DesiredWriteBufferSize))
  return (RETURN_ARG_ERR);

DISABLE_VM;
CreateTask (WBUF_ALLOC_NAME, 5L, AllocNewCache, 1000L);
ENABLE_VM;

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxVMPri (struct RexxMsg *rmsg)

{
LONG pri;

if (!ReadInt (rmsg, 2, &pri) || (pri < -128) || (pri > 127))
  return (RETURN_ARG_ERR);

CurrentConfig.VMPriority = pri;

Forbid ();
if (VirtMem->mh_Node.ln_Name != NULL)      
  {
  Remove ((struct Node*)VirtMem);
  VirtMem->mh_Node.ln_Pri = CurrentConfig.VMPriority;
  Enqueue (&(SysBase->MemList), (struct Node*)VirtMem);
  }
Permit ();

return (RETURN_OK);
}

/*********************************************************************/

PRIVATE int RexxHelp (struct RexxMsg *rmsg)

{
char filename [100];
BPTR HelpFile;
struct RexxFuncEntry *rfe;

GetNthString (ARG0 (rmsg), filename, 2);
if (filename [0] == 0)
  return (RETURN_ARG_ERR);

DISABLE_VM;
if ((HelpFile = Open (filename, MODE_NEWFILE)) == NULL)
  {
  ENABLE_VM;
  return (RETURN_NO_MEM);
  }

rfe = &(RexxFuncs [0]);
while (rfe->FuncName != NULL)
  {
  FPrintf (HelpFile, "%s\n", rfe->FuncName);
  rfe++;
  }

Close (HelpFile);

ENABLE_VM;
return (RETURN_OK);
}

/*********************************************************************/

BOOL HandleRexxMsg (void)

{
int primary;
BOOL Quit = FALSE;
int rc;
char cmd [30];
struct RexxMsg *rmsg;

while ((rmsg = (struct RexxMsg*)GetMsg (RexxPort)) != NULL)
  {
  GetNthString (ARG0 (rmsg), cmd, 1);

  PRINT_DEB ("Received RexxMsg", 0L);

  primary = RETURN_OK;
  PRINT_DEB ("cmdname is ", 0L);
  PRINT_DEB (cmd, 0L);
  rmsg->rm_Result2 = 0;
  switch (RexxCommandNum (cmd))
    {
    case REXX_INV_CMD: primary = RETURN_CMD_UNKNOWN; break;

    case REXX_ADDPROG: primary = RexxAddProg (rmsg);
                       break;

    case REXX_REMPROG: primary = RexxRemProg (rmsg);
                       break;

    case REXX_ENABLE:  if (!VMEnabled)
                         {
                         VMEnabled = TRUE;
                         ENABLE_VM;
                         }
                       break;

    case REXX_DISABLE: if (VMEnabled)
                         {
                         VMEnabled = FALSE;
                         DISABLE_VM;
                         }
                       break;

    case REXX_QUIT:    if (TryToQuit ())
                         Quit = TRUE;
                       break;

    case REXX_HIDE:    HideGUI ();
                       break;

    case REXX_SHOW:    if ((rc = ShowGUI ()) != SUCCESS)
                         {
                         RunTimeError (rc);
                         primary = RETURN_ERROR;
                         }
                       break;

    case REXX_INFO:    primary = RexxInfo (rmsg);
                       break;

    case REXX_STAT:    primary = RexxStat (rmsg);
                       break;

    case REXX_MEM:     primary = RexxMem (rmsg);
                       break;

    case REXX_MEMTYPE: primary = RexxMemType (rmsg);

    case REXX_WRITEBUFFER: primary = RexxWriteBuffer (rmsg);
                           break;

    case REXX_VMPRI:      primary = RexxVMPri (rmsg);
                          break;

    case REXX_PATCHWB:    primary = RexxPatchWB (rmsg);
                          break;

    case REXX_MEMTRACK:   primary = RexxMemTrack (rmsg);
                          break;

    case REXX_MINVMALLOC: primary = RexxMinVMAlloc (rmsg);
                          break;

    case REXX_HELP:       primary = RexxHelp (rmsg);
                          break;

    case REXX_ZOOM:       primary = RexxZoom (rmsg);
                          break;
    }
  
  rmsg->rm_Result1 = primary;
  ReplyMsg ((struct Message*)rmsg);
  }

return (Quit);
}
