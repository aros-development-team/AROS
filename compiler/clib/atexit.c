/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include "__exitfunc.h"

int atexit(void (*func)(void))
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->node.ln_Type = AEN_VOID;
    aen->func.fvoid = func;

    return __addexitfunc(aen);
}
