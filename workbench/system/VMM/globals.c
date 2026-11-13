#include <exec/types.h>
#include <exec/alerts.h>
#include <proto/alib.h>

static char rcsid [] = "$Id: globals.c,v 3.6 95/12/16 18:36:41 Martin_Apel Exp $";

#undef GLOBAL
#define GLOBAL

#include "defs.h"

#if !defined(__AROS__)
#define ERROR_STACK 2000L
#else
#include <exec/memheaderext.h>
#define ERROR_STACK 20000L
#endif

PRIVATE UWORD ErrorLevel;
PRIVATE const char *ErrorString;
PRIVATE struct Task *ErringTask;
PRIVATE char ErrorBuffer [500];
PRIVATE IPTR ErrorHandlerStack [ERROR_STACK/sizeof (IPTR)];
PRIVATE struct Task ErrorHandlerTask;

PRIVATE struct EasyStruct ErrorReq =
  {
  sizeof (struct EasyStruct),
  0,
  PROGNAME,
  ErrorBuffer,
  NULL
  };

PRIVATE IPTR ErrorMsg [] = 
  {
  0,        /* SUCCESS */
  0,        /* ERR_MSG_POSTED */
  msgNotEnoughMem,
  msgNoPagingFile,
  msgNoSpace,
  msgFailedIo,
  msgFileIsDir,
  msgInternal,
  msgInhibitFailed,
  msgWrongBlocksize,
  msgNoDiskinfo,
  msgNotValidated,
  msgOutOfBounds,
  msgNoFFS,
  msgNoConfigFile,
  msgCorruptCfgFile,
  msgVolumeNotFound,
  msgLowVersion,
  msgNoDOS,
  msgNoIntuition,
  msgNoGfx,
  msgNoUtility,
  msgNoExpansion,
  msgNoCommodities,
  msgWrongCPU,
  msgNoAddrSpace,
  msgNoPrefs,
  msgNoStatwindow,
  msgNoRexx,
  msgDynMapFailed
  };


/*********************************************************************/

PRIVATE void ErrorTask (void)

{
PRINT_DEB ("ErrorTask alive", 0L);

#if !defined(__AROS__)
sprintf (ErrorBuffer, _(msgGenericError),
         (ErrorLevel == ERR_NOERROR) ? _(msgMessage) : _(msgError),
         ErringTask->tc_Node.ln_Name, ErrorString);

if (ErrorLevel == ERR_FATAL)
  ErrorReq.es_GadgetFormat = _(msgReboot);
else
  ErrorReq.es_GadgetFormat = _(msgOK);

    STRPTR format = CreateFormatStringFromEasyStruct(&ErrorReq);
    EasyRequestArgs(NULL, &ErrorReq, NULL, format);
    FreeFormatString(format);

if (ErrorLevel == ERR_FATAL)
  {
  PRINT_DEB ("Fatal error. Rebooting...", 0L);
  ColdReboot ();
  }
else
  PRINT_DEB ("Continuing after error", 0L);
#endif

ENABLE_VM;          /* Corresponding DISABLE_VM has been done by ReportError */
}

/*********************************************************************/

void ReportError (const char *error_string, UWORD error_level)

{
PRINT_DEB ("ReportError called", 0L);

DISABLE_VM;
if (FindTask (ERR_NAME) != NULL)
  {
  PRINT_DEB ("Couldn't post error message. Error task already existed", 0L);
  return;
  }

ErrorHandlerTask.tc_Node.ln_Type = NT_TASK;
ErrorHandlerTask.tc_Node.ln_Name = ERR_NAME;
ErrorHandlerTask.tc_Node.ln_Pri = 10;
ErrorHandlerTask.tc_SPReg = &(ErrorHandlerStack [ERROR_STACK/sizeof (ULONG) - 1]);
ErrorHandlerTask.tc_SPLower = &(ErrorHandlerStack [0]);
ErrorHandlerTask.tc_SPUpper = &(ErrorHandlerStack [ERROR_STACK/sizeof (ULONG) - 1]);
NewList (&(ErrorHandlerTask.tc_MemEntry));

ErrorLevel = error_level;
ErringTask = FindTask (NULL);
ErrorString = error_string;

PRINT_DEB ("Creating error task", 0L);
#if defined(__AROS__)
struct TagItem _task_tags[] = \
         {{TASKTAG_ARG1, (IPTR)SysBase}, {TAG_END, 0}};
    
if (NewAddTask(&ErrorHandlerTask, ErrorTask, NULL, _task_tags) == NULL)
#else
if (AddTask (&ErrorHandlerTask, ErrorTask, NULL) == NULL)
#endif
  {
  PRINT_DEB ("Error creating error task", 0L);
  if (ErrorLevel == ERR_FATAL)
    Alert (AT_DeadEnd | AG_IOError | AN_Unknown);
  else
    Alert (AT_Recovery | AG_IOError | AN_Unknown);
  }
PRINT_DEB ("Successfully created error task", 0L);
}

