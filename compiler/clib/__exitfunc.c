/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include "__exitfunc.h"

#ifndef _CLIB_KERNEL_
static struct MinList __atexit_list;
extern LONG    __startup_error;
#endif

int __addexitfunc(struct AtExitNode *aen)
{
    GETUSER;
    AROS_GET_SYSBASE;
  
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
    AROS_GET_SYSBASE;
  
    struct AtExitNode *aen;

    while ((aen = (struct AtExitNode *)RemHead((struct List *)&__atexit_list)))
    {
        switch (aen->node.ln_Type)
        {
	case AEN_VOID:
	    aen->func.fvoid();
	    break;
	  
	case AEN_PTR:
	    aen->func.fptr(__startup_error, aen->ptr);
	    break;
	}
    }
}

ADD2INIT(__init_atexit, 100);
ADD2EXIT(__exit_atexit, 100);
