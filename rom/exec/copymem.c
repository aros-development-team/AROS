/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:07  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/libcall.h>
#include "machine.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH3I(void, CopyMem,

/*  SYNOPSIS */
	__AROS_LA(APTR,  source, A0),
	__AROS_LA(APTR,  dest,   A1),
	__AROS_LA(ULONG, size,   D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 104, Exec)

/*  FUNCTION
	Copy some memory from one destination in memory to another using
	a fast copying method.

    INPUTS
	source - Pointer to source area
	dest   - Pointer to destination
	size   - number of bytes to copy

    RESULT

    NOTES
	The source and destination area are not allowed to overlap.

    EXAMPLE

    BUGS

    SEE ALSO
	CopyMemQuick()

    INTERNALS

    HISTORY
	24-10-95    Created by M. Fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT

    UBYTE *src=(UBYTE *)source,*dst=(UBYTE *)dest;
    ULONG mis,low,high;

    /*
	I try to fall back to copying LONGs if possible. To do this I copy
	the misaligned leading bytes of the source first. I use sizeof(LONG)
	instead of LONGALIGN because it is sometimes faster.
    */
    mis =(ULONG)src&(sizeof(LONG)-1);
    if(mis>size)
        mis=size;
    size-=mis;

    if(mis)
      do
	*dst++=*src++;
      while(--mis);

    /*
	The source has the right alignment now. All I need to do is to
	check if this is true for the destination, too.
    */
    if(!((ULONG)dst&(LONGALIGN-1)))
    {
	/* Yes. I may copy LONGs. */
	LONG *s=(LONG *)src,*d=(LONG *)dst;
	ULONG longs;

	/* How many of them? */
	longs=size/sizeof(LONG);

	/*
	    To minimize the loop overhead I copy more than one (eight) LONG per
	    iteration. Therefore I need to split size into size/8 and the rest.
	*/
	low =longs&7;
	high=longs/8;

	/* Then copy for both parts */
	if(low)
	    do
		*d++=*s++;
	    while(--low);

	if(high)
	    do
	    {
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
		*d++=*s++;
	    }while(--high);

	/* Get the rest. */
	size&=sizeof(LONG)-1;
	src=(UBYTE *)s;
	dst=(UBYTE *)d;
    }

    /* The remaining job can only be done by copying single bytes. */
    low =size&7;
    high=size/8;

    /* Copy for both parts */
    if(low)
      do
	*dst++=*src++;
      while(--low);

    /*
	Partly unrolled copying loop. The predecrement helps the compiler to
	find the best possible loop. The if is necessary to do this.
    */
    if(high)
	do
	{
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	}while(--high);
    __AROS_FUNC_EXIT
} /* CopyMem */

