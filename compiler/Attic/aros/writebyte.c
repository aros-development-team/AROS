/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian byte from a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <exec/types.h>

	BOOL WriteByte (

/*  SYNOPSIS */
	BPTR  fh,
	UBYTE data)

/*  FUNCTION
	Writes one big endian 8bit value to a file.

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
    return FPutC (fh, data) != EOF;
} /* WriteByte */

