/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian word (16bit) from a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <clib/alib_protos.h>

	BOOL WriteWord (

/*  SYNOPSIS */
	BPTR  fh,
	UWORD data)

/*  FUNCTION
	Writes one big endian 16bit value to a file.

    INPUTS
	fh - Write to this file
	data - Data to be written

    RESULT
	The function returns TRUE on success and FALSE otherwise.
	See IoErr() for the reason in case of an error.

    NOTES
	This function writes big endian values to a file even on little
	endian machines.

    EXAMPLE

    BUGS

    SEE ALSO
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(), ReadDouble(),
	ReadString(), WriteWord(), WriteLong(), WriteDouble(),
	WriteString()

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    if (FPutC (fh, data >> 8) == EOF)
	return FALSE;

    return (FPutC (fh, data & 0xFF) != EOF);
} /* WriteWord */

