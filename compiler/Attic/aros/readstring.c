/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read one C string from a file
    Lang: english
*/
#include <clib/dos_protos.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>

/******************************************************************************

    NAME */
#include <stdio.h>
#include <clib/alib_protos.h>

	BOOL ReadString (

/*  SYNOPSIS */
	BPTR	 fh,
	STRPTR * dataptr)

/*  FUNCTION
	Reads one C string from a file.

    INPUTS
	fh - Read from this file
	dataptr - Put the data here. If you don't need the string anymore,
	    call FreeVec() to free it.

    RESULT
	The function returns TRUE on success. On success, the string
	read is written into dataptr. On failure, FALSE is returned and the
	contents of dataptr are not changed. The string must be freed with
	FreeVec().

    NOTES
	This function reads big endian values from a file even on little
	endian machines.

    EXAMPLE

    BUGS

    SEE ALSO
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(), ReadFloat(),
	ReadDouble(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    STRPTR buffer;
    LONG   size, maxsize;
    LONG   c;

    size = maxsize = 0;
    buffer = NULL;

    for (;;)
    {
	c = FGetC (fh);

	if (c == EOF)
	{
	    FreeVec (buffer);
	    return FALSE;
	}

	if (size == maxsize)
	{
	    STRPTR tmp;

	    tmp = AllocVec (maxsize + 16, MEMF_ANY);

	    if (!tmp)
	    {
		FreeVec (buffer);
		return FALSE;
	    }

	    if (buffer)
	    {
		strcpy (tmp, buffer);
		FreeVec (buffer);
	    }

	    buffer = tmp;
	}

	buffer[size ++] = c;

	if (!c)
	    break;
    }

    *dataptr = buffer;

    return TRUE;
} /* ReadString */

