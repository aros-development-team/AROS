/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"
#include "__stdio.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int getc_unlocked (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION

    INPUTS
	stream - Read from this stream

    RESULT
	The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	getc(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    return getc(stream);
}
