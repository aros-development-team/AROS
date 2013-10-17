/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strtok().
*/

#include "__stdc_intbase.h"
#include <aros/symbolsets.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * strtok (

/*  SYNOPSIS */
	char	   * str,
	const char * sep)

/*  FUNCTION
	Separates a string by the characters in sep.

    INPUTS
	str - The string to check or NULL if the next word in
		the last string is to be searched.
	sep - Characters which separate "words" in str.

    RESULT
	The first word in str or the next one if str is NULL.

    NOTES
	The function changes str !

    EXAMPLE
	char buffer[64];

	strcpy (buffer, "Hello, this is a test.");

	// Init. Returns "Hello"
	strtok (str, " \t,.");

	// Next word. Returns "this"
	strtok (NULL, " \t,.");

	// Next word. Returns "is"
	strtok (NULL, " \t");

	// Next word. Returns "a"
	strtok (NULL, " \t");

	// Next word. Returns "test."
	strtok (NULL, " \t");

	// Next word. Returns NULL.
	strtok (NULL, " \t");

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (str != NULL)
	StdCBase->last = str;
    else
	str = StdCBase->last;

    str += strspn (str, sep);

    if (*str == '\0')
	return NULL;

    StdCBase->last = str;

    StdCBase->last += strcspn (str, sep);

    if (*StdCBase->last != '\0')
	*StdCBase->last ++ = '\0';

    return str;
} /* strtok */


static int __strtok_init(struct StdCIntBase *StdCBase)
{
    StdCBase->last = NULL;

    return 1;
}

ADD2OPENLIB(__strtok_init, 0);
