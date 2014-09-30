/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int main (int argc, char ** argv)
{
    short v[3], x;
    short * ptr;

    ptr = v;

    v[1] = 0;

    x = *ptr++;  /* UMR */
    x = *ptr++;  /* ok */

    *ptr++ = 0; /* ok */
    *ptr++ = 0; /* illegal write */
}
