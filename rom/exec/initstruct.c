/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Initialize a structure.
*/

#include <aros/config.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_debug.h"
#include "exec_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH3(void, InitStruct,

/*  SYNOPSIS */
        AROS_LHA(CONST_APTR,  initTable, A1),
        AROS_LHA(APTR,  memory,    A2),
        AROS_LHA(ULONG, size,      D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 13, Exec)

/*  FUNCTION
        Initialize some library base or other structure depending on the
        information in the init table. The init table consists of
        instructions starting with an action byte followed by more
        information. The instruction byte looks like:

        iisscccc where ii is the instruction code:
                          0 - copy following c+1 elements
                          1 - repeat following element c+1 times
                          2 - take next byte as offset, then copy
                          3 - take the next 3 bytes (in the machine's
                              particular byte ordering) as offset, then
                              copy
                       ss is the element size
                          0 - LONGs
                          1 - WORDs
                          2 - BYTEs
                          3 - QUADs
                       cccc is the element count-1

        Instruction bytes must follow the same alignment restrictions as LONGs;
        the following elements are aligned to their particular restrictions.

        A 0 instruction ends the init table.

    INPUTS
        initTable - Pointer to init table.
        memory    - Pointer to uninitialized structure.
        size      - Size of memory area to zero out before decoding or 0
                    for no filling.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG  cnt;
    ULONG offset=0;
    UBYTE *it,*dst,*src;
    int   s,t;
    UWORD n,total;
    IPTR  align;

    /* Size and alignment of an element, indexed by the size field of a command */
    static const UBYTE elemSize[4]  = { sizeof(LONG), sizeof(WORD), sizeof(BYTE), sizeof(QUAD) };
    static const UBYTE elemAlign[4] = { AROS_LONGALIGN, AROS_WORDALIGN, 1, AROS_QUADALIGN };

    DCREATELIBRARY("InitStruct(0x%p, 0x%p, %lu)", initTable, memory, size);

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* On 'real' Amigas, only the lower 16 bits are valid */
    size &= 0xffff;
#endif

    /* Clear Memory area. Use librom's memset() */
    memset(memory, 0x00, size);

    it =(UBYTE *)initTable;
    dst=(UBYTE *)memory;

    /* As long as there's something to do */
    while(*it!=0)
    {
        /* What to do. */
        t=*it>>6&3;

        /* Element size. */
        s=*it>>4&3;

        /* Number of things to do (-1). */
        cnt=*it&15;

        /* Depending on the action there may be more information */
        switch(t)
        {
            case 0:
            case 1:
                /* Skip the action byte */
                it++;
                break;
            case 2:
                /* Skip the action byte, get the offset */
                it++;
                offset=*it++;
                break;
            case 3:
                /*
                    Get 24bit offset. It's the programmer's responsibility
                    to align the action byte with a LONG instruction before
                    this.
                */
#if AROS_BIG_ENDIAN
                offset=*(ULONG *)it&0xffffff;
#else
                offset=it[1] | ((*(UWORD *)&it[2]) << 8);
#endif
                it+=sizeof(LONG);
                break;
        }

        /* Align source and destination pointers */
        align = elemAlign[s] - 1;
        it =(UBYTE *)(((IPTR)it +align)&~align);
        dst=(UBYTE *)(((IPTR)dst+align)&~align);

        /* Element size and number of bytes to write */
        n = elemSize[s];
        total = n * (UWORD)(cnt + 1);

        /* Action is: Add offset then copy */
        if (t >= 2)
            dst=(BYTE *)memory+offset;

        /*
            Action is either: Repeat the next element <cnt> times, which takes
            one element from the table, or: Copy the next <cnt> elements to the
            current location.
        */
        src = it;
        it += (t == 1) ? n : total;

        do
        {
            *dst++ = *src++;

            /* When repeating start with the same element again */
            if (t == 1 && src == it)
                src -= n;
        }while(--total);

        /* Align next instruction byte */
        it=(UBYTE *)(((IPTR)it+AROS_WORDALIGN-1)&~(AROS_WORDALIGN-1));
    }
    AROS_LIBFUNC_EXIT
}

