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

/*****************************************************************************

    NAME */
#include "__upath.h"


	const char *__path_a2u(

/*  SYNOPSIS */
        const char *apath)

/*  FUNCTION
        Translates an AmigaDOS-style path into an unix one.

    INPUTS
        apath - AmigaDOS-style path to translate into an unix-style equivalent.

    RESULT
        A pointer to a string containing the unix-style path, or NULL in
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
    const char *old_apath = apath;
    char ch, *upath, *old_upath;
    size_t size = 0;
    int run;
    register enum
    {
        S_START0,
	S_START1,
        S_START,
        S_VOLUME,
        S_PARENT,
        S_SLASH
    } state;

    /* Safety check.  */
    if (apath == NULL)
    {
        errno = EFAULT;
        return NULL;
    }

    while ((ch = *apath++))
    {
         if (ch == '/')
	     size += 3;
	 else
	     size += 1;
    }

    if (size == 0)
        return "";

    old_upath = realloc_nocopy(__apathbuf, 1 + size + 1);
    if (old_upath == NULL)
    {
	errno = ENOMEM;
	return NULL;
    }

    __apathbuf = old_upath;
    upath = ++old_upath;
    apath = old_apath;

    run = 1;
    state = S_START0;
    while (run)
    {
        register char ch = apath[0];

        switch (state)
	{
	    case S_START0:
	        if (ch == '/')
		    state = S_PARENT;
		else
		{
		    state = S_START1;
		    continue;
		}

		break;

	    case S_START1:
		if (ch == ':')
		    state = S_VOLUME;
		else
	    case S_START:
	        if (ch == '/')
		    state = S_SLASH;
		else
		if (ch == '\0')
		    run = 0;
		else
		    upath++[0] = ch;

		break;

	    case S_VOLUME:
	        (--old_upath)[0] = '/';
		state = S_SLASH;
		continue;

		break;

	    case S_SLASH:
	        upath++[0] = '/';

		if (ch == '/')
		    state = S_PARENT;
		else
		{
		    state = S_START;
		    continue;
		}

		break;

	    case S_PARENT:
	        upath[0] = '.'; upath[1] = '.'; upath[2] = '/'; upath += 3;

		if (ch != '/')
		{
		    state = S_START;
		    continue;
		}

		break;
        }

	upath[0] = '\0';
	apath++;
    }

    upath[0] = '\0';
    return old_upath;
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
    register char dir_sep = '\0';
    register int  makevol = 0;
    register enum
    {
        S_START0,
        S_START,
        S_DOT1,
        S_DOT2,
        S_SLASH,
    } state = S_START0;

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
	        if (ch == '.')
		    state = S_DOT1;
		else
		{
		    state = S_START;
		    continue;
		}
		break;
	    case S_START:
	        if (ch == '/')
		{
		    dir_sep = '/';
		    state = S_SLASH;
		}
		else
	        if (ch == ':')
		{
		    dir_sep = ':';
		    state = S_SLASH;
		}
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
	        if (ch == '/' || ch == '\0')
		{
		    dir_sep = '/';
		    state = S_SLASH;
		    continue;
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

  	            state = S_START0;

		    continue;
		}

		break;
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
    if (argc != 3)
        return 20;

    printf("%s\n", __path_u2a(argv[1]));
    printf("%s\n", __path_a2u(argv[2]));
}
#endif

