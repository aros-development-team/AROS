#ifdef DEBUG

#include <exec/types.h>
#include "defs.h"

static char rcsid [] = "$Id: debug.c,v 3.6 95/12/16 18:36:45 Martin_Apel Exp $";

/* Double-buffered */
PRIVATE IPTR MemListEntries [2] [2000];
PRIVATE ULONG CurrentBuffer;

PRIVATE struct DebugBuffer *DebugBuffer;

/******************************************************************/

BOOL OpenDebugWindow (void)

{
ULONG debug_start;
struct MemHeader *mh;
BOOL success = FALSE;
int i;
ULONG *VectorTable;
extern ULONG OrigFuncs [];

#if !defined(__AROS__)
Forbid ();

for (mh = (struct MemHeader*) SysBase->MemList.lh_TailPred;
     !success && (mh->mh_Node.ln_Pred != NULL); 
     mh = (struct MemHeader*) mh->mh_Node.ln_Pred)
  {
  debug_start = (ULONG) mh->mh_Upper - sizeof (struct DebugBuffer);
  DebugBuffer = AllocAbs (sizeof (struct DebugBuffer), (APTR)debug_start);
  if (DebugBuffer != NULL)
    success = TRUE;
  }

Permit ();
if (!success)
  {
  DebugBuffer = AllocMem (sizeof (struct DebugBuffer), MEMF_PUBLIC | MEMF_REVERSE);
  if (DebugBuffer == NULL)
    return (FALSE);
  }

DebugBuffer->Magic1 = DEBUG_MAGIC1;
DebugBuffer->Magic2 = DEBUG_MAGIC2;
DebugBuffer->ThisBuffer = DebugBuffer;
DebugBuffer->NextFree = DebugBuffer->DbgInfo;

/* Install crash handler */

Disable ();
VectorTable = (ULONG*)ReadVBR ();

for (i = 3; i < 8; i++)
  {
  OrigFuncs [i] = *(VectorTable + i);
  *(VectorTable + i) = (ULONG)&CrashHandler;
  }

for (i = 9; i < 16; i++)
  {
  if (i != 11)
    {
    OrigFuncs [i] = *(VectorTable + i);
    *(VectorTable + i) = (ULONG)&CrashHandler;
    }
  }

for (i = 48; i < 59; i++)
  {
  OrigFuncs [i] = *(VectorTable + i);
  *(VectorTable + i) = (ULONG)&CrashHandler;
  }

Enable ();

OrigAlert = (void (*) ()) SetFunction ((struct Library*)SysBase,
                                   -0x6c, (ULONG (*) ()) AlertPatch);
#endif

return (TRUE);
}

/******************************************************************/

void CloseDebugWindow (void)

{
#if !defined(__AROS__)
SetFunction ((struct Library*)SysBase, -0x6c, (ULONG (*) ()) OrigAlert);

if (DebugBuffer != NULL)
  {
  FreeMem (DebugBuffer, sizeof (struct DebugBuffer));
  DebugBuffer = NULL;
  }
#endif
}

/******************************************************************/

#define SR_SV 0x2000

void PrintDebugMsg (char *string, long val)

{
#if !defined(__AROS__)
int length;
char *Free;
struct Task *ThisTask;
struct Process *ThisProcess;
char *name,
     *name_filepart;
BOOL SVMode;
char tmp_string [100];
char *my_string;

if (DebugBuffer == NULL)
  return;

/* Be very careful. The string might be in virtual memory and must not
 * be accessed then.
 */

SVMode = SetSR (0L, 0L) & SR_SV;

ThisTask = SysBase->ThisTask;
ThisProcess = (struct Process*)ThisTask;

if ((ThisTask->tc_Node.ln_Type == NT_PROCESS) && (ThisProcess->pr_CLI != NULL) &&
  (((struct CommandLineInterface*)BADDR(ThisProcess->pr_CLI))->cli_CommandName != NULL))
  {
  name = (char*)BADDR(((struct CommandLineInterface*)
                          BADDR(ThisProcess->pr_CLI))->cli_CommandName) + 1;
  if (*name == 0)
    {
    name = ThisTask->tc_Node.ln_Name;
    name_filepart = name;
    }
  else if (SVMode)
    name_filepart = name;
  else
    name_filepart = FilePart (name);
  }
else
  {
  name = ThisTask->tc_Node.ln_Name;
  name_filepart = name;
  }

length = strlen (name_filepart);

if ((IPTR)string >= VirtAddrStart && (IPTR)string < VirtAddrEnd)
  {
  strcpy (tmp_string, string);
  my_string = tmp_string;
  }
else
  my_string = string;

length += strlen (my_string) + 2;          /* for ": " */
if (SVMode)
  length += 3;                          /* for "SV " */

Disable ();

if (DebugBuffer->NextFree + length + 5 >= (char*)&(DebugBuffer->Magic2))
  DebugBuffer->NextFree = DebugBuffer->DbgInfo;    /* wrap around */

Free = DebugBuffer->NextFree;

if (SVMode)
  {
  strcpy (Free, "SV ");
  strcat (Free, name_filepart);
  }
else
  strcpy (Free, name_filepart);

strcat (Free, ": ");
strcat (Free, my_string);
Free += length + 1;
if ((ULONG)Free & 1)
  Free += 1;

*((long*)Free) = val;
Free += 4;
DebugBuffer->NextFree = Free;
Enable ();
if (CPushP != NULL)           /* This is called very early before the */
  (*CPushP) ((ULONG)Free);    /* processor type has been determined */
#endif
}

