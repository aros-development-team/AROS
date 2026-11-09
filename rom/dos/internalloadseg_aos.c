/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>

#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosasl.h>
#include <dos/doshunks.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <aros/macros.h>

#include "dos_intern.h"
#include "internalloadseg.h"
#include "include/loadseg.h"

#define GETHUNKPTR(x) ((UBYTE*)(BADDR(hunktab[x]) + sizeof(BPTR)))

#ifndef __mc68000
/* The relocations applied so far, for GSLI_68KRELOCS. An allocation failure
   drops them and marks the list lost. */
static BOOL acc_room(APTR *p, ULONG *cap, ULONG used, ULONG need, ULONG elem)
{
    ULONG want = used + need, ncap = *cap ? *cap : 16;
    APTR n;

    if (want < used || want > ~(ULONG)0 / elem)
        return FALSE;
    while (ncap < want)
        ncap = ncap > want / 2 ? want : ncap * 2;
    if (ncap == *cap)
        return TRUE;
    if (!(n = AllocVec(ncap * elem, MEMF_ANY)))
        return FALSE;
    if (*p)
    {
        CopyMem(*p, n, used * elem);
        FreeVec(*p);
    }
    *p = n;
    *cap = ncap;
    return TRUE;
}

static void note_group(struct HunkRelocList *acc, ULONG hunk, ULONG target, ULONG kind, ULONG count)
{
    struct HunkRelocGroup *g;

    if (acc->lost)
        return;
    if (!acc_room((APTR *)&acc->groups, &acc->gcap, acc->ngroups, 1, sizeof(*acc->groups)) ||
        !acc_room((APTR *)&acc->offsets, &acc->ocap, acc->noffsets, count, sizeof(*acc->offsets)))
    {
        acc->lost = TRUE;
        return;
    }
    g = &acc->groups[acc->ngroups++];
    g->hrg_Hunk    = hunk;
    g->hrg_Target  = target;
    g->hrg_Kind    = kind;
    g->hrg_Count   = 0;
    g->hrg_Offsets = NULL;
}

static void note_offset(struct HunkRelocList *acc, ULONG offset)
{
    if (acc->lost)
        return;
    acc->offsets[acc->noffsets++] = offset;
    acc->groups[acc->ngroups - 1].hrg_Count++;
}
#define NOTE_GROUP(h, t, k, n) do { if (DOSBase) note_group(&relocacc, (h), (t), (k), (n)); } while (0)
#define NOTE_OFFSET(o)          do { if (DOSBase) note_offset(&relocacc, (o)); } while (0)
#else
#define NOTE_GROUP(h, t, k, n)  do { } while (0)
#define NOTE_OFFSET(o)          do { } while (0)
#endif

#define LOADSEG_HUNK_BUFFER 2048

struct SRBuffer
{
    APTR buffer;
    ULONG offset;
    ULONG avail;
};

static int read_block_buffered(BPTR file, APTR buffer, ULONG size, SIPTR * funcarray, struct SRBuffer * srb, struct DosLibrary * DOSBase)
{
    if (!srb) {
        return ilsRead(file, buffer, size) == size ? 0 : 1;
    }
    if (!srb->buffer) {
        srb->buffer = ilsAllocMem(LOADSEG_HUNK_BUFFER, 0);
        if (!srb->buffer)
            return 1;
        srb->avail = 0;
    }
    while (size) {
        ULONG tocopy;
        if (srb->avail == 0) {
            srb->offset = 0;
            srb->avail = ilsRead(file, srb->buffer, LOADSEG_HUNK_BUFFER);
        }
        if (srb->avail == 0)
            return 1;
        tocopy = srb->avail > size ? size : srb->avail;
        CopyMem(srb->buffer + srb->offset, buffer, tocopy);
        size -= tocopy;
        buffer += tocopy;
        srb->avail -= tocopy;
        srb->offset += tocopy;
    }
    return 0;
}

/* Seek forward by count ULONGs.
 * Returns 0 on success, 1 on failure.
 */
