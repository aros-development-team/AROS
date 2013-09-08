/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strstr().
*/

/*****************************************************************************

    NAME */
#include <string.h>

	char * strstr (

/*  SYNOPSIS */
	const char * str,
	const char * search)

/*  FUNCTION
	Searches for a string in a string.

    INPUTS
	str - Search this string
	search - Look for this string

    RESULT
	A pointer to the first occurence of search in str or NULL if search
	is not found in str.

    NOTES

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello ");

	// This returns a pointer to the first l in buffer.
	strstr (buffer, "llo ");

	// This returns NULL
	strstr (buffer, "llox");

    BUGS

    SEE ALSO
	strchr(), strrchr(), strpbrk()

    INTERNALS

******************************************************************************/
{
    size_t	 done;
    size_t	 len_s = strlen (search);
    const char * t;

    do
    {
	/* If the first character matches */
	if (*search == *str)
	{
	    /* How many characters to compare */
	    done = len_s;

	    /* Skip the first char (we have checked this one already) */
	    t = search + 1;
	    str ++;

	    /*
		Until all characters have been compared and the two
		characters at the current position are equal ...
	    */
	    while ((--done) && (*t == *str))
	    {
		/* Next character */
		t ++;
		str ++;
	    }

	    /* All character compared ? */
	    if (!done)
	    {
		/*
		    Then we have found what we were looking for. str points
		    now to the last character of the string we look for.
		    Therefore we must move it backward to the beginning of
		    the string.
		*/
		str -= len_s;
		return ((char *)str);
	    }
	    else
	    {
		/*
		    The strings are the same upto this character. I must
		    go back since the pattern might be something like
		    "ccbba" and the string "cccbba".
		*/
		str -= len_s - done;
	    }
	}
    } while (*str++);

    /* nothing found */
    return(0);
} /* strstr */

#ifdef TEST
#include <stdio.h>
#include <string.h>

int main (int argc, char ** argv)
{
    char * ptr;

    if (argc != 3)
    {
	printf ("Usage: %s string search\n", argv[0]);
	return 10;
    }

    ptr = strstr (argv[1], argv[2]);

    if (!ptr)
	printf ("%s not found in %s\n", argv[2], argv[1]);
    else
	printf ("%s found in %s as %s\n", argv[2], argv[1], ptr);

    return 0;
}
#endif /* TEST */
