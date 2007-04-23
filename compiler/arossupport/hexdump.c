/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prints a hexdump of a memory region
    Lang: english
*/

#include <ctype.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

	void hexdump (

/*  SYNOPSIS */
	const void * data,
	IPTR	     offset,
	ULONG	     count)

/*  FUNCTION
	Prints a hexdump of the data beginning at data. The format
	is liks this:

	xxxxxxxx: dddddddd dddddddd dddddddd dddddddd aaaaaaaaaaaaaaaa

	Where x is the address (8 chars hex), dd is a data byte (2 chars
	hex) and a is the ASCII representation of a data byte or "." if
	the data byte is not printable.

    INPUTS
	data - Start here with the dump
	offset - This offset is used as the address in the output. If
		you give 0L here, then the first address will be
		00000000. If you give (IPTR)data here, then the
		first address will be the memory address of the data.
	count - How many bytes to print.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	05-12-96    digulla created

******************************************************************************/
{
    ULONG t, end;
    int   i;

    end = (count + 15) & -16;

    for (t=0; t<end; t++)
    {
	if ((t&15) == 0)
	    kprintf ("%08lx:", offset+t);

	if ((t&3) == 0)
	    kprintf (" ");

	if (t < count)
	    kprintf ("%02x", ((UBYTE *)data)[t]);
	else
	    kprintf ("  ");

	if ((t&15) == 15)
	{
	    kprintf (" ");

	    for (i=15; i>=0; i--)
	    {
		if (isprint (((UBYTE *)data)[t-i]))
		    kprintf ("%c", ((UBYTE *)data)[t-i]);
		else
		    kprintf (".");
	    }

	    kprintf ("\n");
	}
    }
} /* hexdump */
