/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/symbolsets.h>

#include <stdlib.h>

struct AtExitNode
{
    struct MinNode node;
    void (*func)(void);
};

#ifndef _CLIB_KERNEL_
static struct MinList __atexit_list;
#endif

int atexit(void (*func)(void))
{
    GETUSER;
    AROS_GET_SYSBASE

    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (!aen) return -1;

    aen->func = func;
    AddHead((struct List *)&__atexit_list, (struct Node *)aen);

    return 0;
}

int __init_atexit(void)
{
    GETUSER;

    NEWLIST((struct List *)&__atexit_list);

    return 0;
}

void __exit_atexit(void)
{
    GETUSER;
    AROS_GET_SYSBASE

    struct AtExitNode *aen;

    while ((aen = (struct AtExitNode *)RemHead((struct List *)&__atexit_list)))
    {
        aen->func();
    }
}

ADD2INIT(__init_atexit, 100);
ADD2EXIT(__exit_atexit, 100);
