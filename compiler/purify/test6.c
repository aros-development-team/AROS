/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
    char * str;
    int i;

    str = malloc (8);

    printf ("str=%p (malloc)\n", str);

    i = str[3]; /* UMR */

    str[-1] = 0; /* IWR */
    str[8] = 0; /* IWR */

    str = calloc (1, 3); /* MLK */

    printf ("str=%p (calloc)\n", str);

    i = str[1]; /* ok */
    str[4] = 0; /* IWR */

    str = realloc (str, 7);

    printf ("str=%p (realloc)\n", str);

    str[4] = 0; /* ok */
    i = str[1]; /* ok */
    i = str[5]; /* UMR */

    free (str);

    i = str[1]; /* FMR */
    str[1] = 1; /* FMW */

    return 0;
}
