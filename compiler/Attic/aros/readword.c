/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian word (16bit) from a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <clib/alib_protos.h>

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
	Open(), Close(), ReadByte(), ReadLong(), ReadFloat(),
	ReadDouble(), ReadString(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    LONG value, c;

    c = FGetC (fh); /* High byte */

    if (c == EOF)
	return FALSE;

    value = FGetC (fh); /* Low Byte */

    if (value == EOF)
	return FALSE;

    *dataptr = (c << 8) + value;

    return TRUE;
} /* ReadWord */

