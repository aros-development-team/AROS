#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __GNUC__
#include "../include/VMM_Stat.h"
#else
#include "/include/VMM_Stat.h"
#endif

void main (void)

{
struct VMUsageMsg *UsageMsg;
LONG   VMSignal;
struct MsgPort *VMPort;
BOOL success = FALSE;
struct TaskVMUsage *tmp;

if ((UsageMsg = AllocMem (sizeof (struct VMUsageMsg), MEMF_PUBLIC)) == NULL)
  {
  printf ("No mem\n");
  exit (10);
  }

if ((VMSignal = AllocSignal (-1L)) == -1L)
  {
  printf ("No signal\n");
  FreeMem (UsageMsg, sizeof (struct VMUsageMsg));
  exit (10);
  }

UsageMsg->VMMessage.mn_Length = sizeof (struct VMUsageMsg);
UsageMsg->Sender      = FindTask (NULL);
UsageMsg->Command     = VMCMD_AskVMUsage;
UsageMsg->ReplySignal = VMSignal;

Forbid ();
if ((VMPort = FindPort ("VMM_Port")) != NULL)
  {
  PutMsg (VMPort, (struct Message*)UsageMsg);
  success = TRUE;
  }
else
  printf ("VMM is not running\n");

Permit ();

if (success)
  {
  Wait (1L << VMSignal);
  while ((tmp = (struct TaskVMUsage*)RemHead (UsageMsg->TaskList)) != NULL)
    {
    printf ("%-30s %8ld\n", tmp->vu_Node.ln_Name, tmp->VMInUse);
    FreeMem (tmp, tmp->FreeSize);
    }
  FreeMem (UsageMsg->TaskList, sizeof (struct List));
  }

FreeMem (UsageMsg, sizeof (struct VMUsageMsg));
FreeSignal (VMSignal);
exit (0);
}
