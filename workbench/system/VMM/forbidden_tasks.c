#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: forbidden_tasks.c,v 3.5 95/12/16 18:36:49 Martin_Apel Exp $";

#define MIN(a,b) (((a)<(b))?(a):(b))

struct TaskNameEntry
  {
  struct MinNode TNE_Node;
  char *TaskName;
  ULONG MinPublic;
  ULONG MinNonPublic;
  BOOL SwapCode;
  BOOL IsDir;
  BPTR LoadDir;        /* Only used if IsDir == TRUE */
  };

#ifdef DEBUG
ULONG HashTableUsage [HASHTABSIZE];
#endif

PRIVATE struct List TaskInfo;

/**********************************************************************
 * The index into the hash table is the lower six bits of the first
 * character of the name
 **********************************************************************/

PRIVATE void EnterIntoHashTab (char *name, int name_len,
                               ULONG MinPublic, ULONG MinNonPublic)
/* name_len is the length of the name excluding the trailing zero byte
 * or the leading length byte for B-Strings.
 */
{
static int next_free = -1;
BOOL found = FALSE;
struct HashEntry *HE;
LONG offset;

offset = HASH_VAL ((IPTR)name);

PRINT_DEB ("EnterIntoHashTab: ", 0L);
PRINT_DEB (name, 0L);

#ifdef DEBUG
HashTableUsage [offset]++;
#endif

Forbid ();

/* First check if it's already part of the hash table. Because the
 * string is stored as an array of longwords, it might be that
 * the bytes after the end of the string have changed. The longword
 * approach is chosen for performance reasons in AllocMem. If it
 * is found, copy the current string with the following bytes to the
 * hash entry.
 */

for (HE = HashTab [offset]; HE != NULL; HE = HE->NextEntry)
  {
  if (strncmp (name, (char*)HE->Name, name_len + 1) == 0)
    {
    PRINT_DEB ("Task already in hash table. Copying trailing bytes", 0L);
    *(HE->Name + HE->NumLongsM1) = *((ULONG*)name + HE->NumLongsM1);
    Permit ();
    return;
    }
  }

/* The task has not been found in the hash table */
while (!found)
  {
  next_free = (next_free + 1) % HASHTABSIZE;
  HE = &(HashEntries [next_free]);
  if (HE->Referenced)
    {
    HE->Referenced = FALSE;
    PRINT_DEB ("EnterIntoHashTab: Marked entry as unused", 0L);
    }
  else
    {
    /* use this entry. First check if this entry was used before */
    if (HE->Name != NULL)
      {
      /* it was used. Remove it from the corresponding hash chain. */
      struct HashEntry **OldEntry;
      int old_offset;

      PRINT_DEB ("EnterIntoHashTab: Evicting old hash table entry", 0L);

      old_offset = HE->HashIndex;
      for (OldEntry = &(HashTab [old_offset]); *OldEntry != HE;
           OldEntry = &((*OldEntry)->NextEntry))
        PRINT_DEB ("Looping through old hash chain", 0L);

      *OldEntry = HE->NextEntry;

      FreeMem (HE->Name, (ULONG) (HE->NumLongsM1 + 1) * sizeof (ULONG));
      }

    HE->MinPublic    = MinPublic;
    HE->MinNonPublic = MinNonPublic;
    HE->Referenced   = TRUE;
    HE->HashIndex    = offset;

    /* name_len + 1 is either the length of a null-terminated string
     * including the zero or the length of BCPL string including the
     * leading size byte.
     */
    HE->NumLongsM1 = ALIGN_UP (name_len + 1, sizeof (ULONG)) / sizeof (ULONG) - 1;
    if ((HE->Name = DoOrigAllocMem ((ULONG)(HE->NumLongsM1 + 1) * sizeof (ULONG),
                                     MEMF_PUBLIC)) == NULL)
      {
      PRINT_DEB ("No space for hash table entry", 0L);
      Permit ();
      return;
      }
    CopyMem (name, HE->Name, (ULONG) (HE->NumLongsM1 + 1) * sizeof (ULONG));

    HE->NextEntry = HashTab [offset];
    HashTab [offset] = HE;
    found = TRUE;
    }
  }
Permit ();
}

/**********************************************************************
 * This routine is called by the VM_Manager, for a task
 * which does an AllocMem but hasn't found itself in the hash table.
 **********************************************************************/

void ExtCheckVirtMem (struct Task *AskingTask)

