/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian long (32bit) from a file
    Lang: english
*/
#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <proto/alib.h>

	BOOL ReadLong (

/*  SYNOPSIS */
	BPTR	fh,
	ULONG * dataptr)

/*  FUNCTION
	Reads one big endian 32bit value from a file.

    INPUTS
	fh - Read from this file
	data - Put the data here

    RESULT
	The function returns TRUE on success. On success, the value
	read is written into dataptr. On failure, FALSE is returned and the
	contents of dataptr are not changed.

    NOTES
	This function reads big endian values from a file even on little
	endian machines.

    EXAMPLE

    BUGS

    SEE ALSO
	Open(), Close(), ReadByte(), ReadWord(), ReadFloat(),
	ReadDouble(), ReadString(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    ULONG   value;
    LONG    c;
    UBYTE * ptr;

#if AROS_BIG_ENDIAN
    ptr = (UBYTE *)&value;
#   define NEXT ++
#else
    ptr = ((UBYTE *)&value) + 3;
#   define NEXT --
#endif

#define READ_ONE_BYTE		    \
    if ((c = FGetC (fh)) == EOF)    \
	return FALSE;		    \
				    \
    *ptr NEXT = c

    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;

    *dataptr = value;

    return TRUE;
} /* ReadLong */

