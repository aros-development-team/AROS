/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include <oop/root.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(VOID, DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(Object  *, obj, A0),

/*  LOCATION */
	struct Library *, OOPBase, 10, OOP)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    ULONG mid = M_Root_Dispose;
    
    EnterFunc(bug("DisposeObject(classID=%s)\n",
    		OCLASS(obj)->ClassNode.ln_Name));


    D(bug("Reducing ObjectCount\n"));    		
    ((struct IntClass *)OCLASS(obj))->ObjectCount --;
		
    D(bug("Calling DoMethodA\n"));    		
    DoMethodA(obj, (Msg)&mid);    		

        
    ReturnVoid("DisposeObject");
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
