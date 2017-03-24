/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>
#include <string.h>

static char s[256];

#define FPU_TEST 1

#if FPU_TEST
#  define TYPE double
#  define VAL1 1.0
#  define VAL2 2.0
#  define FORMAT_STRING "%f %f %f %f"
#else
#  define TYPE long
#  define VAL1 1
#  define VAL2 2
#  define FORMAT_STRING "%d %d %d %d"
#endif

void arrgh(TYPE a, TYPE b, TYPE shouldbe_a, TYPE shouldbe_b)
{
    sprintf(s,"\n\n\n******* CONDITION CODES TERRIBLY WRONG ******* " FORMAT_STRING "\n\n\n", a, b, shouldbe_a, shouldbe_b);
    bug(s);
}

int main(void)
{
    TYPE a, b;

    while((SetSignal(0, 0) & SIGBREAKF_CTRL_C) == 0)
    {
	a = VAL1; b = VAL1;
	if (!(a == b)) arrgh(a,b,VAL1,VAL1);
	a = VAL2; b = VAL1;
	if (!(a > b)) arrgh(a,b,VAL2,VAL1);
	if (!(b < a)) arrgh(a,b,VAL2,VAL1);
	if (!(b != a)) arrgh(a,b,VAL2,VAL1);
	if (a == b) arrgh(a,b,VAL2,VAL1);
	if (a <= b) arrgh(a,b,VAL2,VAL1);
    }

    return 0;
}
