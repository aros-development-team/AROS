/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: libinit library - functions calling when opening/closing libs
*/

#include <exec/types.h>
#include <aros/asmcall.h>

typedef AROS_UFP1(int, (*libfunc),
		  AROS_UFPA(void*, libbase, A6)
		 );

AROS_UFH3(int, set_call_libfuncs,
	  AROS_UFHA(libfunc*, list, A0),
	  AROS_UFHA(int, order, D0),
	  AROS_UFHA(void*, libbase, A6)
	 )
{
    AROS_USERFUNC_INIT
    
    int n;

    if (order>=0)
    {
        n = 1;
        while(list[n])
        {
            if (!AROS_UFC1(int, list[n++], 
		AROS_UFCA(void *, libbase, A6))) return FALSE;
        }
    }
    else
    {
        n = ((int *)list)[0];
        
        while (n)
        {
            AROS_UFC1(int, list[n--],
		AROS_UFCA(void *, libbase, A6));
        }
    }

    return TRUE;
    
    AROS_USERFUNC_EXIT
}
