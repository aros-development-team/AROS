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

#ifdef BUILD_TEST
    static int  __doupath = 1;
    static char *__apathbuf;
#   define realloc_nocopy realloc
#endif

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

	The pointer is valid only until next call to this function, so if
	you need to call this function recursively, you must save the string
	pointed to by the pointer before calling this function again.

    NOTES
        This function is for private usage by system code. Do not use it
        elsewhere.

    INTERNALS

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

typedef enum
{
    S_START0,
    S_START,
    S_DOT1,
    S_DOT2,
    S_SLASH,
    S_COLUMN
} path_state;

static void __path_normalstuff_u2a(const char *path, char *buf)
{
    register char       dir_sep = '\0';
    register int        makevol = 0;
    register path_state state   = S_START0;

    int run = 1;

    while (path[0] == '/')
    {
	path++;
	makevol = 1;
    }

    while (run)
    {
        register char ch = path[0];

	switch (state)
	{
	    case S_START0:
	        if (ch == ':')
		    state = S_COLUMN;
		else
	    case S_START:
	        if (ch == '/')
		{
		    dir_sep = '/';
		    state = S_SLASH;
		}
		else
	        if (ch == '.')
		    state = S_DOT1;
		else
		if (ch == '\0')
		    run = 0;
		else
		    buf++[0] = ch;

		break;

	    case S_DOT1:
	        if (ch == '\0')
		    run = 0;
		else
	        if (ch == '.')
		    state = S_DOT2;
		else
		if (ch == '/')
		{
		    dir_sep = '\0';
		    state = S_SLASH;
		}
		else
		{
		    buf[0] = '.';
		    buf[1] = ch;
		    buf += 2;

		    state = S_START;
		}

		break;

	    case S_DOT2:
	        if (ch == '\0')
		{
		    buf++[0] = '/';
		    run = 0;
		}
		else
	        if (ch == '/')
		{
		    dir_sep = '/';
		    state = S_SLASH;
		}
		else
		{
		    buf[0] = '.';
		    buf[1] = '.';
		    buf[2] = ch;
		    buf += 3;

		    state = S_START;
		}

		break;

	    case S_SLASH:
	        if (ch != '/')
		{
		    if (makevol)
		    {
		        makevol = 0;
		        dir_sep = ':';
		    }

		    if (dir_sep != '\0')
		        buf++[0] = dir_sep;

		    state = S_START;

		    continue;
		}

		break;

	    case S_COLUMN:
	        dir_sep = ':';
		state = S_SLASH;

		continue;
	}

	path++;
    }

    if (makevol)
        buf++[0] = ':';
	
    buf[0] = '\0';
}

#ifdef BUILD_TEST

#include <stdio.h>
int main(int argc, char *argv[])
{
    if (argc != 2)
        return 20;

    return printf("%s\n", __path_u2a(argv[1]));
}
#endif

