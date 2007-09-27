/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Write a big endian word (16bit) from a streamhook
    Lang: english
*/

#include <proto/dos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

	BOOL WriteWord (

/*  SYNOPSIS */
	struct Hook * hook,
	UWORD	      data,
	void	    * stream)

/*  FUNCTION
	Writes one big endian 16bit value to a streamhook.

    INPUTS
	hook - Write to this streamhook
	data - Data to be written
	stream - Stream passed to streamhook

    RESULT
	The function returns TRUE on success and FALSE otherwise.
	See IoErr() for the reason in case of an error.

    NOTES
	This function writes big endian values to a file even on little
	endian machines.

    EXAMPLE

    BUGS

    SEE ALSO
	ReadByte(), ReadWord(), ReadLong(), ReadFloat(), ReadDouble(),
	ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

    HISTORY

******************************************************************************/
{
    struct BEIOM_Write wr = { BEIO_WRITE, data >>8 };
    if (CallHookA (hook, stream, &wr) == EOF)
	return FALSE;

    wr.Data = data & 0xff;
    return (CallHookA (hook, stream, &wr) != EOF);
} /* WriteWord */