{
struct TaskNameEntry *TNE;
struct Process *AskingProcess;
char *name,         /* This is the one to be entered into the hash table */
     *name_filepart;
char complete_path [200];
int name_len;
BOOL IsCLI = FALSE;

AskingProcess = NULL;

PRINT_DEB ("ExtCheckVirtMem called for task", 0L);

if (AskingTask->tc_Node.ln_Type == NT_PROCESS)
  {
  AskingProcess = (struct Process*)AskingTask;
  if (AskingProcess->pr_CLI != NULL)
    {
    struct CommandLineInterface *CLI;
  
    CLI = (struct CommandLineInterface*)BADDR(AskingProcess->pr_CLI);
  
    if (CLI->cli_CommandName != NULL)
      {
      name = (char*)BADDR(CLI->cli_CommandName);
      if (*(name + 1) != 0)
        {
        name_len = (int)*name;
        strncpy (complete_path, name + 1, (size_t)name_len);
        complete_path [name_len] = 0;
        name_filepart = FilePart (complete_path);
        IsCLI = TRUE;
        }
      }
    }
  }

if (!IsCLI)
  {
  name = AskingTask->tc_Node.ln_Name;
  name_filepart = name;
  name_len = strlen (name);
  }

PRINT_DEB (name_filepart, 0L);

for (TNE = (struct TaskNameEntry*)TaskInfo.lh_Head;
     TNE->TNE_Node.mln_Succ != NULL; 
     TNE = (struct TaskNameEntry*) TNE->TNE_Node.mln_Succ)
  {
  if (TNE->IsDir)
    {
    PRINT_DEB ("Checking directory", 0L);
    if (AskingProcess != NULL)
      {
      PRINT_DEB ("TNE->LoadDir = %08lx", (IPTR)TNE->LoadDir);
      PRINT_DEB ("HomeDir = %08lx", (IPTR)AskingProcess->pr_HomeDir);
      if (SameLock (TNE->LoadDir, AskingProcess->pr_HomeDir) == LOCK_SAME)
        {
        PRINT_DEB ("File has been loaded from directory", 0L);
        PRINT_DEB (TNE->TaskName, 0L);
        EnterIntoHashTab (name, name_len, TNE->MinPublic, TNE->MinNonPublic);
        return;
        }
      }
    }
  else  if (MatchPatternNoCase (TNE->TaskName, name_filepart))
    {
    /* Enter this task name into the hash table  */
    EnterIntoHashTab (name, name_len, TNE->MinPublic, TNE->MinNonPublic);
    return;
    }
  }

/* The task was not found in the TaskInfo list. Enter it into the hash table
 * anyway and return the default value for allowance of virtual memory
 */

PRINT_DEB ("Didn't find task in task info list", 0L);
EnterIntoHashTab (name, name_len, CurrentConfig.DefaultMinPublic, 
                  CurrentConfig.DefaultMinNonPublic);
}

/**********************************************************************/

BOOL CodePagingAllowed (const char *filename)

{
struct TaskNameEntry *TNE;
char *fp = FilePart (filename);

PRINT_DEB ("Checking code paging for file", 0L);
PRINT_DEB ((char *)filename, 0L);

for (TNE = (struct TaskNameEntry*)TaskInfo.lh_Head;
     TNE->TNE_Node.mln_Succ != NULL; 
     TNE = (struct TaskNameEntry*) TNE->TNE_Node.mln_Succ)
  {
  if (TNE->IsDir)
    {
    BPTR ProgLock,
         ParentLock;

    ProgLock = Lock (filename, ACCESS_READ);
    if (ProgLock != NULL)
      {
      ParentLock = ParentDir (ProgLock);
      UnLock (ProgLock);
      if (SameLock (TNE->LoadDir, ParentLock) == LOCK_SAME)
        {
        struct Process *LoadingProcess;
        struct CommandLineInterface *CLI;

        PRINT_DEB ("File is in directory", 0L);
        PRINT_DEB (TNE->TaskName, 0L);
        UnLock (ParentLock);

        /* The name of the loaded file has to be entered into the hash table 
         * because during loading virtual memory is allocated
         * although the HomeDir of the process has not yet been set.
         * Because the name is entered with a pointer to the original
         * location, we hope that the CLI will have already set its name correctly.
         * This would confuse ExtCheckVirtMem.
         */

        LoadingProcess = (struct Process*) FindTask (NULL);
        CLI = (struct CommandLineInterface*)BADDR (LoadingProcess->pr_CLI);
        if (CLI != NULL && CLI->cli_CommandName != NULL)
          {
          char *name;
          
          name = (char*)BADDR (CLI->cli_CommandName);
          if (*(name + 1) != 0)
            EnterIntoHashTab (name, (int)*name, TNE->MinPublic, TNE->MinNonPublic);
          }
                        
        return (TNE->SwapCode);
        }
      UnLock (ParentLock);
      }
    }
  else if (MatchPatternNoCase (TNE->TaskName, fp))
    {
    return (TNE->SwapCode);
    }
  }

return (CurrentConfig.DefaultCodePaging);
}

/**********************************************************************/

PRIVATE void FlushHashTable (void)

{
int i;
struct HashEntry *HE;

Forbid ();
for (i = 0; i < HASHTABSIZE; i++)
  {
  HE = &(HashEntries [i]);
  if (HE->Name != NULL)
    {
    FreeMem (HE->Name, (ULONG) (HE->NumLongsM1 + 1) * sizeof (ULONG));
    HE->Name = NULL;
    }
  HashTab [i] = NULL;
  }
Permit ();
}

/**********************************************************************/

int EnterTask (const char *name, ULONG MinPublic, ULONG MinNonPublic, 
               BOOL SwapCode, BOOL InsertFront)

