/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include "__exitfunc.h"

int on_exit(void (*func)(int, void *), void *arg)
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->node.ln_Type = AEN_PTR;
    aen->func.fptr = func;
    aen->ptr = arg;

    return __addexitfunc(aen);
}