static int seek_forward(BPTR fd, ULONG count, SIPTR *funcarray, struct SRBuffer *srb, struct DosLibrary *DOSBase)
{
    int err = 0;
    ULONG tmp;

    /* For AOS compatibility, we can't use DOS/Seek() here,
     * as AOS callers to InternalLoadSeg will not pass
     * in a Seek element of the funcarray, and the read
     * callback of funcarray may be for reading in-memory
     * instead of pointing to DOS/Read.
     *
     * Luckily, reading HUNKs is linear, so we can just
     * read ahead.
     */
    while (count && !(err = read_block_buffered(fd, &tmp, sizeof(tmp), funcarray, srb, DOSBase)))
        count--;

    return err;
}

/* What every read of the loader needs besides the destination */
struct HunkReader
{
    BPTR               fh;
    SIPTR             *funcarray;
    struct SRBuffer   *srb;
    struct DosLibrary *DOSBase;
};

/* Read size bytes of the file. Returns 0 on success, 1 on failure. */
static int hunk_read(struct HunkReader *hr, APTR buffer, ULONG size)
{
    return read_block_buffered(hr->fh, buffer, size, hr->funcarray, hr->srb, hr->DOSBase);
}

/* Read one longword, as stored in the file (big endian) */
static int hunk_long(struct HunkReader *hr, ULONG *value)
{
    return hunk_read(hr, value, sizeof(ULONG));
}

/* Read one word, as stored in the file (big endian) */
static int hunk_word(struct HunkReader *hr, UWORD *value)
{
    return hunk_read(hr, value, sizeof(UWORD));
}

/* Skip count longwords of the file */
static int hunk_skip(struct HunkReader *hr, ULONG count)
{
    return seek_forward(hr->fh, count, hr->funcarray, hr->srb, hr->DOSBase);
}

BPTR InternalLoadSeg_AOS(BPTR fh,
                         BPTR table,
                         SIPTR * funcarray,
                         LONG  * stacksize,
                         struct DosLibrary * DOSBase)
{
  #define ERROR(a)    { *error=a; goto end; }


  BPTR *hunktab = BADDR(table);
  BPTR firsthunk = BNULL, prevhunk = BNULL;
  ULONG lcount, hunktype, first, last, curhunk, lasthunk, numhunks;
  UWORD wcount;
  LONG t;
  UBYTE name_buf[255];
  register int i;
  BPTR last_p = 0;
  UBYTE *overlaytable = NULL;
  ULONG tmp, req;
  SIPTR dummy;
  struct SRBuffer srbbuf, *srb;
  struct HunkReader hr;
#ifndef __mc68000
  struct HunkRelocList relocacc = { NULL, 0, 0, NULL, 0, 0, FALSE };
#endif
#if DEBUG
  static STRPTR segtypes[] = { "CODE", "DATA", "BSS", };
#endif

  SIPTR *error = &dummy;
  
  if (DOSBase) {
    struct Process *me = (struct Process *)FindTask(NULL);
    ASSERT_VALID_PROCESS(me);

    error =&me->pr_Result2;
  }
  srb = &srbbuf;
  srb->buffer = NULL;
  hr.fh = fh;
  hr.funcarray = funcarray;
  hr.srb = srb;
  hr.DOSBase = DOSBase;

  curhunk = lasthunk = 0; /* keep GCC quiet */
  /* start point is HUNK_HEADER + 4 */
  while (1)
  {
    if (hunk_long(&hr, &lcount))
      goto end;
    if (lcount == 0L)
      break;
    lcount = AROS_BE2LONG(lcount);
    lcount *= 4;
    if (hunk_read(&hr, name_buf, lcount))
      goto end;
    D(bug("\tlibname: \"%.*s\"\n", lcount, name_buf));
  }
  if (hunk_long(&hr, &numhunks))
    goto end;

  numhunks = AROS_BE2LONG(numhunks);

  D(bug("\tHunk count: %ld\n", numhunks));

