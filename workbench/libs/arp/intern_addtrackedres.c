#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/arp.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <exec/types.h>
#include "arp_intern.h"

void intern_AddTrackedResource(struct ArpBase * ArpBase,
                               WORD ID,
                               APTR Stuff)
{
  const struct Task * ThisTask = FindTask(NULL);
  struct ArpResList * ARL = (struct ArpResList *)ArpBase->ResLists.mlh_Head;
  struct TrackedResource * TR;
  /* Let's look for this task's first entry in the list */
  while (NULL != ARL)
  {
    if ((ULONG)ThisTask == ARL -> TaskID)
      break;
    ARL = (struct ArpResList *)ARL -> ARL_node.mln_Succ;
  }

  /* If there was no entry for this task yet, then create one! */
  if (NULL == ARL)
    ARL = CreateTaskResList();

  /* Now we can create a TrackedResource and insert it into the list */
  TR = (struct TrackedResource *) AllocMem(sizeof(struct TrackedResource),
                                           MEMF_CLEAR|MEMF_PUBLIC);
  if (NULL != TR)
  {
    TR->TR_ID = ID;
    TR->TR_Stuff = Stuff;
    /* insert as Head into the List */
    AddHead((struct List *)&ARL->FirstItem, (struct Node *)TR);
  }
}
