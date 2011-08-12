/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strerror().
*/

#include "__arosc_privdata.h"

#include <proto/dos.h>
#include <errno.h>
#include <stdio.h>

extern const char * _errstrings[];

/*****************************************************************************

    NAME */
#include <string.h>

	char * strerror (

/*  SYNOPSIS */
	int n)

/*  FUNCTION
	Returns a readable string for an error number in errno.

    INPUTS
	n - The contents of errno or a #define from errno.h

    RESULT
	A string describing the error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (n > MAX_ERRNO)
    {
        #define buf (__get_arosc_privdata()->acpd_fault_buf)

	Fault(n - MAX_ERRNO, NULL, buf, sizeof(buf));

	return buf;
    }

    return (char *)_errstrings[n];
} /* strerror */

