/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - functions sets handling
    Lang: english
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>

#define DEBUG 0
int set_call_funcs(const void *set[], int direction, int test_fail)
{
    int pos, (*func)(void);

    D(bug("entering set_call_funcs() - %p\n", set));
    
    D
    (
        ForeachElementInSet(set, direction, pos, func)
        {
            bug("pos = %d, func = %p\n", pos, func);
        }
    )
    
    ForeachElementInSet(set, direction, pos, func)
    {
        if (test_fail)
	{
	    if (!(*func)())
	        return 0;
	}
	else
	{
	    (void)(*func)();
	}
    }
    
    return 1;
}
