/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int * x;
const char str[] = "hallo";

static const char str2[] = "hallo";

int main (int argc, char ** argv)
{
    int c;
    int * y;

    x = &c;
    y = x;

    c = *y; /* UMR */

    c = str[3]; /* ok */
    c = str[-1]; /* ILR */
    c = str[8]; /* ILR */

    c = str2[3]; /* ok */
    c = str2[-1]; /* ILR */
    c = str2[8]; /* ILR */

    return 0;
}
