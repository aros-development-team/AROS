/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy memory.
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH3I(void, CopyMem,

/*  SYNOPSIS */
	AROS_LHA(CONST_APTR,  source, A0),
	AROS_LHA(APTR,  dest,   A1),
	AROS_LHA(ULONG, size,   D0),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE *src,*dst;
    ULONG mis,low,high;

    /*
	I try to fall back to copying LONGs if possible. To do this I copy
	the misaligned leading bytes of the source first. I use sizeof(LONG)
	instead of LONGALIGN because it is sometimes faster.
    */
    
    if (!size) return;
    
    src = (UBYTE *)source;
    dst = (UBYTE *)dest;
     
#if 0 /* stegerg: this is the wrong way round??? */
    mis =(IPTR)src&(sizeof(LONG)-1);
#else
    mis = (sizeof(LONG) - 1) - (((IPTR)src - 1) & (sizeof(LONG) - 1));
#endif
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
    if(!((IPTR)dst&(AROS_LONGALIGN-1)))
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
    AROS_LIBFUNC_EXIT
} /* CopyMem */