  if (!hunktab) {
    hunktab = ilsAllocVec(sizeof(BPTR) * (numhunks + 1 + 1), MEMF_CLEAR);
    if (hunktab == NULL)
      ERROR(ERROR_NO_FREE_STORE);
  }

  if (hunk_long(&hr, &first))
    goto end;

  first = AROS_BE2LONG(first);

  D(bug("\tFirst hunk: %ld\n", first));
  curhunk = first;
  if (hunk_long(&hr, &last))
    goto end;

  last = AROS_BE2LONG(last);

  D(bug("\tLast hunk: %ld\n", last));
        
  for (i = first; i <= numhunks; i++) {
    UBYTE *hunkptr;
    ULONG hunksize;

    if (i > last) {
      hunktab[i] = BNULL;
      continue;
    }

    if (hunk_long(&hr, &lcount))
      goto end;

    lcount = AROS_BE2LONG(lcount);
    tmp = lcount & (HUNKF_FAST | HUNKF_CHIP);
    lcount &= 0xFFFFFF;
    D(bug("\tHunk %d size: 0x%06lx bytes in ", i, lcount*4));
    req = MEMF_CLEAR | MEMF_PUBLIC;
    if (tmp == (HUNKF_FAST | HUNKF_CHIP)) {
      if (hunk_long(&hr, &req))
        goto end;
      req = AROS_BE2LONG(req);
      D(bug("FLAGS=%08x", req));
    } else if (tmp == HUNKF_FAST) {
      D(bug("FAST"));
      req |= MEMF_FAST;
    } else if (tmp == HUNKF_CHIP) {
      D(bug("CHIP"));
      req |= MEMF_CHIP;
    }
    D(bug(" memory"));

    /*
     * Allocate enough space to store the code, the hunk length, and the
     * next hunk pointer.
     * NB - On 64bit systems, Hunk segments are ALWAYS
     * allocated in the 32bit address space using MEMF_31BIT.
     * This is to make sure that relocatable symbols can correctly be
     * resolved, allowing hunk files to be LoadSeged on 64bit
     * architectures, and so that traditional AmigaOS Font, and
     * Keymap files can be correctly processed and used.
     */
    hunksize = lcount * 4 + sizeof(ULONG) + sizeof(BPTR);
    hunkptr = ilsAllocVec(hunksize, req | MEMF_31BIT);
    if (!hunkptr)
      ERROR(ERROR_NO_FREE_STORE);
    hunktab[i] = MKBADDR(hunkptr);
    D(bug(" @%p\n", hunkptr));
    if (!firsthunk)
      firsthunk = hunktab[i];
      /* Link hunks
         if this is not the first hunk that is loaded, then connect
         it to the previous one (pointer to the field where the
         pointer to the next hunk is located)
       */
    if (prevhunk)
      ((BPTR *)(BADDR(prevhunk)))[0] = hunktab[i];
    prevhunk = hunktab[i];
  }

