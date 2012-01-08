/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function gets().
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"

#include <string.h>
#include <stdio.h>

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
	replaced by '\0'. The last character in the buffer is always '\0'.

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
    char *s = fgets(buffer, BUFSIZ, stdin);
    if (s)
    {
	/* strip trailing \n */
	size_t sl = strlen(s);
	if ( (sl > 0) && (s[sl-1] == '\n') )
	{
	    s[sl-1] = '\0';
	}
    }
    return s;
} /* gets */
