#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef __GNUC__
#include <clib/exec_protos.h>
#else
#include <inline/exec.h>
#endif

#define DEBUG

#ifdef __GNUC__
#include "../shared_defs.h"
#else
#include "/shared_defs.h"
#endif

static char rcsid [] = "$Id: print_debug.c,v 1.2 93/09/02 13:35:26 Martin_Apel Exp $";

#define FILBUF_SIZE (64L * 1024L)
#define OFFSET_OF(struct_name, member) ((ULONG)&(((struct struct_name*)0)->member))

extern struct ExecBase *SysBase;
struct DebugBuffer *DebugBuffer;

main ()

{
char *FileBuffer;
char *tmp;
int length;
FILE *debout;
ULONG debug_start;
struct MemHeader *mh;
BOOL success = FALSE,
     Allocated = FALSE;

Forbid ();

for (mh = (struct MemHeader*) SysBase->MemList.lh_TailPred;
     !success && (mh->mh_Node.ln_Pred != NULL); 
     mh = (struct MemHeader*) mh->mh_Node.ln_Pred)
  {
  debug_start = (ULONG) mh->mh_Upper - sizeof (struct DebugBuffer);
  DebugBuffer = (struct DebugBuffer*) debug_start;
  if ((DebugBuffer->Magic1 == DEBUG_MAGIC1) && 
      (DebugBuffer->Magic2 == DEBUG_MAGIC2) &&
      (DebugBuffer->ThisBuffer == DebugBuffer))
    {
    success = TRUE;
    if (AllocAbs (sizeof (struct DebugBuffer), (APTR)debug_start) == NULL)
      {
      Permit ();
      printf ("Found debug buffer, but could not allocate it\n");
      printf ("Trying to read it anyway. Contents may be garbled\n");
      }
    else
      Allocated = TRUE;
    }
  }

Permit ();

if (!success)
  {
  /* Try to find debug buffer somewhere in memory */
  ULONG *mem_ptr;

  printf ("Didn't find debug buffer at default location\nSearching memory...\n");

  for (mh = (struct MemHeader*)SysBase->MemList.lh_Head; 
       !success && mh->mh_Node.ln_Succ != NULL; 
       mh = (struct MemHeader*)mh->mh_Node.ln_Succ)
    {
    for (mem_ptr = (ULONG*)mh->mh_Lower; 
         mem_ptr < (ULONG*)mh->mh_Upper;
         mem_ptr++)
      {
      if (*mem_ptr == DEBUG_MAGIC1)
        {
        /* Probably found it */
        DebugBuffer = (struct DebugBuffer*) ((UBYTE*)mem_ptr - OFFSET_OF (DebugBuffer, Magic1));
        if (DebugBuffer->ThisBuffer == DebugBuffer && DebugBuffer->Magic2 == DEBUG_MAGIC2)
          {
          success = TRUE;
          if (AllocAbs (sizeof (struct DebugBuffer), (APTR)DebugBuffer) == NULL)
            {
            printf ("Found debug buffer, but could not allocate it\n");
            printf ("Trying to read it anyway. Contents may be garbled\n");
            }
          else
            Allocated = TRUE;
          break;
          }
        }
      }
    }

  if (!success)
    {
    printf ("Didn't find valid debug buffer\nExiting...\n");
    exit (10);
    }
  }

printf ("Found debug buffer\n");

if ((FileBuffer = AllocMem (FILBUF_SIZE, MEMF_PUBLIC)) == NULL)
  {
  printf ("couldn't allocate mem for file buffer\n");
  if (Allocated)
    FreeMem (DebugBuffer, sizeof (struct DebugBuffer));
  exit (5);
  }

if ((debout = fopen ("trace", "w")) == NULL)
  {
  printf ("Couldn't open trace file\n");
  if (Allocated)
    FreeMem (DebugBuffer, sizeof (struct DebugBuffer));
  exit (5);
  }

setvbuf (debout, FileBuffer, _IOFBF, FILBUF_SIZE);

for (tmp = DebugBuffer->DbgInfo; tmp < DebugBuffer->NextFree;)
  {
  length = strlen (tmp);
  if (!(length & 1))
    length++;
  fprintf (debout, tmp, *((ULONG*)(tmp + length + 1)));
  fputc ('\n', debout);
  tmp += length + 5;
  }

if (Allocated)
  FreeMem (DebugBuffer, sizeof (struct DebugBuffer));
fclose (debout);
FreeMem (FileBuffer, FILBUF_SIZE);
exit (0);
}
