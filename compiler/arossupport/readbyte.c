/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a big endian byte from a streamhook
    Lang: english
*/

#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

	BOOL ReadByte (

/*  SYNOPSIS */
	struct Hook * hook,
	UBYTE	    * dataptr,
	void	    * stream)

/*  FUNCTION
	Reads one big endian 8bit value from a streamhook.

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
    LONG value;

    struct BEIOM_Read rd = {BEIO_READ};
    
    value = CallHookA (hook, stream, &rd);

    if (value != EOF)
	*dataptr = value;

    return (value != EOF);
} /* ReadByte */

