/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a class to the list of puvlic classes
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

	AROS_LH1(VOID, RemoveServer,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, serverID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 13, OOP)

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
    
    if (serverID)
    {
    	struct Node *sn;
	
    	ObtainSemaphore( &GetOBase(OOPBase)->ob_ServerListLock );
	sn = FindName((struct List *)&GetOBase(OOPBase)->ob_ServerList
		,serverID);
	
	if (sn)
	{
	    Remove(sn);
	    FreeVec(sn->ln_Name);
	    
	    FreeMem(sn, sizeof (struct ServerNode));
	}
    
    	ReleaseSemaphore( & GetOBase(OOPBase)->ob_ServerListLock );
    }
    
    return;
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
