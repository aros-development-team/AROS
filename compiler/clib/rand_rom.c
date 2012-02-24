/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C functions rand() and srand().
*/

#include <stdlib.h>

/* This is the version ofo rand() and srand() for in librom.a */
/* For more info on the functions look in rand.c */
static unsigned int srand_seed = 1;

int rand (void)
{
    srand_seed = srand_seed * 1103515245 + 12345;
    return srand_seed % RAND_MAX;
} /* rand */

void srand (unsigned int seed)
{
    srand_seed = seed;
} /* srand */
