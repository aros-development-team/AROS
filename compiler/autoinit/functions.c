/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - functions sets handling
    Lang: english
*/

#define DEBUG 0

#include <aros/symbolsets.h>
#include <aros/debug.h>

int set_call_funcs(const void * const set[], int direction, int test_fail)
{
    int pos, (*func)(void);

    D(bug("entering set_call_funcs() - %p\n", set));
    
    ForeachElementInSet(set, direction, pos, func)
    {
	D(bug("  %p[%d] %p()", set, pos, func));

        if (test_fail)
	{
	    int ret = (*func)();
	    D(bug(" => %d", ret));
	    if (!ret)
	        return 0;
	}
	else
	{
	    (void)(*func)();
	}
	D(bug("\n"));
    }
    
    return 1;
}