  while(!hunk_long(&hr, &hunktype))
  {
    hunktype = AROS_BE2LONG(hunktype);
    D(bug("Hunk Type: %d\n", hunktype & 0xFFFFFF));

    switch(hunktype & 0xFFFFFF)
    {
      case HUNK_SYMBOL:
        /* The SYMBOL_HUNK looks like this:
             ---------------------
             | n = size of       |  This
             |   symbol in longs |  may
             |-------------------|  be
             | n longwords = name|  repeated
             | of symbol         |  any
             |-------------------|  number
             | value (1 long)    |  of times
             --------------------|
             | 0 = end of HUNK_  |
             |       SYMBOL      |
             --------------------   */

        D(bug("HUNK_SYMBOL (skipping)\n"));
          while(!hunk_long(&hr, &lcount) && lcount)
          {
            lcount = AROS_BE2LONG(lcount) ;

            if (hunk_skip(&hr, lcount+1))
              goto end;
          }
      break;

      case HUNK_UNIT:

        if (hunk_long(&hr, &lcount))
          goto end;

        lcount = AROS_BE2LONG(lcount) ;

        lcount *= 4;
        if (hunk_read(&hr, name_buf, lcount))
          goto end;
        D(bug("HUNK_UNIT: \"%.*s\"\n", lcount, name_buf));
        break;

      case HUNK_CODE:
      case HUNK_DATA:
      case HUNK_BSS:

        if (hunk_long(&hr, &lcount))
          goto end;

        lcount = AROS_BE2LONG(lcount);

        D(bug("HUNK_%s(%d): Length: 0x%06lx bytes in ",
          segtypes[(hunktype & 0xFFFFFF)-HUNK_CODE], curhunk, lcount*4));

        if ((hunktype & 0xFFFFFF) != HUNK_BSS && lcount)
        {
          if (hunk_read(&hr, GETHUNKPTR(curhunk), lcount*4))
            goto end;

        }

        lasthunk = curhunk;
        ++curhunk;
      break;

      case HUNK_RELOC32:
        D(bug("HUNK_RELOC32:\n"));
        while (1)
        {
          ULONG *addr, val;
          ULONG offset;

          if (hunk_long(&hr, &lcount))
            goto end;
          if (lcount == 0L)
            break;

          lcount = AROS_BE2LONG(lcount);

          i = lcount;
          if (hunk_long(&hr, &lcount))
            goto end;

          lcount = AROS_BE2LONG(lcount);

          D(bug("\tHunk #%ld:\n", lcount));
          NOTE_GROUP(lasthunk, lcount, HUNKRELOC_ABS, i);
          while (i > 0)
          {
            if (hunk_read(&hr, &offset, sizeof(offset)))
              goto end;

            offset = AROS_BE2LONG(offset);

            //D(bug("\t\t0x%06lx\n", offset));
            addr = (ULONG *)(GETHUNKPTR(lasthunk) + offset);
            NOTE_OFFSET(offset);

            val = AROS_BE2LONG(*addr) + (IPTR)GETHUNKPTR(lcount);

            D(bug("\t %08x:    %08x ->", addr, *addr));
            *addr = (ULONG)AROS_LONG2BE(val);
            D(bug(" %08x (%08x)\n", *addr, val));

            --i;
          }
        }
      break;

      case HUNK_RELRELOC32:
      case HUNK_ABSRELOC16:
      case HUNK_DREL32: /* For compatibility with V37 */
      case HUNK_RELOC32SHORT:
        {
          /* These hunks share one layout: groups of 16-bit count, 16-bit
             target hunk and 16-bit offsets. They differ in what is added
             to the longword at each offset. */
          ULONG reltype = hunktype & 0xFFFFFF;
          ULONG Wordcount = 0;

          D(if (reltype == HUNK_RELRELOC32) bug("HUNK_RELRELOC32:\n"));
          while (1)
          {
            ULONG *addr, val;
            ULONG offset;

            Wordcount++;
            if (hunk_word(&hr, &wcount))
              goto end;
            if (wcount == 0L)
              break;

            wcount = AROS_BE2WORD(wcount);

            i = wcount;
            Wordcount++;
            if (hunk_word(&hr, &wcount))
              goto end;

            wcount = AROS_BE2WORD(wcount);

            lcount = wcount;
            D(if (reltype == HUNK_RELRELOC32)
                bug("\tHunk #%ld:\n", lcount);
              else
                bug("\tHunk #%ld @%p: %ld relocations\n", lcount, GETHUNKPTR(lasthunk), i));
            NOTE_GROUP(lasthunk, lcount,
                       (reltype == HUNK_RELRELOC32) ? HUNKRELOC_BASE :
                       (reltype == HUNK_ABSRELOC16) ? HUNKRELOC_REL : HUNKRELOC_ABS, i);
            while (i > 0)
            {
              /* read a 16bit number (2 bytes) */
              Wordcount++;
              if (hunk_word(&hr, &wcount))
                goto end;

              offset = AROS_BE2WORD(wcount);

              D(if (reltype != HUNK_RELRELOC32)
                  bug("\t\t0x%06lx += 0x%lx\n", offset, GETHUNKPTR(lcount)));
              addr = (ULONG *)(GETHUNKPTR(lasthunk) + offset);
              NOTE_OFFSET(offset);

              /* The address of the target hunk ... */
              val = AROS_BE2LONG(*addr) + (IPTR)GETHUNKPTR(lcount);
              if (reltype == HUNK_RELRELOC32)
              {
                /* ... relative to this hunk */
                val -= (IPTR)GETHUNKPTR(lasthunk);
              }
              else if (reltype == HUNK_ABSRELOC16)
              {
                /* ... relative to the relocated longword */
                val -= (IPTR)GETHUNKPTR(lasthunk) + offset;
              }
              *addr = (ULONG)AROS_LONG2BE(val);

              --i;
            } /* while (i > 0)*/
          } /* while (1) */

          /* if the amount of words read was odd, then skip the following
           16-bit word   */
          if (0x1 == (Wordcount & 0x1)) {
            hunk_word(&hr, &wcount);
          }
        }
      break;

      case HUNK_END:
      {
        D(bug("HUNK_END\n"));
        /* DOSBase == NULL: Called from RDB filesystem loader which does not
         * know filesystem's original size. Exit if last HUNK_END. This can't
         * be done normally because it would break overlayed executables.
         */
        if (!DOSBase && curhunk > last)
            goto done;
      }
      break;

      case HUNK_RELOC16:
        bug("HUNK_RELOC16 not implemented\n");
        ERROR(ERROR_BAD_HUNK);

      case HUNK_RELOC8:
        bug("HUNK_RELOC8 not implemented\n");
        ERROR(ERROR_BAD_HUNK);

      case HUNK_NAME:
        bug("HUNK_NAME not implemented\n");
        ERROR(ERROR_BAD_HUNK);

      case HUNK_EXT:
        bug("HUNK_EXT not implemented\n");
        ERROR(ERROR_BAD_HUNK);

      case HUNK_DEBUG:
        if (hunk_long(&hr, &lcount))
          goto end;

        lcount = AROS_BE2LONG(lcount);

        D(bug("HUNK_DEBUG (%x Bytes)\n",lcount));
        if (hunk_skip(&hr, lcount))
          goto end;
        break;

      case HUNK_OVERLAY:
      {
        D(bug("HUNK_OVERLAY:\n"));
        if (table) /* overlay inside overlay? */
          ERROR(ERROR_BAD_HUNK);
        if (hunk_long(&hr, &lcount))
          goto end;
        lcount = AROS_BE2LONG(lcount);
        D(bug("Overlay table size: %d\n", lcount));

        /* See above for MEMF_31BIT explanation */
        lcount = lcount * 4 + sizeof(ULONG) + sizeof(ULONG);
        overlaytable = ilsAllocVec(lcount, MEMF_CLEAR | MEMF_31BIT);
        if (overlaytable == NULL)
          ERROR(ERROR_NO_FREE_STORE);
        if (hunk_read(&hr, overlaytable, lcount - sizeof(ULONG)))
            goto end;
        goto done;
      }

      case HUNK_BREAK:
        D(bug("HUNK_BREAK\n"));
        if (!table)
          ERROR(ERROR_BAD_HUNK);
        goto done;

      default:
        if (hunktype & HUNKF_ADVISORY) {
          D(bug("Unknown hunk 0x%06x with advisory flag skipped\n", hunktype & 0xFFFFFF));
          if (hunk_long(&hr, &lcount))
            goto end;
          lcount = AROS_BE2LONG(lcount);
          if (hunk_skip(&hr, lcount * 4))
            goto end;
        } else {
          bug("Hunk type 0x%06x not implemented\n", hunktype & 0xFFFFFF);
          ERROR(ERROR_BAD_HUNK);
        }
    } /* switch */
  } /* while */

done:
  if (hunktab)
  {
    ULONG hunksize;

#if defined(DOCACHECLEAR)
    /* Clear caches */
    for (t = first; t < numhunks && t <= last; t++)
    {
        hunksize = *((ULONG*)BADDR(hunktab[t]) - 1);
        if (hunksize)
          ils_ClearCache(BADDR(hunktab[t]), hunksize, CACRF_ClearI | CACRF_ClearD);
    }
#endif

    if (table)
      return firsthunk;

    hunksize = *((ULONG*)BADDR(hunktab[0]) - 1);
    if (last > first && hunksize >= 32 / 4)
    {
      /* NOTE: HUNK_OVERLAY is not required for overlay mode. */
      ULONG *h = (ULONG*)(BADDR(hunktab[first]));

      if (h[2] == 0x0000abcd)
      {
#if __WORDSIZE != 32
        D(bug("overlay not supported!\n"));
        ERROR(ERROR_BAD_HUNK);
#else
        /* overlay executable */
        h[3] = (ULONG)fh;
        h[4] = (ULONG)overlaytable;
        h[5] = (ULONG)MKBADDR(hunktab);
        D(bug("overlay loaded!\n"));
        return (BPTR)(-(LONG)MKBADDR(h));
#endif
      }
    }

    if (overlaytable) {
      ilsFreeVec(overlaytable);
      ERROR(ERROR_BAD_HUNK);
    }

    last_p = firsthunk;

#ifndef __mc68000
    register_hunk(fh, firsthunk, NULL, &relocacc, DOSBase);
#else
    register_hunk(fh, firsthunk, NULL, NULL, DOSBase);
#endif

    ilsFreeVec(hunktab);
    hunktab = NULL;
  }

end:
#ifndef __mc68000
  FreeVec(relocacc.groups);
  FreeVec(relocacc.offsets);
#endif
  if (hunktab != NULL)
  {
    for (t = 0 /* first */; t < numhunks /* last */; t++)
      ilsFreeVec(BADDR(hunktab[t]));
    ilsFreeVec(hunktab);
  }
  ilsFreeMem(srb->buffer, LOADSEG_HUNK_BUFFER);
  return last_p;
} /* InternalLoadSeg */

