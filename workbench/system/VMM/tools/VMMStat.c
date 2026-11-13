#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_stdio_protos.h>
#ifdef __GNUC__
#include "../shared_defs.h"
#else
#include "/shared_defs.h"
#endif

int main (void)

{
struct VMMsg *StatMsg;
struct MsgPort *VMMPort;
ULONG ReplySignal;

if ((ReplySignal = AllocSignal (-1L)) == -1)
  {
  printf ("Could not allocate signal\n");
  return (5);
  }

if ((StatMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  {
  printf ("Not enough memory for message\n");
  FreeSignal (ReplySignal);
  return (5);
  }

StatMsg->VMSender = FindTask (NULL);
StatMsg->VMCommand = VMCMD_AskStat;
StatMsg->ReplySignal = ReplySignal;

Forbid ();
if ((VMMPort = FindPort (VMPORTNAME)) == NULL)
  {
  Permit ();
  FreeMem (StatMsg, sizeof (struct VMMsg));
  FreeSignal (ReplySignal);
  printf ("VMM is not running\n");
  return (0);
  }

PutMsg (VMMPort, (struct Message*)StatMsg);
Permit ();

Wait (1L << ReplySignal);

printf ("VMMStat V2.1\n\n");
printf ("  Overall VM size:                 %8ld\n", StatMsg->st_VMSize);
printf ("  VM free:                         %8ld\n", StatMsg->st_VMFree);
printf ("  Number of pagefaults:            %8ld\n", StatMsg->st_Faults);
printf ("  Number of pages read:            %8ld\n", StatMsg->st_PagesRead);
printf ("  Number of pages written:         %8ld\n", StatMsg->st_PagesWritten);
printf ("  Number of allocated page frames: %8ld\n", StatMsg->st_Frames);
printf ("  Number of pages used on device:  %8ld\n", StatMsg->st_PagesUsed);
printf ("  Pagesize:                        %8ld\n", StatMsg->st_PageSize);
printf ("  Number of TrapStructs free:      %8ld\n", StatMsg->st_TrapStructsFree);

FreeMem (StatMsg, sizeof (struct VMMsg));
FreeSignal (ReplySignal);
return (0);
}
