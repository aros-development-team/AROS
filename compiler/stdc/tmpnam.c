/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function tmpnam().
    This function is based on the public domain libnix code
*/

#include <stdlib.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/libcall.h>

#include "__stdcio_intbase.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	char *tmpnam(

/*  SYNOPSIS */
	char *s)

/*  FUNCTION
        The tmpnam function generates a string that is a valid file name and
        that is not the same as the name of an existing file. The function
        is potentially capable of generating TMP_MAX different strings, but
        any or all of them may already be in use by existing files and thus
        not be suitable return values.

    INPUTS
        Pointer to a string of at least L_tmpnam characters.

    RESULT
        The resulting file name is returned in the input string pointer
        or a pointer to an internal buffer if NULL was passed to the function.
        If file name generation failed a NULL is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        tmpfile()

    INTERNALS

******************************************************************************/
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();
    BPTR filelock;
    if (s == NULL)
        s = StdCIOBase->tmpnambuffer;

    do /* generate a filename that doesn't exist */
    {
        sprintf(s,"T:tempfile_stdc_%p_%lu",FindTask(NULL),StdCIOBase->filecount++);
        filelock = Lock(s,ACCESS_WRITE);
        if(filelock != 0)
            UnLock(filelock);
    } while(filelock!=0 || IoErr()==ERROR_OBJECT_IN_USE);

    return s;
}
