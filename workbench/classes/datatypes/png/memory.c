/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <stdlib.h>
#include <aros/debug.h>

void *malloc(size_t size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *mem)
{
    FreeVec(mem);
}

void *calloc(size_t nmemb, size_t size)
{
    return AllocVec(nmemb * size, MEMF_ANY | MEMF_CLEAR);
}

void exit(int code)
{
    abort();
}

void abort(void)
{
    bug("png.datatype - abort() called - needs proper implementation");
    exit(1);
}
