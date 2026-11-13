#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: stat.c,v 3.6 95/12/16 18:36:56 Martin_Apel Exp $";

#define MARGIN 10L

PRIVATE struct Window *StatWin;

PRIVATE WORD ZoomVals [] =
  {
  0, 0,
  0, 0                /* Dummy values, filled in later */
  };

PRIVATE char WinTitle [40];

PRIVATE struct TagItem WindowTags [] = 
  { { WA_Left, 0 },
    { WA_Top, 0 },
    { WA_Width, 0 },
    { WA_Height, 0 },   /* Dummy values, filled in later */
    { WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_CHANGEWINDOW },
    { WA_Title, (IPTR)WinTitle},
    { WA_CloseGadget, TRUE },
    { WA_DragBar, TRUE },
    { WA_NoCareRefresh, TRUE },
    { WA_SimpleRefresh, TRUE },
    { WA_DepthGadget, TRUE },
    { WA_Zoom, (IPTR)ZoomVals },
    { TAG_DONE, 0 }
  };

PRIVATE int LineSpacing;
PRIVATE int TitleHeight;
PRIVATE int TitleWidth;
PRIVATE UWORD TimerSignal;
PRIVATE void *TimerStruct;
PRIVATE int max_length = 0;

#ifdef SCHED_STAT
#ifdef DEBUG
#define NUM_LINES 12
#else
#define NUM_LINES 10
#endif
#else
#define NUM_LINES 7
#endif

#define CURRENTLY_ZOOMED (StatWin->Height == TitleHeight + 3)

PRIVATE const char *StatStrings [NUM_LINES];

/******************************************************************/

PRIVATE void PrintStatMsg (int y, const char *string, long val)

{
char buffer [80];
int i;

strcpy (buffer, string);

for (i = strlen (string); i <= max_length; i++)
  buffer [i] = ' ';

sprintf (&(buffer [max_length+1]), "%8ld", val);

Move (StatWin->RPort, MARGIN, (long)y * LineSpacing + TitleHeight + 2);
Text (StatWin->RPort, buffer, (ULONG)strlen (buffer));
}

/******************************************************************/

PRIVATE void PrintFreeVirtMem (void)

{
#ifdef SCHED_STAT
struct TrapStruct *ThisFault;
int i;
#endif

int line_no = 1;

if ((CurrentConfig.StatZoomed && !CURRENTLY_ZOOMED) ||
    (!CurrentConfig.StatZoomed && CURRENTLY_ZOOMED))
  ZipWindow (StatWin);

if (!CURRENTLY_ZOOMED)
  {
  PrintStatMsg (line_no++, StatStrings [0], VirtMem->mh_Free);
  PrintStatMsg (line_no++, StatStrings [1], 
                DoOrigAvailMem (MEMF_PUBLIC|MEMF_FAST));
  PrintStatMsg (line_no++, StatStrings [2], NumPageFaults);
  PrintStatMsg (line_no++, StatStrings [3], PagesRead);
  PrintStatMsg (line_no++, StatStrings [4], PagesWritten);
  PrintStatMsg (line_no++, StatStrings [5], NumPageFrames);
  PrintStatMsg (line_no++, StatStrings [6], SlotsUsed ());
#ifdef SCHED_STAT
  PrintStatMsg (line_no++, StatStrings [7], FramesExamined);

  i = 0;
  for (ThisFault = (struct TrapStruct*)Free.lh_Head; 
       ThisFault->TS_Node.mln_Succ != NULL; 
       ThisFault = (struct TrapStruct*) ThisFault->TS_Node.mln_Succ)
    i++;

  PrintStatMsg (line_no++, StatStrings [8], (long)i);

  PrintStatMsg (line_no++, StatStrings [9], AddedMemSize - 
                VirtMem->mh_Free - sizeof (struct MemHeader));

#ifdef DEBUG
  PrintStatMsg (line_no++, StatStrings [10], EnforcerHits);
  PrintStatMsg (line_no++, StatStrings [11], InstructionFaults);
#endif
#endif
  }
else
  {
  sprintf (WinTitle, "VM: %5ld K, Fast: %5ld K", VirtMem->mh_Free / 1024,
           DoOrigAvailMem (MEMF_PUBLIC|MEMF_FAST) / 1024);
  SetWindowTitles (StatWin, WinTitle, (char*)-1);
  }
AddTimedFunction (TimerStruct, 1L, 0L, PrintFreeVirtMem);
}

/******************************************************************/

PRIVATE int Init_Stat (void)

