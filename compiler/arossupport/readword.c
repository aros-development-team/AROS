/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a big endian word (16bit) from a streamhook
    Lang: english
*/

#include <proto/dos.h>
#include <stdio.h>
#include <assert.h>

/******************************************************************************

    NAME */
#include <aros/bigendianio.h>
#include <proto/alib.h>

	BOOL ReadWord (

/*  SYNOPSIS */
	struct Hook * hook,
	UWORD	    * dataptr,
	void	    * stream)

/*  FUNCTION
	Reads one big endian 16bit value from a streamhook.

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

******************************************************************************/
{
    LONG value, c;

    assert (hook);
    assert (stream);
    assert (dataptr);
    assert (*dataptr);

    /* High byte */
    c = CallHook (hook, stream, BEIO_READ);

    if (c == EOF)
	return FALSE;

    /* Low byte */
    value = CallHook (hook, stream, BEIO_READ);

    if (value == EOF)
	return FALSE;

    *dataptr = (c << 8) + value;

    return TRUE;
} /* ReadWord */