/******************************************************************/

void PrintTrapStruct (struct TrapStruct *ThisFault)

{
ULONG *SP;
PRINT_DEB ("PrintTrapStruct: FaultTask = " str_Addr, (IPTR)ThisFault->FaultTask);
PRINT_DEB ("PrintTrapStruct: FaultAddr = " str_Addr, ThisFault->FaultAddress);
PRINT_DEB ("PrintTrapStruct: TopOfStack = " str_Addr, (IPTR)ThisFault->TopOfStackFrame);
PRINT_DEB ("PrintTrapStruct: WakeupSignal = " str_Addr, (ULONG)ThisFault->WakeupSignal);
SP = (ULONG*)&(ThisFault->TmpStack[TMP_STACKSIZE-16]);
PRINT_DEB ("PrintTrapStruct: 4 lowest longwords from TmpStack", 0L);
PRINT_DEB ("PrintTrapStruct:   " str_Addr, *SP++);
PRINT_DEB ("PrintTrapStruct:   " str_Addr, *SP++);
PRINT_DEB ("PrintTrapStruct:   " str_Addr, *SP++);
PRINT_DEB ("PrintTrapStruct:   " str_Addr, *SP);
PRINT_DEB ("PrintTrapStruct: Addr of page descr: " str_Addr, (IPTR)ThisFault->PageDescrAddr);
PRINT_DEB ("PrintTrapStruct: Page Descr = " str_Addr, *(ThisFault->PageDescrAddr));
PRINT_DEB ("PrintTrapStruct: PhysAddr = " str_Addr, ThisFault->PhysAddr);
}

/******************************************************************/

void CheckMemList (void)

{
struct MemHeader *mh;
struct MemChunk *mc;
ULONG size_sum;
ULONG num_entries;
IPTR *tmp;

Disable ();
for (mh = (struct MemHeader*)SysBase->MemList.lh_Head;
     mh->mh_Node.ln_Succ != NULL;
     mh = (struct MemHeader*)mh->mh_Node.ln_Succ)
  {
  if ((mh->mh_Attributes & (MEMF_PUBLIC | MEMF_FAST)) == (MEMF_PUBLIC | MEMF_FAST))
    {
    /* check only this one */
    size_sum = 0L;
    tmp = &(MemListEntries [CurrentBuffer] [0]);
    for (mc = mh->mh_First; mc != NULL; mc = mc->mc_Next)
      {
      size_sum += mc->mc_Bytes;
      *tmp++ = (IPTR)mc;
      *tmp++ = mc->mc_Bytes;
      }
    *tmp = 0;
    CurrentBuffer = (CurrentBuffer + 1) & 1;      /* switch buffer */
    if (mh->mh_Free != size_sum)
      {
      PrintDebugMsg ("CHECK CONSISTENCY FAILED. MEMSIZE IN HEADER: " str_Addr, mh->mh_Free);
      PrintDebugMsg ("                          MEMSIZE IN LIST  : " str_Addr, size_sum);
      num_entries = 0;
      for (mc = mh->mh_First; mc != NULL; mc = mc->mc_Next)
        {
        PrintDebugMsg ("Free chunk at " str_Addr, (IPTR)mc);
        PrintDebugMsg ("size " str_Addr, mc->mc_Bytes);
        num_entries++;
        }
      PRINT_DEB ("MemList has %ld entries", num_entries);
      PRINT_DEB ("Current SP = " str_Addr, (IPTR)&mh);
      PRINT_DEB ("MemList before inconsistency:", 0L);
      tmp = &(MemListEntries [CurrentBuffer] [0]);
      while (*tmp != 0)
        {
        PRINT_DEB ("Free chunk at " str_Addr, *tmp++);
        PRINT_DEB ("size " str_Addr, *tmp++);
        }
      ColdReboot ();
      }
    Enable ();
    return;
    }
  }
PrintDebugMsg ("CHECKMEMLIST: MEMHEADER NOT FOUND", 0L);
ColdReboot ();
Enable ();
}

#endif
