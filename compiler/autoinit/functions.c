/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - functions sets handling
    Lang: english
*/

#include <aros/symbolsets.h>

int set_call_funcs(const void *set[], int direction, int test_fail)
{
    int pos, (*func)(void);

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
