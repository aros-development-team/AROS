/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian long (32bit) from a file
    Lang: german
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <exec/types.h>

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
	Open(), Close(), ReadByte(), ReadWord(), ReadDouble(),
	ReadString(), WriteByte(), WriteWord(), WriteLong(), WriteDouble(),
	WriteString()

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    ULONG value;
    LONG  c;

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value = c << 24;
#else /* Little endian */
    value = c;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value |= c << 16;
#else /* Little endian */
    value |= c << 8;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value |= c << 8;
#else /* Little endian */
    value |= c << 16;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value |= c;
#else /* Little endian */
    value |= c << 24;
#endif

    *dataptr = value;

    return TRUE;
} /* ReadByte */

