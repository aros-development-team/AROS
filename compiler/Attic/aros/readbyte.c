/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian byte from a file
    Lang: english
*/
#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <proto/alib.h>

	BOOL ReadByte (

/*  SYNOPSIS */
	BPTR	fh,
	UBYTE * dataptr)

/*  FUNCTION
	Reads one big endian 8bit value from a file.

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
	Open(), Close(), ReadWord(), ReadLong(), ReadFloat(),
	ReadDouble(), ReadString(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    LONG value;

    value = FGetC (fh);

    if (value != EOF)
    {
	*dataptr = value;
    }

    return (value != EOF);
} /* ReadByte */

