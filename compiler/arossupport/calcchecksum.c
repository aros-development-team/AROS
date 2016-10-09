/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Calculate a checksum for a given area of memory
    Lang: english
*/

#include <aros/system.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	ULONG CalcChecksum (

/*  SYNOPSIS */
	APTR  memory,
	ULONG size)

/*  FUNCTION
	Calculate a checksum for a given area of memory.

    INPUTS
	memory - Start here
	size - This many bytes. Must be a multiple of sizeof(ULONG)

    RESULT
	The checksum for the memory. If you store the checksum somewhere
	in the area and run CalcChecksum() again, the result will be 0.
	To achieve this, you must set the place, where the checksum will
	be placed later, to 0 before you call the function.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE
	ULONG mem[512];

	mem[0] = 0; // Store checksum here
	mem[0] = CalcChecksum (mem, sizeof (mem));

	if (CalcChecksum (mem, sizeof (mem))
	    printf ("Something is wrong !!\n");
	else
	    printf ("Data is unchanged.\n");

    BUGS

    SEE ALSO
	exec.library/SumKickData(), exec.library/SumLibrary()

    INTERNALS
	The function uses the DOS way: sum all the ULONGs and return the
	negative result. Not very safe, but then it's quite fast :)

    HISTORY
	26-10-95    digulla created

******************************************************************************/
{
    ULONG   sum;
    ULONG * lptr;

    for (sum=0, lptr=(ULONG *)memory; size>0; size-=sizeof(ULONG))
	sum += *lptr ++;

    return -sum;
} /* CalcChecksum */

