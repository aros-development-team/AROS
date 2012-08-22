/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: libinit library - functions calling when opening/closing libs
*/

#include <exec/types.h>
#include <aros/symbolsets.h>

typedef int (*libfunc)(APTR libbase);
typedef int (*opendevfunc)
(
    void *libbase,
    void *ioreq,
    ULONG unitnum,
    ULONG flags
);
typedef int (*closedevfunc)
(
    void *libbase,
    void *ioreq
);

int set_call_libfuncs
(
    const void * const set[],
    int order,
    int test_fail,
    APTR libbase
)
{
    int pos, (*func)(APTR);

    if (!set)
    	    return TRUE;

    ForeachElementInSet(set, order, pos, func)
    {
        if (test_fail)
	{
	    if (!(*func)(libbase))
	        return FALSE;
	}
	else
	{
	    (void)(*func)(libbase);
	}
    }

    return TRUE;
}

int set_call_devfuncs
(
    const void * const set[],
    int order,
    int test_fail,
    void *libbase,
    void *ioreq,
    IPTR unitnum,
    ULONG flags
)
{
    int pos;

    if (!set)
    	    return TRUE;

    if (order>=0)
    {
	int (*func)(APTR, APTR, IPTR, ULONG);
	
	ForeachElementInSet(set, order, pos, func)
	{
	    if (test_fail)
	    {
		if (!(*func)(libbase, ioreq, unitnum, flags))
		    return FALSE;
	    }
	    else
	    {
		(void)(*func)(libbase, ioreq, unitnum, flags);
	    }
	}
    }
    else
    {
	int (*func)(APTR, APTR);
	
	ForeachElementInSet(set, order, pos, func)
	{
	    if (test_fail)
	    {
		if (!(*func)(libbase, ioreq))
		    return FALSE;
	    }
	    else
	    {
		(void)(*func)(libbase, ioreq);
	    }
	}
    }

    return TRUE;
}
