/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: loader for AmigaDOS hunk files.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosasl.h>
#include <dos/doshunks.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include "dos_intern.h"
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

extern struct DosLibrary * DOSBase;

struct hunk
{
  ULONG size;
  UBYTE *memory;
};

static int read_block(BPTR file, APTR buffer, ULONG size)
{
  LONG subsize;
  UBYTE *buf=(UBYTE *)buffer;

  while(size) {
    subsize=Read(file,buf,size);
    if(subsize==0) {
      ((struct Process *)FindTask(NULL))->pr_Result2=ERROR_BAD_HUNK;
      return 1;
    }
    if(subsize<0)
      return 1;
    buf+=subsize;
    size-=subsize;
  }
  return 0;
}

BPTR LoadSeg_AOS(BPTR file)
{
  struct hunk *hunktab = NULL;
  ULONG hunktype, count, first, last, offset, curhunk, numhunks;
  LONG t;
  UBYTE name_buf[255];
  register i;
  BPTR last_p = 0;
  static STRPTR segtypes[] = { "CODE", "DATA", "BSS", };

#define ERROR(a)    { *error=a; goto end; }
  LONG *error=&((struct Process *)FindTask(NULL))->pr_Result2;

  if (Seek(file, 0, OFFSET_BEGINNING) < 0)
    goto end;
  while(!read_block(file, &hunktype, sizeof(hunktype))) {
    switch(hunktype & 0xFFFFFF) {
      ULONG tmp, req;

    case HUNK_SYMBOL:
      D(bug("HUNK_SYMBOL (skipping)\n"));
      while(!read_block(file, &count, sizeof(count)) && count) {
	if (Seek(file, (count+1)*4, OFFSET_CURRENT) < 0)
	  goto end;
      }
      break;
    case HUNK_UNIT:
      if (read_block(file, &count, sizeof(count)))
	goto end;
      count /= 4;
      if (read_block(file, name_buf, count))
	goto end;
      D(bug("HUNK_UNIT: \"%.*s\"\n", count, name_buf));
      break;
    case HUNK_HEADER:
      D(bug("HUNK_HEADER:\n"));
      while (1) {
	if (read_block(file, &count, sizeof(count)))
	  goto end;
	if (count == 0L)
	  break;
	count *= 4;
	if (read_block(file, name_buf, count))
	  goto end;
	D(bug("\tlibname: \"%.*s\"\n", count, name_buf));
      }
      if (read_block(file, &numhunks, sizeof(numhunks)))
	goto end;
      D(bug("\tHunk count: %ld\n", numhunks));
      hunktab = (struct hunk *)AllocVec(sizeof(struct hunk) * numhunks,
					MEMF_CLEAR);
      if (hunktab == NULL)
	ERROR(ERROR_NO_FREE_STORE);
      if (read_block(file, &first, sizeof(first)))
	goto end;
      D(bug("\tFirst hunk: %ld\n", first));
      curhunk = 0 /* first */;
      if (read_block(file, &last, sizeof(last)))
	goto end;
      D(bug("\tLast hunk: %ld\n", last));
      for (i = 0 /* first */; i < numhunks /* last */; i++) {
	if (read_block(file, &count, sizeof(count)))
	  goto end;
	tmp = count & 0xFF000000;
	count &= 0xFFFFFF;
	D(bug("\tHunk %d size: 0x%06lx bytes in ", i, count*4));
	req = MEMF_CLEAR;
	switch(tmp) {
	case HUNKF_FAST:
	  D(bug("FAST"));
	  req |= MEMF_FAST;
	  break;
	case HUNKF_CHIP:
	  D(bug("CHIP"));
	  req |= MEMF_CHIP;
	  break;
	case HUNKF_ADVISORY:
	  D(bug("ADVISORY"));
	  if (read_block(file, &req, sizeof(req)))
	    goto end;
	  break;
	default:
	  D(bug("ANY"));
	  req |= MEMF_ANY;
	  break;
	}
	D(bug(" memory\n"));
	hunktab[i].size = count * 4;
	hunktab[i].memory = (UBYTE *)AllocVec(hunktab[i].size + sizeof(BPTR),
					      req);
	if (hunktab[i].memory == NULL)
	  ERROR(ERROR_NO_FREE_STORE);
	if (i > 0)
	  *((BPTR *)(hunktab[i].memory)) = MKBADDR(hunktab[i-1].memory);
	hunktab[i].memory += sizeof(BPTR);
      }
      break;
    case HUNK_CODE:
    case HUNK_DATA:
    case HUNK_BSS:
      if (read_block(file, &count, sizeof(count)))
	goto end;
      D(bug("HUNK_%s(%d): Length: 0x%06lx bytes in ",
	    segtypes[(hunktype & 0xFFFFFF)-HUNK_CODE], curhunk, count*4));
      switch(hunktype & 0xFF000000) {
      case HUNKF_FAST:
	D(bug("FAST"));
	req = MEMF_FAST;
	break;
      case HUNKF_CHIP:
	D(bug("CHIP"));
	req = MEMF_CHIP;
	break;
      case HUNKF_ADVISORY:
	D(bug("ADVISORY"));
	if (read_block(file, &req, sizeof(req)))
	  goto end;
	break;
      default:
	D(bug("ANY"));
	req = MEMF_ANY;
	break;
      }
      D(bug(" memory\n"));
      if ((hunktype & 0xFFFFFF) != HUNK_BSS && count)
	if (read_block(file, hunktab[curhunk].memory, count*4))
	  goto end;
      break;
    case HUNK_RELOC32:
      D(bug("HUNK_RELOC32:\n"));
      while (1) {
	ULONG *addr;

	if (read_block(file, &count, sizeof(count)))
	  goto end;
	if (count == 0L)
	  break;
	i = count;
	if (read_block(file, &count, sizeof(count)))
	  goto end;
	D(bug("\tHunk #%ld:\n", count));
	while (i > 0) {
	  if (read_block(file, &offset, sizeof(offset)))
	    goto end;
	  D(bug("\t\t0x%06lx\n", offset));
	  addr = (ULONG *)(hunktab[curhunk].memory + offset);
	  *addr += (ULONG)(hunktab[count].memory);
	  --i;
	}
      }
      break;
    case HUNK_END:
      D(bug("HUNK_END\n"));
      ++curhunk;
      break;
    case HUNK_RELOC16:
      D(bug("HUNK_RELOC16 not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    case HUNK_RELOC8:
      D(bug("HUNK_RELOC8 not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    case HUNK_NAME:
      D(bug("HUNK_NAME not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    case HUNK_EXT:
      D(bug("HUNK_EXT not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    case HUNK_DEBUG:
      if (read_block(file, &count, sizeof(count)))
	goto end;
      D(bug("HUNK_DEBUG (%x Bytes)\n",count));
      if (Seek(file, count * 4, OFFSET_CURRENT ) < 0 )
	goto end;
      break;
    case HUNK_OVERLAY:
      D(bug("HUNK_OVERLAY not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    case HUNK_BREAK:
      D(bug("HUNK_BREAK not implemented\n"));
      ERROR(ERROR_BAD_HUNK);
    default:
      D(bug("Hunk type 0x%06lx not implemented\n", hunktype & 0xFFFFFF));
      ERROR(ERROR_BAD_HUNK);
    }
  }
  /* Clear caches */
  for (t=numhunks-1 /* last */; t >= (LONG)0 /*first */; t--) {
    if (hunktab[t].size) {
      CacheClearE(hunktab[t].memory, hunktab[t].size, CACRF_ClearI|CACRF_ClearD);
      ((BPTR *)hunktab[t].memory)[-1] = last_p;
      last_p = MKBADDR((BPTR *)hunktab[t].memory-1);
    }
  }
  FreeVec(hunktab);
  hunktab = NULL;
end:
  if (hunktab != NULL) {
    for (t = 0 /* first */; t < numhunks /* last */; t++)
      if (hunktab[t].memory != NULL)
	FreeVec(hunktab[t].memory - sizeof(BPTR));
    FreeVec(hunktab);
  }
  return last_p;
}
