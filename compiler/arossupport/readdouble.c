/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a big endian double floating point (64bit) from a streamhook
    Lang: english
*/

#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

	BOOL ReadDouble (

/*  SYNOPSIS */
	struct Hook * hook,
	DOUBLE	    * dataptr,
	void	    * stream)

/*  FUNCTION
	Reads one big endian 64bit double precision floating point value
	from a streamhook.

    INPUTS
	hook - Streamhook
	dataptr - Put the data here
	stream - Read from this stream

    RESULT
	The function returns TRUE on success. On success, the value
	read is written into dataptr. On failure, FALSE is returned and the
	contents of dataptr are not changed.

    NOTES
	This function reads big endian values from a streamhook even on
	little endian machines.

    EXAMPLE

    BUGS

    SEE ALSO
	ReadByte(), ReadWord(), ReadLong(), ReadFloat(), ReadDouble(),
	ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    DOUBLE  value;
    UBYTE * ptr;
    LONG    c;
    struct BEIOM_Read rd = {BEIO_READ};
    
    ptr = (UBYTE *)&value;

#if AROS_BIG_ENDIAN
#   define NEXT ++
#else
    ptr += 7;
#   define NEXT --
#endif

#define READ_ONE_BYTE		    \
    if ((c = CallHookA (hook, stream, &rd)) == EOF) \
	return FALSE;		    \
				    \
    *ptr NEXT = c

    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;

    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;
    READ_ONE_BYTE;

    *dataptr = value;

    return TRUE;
} /* ReadDouble */

