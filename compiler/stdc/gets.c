/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function gets().
*/
#include <libraries/stdcio.h>
#include <string.h>

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
	buffer when succesfull. NULL in case of an error or when EOF without any
        characters read. In the latter case buffer array is unchanged.

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
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();
    char *s = buffer;
    int c;

    c = getchar();
    while(c != '\n' && c != EOF)
    {
        *s = c;
        s++;
        c = getchar();
    }

    if (ferror(StdCIOBase->_stdin))
    {
        *s = 0;
        return NULL;
    }
    else if (c == EOF && s == buffer)
    {
        return NULL;
    }
    else
    {
        *s = 0;
        return buffer;
    }
} /* gets */
