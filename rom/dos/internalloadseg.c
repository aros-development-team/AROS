/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: DOS function InternalLoadSeg()
*/

#include <aros/debug.h>

#include <proto/dos.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include "dos_intern.h"
#include <stddef.h>
#include "internalloadseg.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH4(BPTR, InternalLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR       , fh           , D0),
        AROS_LHA(BPTR       , table        , A0),
        AROS_LHA(LONG_FUNC *, funcarray    , A1),
        AROS_LHA(LONG *     , stack        , A2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 126, Dos)

/*  FUNCTION
        Loads from fh.
        Functionarray is a pointer to an array of functions. See below.

        This function really only tries to load the different file
        formats aos, elf and aout.

    INPUTS
        fh            : Filehandle to load from
        table         : ignored
        funcarray : array of functions to be used for read, seek, alloc and free
           FuncTable[0] -> bytes  = ReadFunc(readhandle, buffer, length), DOSBase
                           D0                D1          A0      D0       A6
           FuncTable[1] -> Memory = AllocFunc(size,flags), ExecBase
                           D0                 D0   D1      A6
           FuncTable[2] -> FreeFunc(memory, size), ExecBase
                                    A1       D0    A6
           FuncTable[3] -> pos    = SeekFunc(readhandle, pos, mode), DOSBase
                           D0                D0          D1   D2
        stack         : pointer to storage (LONG) for stacksize.
                        (currently ignored)

    RESULT
        seglist  - pointer to loaded Seglist or NULL in case of failure.

    NOTES
        FuncTable[3] is not used for Amiga HUNK format files, but is required
                     for ELF.

    EXAMPLE

    BUGS
       Use of table and stack are not implemented, yet!

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    typedef struct _segfunc_t
    {
        ULONG id;
        BPTR (*func)(BPTR, BPTR, SIPTR *, LONG *, struct DosLibrary *);
        D(CONST_STRPTR format;)
    } segfunc_t;

    #define SEGFUNC(id, format) {id, InternalLoadSeg_##format D(, (STRPTR)#format)}
    
    static const segfunc_t funcs[] =
    {
        SEGFUNC(0x7f454c46, ELF),
        SEGFUNC(0x000003f3, AOS)
    };
  
    BPTR segs = 0;

    if (fh)
    {
        UBYTE i;
        const UBYTE num_funcs = sizeof(funcs) / sizeof(funcs[0]);
        ULONG id;
        LONG len;

        len = ilsRead(fh, &id, sizeof(id));
        if (len == sizeof(id)) {
            id = AROS_BE2LONG(id);
            for (i = 0; i < num_funcs; i++) {
                if (funcs[i].id == id) {
                    segs = (*funcs[i].func)(fh, BNULL, (SIPTR *)funcarray,
                        stack, DOSBase);
                    D(bug("[InternalLoadSeg] %s loading %p as an %s object.\n",
                        segs ? "Succeeded" : "FAILED", fh, funcs[i].format));
                    if (segs)
                        return segs;
                    else
                        break;
                }
            }
        }
    }

    /* This routine can be called from partition.library, when
     * DOSBase is NULL, from HDToolbox, on m68k, when adding
     * a new device to HDToolbox.
     *
     * Luckily, we're not trying to read ELF, which has a
     * mess of SetIoErr() calls in it.
     */
    if (DOSBase != NULL)
        SetIoErr(ERROR_NOT_EXECUTABLE);

    return BNULL;
  
    AROS_LIBFUNC_EXIT
} /* InternalLoadSeg */

int read_block(BPTR file, APTR buffer, ULONG size, SIPTR * funcarray, struct DosLibrary * DOSBase)
{
  LONG subsize;
  UBYTE *buf=(UBYTE *)buffer;

  while(size)
  {
    subsize = ilsRead(file, buf, size);
    if(subsize==0)
    {
      if (DOSBase) {
          struct Process *me = (struct Process *)FindTask(NULL);
          ASSERT_VALID_PROCESS(me);
          me->pr_Result2=ERROR_BAD_HUNK;
      }
      return 1;
    }

    if(subsize<0)
      return 1;
    buf  +=subsize;
    size -=subsize;
  }
  return 0;
}

/* These two implement the public seglist layout from dos/bptr.h, which
   everything that builds or walks a seglist by hand depends on. */
_Static_assert(offsetof(struct AROSSegment, seg_Next) == sizeof(BPTR),
               "the seglist link must sit one BPTR into the allocation");
_Static_assert(offsetof(struct AROSSegment, seg_Size) == sizeof(BPTR) - sizeof(ULONG),
               "the allocation size must sit directly in front of the link");

/* size is the payload the caller asks for; the segment header is added here,
   so no caller has to know the layout. Returns the seglist link, which is
   what BADDR() of the resulting BPTR addresses. */
APTR _ilsAllocVec(SIPTR *funcarray, ULONG size, ULONG req)
{
    struct AROSSegment *seg;
    ULONG total;

    if (size > ~(ULONG)0 - (ULONG)AROS_SEG_HEADER)
        return NULL;
    total = AROS_SEG_MEMSIZE4LEN(size);

    seg = ilsAllocMem(total, req);

    D(bug("allocmem %p %d\n", seg, total));
    if (!seg)
        return NULL;

    seg->seg_Size = total;
    return AROS_SEG_TOLINK(seg);
}

void _ilsFreeVec(SIPTR *funcarray, void *buf)
{
    struct AROSSegment *seg;
    ULONG size;

    if (!buf)
        return;

    seg = AROS_SEG_FROMLINK(buf);
    size = seg->seg_Size;
    D(bug("freemem %p %d\n", seg, size));
    /* A size of zero marks a segment that does not own its memory */
    if (!size)
        return;

    ilsFreeMem(seg, size);
}
