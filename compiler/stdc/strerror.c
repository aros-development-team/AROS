/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strerror().
*/

#include "__stdc_intbase.h"

#include <proto/dos.h>
#include <clib/macros.h>
#include <stdlib.h>
#include <errno.h>

static const char * _errstrings[];

/*****************************************************************************

    NAME
#include <string.h>

	char * strerror (

    SYNOPSIS
	int n)

    FUNCTION
	Returns a readable string for an error number in errno.

    INPUTS
	n - The contents of errno or a #define from errno.h

    RESULT
	A string describing the error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        __stdc_strerror()

    INTERNALS

******************************************************************************/

/*****************************************************************************

    NAME */
#include <string.h>

	char * __stdc_strerror (

/*  SYNOPSIS */
	int n)

/*  FUNCTION
	Returns a readable string for an error number in errno.

    INPUTS
	n - The contents of errno or a #define from errno.h

    RESULT
	A string describing the error.

    NOTES
        This functions only handles the error codes needed by C99 and the ones
        used in stdc.library. This function is aliased as strerror() in
        libstdc.a
        Other libraries may override this function by providing this function
        also in their libxxx.a file. They can internally call __stdc_strerror
        to get the strings for the errors handled by this function.

    EXAMPLE

    BUGS

    SEE ALSO
        strerror()

    INTERNALS

******************************************************************************/
{
    if (n > MAX_ERRNO)
    {
        struct StdCIntBase *StdCBase =
            (struct StdCIntBase *)__aros_getbase_StdCBase();

        if (StdCBase->fault_buf == NULL)
            /* This is not freed anywhere, will be cleaned when
               libbase is expunged
            */
            StdCBase->fault_buf = malloc(100);

	Fault(n - MAX_ERRNO, NULL, StdCBase->fault_buf, 100);

	return StdCBase->fault_buf;
    }
    else
    {
        char *s;

        s = (char *)_errstrings[MIN(n, __STDC_ELAST+1)];

        if (s == NULL)
            s = (char *)"Errno out of range";

        return s;
    }
} /* strerror */


static const char * _errstrings[__STDC_ELAST+2] =
{
    /* 0	       */	"No error",
    /* NA    	       */	NULL,
    /* ENOENT	       */	"No such file or directory",
    /* NA   	       */	NULL,
    /* EINTR	       */	"Interrupted system call",
    /* NA 	       */	NULL,
    /* NA   	       */	NULL,
    /* NA   	       */	NULL,
    /* ENOEXEC	       */	"Exec format error",
    /* NA   	       */	NULL,
    /* NA    	       */	NULL,
    /* NA     	       */	NULL,
    /* ENOMEM	       */	"Out of memory",
    /* EACCES	       */	"Permission denied",
    /* NA 	       */	NULL,
    /* NA	       */	NULL,
    /* EBUSY	       */	"Device or resource busy",
    /* EEXIST	       */	"File exists",
    /* EXDEV	       */	"Cross-device link",
    /* NA    	       */	NULL,
    /* ENOTDIR	       */	"Not a directory",
    /* NA    	       */	NULL,
    /* EINVAL	       */	"Invalid argument",
    /* NA    	       */	NULL,
    /* NA    	       */	NULL,
    /* NA    	       */	NULL,
    /* NA     	       */	NULL,
    /* NA   	       */	NULL,
    /* NA    	       */	NULL,
    /* NA    	       */	NULL,
    /* NA   	       */	NULL,
    /* NA    	       */	NULL,
    /* NA   	       */	NULL,
    /* EDOM	       */	"Numerical argument out of domain",
    /* ERANGE	       */	"Math result not representable",
    /* NA    	       */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* ENOBUFS	       */	"No buffer space available",
    /* NA     	       */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA	       */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA   	       */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA              */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA    	       */	NULL,
    /* NA    	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA    	       */	NULL,
    /* NA    	       */	NULL,
    /* NA              */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA   	       */	NULL,
    /* NA    	       */	NULL,
    /* NA              */	NULL,
    /* EILSEQ	       */	"Illegal byte sequence",
    /* Too high        */	NULL,
};
