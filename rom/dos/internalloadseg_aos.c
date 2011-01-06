/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#define DEBUG 0
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosasl.h>
#include <dos/doshunks.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/macros.h>

#include "dos_intern.h"
#include "internalloadseg.h"

static int read_block(BPTR file, APTR buffer, ULONG size, SIPTR * funcarray, struct DosLibrary * DOSBase);

#include <proto/dos.h>



BPTR InternalLoadSeg_AOS(BPTR fh,
                         BPTR table,
                         SIPTR * funcarray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase)
{
  #define ERROR(a)    { *error=a; goto end; }


  BPTR *hunktab = BADDR(table);
  BPTR firsthunk = BNULL, prevhunk = BNULL;
  ULONG hunktype, count, first, last, offset, curhunk, numhunks;
  LONG t;
  UBYTE name_buf[255];
  register int i;
  BPTR last_p = 0;
  UBYTE *overlaytable = NULL;
  ULONG tmp, req;
#if DEBUG
  static STRPTR segtypes[] = { "CODE", "DATA", "BSS", };
#endif


  LONG *error=&((struct Process *)FindTask(NULL))->pr_Result2;

  curhunk = 0; /* keep GCC quiet */
  /* start point is HUNK_HEADER + 4 */
  while (1)
  {
    if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
      goto end;
    if (count == 0L)
      break;
    count = AROS_BE2LONG(count);
    count *= 4;
    if (read_block(fh, name_buf, count, funcarray, DOSBase))
      goto end;
    D(bug("\tlibname: \"%.*s\"\n", count, name_buf));
  }
  if (read_block(fh, &numhunks, sizeof(numhunks), funcarray, DOSBase))
    goto end;

  numhunks = AROS_BE2LONG(numhunks);

  D(bug("\tHunk count: %ld\n", numhunks));

  if (!hunktab) {
    hunktab = loadseg_alloc((SIPTR*)funcarray[1], sizeof(BPTR) * (numhunks + 1 + 1), MEMF_CLEAR);
    if (hunktab == NULL)
      ERROR(ERROR_NO_FREE_STORE);
  }

  if (read_block(fh, &first, sizeof(first), funcarray, DOSBase))
    goto end;

  first = AROS_BE2LONG(first);

  D(bug("\tFirst hunk: %ld\n", first));
  curhunk = first;
  if (read_block(fh, &last, sizeof(last), funcarray, DOSBase))
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

    if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
      goto end;

    count = AROS_BE2LONG(count);

    tmp = count & 0xFF000000;
    count &= 0xFFFFFF;
    D(bug("\tHunk %d size: 0x%06lx bytes in ", i, count*4));
    req = MEMF_CLEAR;

    switch(tmp)
    {
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
      if (read_block(fh, &req, sizeof(req), funcarray, DOSBase))
        goto end;
      req = AROS_BE2LONG(req);
      break;

      default:
      D(bug("ANY"));
      req |= MEMF_ANY;
      break;
    }

    D(bug(" memory"));
    /* we need space for the code, the length of this hunk and
       for a pointer to the next hunk
     */
    hunksize = count * 4 + sizeof(ULONG) + sizeof(BPTR);
    hunkptr = loadseg_alloc((SIPTR*)funcarray[1], hunksize, req);
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

  while(!read_block(fh, &hunktype, sizeof(hunktype), funcarray, DOSBase))
  {
    hunktype = AROS_BE2LONG(hunktype);

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
          while(!read_block(fh, &count, sizeof(count), funcarray, DOSBase) && count)
          {
            count = AROS_BE2LONG(count) ;

            if (Seek(fh, (count+1)*4, OFFSET_CURRENT) < 0)
              goto end;
          }
      break;

      case HUNK_UNIT:

        if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
          goto end;

        count = AROS_BE2LONG(count) ;

        count *= 4;
        if (read_block(fh, name_buf, count, funcarray, DOSBase))
          goto end;
        D(bug("HUNK_UNIT: \"%.*s\"\n", count, name_buf));
        break;

      case HUNK_CODE:
      case HUNK_DATA:
      case HUNK_BSS:
        if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
          goto end;

          count = AROS_BE2LONG(count);

          D(bug("HUNK_%s(%d): Length: 0x%06lx bytes in ",
          segtypes[(hunktype & 0xFFFFFF)-HUNK_CODE], curhunk, count*4));

        switch(hunktype & 0xFF000000)
        {
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
            if (read_block(fh, &req, sizeof(req), funcarray, DOSBase))
              goto end;

            req = AROS_BE2LONG(req);

          break;

          default:
            D(bug("ANY"));
            req = MEMF_ANY;
          break;
        }

        D(bug(" memory\n"));
        if ((hunktype & 0xFFFFFF) != HUNK_BSS && count)
	{
          if (read_block(fh, BADDR(hunktab[curhunk] + 1), count*4, funcarray, DOSBase))
            goto end;

    	}
      break;

      case HUNK_RELOC32:
        D(bug("HUNK_RELOC32:\n"));
        while (1)
        {
          ULONG *addr;

          if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
            goto end;
          if (count == 0L)
            break;

          count = AROS_BE2LONG(count);

          i = count;
          if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
            goto end;

          count = AROS_BE2LONG(count);

          D(bug("\tHunk #%ld:\n", count));
          while (i > 0)
          {
            if (read_block(fh, &offset, sizeof(offset), funcarray, DOSBase))
              goto end;

            offset = AROS_BE2LONG(offset);

            //D(bug("\t\t0x%06lx\n", offset));
            addr = (ULONG *)(BADDR(hunktab[curhunk] + 1) + offset);

            *addr = AROS_BE2LONG(*addr) + (ULONG)(BADDR(hunktab[count] + 1));

            --i;
          }
        }
      break;

      case HUNK_DREL32: /* For compatibility with V37 */
      case HUNK_RELOC32SHORT:
        {
          ULONG Wordcount = 0;

          while (1)
          {
            ULONG *addr;
    	    UWORD word;
	    
	    Wordcount++;
	    
            if (read_block(fh, &word, sizeof(word), funcarray, DOSBase))
              goto end;
            if (word == 0L)
              break;

            word = AROS_BE2LONG(word);

            i = word;
	    Wordcount++;
            if (read_block(fh, &word, sizeof(word), funcarray, DOSBase))
              goto end;

            word = AROS_BE2WORD(word);

    	    count = word;
            D(bug("\tHunk #%ld:\n", count));
            while (i > 0)
            {
              Wordcount++;
              /* read a 16bit number (2 bytes) */
              if (read_block(fh, &word, sizeof(word), funcarray, DOSBase))
                goto end;

              word = AROS_BE2WORD(word);

              offset = word;
              /* offset now contains the byte offset in it`s 16 highest bits.
                 These 16 highest bits have to become the 16 lowest bits so
                 we get the word we need.  */
              //(ULONG)offset >>= ((sizeof(offset)-2)*8);
              //D(bug("\t\t0x%06lx\n", offset));
              addr = (ULONG *)(BADDR(hunktab[curhunk] + 1) + offset);

              *addr = AROS_BE2LONG(*addr) + (ULONG)(BADDR(hunktab[count] + 1));

              --i;
            } /* while (i > 0)*/
          } /* while (1) */

          /* if the amount of words read was odd, then skip the following
           16-bit word   */
          if (0x1 == (Wordcount & 0x1)) {
            UWORD word;
            read_block(fh, &word, sizeof(word), funcarray, DOSBase);
          }
        }
      break;

      case HUNK_END:
      {
        D(bug("HUNK_END\n"));
        ++curhunk;
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
        if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
          goto end;

        count = AROS_BE2LONG(count);

        D(bug("HUNK_DEBUG (%x Bytes)\n",count));
        if (Seek(fh, count * 4, OFFSET_CURRENT ) < 0 )
          goto end;
        break;

      case HUNK_OVERLAY:
      {
        D(bug("HUNK_OVERLAY:\n"));
        if (table) /* overlay inside overlay? */
          ERROR(ERROR_BAD_HUNK);
        if (read_block(fh, &count, sizeof(count), funcarray, DOSBase))
          goto end;
        count = AROS_BE2LONG(count);
        D(bug("Overlay table size: %d\n", count));
        count = count * 4 + sizeof(ULONG) + sizeof(ULONG);
        overlaytable = loadseg_alloc((SIPTR*)funcarray[1], count, req);
        if (overlaytable == NULL)
          ERROR(ERROR_NO_FREE_STORE);
        if (read_block(fh, overlaytable, count - sizeof(ULONG), funcarray, DOSBase))
            goto end;
        goto done;
      }

      case HUNK_BREAK:
        D(bug("HUNK_BREAK\n"));
        if (!table)
          ERROR(ERROR_BAD_HUNK);
        goto done;

      default:
        bug("Hunk type 0x%06lx not implemented\n", hunktype & 0xFFFFFF);
        ERROR(ERROR_BAD_HUNK);
    } /* switch */
  } /* while */
done:
  if (hunktab)
  {
    ULONG hunksize;
    /* Clear caches */
    for (t = first; t < numhunks && t <= last; t++)
    {
      hunksize = *((ULONG*)BADDR(hunktab[t]) - 1);
      if (hunksize)
      {
        bug("%p %d\n", BADDR(hunktab[t]), hunksize);
        CacheClearE(BADDR(hunktab[t]), hunksize, CACRF_ClearI | CACRF_ClearD);
      }
    }

    if (table)
      return firsthunk;

    hunksize = *((ULONG*)BADDR(hunktab[t]) - 1);
    if (last > first && hunksize >= 32 / 4) {
      /* NOTE: HUNK_OVERLAY is not required for overlay mode. */
      ULONG *h = (ULONG*)(BADDR(hunktab[first]));
      if (h[2] == 0x0000abcd) {
        /* overlay executable */
        h[3] = (ULONG)fh;
        h[4] = (ULONG)overlaytable;
        h[5] = MKBADDR(hunktab);
        D(bug("overlay loaded!\n"));
        return (BPTR)(-(LONG)MKBADDR(h));
      }
    }

    if (overlaytable) {
      loadseg_free((SIPTR*)funcarray[2], overlaytable);
      ERROR(ERROR_BAD_HUNK);
    }

    last_p = firsthunk;

    loadseg_free((SIPTR*)funcarray[2], hunktab);
    hunktab = NULL;
  }

end:
  if (hunktab != NULL)
  {
    for (t = 0 /* first */; t < numhunks /* last */; t++)
      loadseg_free((SIPTR*)funcarray[2], BADDR(hunktab[t]));
    loadseg_free((SIPTR*)funcarray[2], hunktab);
  }
  return firsthunk;
} /* InternalLoadSeg */