/***********************************************************************/

void InitError (int num)

{
if (num == ERR_MSG_POSTED)
  return;

if (num > ERR_MAX_ERRNO)
  {
  PRINT_DEB ("InitError: Illegal error number %ld", (ULONG)num);
#ifdef DEBUG
  ColdReboot ();
#endif
  num = ERR_INTERNAL;
  }

ReportError (_(ErrorMsg [num]), ERR_CONTINUE);
}

/***********************************************************************/

void RunTimeError (int num)

{
InitError (num);
}

/***********************************************************************/

void FatalError (int num)

{
if (num > ERR_MAX_ERRNO)
  {
  PRINT_DEB ("FatalError: Illegal error number %ld", (ULONG)num);
#ifdef DEBUG
  ColdReboot ();
#endif
  num = ERR_INTERNAL;
  }

ReportError (_(ErrorMsg [num]), ERR_FATAL);
Wait (0L);          /* Wait forever */
}

/***********************************************************************/

void *AllocAligned (ULONG size, ULONG flags, ULONG alignment, ULONG priority)

/* Allocates "size" bytes aligned at an "alignment" boundary.
 * "alignment" must be a power of two and larger than the minimum
 * memory size (currently 8 bytes).
 * Supported flags are MEMF_PUBLIC, MEMF_CHIP, MEMF_FAST, MEMF_REVERSE.
 * Priority is a hint on how much time should be spent in AllocAligned.
 * 0 is the highest priority, it means: Do whatever you can to make
 * memory available. For lower values, 'priority' gives the number of
 * bytes which must be available in a memory region to make AllocAligned
 * step into it and search.
 */
{
struct MemHeader *mh;
struct MemChunk **cur_mc,
                **chosen_mc;

#ifdef DEBUG
if (size % alignment != 0)
  {
  ReportError ("Internal error in AllocAligned:\n"
               "size not multiple of alignment", ERR_CONTINUE);
  return (NULL);
  }
#endif

#if !defined(__AROS__)
Forbid ();
CHECK_CONSISTENCY;
chosen_mc = NULL;
for (mh = (struct MemHeader*)SysBase->MemList.lh_Head;
     mh->mh_Node.ln_Succ != NULL;
     mh = (struct MemHeader*)mh->mh_Node.ln_Succ)
  {
  /* Check flags */
  if (((mh->mh_Attributes & flags) == (flags & (MEMF_CHIP | MEMF_FAST | MEMF_PUBLIC)))
      && (mh->mh_Free > priority))
    {
    if (flags & MEMF_REVERSE)
      {
      /* Use last fitting chunk */
      for (cur_mc = &(mh->mh_First); *cur_mc != NULL;
           cur_mc = &((*cur_mc)->mc_Next))
        {
        if (((*cur_mc)->mc_Bytes >= size) &&
            (ALIGN_UP ((ULONG)*cur_mc, alignment) + size <=
                                   (ULONG)*cur_mc + (*cur_mc)->mc_Bytes))
          chosen_mc = cur_mc;
        }
      }
    else
      {
      /* Use first fitting chunk */
      for (cur_mc = &(mh->mh_First); *cur_mc != NULL;
           cur_mc = &((*cur_mc)->mc_Next))
        {
        if (((*cur_mc)->mc_Bytes >= size) &&
            (ALIGN_UP ((ULONG)*cur_mc, alignment) + size <=
                                   (ULONG)*cur_mc + (*cur_mc)->mc_Bytes))

          {
          chosen_mc = cur_mc;
          break;
          }
        }
      }
    if (chosen_mc != NULL)
      break;
    }
  }

if (chosen_mc == NULL)
  {
  void *tmp_buffer = NULL;

  CHECK_CONSISTENCY;
  if (priority == 0L)
    {
    PRINT_DEB ("AllocAligned: Executing avail flush", 0L);
    if (FindTask (NULL) == PageHandlerTask)
      {
      PRINT_DEB ("Pagehandler is executing avail flush !!!", 0L);
      tmp_buffer = DoOrigAllocMem (2 * size, flags);
      }
    else
      tmp_buffer = AllocMem (2 * size, flags);

    if (tmp_buffer != NULL)
      {
      FreeMem (tmp_buffer, 2 * size);
      tmp_buffer = AllocAligned (size, flags, alignment, 1L);
      }
    }
  Permit ();
  return (tmp_buffer);
  }
else
  {
  ULONG start_alloc;
  struct MemChunk *new_chunk,
                  *tmp_chunk;
  ULONG chunk_size;

  tmp_chunk = *chosen_mc;
  /* Found a fitting chunk. Do the allocation */
  if (flags & MEMF_REVERSE)
    {
    start_alloc = ALIGN_DOWN ((ULONG)tmp_chunk + tmp_chunk->mc_Bytes, alignment)
                  - size;
    }
  else
    start_alloc = ALIGN_UP ((ULONG)tmp_chunk, alignment);

  chunk_size = tmp_chunk->mc_Bytes - size;     /* size of remaining mem */

  if ((ULONG)tmp_chunk < start_alloc)
    {
    /* Make this chunk smaller */
    tmp_chunk->mc_Bytes = start_alloc - (ULONG)tmp_chunk;
    chunk_size -= tmp_chunk->mc_Bytes;
    chosen_mc = &(tmp_chunk->mc_Next);
    }

  if (chunk_size > 0)
    {
    new_chunk = (struct MemChunk*)(start_alloc + size);
    new_chunk->mc_Next = tmp_chunk->mc_Next;
    new_chunk->mc_Bytes = chunk_size;
    *chosen_mc = new_chunk;
    }
  else
    *chosen_mc = tmp_chunk->mc_Next;

  mh->mh_Free -= size;

  CHECK_CONSISTENCY;
  Permit ();
  return ((void*)start_alloc);
  }
#else
  APTR res = NULL;
  bug("[VMM-Handler] %s(%u, %08x)\n", __func__, size, flags);
  Forbid ();
  /* Loop over MemHeader structures */
  ForeachNode(&SysBase->MemList, mh)
  {
      bug("[VMM-Handler] %s: mh @ 0x%p (%08x)\n", __func__, mh, mh->mh_Attributes);
        /*
         * Check for the right requirements and enough free memory.
         * The requirements are OK if there's no bit in the
         * 'attributes' that isn't set in the 'mh->mh_Attributes'.
         */
        if ((mh->mh_Attributes & flags) != (flags & (MEMF_CHIP | MEMF_FAST | MEMF_PUBLIC)))
        {
          bug("[VMM-Handler] %s: not suitable\n", __func__);
          continue;
        }

        if (mh->mh_Free < size)
        {
          bug("[VMM-Handler] %s: too small\n", __func__);
          continue;
        }

        if (IsManagedMem(mh))
        {
            struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
            bug("[VMM-Handler] %s: managed mem\n", __func__);
            if (mhe->mhe_AllocAligned)
            {
                res = mhe->mhe_AllocAligned(mhe, size, alignment, &flags);
                bug("[VMM-Handler] %s: mhe_AllocAligned returned 0x%p\n", __func__, res);
            }
        }
        else
        {
            bug("[VMM-Handler] %s: unmanaged mem\n", __func__);
            res = Allocate(mh, size + alignment - 1);
            if (res)
            {
              IPTR alignptr = ((IPTR) res + alignment - 1) & (IPTR) -alignment;
              bug("[VMM-Handler] %s: orig 0x%p, alligned 0x%p\n", __func__, res, alignptr);
              FreeMem(res, size + alignment - 1);
              res = AllocAbs(size, (APTR) alignptr);
              if (res && (flags & MEMF_CLEAR))
                memset((APTR)alignptr, 0, size);
            }
        }
        if (res)
            break;
  }
  Permit ();
  bug("[VMM-Handler] %s: returning 0x%p\n", __func__, res);
  return ((void*)res);
#endif
}

