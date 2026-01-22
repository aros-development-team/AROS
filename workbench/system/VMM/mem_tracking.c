#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: mem_tracking.c,v 3.7 95/12/16 18:37:03 Martin_Apel Exp $";

/**********************************************************************/

/* The following structure is exported to the public. It should never 
 * change. If it ever does, it has to be changed in include/VMM_Stat.h 
 * too.
 */

struct TaskVMUsage
  {
  struct Node vu_Node;
  ULONG  VMInUse;
  ULONG  StructSize;
  };

/**********************************************************************/
/* These are private */

struct TaskNameInfo
  {
  struct MinNode TN_Node;
  UWORD Size;                 /* of this struct plus taskname buffer  */
  char *Name;                 /* Note: this is the same offset as in  */
                              /* a struct Node. This way FindName can */
                              /* be used.                             */
  ULONG UsageCnt;             /* How often is this task name used,    */
  };                          /* if == 0, remove this entry           */

struct TrackInfo
  {
  struct MinNode MT_Node;
  ULONG  Size;           /* Size of this allocation */
  struct TaskNameInfo *TaskName;
  };

PRIVATE struct List TaskNameList;
PRIVATE struct List TrackAllocList;
PRIVATE void *TrackPool;

/* Used for a temporary buffer for the task name. This could be allocated
 * locally to CreateTrackInfo, but that may result in problems with
 * tasks running low on stack. As CreateTrackInfo is only called within
 * MEM_FORBID, it need not be reentrant.
 * name_start is simply a pointer to the name_buffer if it's called from
 * a normal task or the pointer to the last component of the path if it's
 * called from a CLI
 */

PRIVATE char name_buffer [100];
PRIVATE char *name_start;

/**********************************************************************/

/* CreateTrackInfo and FreeTrackInfo must only be called from inside
 * MEM_FORBID and MEM_PERMIT. They are protected by the same semaphore
 * that protects virtual memory.
 */

/**********************************************************************/

PRIVATE struct Node *FindIName (struct List *list, char *name)

{
struct Node *tmp;

/* Does the same as FindName, but case-insensitive. */
for (tmp = list->lh_Head; tmp->ln_Succ != NULL; tmp = tmp->ln_Succ)
  {
  if (Stricmp (tmp->ln_Name, name) == 0)
    return (tmp);
  }

return (NULL);
}

/**********************************************************************/

PRIVATE struct TaskNameInfo *EnterTaskName (void)

{
/* Enters the task name starting at 'name_start' into the name list.
 * If it was already there, only its usage count is increased,
 * otherwise a new node is created.
 */

struct TaskNameInfo *tmp;
ULONG size;

if ((tmp = (struct TaskNameInfo*) FindIName (&TaskNameList, name_start)) != NULL)
  {
  tmp->UsageCnt++;
  return (tmp);
  }

/* Create a new entry */
size = sizeof (struct TaskNameInfo) + strlen (name_start) + 1L;
if ((tmp = LibAllocPooled (TrackPool, size)) == NULL)
  return (NULL);

tmp->Size = size;
strcpy ((char*)(tmp + 1), name_start);
tmp->Name = (char*)(tmp + 1);
tmp->UsageCnt = 1;
AddHead (&TaskNameList, (struct Node*)tmp);
return (tmp);
}

/**********************************************************************/

ULONG *CreateTrackInfo (ULONG *buffer, ULONG orig_size)

{
/* Creates a header for the current allocation and returns the
 * start of the buffer as it should be returned to the requesting task.
 */

struct Task *AskingTask = FindTask (NULL);
struct Process *AskingProcess;
BOOL IsCLI = FALSE;
char *name;
struct TaskNameInfo *tni;
struct TrackInfo *ti;

AskingProcess = (struct Process*)AskingTask;

if ((AskingTask->tc_Node.ln_Type == NT_PROCESS) && 
    (AskingProcess->pr_CLI != NULL))
  {
  struct CommandLineInterface *CLI;

  CLI = (struct CommandLineInterface*) BADDR(AskingProcess->pr_CLI);
  if (CLI->cli_CommandName != NULL)
    {
    name = (char*) BADDR (CLI->cli_CommandName);

    if (*(name + 1) != 0)
      {
      IsCLI = TRUE;
      strncpy (name_buffer, name + 1, *name);
      name_buffer [*name] = 0;
      name_start = FilePart (name_buffer);
      }
    }
  }

if (!IsCLI)
  name_start = AskingTask->tc_Node.ln_Name;

tni = EnterTaskName ();
if ((ti = LibAllocPooled (TrackPool, sizeof (struct TrackInfo))) == NULL)
  {
  /* Don't track this allocation */
  return (buffer);
  }

ti->Size = (orig_size + 7) & ~0x7;
ti->TaskName = tni;
AddHead (&TrackAllocList, (struct Node*)ti);

*buffer++ = (ULONG) ti;
*buffer++ = TRACK_MAGIC;
return (buffer);
}

