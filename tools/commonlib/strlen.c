/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "commonlib.h"

#undef	TEST
#undef	CommonLib_NoInline
#undef	strlen

size_t strlen1 (CONST char * str)
{
#   include "strlen.h"
    return strlen (str);
}

#define CommonLib_NoInline
#undef	strlen
#undef	COMMONLIB_STRLEN_H
#define strlen strlen2

#include "strlen.h"

#undef	strlen

const char * TestStrings[] =
{
    "",
    "1",
    "22",
    "333"
};

int main (int argc, char * argv[])
{
    int len1, len2, t;
    char ** testArray;
    int     count;

    if (argc == 1)
    {
	t	  = 0;
	count	  = sizeof(TestStrings) / sizeof(TestStrings[0]);
	testArray = (char **)TestStrings;
    }
    else
    {
	t	  = 1;
	count	  = argc - 1;
	testArray = argv+1;
    }

    for ( ; t<count; t++)
    {
	len1 = strlen1 (testArray[t]);
	len2 = strlen2 (testArray[t]);

	printf ("strlen(\"%s\") = %d / %d\n", testArray[t], len1, len2);

	if (len1 != len2)
	{
	    fprintf (stderr, "Length differs");
	    exit (10);
	}
    }

    return 0;
}