{
struct Screen *DefaultScreen;
int i;

StatQuitSignal   = AllocSignal (-1L);

DISABLE_VM;
if ((TimerStruct = InitTimer (1, &TimerSignal)) == NULL)
  {
  ReportError (_(msgNoTimer), ERR_CONTINUE);
  ENABLE_VM;
  return (ERR_MSG_POSTED);
  }

PRINT_DEB ("Timer initialized", 0L);

StatStrings [0] = _(msgVMFree);
StatStrings [1] = _(msgPublicFastFree);
StatStrings [2] = _(msgNumPF);
StatStrings [3] = _(msgNumRead);
StatStrings [4] = _(msgNumWritten);
StatStrings [5] = _(msgNumFrames);
StatStrings [6] = _(msgPagesUsed);

#ifdef SCHED_STAT
StatStrings [7] = "FramesExamined:";
StatStrings [8] = "Trapstructs free:";
StatStrings [9] = "VM used:";
#ifdef DEBUG
StatStrings [10] = "Enforcer hits:";
StatStrings [11] = "InstructionFaults:";
#endif
#endif

for (i = 0; i < NUM_LINES; i++)
  {
  if (strlen (StatStrings [i]) > max_length)
    max_length = strlen (StatStrings [i]);
  }

/* Window.Width */
Forbid ();
ZoomVals [2] = 
WindowTags [2].ti_Data = GfxBase->DefaultFont->tf_XSize * (max_length + 9)    /* for space and 8 digits */
                         + 2 * MARGIN;
LineSpacing = GfxBase->DefaultFont->tf_YSize + 2;
Permit ();

DefaultScreen = LockPubScreen (NULL);
TitleHeight = DefaultScreen->RastPort.Font->tf_YSize;
TitleWidth = TextLength (&(DefaultScreen->RastPort), "VM: 00000 K, Fast: 00000 K", 26L); 

strcpy (WinTitle, PROGNAME);

if (CurrentConfig.StatZoomed)
  {
  WindowTags [0].ti_Data = CurrentConfig.ZLeftEdge;
  WindowTags [1].ti_Data = CurrentConfig.ZTopEdge;
  WindowTags [2].ti_Data = TitleWidth + 3 * 25;
  WindowTags [3].ti_Data = TitleHeight + 3;
  ZoomVals [0] = CurrentConfig.UnZLeftEdge;
  ZoomVals [1] = CurrentConfig.UnZTopEdge;
  ZoomVals [3] = TitleHeight + (NUM_LINES + 1) * LineSpacing;
  }
else
  {
  WindowTags [0].ti_Data = CurrentConfig.UnZLeftEdge;
  WindowTags [1].ti_Data = CurrentConfig.UnZTopEdge;
  WindowTags [3].ti_Data = TitleHeight + (NUM_LINES + 1) * LineSpacing;
  ZoomVals [0] = CurrentConfig.ZLeftEdge;
  ZoomVals [1] = CurrentConfig.ZTopEdge;
  ZoomVals [2] = TitleWidth + 3 * 25;
  ZoomVals [3] = TitleHeight + 3;
  }

PRINT_DEB ("Opening window", 0L);

if ((StatWin = OpenWindowTagList (NULL, WindowTags)) == NULL)
  {
  PRINT_DEB ("Couldn't open StatWindow", 0L);
  ENABLE_VM;
  return (ERR_NO_STATWINDOW);
  }

UnlockPubScreen (NULL, DefaultScreen);

PRINT_DEB ("Window opened", 0L);
SetAPen (StatWin->RPort, 1L);
AddTimedFunction (TimerStruct, 1L, 0L, PrintFreeVirtMem);
ENABLE_VM;
return (SUCCESS);
}

/******************************************************************/

PRIVATE void Cleanup_Stat (void)

{
if (StatWin != NULL)
  CloseWindow (StatWin);

CurrentConfig.StatEnabled = FALSE;
CloseTimer (TimerStruct);

FreeSignal ((ULONG)StatQuitSignal);

PRINT_DEB ("Exiting", 0L);
}

/******************************************************************/

void Statistics (void)

{
ULONG ReceivedSignals;
ULONG WaitMask;
struct IntuiMessage *IMsg;
struct VMMsg *InitMsg;
BOOL quit = FALSE;
int rc;

if ((rc = Init_Stat ()) != SUCCESS)
  {
  InitError (rc);
  Cleanup_Stat ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

PRINT_DEB ("Initialization ready", 0L);

/* Tell VM_Manager that the stat task has been initialized correctly */
if ((InitMsg = DoOrigAllocMem (sizeof (struct VMMsg), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  {
  PRINT_DEB ("Couldn't allocate memory for init msg", 0L);
  InitError (ERR_NOT_ENOUGH_MEM);
  Cleanup_Stat ();
  Signal ((struct Task*)VM_ManagerProcess, 1L << InitPort->mp_SigBit);
  return;
  }

InitMsg->VMSender = FindTask (NULL);
InitMsg->VMCommand = VMCMD_InitReady;
InitMsg->ReplySignal = 0;                 /* Let VM_Manager free it */
PutMsg (InitPort, (struct Message*)InitMsg);

WaitMask = (1L << TimerSignal) | (1L << StatQuitSignal) |
           (1L << StatWin->UserPort->mp_SigBit);

while (!quit)
  {
  PRINT_DEB ("Waiting for signals", 0L);
  ReceivedSignals = Wait (WaitMask);
  PRINT_DEB ("Received signals", ReceivedSignals);

  if (ReceivedSignals & (1L << TimerSignal))
    HandleTimerReturn (TimerStruct);

  if (ReceivedSignals & (1L << StatQuitSignal))
    quit = TRUE;

  if (ReceivedSignals & (1L << StatWin->UserPort->mp_SigBit))
    {
    if ((IMsg = (struct IntuiMessage*)GetMsg (StatWin->UserPort)) != NULL)
      {
      switch (IMsg->Class)
        {
        case IDCMP_CLOSEWINDOW:
             quit = TRUE;
             break;

        case IDCMP_CHANGEWINDOW:
             CurrentConfig.StatZoomed = CURRENTLY_ZOOMED;
             if (!CurrentConfig.StatZoomed)
               {
               SetWindowTitles (StatWin, PROGNAME, (char*)-1);
               CurrentConfig.UnZLeftEdge = StatWin->LeftEdge;
               CurrentConfig.UnZTopEdge = StatWin->TopEdge;
               }
             else
               {
               CurrentConfig.ZLeftEdge = StatWin->LeftEdge;
               CurrentConfig.ZTopEdge = StatWin->TopEdge;
               }
             break;
        }
      ReplyMsg ((struct Message*)IMsg);
      }
    }
  }
Cleanup_Stat ();
}
