/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian string to a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <clib/alib_protos.h>

	BOOL WriteString (

/*  SYNOPSIS */
	BPTR   fh,
	STRPTR data)

/*  FUNCTION
	Writes one big endian string to a file.

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
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(), ReadFloat(),
	ReadDouble(), ReadString(), WriteWord(), WriteLong(), WriteFloat(),
	WriteDouble()

    HISTORY
	27.11.96    ada created

******************************************************************************/
{
    do
    {
	if (FPutC (fh, *data) == EOF)
	    return FALSE;
    } while (*data ++);

    return TRUE;
} /* WriteString */

