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
	  AROS_UFHA(const libfunc*, list, A0),
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

AROS_UFH6(int, set_call_devfuncs,
	  AROS_UFHA(const void**, list, A0),
	  AROS_UFHA(int, order, D2),
	  AROS_UFHA(void*, ioreq, A1),
	  AROS_UFHA(ULONG, unitnum, D0),
	  AROS_UFHA(ULONG, flags, D1),
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
            if
	    (
	        !AROS_UFC4(int, list[n++], 
			   AROS_UFCA(void*, ioreq, A1),
			   AROS_UFCA(ULONG, unitnum, D0),
			   AROS_UFCA(ULONG, flags, D1),
			   AROS_UFCA(void *, libbase, A6)
		 )
	    )
		return FALSE;
        }
    }
    else
    {
        n = ((int *)list)[0];
        
        while (n)
        {
	    AROS_UFC4(int, list[n--], 
		      AROS_UFCA(void*, ioreq, A1),
		      AROS_UFCA(ULONG, unitnum, D0),
		      AROS_UFCA(ULONG, flags, D1),
		      AROS_UFCA(void *, libbase, A6)
	    );
        }
    }

    return TRUE;
    
    AROS_USERFUNC_EXIT
}