/**********************************************************************/

void FreeTrackInfo (void *TrackBuffer)

{
struct TrackInfo *ti = (struct TrackInfo*) TrackBuffer;
struct TaskNameInfo *tni;

if (ti == NULL)
  return;

tni = ti->TaskName;

Remove ((struct Node*) ti);
LibFreePooled (TrackPool, ti, sizeof (struct TrackInfo));

if (tni == NULL)
  return;

if (--tni->UsageCnt != 0)
  return;

Remove ((struct Node*)tni);
LibFreePooled (TrackPool, tni, (ULONG)tni->Size);
}

/**********************************************************************/

void ChangeOwner (ULONG *buffer)

{
struct TrackInfo *ti = (struct TrackInfo*) *(buffer - 2);
struct TaskNameInfo *tni;
struct LoadingTaskStruct *cur;
char *new_name;

PRINT_DEB ("ChangeOwner called", 0L);

if ((*(buffer - 1) != TRACK_MAGIC) || ti == NULL)
  return;

if ((tni = ti->TaskName) == NULL)
  return;

Forbid ();
for (cur = (struct LoadingTaskStruct*)LoadingTasksList.lh_Head; 
     cur->lt_Node.mln_Succ != NULL && cur->LoadingTask != SysBase->ThisTask;
     cur = (struct LoadingTaskStruct*)cur->lt_Node.mln_Succ);
Permit ();

if (cur->LoadingTask != SysBase->ThisTask)
  return;

new_name = FilePart (cur->LoadfileName);

if (strcmp (tni->Name, new_name) == 0)
  {
  PRINT_DEB ("Old owner is new owner", 0L);
  return;
  }

PRINT_DEB ("Old owner was", 0L);
PRINT_DEB (tni->Name, 0L);
PRINT_DEB ("New owner is", 0L);
PRINT_DEB (new_name, 0L);

OBTAIN_VM_SEMA;
name_start = new_name;

if (--tni->UsageCnt == 0)
  {
  PRINT_DEB ("Removing old TaskNameInfo", 0L);
  Remove ((struct Node*) tni);
  LibFreePooled (TrackPool, tni, (ULONG) tni->Size);
  }

tni = EnterTaskName ();
ti->TaskName = tni;
RELEASE_VM_SEMA;
}

/**********************************************************************/

PRIVATE void EnterUsage (struct List *TaskList, char *name, ULONG amount)

{
struct TaskVMUsage *tmp;
ULONG free_size;
char *file_part;

if ((tmp = (struct TaskVMUsage*)FindIName (TaskList, name)) != NULL)
  {
  /* Task is already in list. Add VMUsage */
  tmp->VMInUse += amount;
  }
else
  {
  free_size = sizeof (struct TaskVMUsage) + (ULONG)strlen (name) + 1;
  if ((tmp = DoOrigAllocMem (free_size, MEMF_PUBLIC)) != NULL)
    {
    tmp->VMInUse = amount;
    strcpy ((char*)(tmp + 1), name);
    tmp->vu_Node.ln_Name = (char*)(tmp + 1);
    tmp->StructSize = free_size;
    AddHead (TaskList, (struct Node*)tmp);
    }
  }
}

/*****************************************************************/

void VMUsageInfo (struct VMMsg *UsageMsg)

{
/* This function creates a list of tasks currently using virtual memory.
 * The list entries are of type 'struct TaskVMUsage'. After the requesting  
 * task has used the information it must free every single node and
 * the list structure.
 */
struct TrackInfo *ti;
struct List **TaskList;

PRINT_DEB ("CreateUsageInfo called", 0L);

TaskList = &(UsageMsg->UsageList);

if ((*TaskList = DoOrigAllocMem (sizeof (struct List), MEMF_PUBLIC)) == NULL)
  return;

NewList (*TaskList);

OBTAIN_VM_SEMA;

for (ti = (struct TrackInfo*) TrackAllocList.lh_Head; 
     ti->MT_Node.mln_Succ != NULL; 
     ti = (struct TrackInfo*) ti->MT_Node.mln_Succ)
  {
  EnterUsage (*TaskList, ti->TaskName->Name, ti->Size);
  }

RELEASE_VM_SEMA;

/* Reply to requesting task */
PRINT_DEB ("Replying to requesting task", 0L);
Signal (UsageMsg->VMSender, 1L << UsageMsg->ReplySignal);
}

/*****************************************************************/

int InitTrackInfo (void)

{
NewList (&TaskNameList);
NewList (&TrackAllocList);

if ((TrackPool = LibCreatePool (MEMF_PUBLIC, 10000L, 1000L)) == NULL)
  return (ERR_NOT_ENOUGH_MEM);

return (SUCCESS);
}

/*****************************************************************/

void KillTrackInfo (void)

{
if (TrackPool != NULL)
  LibDeletePool (TrackPool);
}
