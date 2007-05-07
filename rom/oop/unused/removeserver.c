/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a class to the list of puvlic classes
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(VOID, OOP_RemoveServer,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, serverID, A0),

/*  LOCATION */
	struct Library *, OOPBase, 13, OOP)

/*  FUNCTION
	Remove a pulic server object that has previously
	been added to the public server list by AddServer().

    INPUTS
    	serverID - ID of server object to remove.

    RESULT
    	None.

    NOTES
    	Probably not a good API. Implemented
	just to show how one can call methods
	across process-borders.

    EXAMPLE

    BUGS

    SEE ALSO
    	OOP_AddServer(), OOP_FindServer()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (serverID)
    {
    	struct Node *sn;
	
	/* This is a public list that must be protected */
    	ObtainSemaphore( &GetOBase(OOPBase)->ob_ServerListLock );
	
	/* Try to find the server */
	sn = FindName((struct List *)&GetOBase(OOPBase)->ob_ServerList
		,serverID);
	
	if (sn)
	{
	    /* If found, remove the node */
	    Remove(sn);
	    /* Free the copied ID */
	    FreeVec(sn->ln_Name);
	    /* Free the servernode */
	    FreeMem(sn, sizeof (struct ServerNode));
	}
    
    	ReleaseSemaphore( & GetOBase(OOPBase)->ob_ServerListLock );
    }
    
    return;
    AROS_LIBFUNC_EXIT
} /* OOP_RemoveServer */