/***********************************************************************/

#define isspace(c) (((c)==' ') || ((c) == '\t') || ((c) == '\n'))

void GetNthString (char *from, char *to, int n)

{
/* Extracts the nth string in the line "from" and copies it to "to".
 * If the nth string does not exist 'to' is set to an empty string.
 */
char *tmp1, *tmp2;

tmp1 = from;
tmp2 = to;
while (*tmp1 != 0 && isspace (*tmp1)) tmp1++;    /* skip leading whitespace */

while (--n > 0)
  {
  while (*tmp1 != 0 && !isspace (*tmp1)) tmp1++; /* skip word */
  while (*tmp1 != 0 &&  isspace (*tmp1)) tmp1++; /* skip blanks */
  }

while (*tmp1 != 0 && !isspace(*tmp1)) *tmp2++ = *tmp1++;
*tmp2 = 0;
}

/***********************************************************************/

void StrToHex (char *string, ULONG *val)

{
char *tmp;

tmp = string;
*val = 0;
while (*tmp != 0 && isspace(*tmp))
  tmp++;

while (*tmp != 0 && !isspace (*tmp))
  {
  *val = (*val << 4);

  if (*tmp >= '0' && *tmp <= '9')
    *val += *tmp - '0';
  else if (*tmp >= 'a' && *tmp <= 'f')
    *val += *tmp - 'a' + 10;
  else if (*tmp >= 'A' && *tmp <= 'F')
    *val += *tmp - 'F' + 10;
  else
    {
    return;
    }

  tmp++; 
 }
}

