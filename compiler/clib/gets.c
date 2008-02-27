/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function gets().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__open.h"
#include "__errno.h"

#include <stdio.h>
#undef gets

/*****************************************************************************

    NAME */
#include <stdio.h>

	char * gets (

/*  SYNOPSIS */
	char * buffer)

/*  FUNCTION
	Read one line of characters from the standard input stream into
        the buffer. Reading will stop, when a newline ('\n') is encountered,
        EOF or when the buffer is full. If a newline is read, then it is
	put into the buffer. The last character in the buffer is always
	'\0' (Therefore at most size-1 characters can be read in one go).

    INPUTS
	buffer - Write characters into this buffer

    RESULT
	buffer or NULL in case of an error or EOF.

    NOTES

    EXAMPLE

    BUGS
        Never use this function. gets() does not know how large the buffer
        is and will continue to store characters past the end of the buffer
        if it has not encountered a newline or EOF yet. Use fgets() instead.

    SEE ALSO
	fgets()

    INTERNALS

******************************************************************************/
{
    return fgets(buffer, BUFSIZ, stdin);
} /* gets */