{
struct TaskNameEntry *NewEntry;
char *token_buffer;

DISABLE_VM;
if ((NewEntry = AllocVec ((ULONG)(sizeof (struct TaskNameEntry) + 2 * strlen (name) + 2),
                         MEMF_PUBLIC)) == NULL)
  {
  ENABLE_VM;
  PRINT_DEB ("EnterTask: Couldn't allocate mem for TaskNameEntry", 0L);
  return (ERR_NOT_ENOUGH_MEM);
  }

ENABLE_VM;
token_buffer = (char*)(NewEntry + 1);

if (*(name + strlen (name) - 1) == '/' ||
    *(name + strlen (name) - 1) == ':')
  {
  PRINT_DEB ("Entering directory", 0L);
  PRINT_DEB ((char *)name, 0L);
  NewEntry->IsDir = TRUE;
  strcpy (token_buffer, name);
  DISABLE_VM;
  NewEntry->LoadDir = Lock (name, ACCESS_READ);
  PRINT_DEB ("Lock is " str_Addr, (IPTR)NewEntry->LoadDir);
  ENABLE_VM;
  }
else
  {
  NewEntry->IsDir = FALSE;
  ParsePatternNoCase (name, token_buffer, (LONG)(2 * strlen (name) + 2));
  }

NewEntry->TaskName = token_buffer;
NewEntry->MinPublic = MinPublic;
NewEntry->MinNonPublic = MinNonPublic;
NewEntry->SwapCode = SwapCode;

Forbid ();
FlushHashTable ();
if (InsertFront)
  AddHead (&TaskInfo, (struct Node*)NewEntry);
else
  AddTail (&TaskInfo, (struct Node*)NewEntry);

Permit ();
return (SUCCESS);
}

/**********************************************************************/

BOOL RemoveTask (const char *name)

{
/* Removes a task from the task info list. If it is found, TRUE is returned,
 * otherwise FALSE.
 * Because the name itself is not stored in TaskNameEntry but only the parsed pattern,
 * name is first parsed and then compared. This assumes that the parsed string is also
 * terminated by a null byte.
 */

struct TaskNameEntry *tmp;
char token_buffer [200];

ParsePatternNoCase (name, token_buffer, 200L);

for (tmp = (struct TaskNameEntry *) TaskInfo.lh_Head; tmp->TNE_Node.mln_Succ != NULL;
     tmp = (struct TaskNameEntry *) tmp->TNE_Node.mln_Succ)
  {
  if ((tmp->IsDir && strcmp (tmp->TaskName, name) == 0) ||
      (!tmp->IsDir && (strcmp (tmp->TaskName, token_buffer) == 0)))
    {
    Forbid ();
    Remove ((struct Node*)tmp);
    FlushHashTable ();
    Permit ();
    if (tmp->IsDir)
      {
      PRINT_DEB ("Unlocking directory", 0L);
      PRINT_DEB (tmp->TaskName, 0L);
      DISABLE_VM;
      UnLock (tmp->LoadDir);
      ENABLE_VM;
      }
    return (TRUE);
    }
  }

return (FALSE);
}

/**********************************************************************/

void NoMoreVM (void)

{
/* Removes all entries from the hash table and the task table.
 * It also sets DefaultVirtMem not to use VM anymore
 */

struct TaskNameEntry *TNE;

PRINT_DEB ("NoMoreVM called", 0L);
Forbid ();
FlushHashTable ();
while ((TNE = (struct TaskNameEntry*)RemHead (&TaskInfo)) != NULL)
  {
  if (TNE->IsDir)  
    {
    PRINT_DEB ("Unlocking directory", 0L);
    PRINT_DEB (TNE->TaskName, 0L);
    DISABLE_VM;
    UnLock (TNE->LoadDir);
    ENABLE_VM;
    }
  FreeVec (TNE);
  }

CurrentConfig.DefaultMinPublic = USE_NEVER;
CurrentConfig.DefaultMinNonPublic = USE_NEVER;
Permit ();
}

/**********************************************************************/

int InitTaskTable (void)

{
int i;

NewList (&TaskInfo);

for (i = 0; i < HASHTABSIZE; i++)
  {
  HashTab [i] = NULL;
  HashEntries [i].Name = NULL;
  HashEntries [i].Referenced = FALSE;
  }

return (SUCCESS);
}

/**********************************************************************/

void KillTaskTable (void)

{
struct TaskNameEntry *current;

if (TaskInfo.lh_Head == NULL)
  return;

while ((current = (struct TaskNameEntry*)RemHead (&TaskInfo)) != NULL)
  {
  if (current->IsDir)  
    {
    PRINT_DEB ("Unlocking directory", 0L);
    PRINT_DEB (current->TaskName, 0L);
    UnLock (current->LoadDir);
    }
  FreeVec (current);
  }

FlushHashTable ();

#ifdef DEBUG
  {
  ULONG i;

  for (i = 0; i < HASHTABSIZE; i++)
    {
    PRINT_DEB ("HashTable entry %ld", i);
    PRINT_DEB ("was used %ld times", HashTableUsage [i]);
    }
  }
#endif
}