/***********************************************************************/

void EmptyPageCollector (void)

/* Routine for a task that is created after a specified number of FreeMems on
 * virtual memory. It traverses the memory list for virtual memory and
 * frees the pages and frames of unused pages.
 */
{
struct VMMsg *FreeMsg;
UWORD ReplySignal;
struct MemChunk *chunk;

PRINT_DEB ("EmptyPageCollector running", 0L);

if ((FreeMsg = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) == NULL)
  return;

ReplySignal = AllocSignal (-1L);        /* Guaranteed to succeed */
FreeMsg->VMSender = FindTask (NULL);
FreeMsg->VMCommand = VMCMD_FreePages;
FreeMsg->ReplySignal = ReplySignal;

PRINT_DEB ("Obtaining VM sema", 0L);
OBTAIN_VM_SEMA;
PRINT_DEB ("VM sema obtained", 0L);

for (chunk = VirtMem->mh_First; chunk != NULL; chunk = chunk->mc_Next)
  {
  if ((IPTR)chunk + chunk->mc_Bytes > ALIGN_UP ((IPTR)(chunk + 1) + PAGESIZE, PAGESIZE))
    {
    /* There is at least one page contained in this are */
    FreeMsg->FreeAddress = (IPTR)(chunk + 1);
    FreeMsg->FreeSize = chunk->mc_Bytes - sizeof (struct MemChunk);
    PutMsg (PageHandlerPort, (struct Message*) FreeMsg);
    Wait (1L << ReplySignal);
    }
  }

PRINT_DEB ("Releasing VM sema", 0L);
RELEASE_VM_SEMA;
FreeMem (FreeMsg, sizeof (struct VMMsg));
FreeSignal ((LONG)ReplySignal);

VMFreeCounter = COLLECT_INTERVAL;
PRINT_DEB ("EmptyPageCollector terminating", 0L);
}
