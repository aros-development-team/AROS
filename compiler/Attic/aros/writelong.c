/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian long (32bit) from a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <exec/types.h>

	BOOL WriteLong (

/*  SYNOPSIS */
	BPTR  fh,
	ULONG data)

/*  FUNCTION
	Writes one big endian 32bit value to a file.

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
	ReadString(), WriteByte(), WriteWord(), WriteDouble(),
	WriteString()

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
#if BIG_ENDIAN
    return
    (
	FPutC (fh, data >> 24) != EOF
	&& FPutC (fh, data >> 16) != EOF
	&& FPutC (fh, data >> 8) != EOF
	&& FPutC (fh, data) != EOF
    );
#else
    return
    (
	FPutC (fh, data) != EOF
	&& FPutC (fh, data >> 8) != EOF
	&& FPutC (fh, data >> 16) != EOF
	&& FPutC (fh, data >> 24) != EOF
    );
#endif
} /* WriteWord */

