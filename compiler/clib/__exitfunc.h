/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>

struct AtExitNode
{
    struct Node node;
    union
    {
        void (*fvoid)(void);
        void (*fptr)(int, void *);
    } func;
    void *ptr;
};

#define AEN_VOID 0
#define AEN_PTR 1

#ifndef _CLIB_KERNEL_
extern struct MinList __atexit_list;
#endif

int __addexitfunc(struct AtExitNode *aen);
