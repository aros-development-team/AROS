/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function rename().
*/

#include <proto/dos.h>
#include <stdlib.h>
#include <string.h>
#include "__errno.h"
#include "__upath.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int rename (

/*  SYNOPSIS */
	const char * oldpath,
	const char * newpath)

/*  FUNCTION
	Renames a file or directory.

    INPUTS
	oldpath - Complete path to existing file or directory.
	newpath - Complete path to the new file or directory.

    RESULT
	0 on success and -1 on error. In case of an error, errno is set.
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
          char *aoldpath = strdup(__path_u2a(oldpath));
    const char *anewpath =        __path_u2a(newpath);

    if (!Rename (aoldpath, anewpath))
    {
	int ioerr = IoErr();
	errno = IoErr2errno (ioerr);
	D(bug("rename(%s, %s) errno=%d, IoErr=%d\n", aoldpath, anewpath,
	      errno, ioerr));
	free(aoldpath);
	return -1;
    }

    free(aoldpath);
    return 0;

} /* rename */

