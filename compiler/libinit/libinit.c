/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: libinit library - functions calling when opening/closing libs
*/

#include <exec/types.h>
#include <aros/asmcall.h>

typedef AROS_UFH1(int, (*libfunc),
		  AROS_UFHA(void*, libbase, A6)
		 );

AROS_UFH3(int, set_call_libfuncs,
	  AROS_UFHA(libfunc*, list, A0),
	  AROS_UFHA(int, order, D0),
	  AROS_UFHA(void*, libbase, A6)
	 )
{
    int n;

    if (order>=0)
    {
        n = 1;
        while(list[n])
        {
            if (!list[n++](libbase)) return FALSE;
        }
    }
    else
    {
        n = ((int *)list)[0];
        
        while (n)
        {
            (void)list[n--](libbase);
        }
    }

    return TRUE;
}