static int read_block(BPTR file, APTR buffer, ULONG size, SIPTR * funcarray, struct DosLibrary * DOSBase)
{
  LONG subsize;
  UBYTE *buf=(UBYTE *)buffer;

  while(size)
  {
    subsize = loadseg_read((SIPTR*)funcarray[0], file, buf, size, DOSBase);
    if(subsize==0)
    {
      ((struct Process *)FindTask(NULL))->pr_Result2=ERROR_BAD_HUNK;
      return 1;
    }

    if(subsize<0)
      return 1;
    buf  +=subsize;
    size -=subsize;
  }
  return 0;
}

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)

AROS_UFP4(LONG, ReadFunc,
	AROS_UFPA(BPTR, file,   D1),
	AROS_UFPA(APTR, buffer, D2),
	AROS_UFPA(LONG, length, D3),
	AROS_UFPA(struct DosLibrary *, DOSBase, A6));

AROS_UFH4(BPTR, LoadSeg_Overlay,
    AROS_UFHA(UBYTE*, name, D1),
    AROS_UFHA(BPTR, hunktable, D2),
    AROS_UFHA(BPTR, fh, D3),
    AROS_UFHA(struct DosLibrary *, DosBase, A6))
{
    AROS_USERFUNC_INIT

    void (*FunctionArray[3])();
    ULONG hunktype;

    FunctionArray[0] = (void(*))ReadFunc;
    FunctionArray[1] = __AROS_GETVECADDR(SysBase, 33); /* AllocMem() */
    FunctionArray[2] = __AROS_GETVECADDR(SysBase, 35); /* FreeMem() */

    D(bug("LoadSeg_Overlay. table=%x fh=%x\n", hunktable, fh));
    if (read_block(fh, &hunktype, sizeof(hunktype), (SIPTR*)FunctionArray, DosBase))
    	return BNULL;
    hunktype = AROS_BE2LONG(hunktype);
    if (hunktype != HUNK_HEADER)
    	return BNULL;
    return InternalLoadSeg_AOS(fh, hunktable, (SIPTR*)FunctionArray, NULL, DosBase);

    AROS_USERFUNC_EXIT
}

#endif
