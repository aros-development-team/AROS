/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian word (16bit) from a file
    Lang: german
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <exec/types.h>

	BOOL ReadWord (

/*  SYNOPSIS */
	BPTR	fh,
	UWORD * dataptr)

/*  FUNCTION
	Reads one big endian 16bit value from a file.

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
	Open(), Close(), ReadByte(), ReadLong(), ReadDouble(),
	ReadString(), WriteByte(), WriteWord(), WriteLong(), WriteDouble(),
	WriteString()

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    LONG value, c;

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

    value = FGetC (fh);

    if (value == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    *dataptr = (c << 8) + value;
#else /* Little endian */
    *dataptr = (value << 8) + c;
#endif

    return TRUE;
} /* ReadByte */

