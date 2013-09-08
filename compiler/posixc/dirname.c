/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function dirname().
*/

#include "__upath.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <libgen.h>

	char *dirname(

/*  SYNOPSIS */
	char *filename)

/*  FUNCTION
	Returns the string up to the latest '/'.
	
    INPUTS
	filename - Path which should be split

    RESULT
	Directory part of the path.
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	basename()

    INTERNALS

******************************************************************************/
{
    char *uname;
    char *pos;

    if (!filename || *filename == '\0')
    {
	D(bug("dirname()=.\n")); 
        return ".";
    }

    uname = (char *)__path_a2u(filename);

    pos = uname;

    if (pos[0] == '/' && pos[1] == '\0')
    {
	D(bug("dirname(/)=/\n"));
        return uname;
    }

    D(bug("dirname(%s)=", filename));

    pos = uname + strlen(uname);
    while (pos[-1] == '/')
    {
        --pos;
	pos[0] = '\0';
    }
    while (--pos > uname)
    {
        if (pos[0] == '/')
        {
            pos[0] = '\0';
            break;
        }
    }

    if (pos == uname)
	uname = ".";

    D(bug("%s\n", uname));
    return uname;
}

