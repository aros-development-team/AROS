/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize a structure.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/alerts.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH3(void, InitStruct,

/*  SYNOPSIS */
	AROS_LHA(APTR,  initTable, A1),
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
		       cccc is the element count-1

	Instruction bytes must follow the same alignement restrictions as LONGs,
	the following elements are aligned to their particular restrictions.

	A 0 instruction ends the init table.

    INPUTS
	initTable - Pointer to init table.
	memory	  - Pointer to uninitialized structure.
	size	  - Size of memory area to 0 out before decoding or 0
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
    ULONG offset=0,src;
    UBYTE *it,*dst;
    int   s,t;

    /* Clear Memory area fast. Get number of longs and clear them. */
    cnt=size/sizeof(LONG);
    size&=(sizeof(LONG)-1);
    dst=(UBYTE *)memory;
    if(cnt)
	do
	{
	    *(LONG *)dst=0;
	    dst+=sizeof(LONG);
	}
	while(--cnt);

    /* Clear the rest. */
    cnt=size;
    if(cnt)
	do
	    *dst++=0;
	while(--cnt);

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
	switch(s)
	{
	    case 0:
		/* Align pointer to LONG requirements */
		it =(UBYTE *)(((IPTR)it +AROS_LONGALIGN-1)&~(AROS_LONGALIGN-1));
		dst=(UBYTE *)(((IPTR)dst+AROS_LONGALIGN-1)&~(AROS_LONGALIGN-1));
		break;
	    case 1:
		/* Same for WORDs */
		it =(UBYTE *)(((IPTR)it +AROS_WORDALIGN-1)&~(AROS_WORDALIGN-1));
		dst=(UBYTE *)(((IPTR)dst+AROS_WORDALIGN-1)&~(AROS_WORDALIGN-1));
		break;
	    case 2:
		/* Nothing to do for bytes */
		break;
	    default:
		/* And an Alert for nibbles ;-) */
		Alert(ACPU_AddressErr);

		/*
		    Tell the compiler that he doesn't need to
		    care about side effects of Alert()
		*/
		return;
	}

	/* Switch over action */
	switch(t)
	{
	    case 2:
	    case 3:
		/* Action is: Add offset then copy */
		dst=(BYTE *)memory+offset;

		/* Fall through */
	    case 0:
		/* Action is: Copy the next <cnt> elements to the current location */
		switch(s)
		{
		    case 0:
			/* Copy loop */
			do
			{
			    *(LONG *)dst=*(LONG *)it;
			    dst+=sizeof(LONG);
			    it +=sizeof(LONG);
			}while(--cnt>=0);
			break;
		    case 1:
			do
			{
			    *(WORD *)dst=*(WORD *)it;
			    dst+=sizeof(WORD);
			    it +=sizeof(WORD);
			}while(--cnt>=0);
			break;
		    case 2:
			do
			    *dst++=*it++;
			while(--cnt>=0);
			break;
		}
		break;
	    case 1:
		/* Action is: Repeat the next element <cnt> times */
		switch(s)
		{
		    case 0:
			/* Get source */
			src=*(LONG *)it;
			it +=sizeof(LONG);

			/* And write it. */
			do
			{
			    *(LONG *)dst=src;
			    dst+=sizeof(LONG);
			}while(--cnt>=0);
			break;
		    case 1:
			src=*(WORD *)it;
			it +=sizeof(WORD);
			do
			{
			    *(WORD *)dst=src;
			    dst+=sizeof(WORD);
			}while(--cnt>=0);
			break;
		    case 2:
			src=*it++;
			do
			    *dst++=src;
			while(--cnt>=0);
			break;
		}
		break;
	}

	/* Align next instruction byte */
	it=(UBYTE *)(((IPTR)it+AROS_WORDALIGN-1)&~(AROS_WORDALIGN-1));
    }
    AROS_LIBFUNC_EXIT
}