#ifdef __mc68000
static AROS_UFH4(LONG, ReadFunc,
        AROS_UFHA(BPTR, file,   D1),
        AROS_UFHA(APTR, buffer, D2),
        AROS_UFHA(LONG, length, D3),
        AROS_UFHA(struct DosLibrary *, DOSBase, A6)
)
{
    AROS_USERFUNC_INIT

    return FRead(file, buffer, 1, length);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(APTR, AllocFunc,
        AROS_UFHA(ULONG, length, D0),
        AROS_UFHA(ULONG, flags,  D1),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    return AllocMem(length, flags);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, FreeFunc,
        AROS_UFHA(APTR, buffer, A1),
        AROS_UFHA(ULONG, length, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    FreeMem(buffer, length);

    AROS_USERFUNC_EXIT
}

AROS_UFH4(BPTR, LoadSeg_Overlay,
    AROS_UFHA(UBYTE*, name, D1),
    AROS_UFHA(BPTR, hunktable, D2),
    AROS_UFHA(BPTR, fh, D3),
    AROS_UFHA(struct DosLibrary *, DosBase, A6))
{
    AROS_USERFUNC_INIT

    void (*FunctionArray[3])();
    ULONG hunktype;

    FunctionArray[0] = (APTR)ReadFunc;
    FunctionArray[1] = (APTR)AllocFunc;
    FunctionArray[2] = (APTR)FreeFunc;

    D(bug("LoadSeg_Overlay. table=%x fh=%x\n", hunktable, fh));
    if (read_block_buffered(fh, &hunktype, sizeof(hunktype), (SIPTR*)FunctionArray, NULL, DosBase))
        return BNULL;
    hunktype = AROS_BE2LONG(hunktype);
    if (hunktype != HUNK_HEADER)
        return BNULL;
    return InternalLoadSeg_AOS(fh, hunktable, (SIPTR*)FunctionArray, NULL, DosBase);

    AROS_USERFUNC_EXIT
}

#endif
