/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include <stdio.h>
#include <string.h>
#include "test.h"

APTR pool = NULL;
APTR poolclear = NULL;

#define ALLOCTEST(cmd)              \
    mem = cmd;                      \
    TEST((mem == NULL))             \

#define FREETEST(cmd)               \
    cmd;                            \
    TEST((TRUE))                    \

int main(int argc, char **argv)
{
    APTR mem = NULL;
    pool        = CreatePool(MEMF_ANY, 16384, 8192);
    poolclear   = CreatePool(MEMF_ANY|MEMF_CLEAR, 16384, 8192);

    /* Behavior validated with OS3.x, OS4.x and MorphOS */

    ALLOCTEST(AllocMem(0, MEMF_ANY))
    ALLOCTEST(AllocMem(0, MEMF_ANY | MEMF_CLEAR))
    FREETEST(FreeMem(NULL, 0))


    ALLOCTEST(AllocVec(0, MEMF_ANY))
    ALLOCTEST(AllocVec(0, MEMF_ANY | MEMF_CLEAR))
    FREETEST(FreeVec(NULL))


    ALLOCTEST(AllocPooled(pool, 0))
    ALLOCTEST(AllocPooled(poolclear, 0))
    FREETEST(FreePooled(pool, NULL, 0))


    ALLOCTEST(AllocVecPooled(pool, 0))
    ALLOCTEST(AllocVecPooled(poolclear, 0))
    FREETEST(FreeVecPooled(pool, NULL))

    cleanup();

    return 0;
}

void cleanup()
{
    DeletePool(pool);
    DeletePool(poolclear);
}
