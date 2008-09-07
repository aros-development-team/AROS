/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Compare the tails of two strings
    Lang: english
*/

#include <ctype.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int strrncasecmp (

/*  SYNOPSIS */
	const char * str1,
	const char * str2,
	int	     cnt)

/*  FUNCTION
	Compare the end parts of two strings. Case is ignored.

    INPUTS
	str1, str2 - Strings to compare
	cnt - Compare that many characters

    RESULT
	Returns 0 if the strings are equal and != 0 otherwise.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE
	// rc = 0
	rc = strrncasecmp ("disk.info", ".INFO", 5);

	// rc <> 0
	rc = strrncasecmp ("disk.info", ".c", 2);

    BUGS

    SEE ALSO
	clib/strcmp(), clib/strcasecmp(), clib/strncasecmp(), clib/strrchr()

    INTERNALS

    HISTORY

******************************************************************************/
{
    const char * ptr1, * ptr2;
    int diff = 0;

    /* If any string is empty, the strings are equal */
    if (!*str1 || !*str2)
	return 0;

    ptr1 = str1;

    while (*ptr1)
	ptr1++;

    ptr2 = str2;

    while (*ptr2)
	ptr2++;

    do
    {
	if (!cnt)
	    break;

	cnt --;
	ptr1 --;
	ptr2 --;

	diff = tolower (*ptr1) - tolower (*ptr2);
    }
    while (!diff && ptr1 != str1 && ptr2 != str2);

    /* Compared all neccessary chars ? */
    if (!cnt)
	return diff;

    /* Run out of chars in only one of the two strings ? */
    if (ptr1 != str1)
	diff = 1;
    else if (ptr2 != str2)
	diff = -1;

    /* The strings differ */
    return diff;
} /* strrncasecmp */

#ifdef TEST
#include <stdio.h>

int main (int argc, char ** argv)
{
    int n;

    if (argc != 4)
    {
	fprintf (stderr, "Usage: %s string1 string2 len\n", argv[0]);
	return 5;
    }

    n = atoi (argv[3]);

    printf ("strrncasecmp (\"%s\", \"%s\", %d) = %d\n",
	argv[1],
	argv[2],
	n,
	strrncasecmp (argv[1], argv[2], n)
    );

} /* main */

#endif /* TEST */
