/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"

#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <exec/lists.h>
#include "__exitfunc.h"

int __addexitfunc(struct AtExitNode *aen)
{
    ADDHEAD((struct List *)&__atexit_list, (struct Node *)aen);

    return 0;
}

int __init_atexit(void)
{
    NEWLIST((struct List *)&__atexit_list);

    return 1;
}

void __exit_atexit(void)
{
    {
	struct AtExitNode *aen;

	while ((aen = (struct AtExitNode *) REMHEAD(
	                           (struct List *) &__atexit_list)))
	{
	    switch (aen->node.ln_Type)
	    {
	    case AEN_VOID:
		aen->func.fvoid();
		break;

	    case AEN_PTR:
		aen->func.fptr(__aros_startup_error, aen->ptr);
		break;
	    }
	}
    }
}

ADD2INIT(__init_atexit, 100);
ADD2EXIT(__exit_atexit, 100);
