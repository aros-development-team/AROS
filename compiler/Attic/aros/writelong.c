/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian long (32bit) from a file
    Lang: english
*/
#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <proto/alib.h>

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

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    UBYTE * ptr;

#if AROS_BIG_ENDIAN
    ptr = (UBYTE *)&data;
#   define NEXT ++
#else
    ptr = ((UBYTE *)&data) + 3;
#   define NEXT --
#endif

#define WRITE_ONE_CHAR			\
    if (FPutC (fh, *ptr NEXT) == EOF)   \
	return FALSE

    WRITE_ONE_CHAR;
    WRITE_ONE_CHAR;
    WRITE_ONE_CHAR;
    WRITE_ONE_CHAR;

    return TRUE;
} /* WriteLong */

