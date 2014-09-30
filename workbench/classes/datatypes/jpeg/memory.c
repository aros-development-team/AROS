/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>

void *malloc(size_t size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *mem)
{
    FreeVec(mem);
}

char *getenv (const char *name)
{ 
    /* This function is not thread-safe */
    static TEXT buff[128] = {};
    
    if (GetVar(name, buff, 128, GVF_BINARY_VAR) == -1)
        return NULL;
    else
        return buff;
}
