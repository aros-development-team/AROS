#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"


void addprocesstoroot(struct Process * process, struct DosLibrary * DOSBase)
{
  ULONG * taskarray;
  ULONG * newtaskarray;
  ULONG size;
  ULONG i;
  
  Forbid();
  
  taskarray = BADDR(DOSBase->dl_Root->rn_TaskArray);
  size = taskarray[0];
  
  /*
  ** Check out the taskarray for an empty slot
  */
  i = 1;
  while (i <= size)
  {
    if (0 == taskarray[i])
    {
      taskarray[i] = (ULONG)&process->pr_MsgPort;
      process->pr_TaskNum = i;
      Permit();
      return;
    }

    i++;
  }

  /*
  ** it seems like a new taskarray is needed 
  */
  newtaskarray = AllocMem(sizeof(ULONG)+(size+1)*sizeof(APTR),MEMF_ANY);

  newtaskarray[0] = size+1;
  i = 1;
  while (i <= size)
  {
    newtaskarray[i] = taskarray[i];
    i++;
  }
  
  newtaskarray[size+1] = (ULONG)&process->pr_MsgPort;
  process->pr_TaskNum = size+1;
  
  DOSBase->dl_Root->rn_TaskArray = MKBADDR(newtaskarray);
  
  FreeMem(taskarray, sizeof(ULONG)+size*sizeof(APTR));

  Permit();
}

void removefromrootnode(struct Process * process, struct DosLibrary * DOSBase)
{
  ULONG size;
  ULONG * taskarray;
  ULONG i;
  
  Forbid();
  
  taskarray = BADDR(DOSBase->dl_Root->rn_TaskArray);
  size = taskarray[0];
  
  i = 1;
  while (i <= size)
  {
    if (taskarray[i] == (ULONG)&process->pr_MsgPort)
    {
      taskarray[i] = 0;
      break;
    }
    i++;
  }
  
  Permit();
}
