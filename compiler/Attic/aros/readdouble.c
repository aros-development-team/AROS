/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian double floating point (64bit) from a file
    Lang: english
*/
#include <clib/dos_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <exec/types.h>

	BOOL ReadFloat (

/*  SYNOPSIS */
	BPTR	 fh,
	DOUBLE * dataptr)

/*  FUNCTION
	Reads one big endian 64bit double precision floating point value
	from a file.

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
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(),
	ReadFloat(), ReadString(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY:
	14.09.93    ada created

******************************************************************************/
{
    ULONG   value[2];
    ULONG * lptr;
    LONG  c;

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[0] = c << 24;
#else /* Little endian */
    value[1] = c;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[0] |= c << 16;
#else /* Little endian */
    value[1] |= c << 8;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[0] |= c << 8;
#else /* Little endian */
    value[1] |= c << 16;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[0] |= c;
#else /* Little endian */
    value[1] |= c << 24;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[1] = c << 24;
#else /* Little endian */
    value[0] = c;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[1] |= c << 16;
#else /* Little endian */
    value[0] |= c << 8;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[1] |= c << 8;
#else /* Little endian */
    value[0] |= c << 16;
#endif

    c = FGetC (fh);

    if (c == EOF)
	return FALSE;

#if AROS_BIG_ENDIAN
    value[1] |= c;
#else /* Little endian */
    value[0] |= c << 24;
#endif

    lptr = (ULONG *)dataptr;
    lptr[0] = value[0];
    lptr[1] = value[1];

    return TRUE;
} /* ReadDouble */

