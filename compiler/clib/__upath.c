/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: utility internal function __path_u2a()
    Lang: english
*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>

static const char *__path_devstuff_u2a(const char *path);
static void  __path_normalstuff_u2a(const char *path, char *buf);

/*****************************************************************************

    NAME */
#include "__upath.h"

	const char *__path_u2a(

/*  SYNOPSIS */
        const char *upath)

/*  FUNCTION
        Translates an unix-style path into an AmigaDOS one.

    INPUTS
        upath - Unix-style path to translate into an AmigaDOS-style equivalent.

    RESULT
        A pointer to a string containing the AmigaDOS-style path, or NULL in 
        case of error.

    NOTES
        This function is for private usage by system code. Do not use it
        elsewhere.

        This function, as it is designed at the moment, is not reentrant.
        Do not use it in multithreaded applications.

    INTERNALS
        This function uses an internal dynamically allocated buffer, whose
        pointer is kept in a static variable. This buffer is then enlarged
        or shrinked as needed. That also means that this function is not
        reentrant, and therefore must not be used in multithreaded code.

        When libpthread will get implemented this function will have to be
        reworked.

    SEE ALSO

******************************************************************************/
{
    const char *newpath;

    /* Does the path really need to be converted?  */
    if (!__doupath)
        return upath;

    /* Safety check.  */
    if (upath == NULL)
    {
        errno = EFAULT;
        return NULL;
    }

    /*
       First see whether the path is in the /dev/#? form and,
       if so, if it's handled internally
    */
    newpath = __path_devstuff_u2a(upath);
    if (!newpath)
    {
        /* Else, convert it normally */
	newpath = realloc_nocopy(__apathbuf, strlen(upath) + 1);

	if (newpath == NULL)
	{
	    errno = ENOMEM;
	    return NULL;
	}

        __apathbuf = (char *)newpath;
	__path_normalstuff_u2a(upath, __apathbuf);
    }

    return newpath;
}

static const char *__path_devstuff_u2a(const char *path)
{
    /*
        Translate the various /dev/#? most used files into the AROS equivalent.
        Use a tree-like handmade search to speed things up
    */

    if (path[0] == '/' && path[1] == 'd' && path[2] == 'e' && path[3] == 'v')
    {
        if (path[4] == '/')
        {
            if (path[5] == 'n' && path[6] == 'u' && path[7] == 'l' && path[8] == 'l' && path[9] == '\0')
                return "NIL:";
            else
            if (path[5] == 'z' && path[6] == 'e' && path[7] == 'r' && path[8] == 'o' && path[9] == '\0')
                return "ZERO:";
            else
            if (path[5] == 's' && path[6] == 't' && path[7] == 'd')
            {
                if (path[8] == 'i' && path[9] == 'n' && path[10] == '\0')
                    return "IN:";
                else
                if (path[8] == 'o' && path[9] == 'u' && path[10] == 't' && path[11] == '\0')
                    return "OUT:";
                else
                if (path[8] == 'e' && path[9] == 'r' && path[10] == 'r' && path[11] == '\0')
                    return "ERR:";
            }
            else
            if (path[5] == '\0')
                return "DEV:";
	}
        else
        if (path[4] == '\0')
            return "DEV:";
    }

    return NULL;
}

static void __path_normalstuff_u2a(const char *path, char *buf)
{
    register int  makevol   = 0;
    register char prec_char = '\0';

    if (path[0] == '/')
    {
	path++;
        prec_char = '/';
	makevol   = 1;
    }

    for (; path[0]; prec_char = path[0], path++)
    {
        if (path[0] == '/')
        {
            if (path[1] == '/' || prec_char == '/')
	    continue;

            if (makevol)
	    {
	        buf++[0] = ':';
	        makevol = 0;
	    }
	    else
	    {
	        buf++[0] = '/';
	    }
        }
        else
        if
        (
            (path[0]   == '.' && path[1]   == '.')  &&
	    (path[2]   == '/' || path[2]   == '\0') &&
	    (prec_char == '/' || prec_char == '\0')
        )
        {
            buf++[0] = '/';
	    path += 1 + (path[2] == '/');
        }
        else
        if
        (
            (path[0]   == '.')                      &&
	    (path[1]   == '/' || path[1]   == '\0') &&
	    (prec_char == '/' || prec_char == '\0')
        )
        {
            path += (path[1] == '/');
        }
        else
        {
            buf++[0] = path[0];
        }
    }

    if (makevol)
    {
        buf++[0] = ':';
    }

    buf[0] = '\0';
}
