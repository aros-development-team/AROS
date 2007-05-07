/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a server to the list of public servers
    Lang: english
*/
#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <aros/debug.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH2(BOOL, OOP_AddServer,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object  *, serverPtr,	A0),
	AROS_LHA(STRPTR	  , serverID, 	A1),

/*  LOCATION */
	struct Library *, OOPBase, 12, OOP)

/*  FUNCTION
	Adds a server object to to the list of public servers.
	Other processes might then obtain a pointer to the server,
	and use the server to obtain proxies for objects the
	server controls.

    INPUTS
    	serverPtr - Pointer to a valid server object.
	serverID - ID that identifies the server.

    RESULT
    	TRUE if successfull, FALSE otherwise.

    NOTES
    	Probably not a good API. Implemented
	just to show how one can call methods
	across process-borders.

    EXAMPLE

    BUGS

    SEE ALSO
    	OOP_FindServer(), OOP_RemoveServer()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    EnterFunc(bug("OOP_AddServer(server=%p, serverID=%s)\n",
    	serverPtr, serverID));
    
    if (serverPtr && serverID)
    {
    	struct ServerNode *sn;
	
	/* Allocate a listnode, so we can add the server to the list.
	** If objects have a node by default, then this won't be necessary.
	*/
	sn = AllocMem(sizeof (struct ServerNode), MEMF_PUBLIC);
    	if (sn)
	{
	    /* Copy the ID */
	    sn->sn_Node.ln_Name = AllocVec(strlen (serverID) + 1, MEMF_ANY);
	    if (sn->sn_Node.ln_Name)
	    {
	    	sn->sn_Server = serverPtr;
		
		strcpy(sn->sn_Node.ln_Name, serverID);
		
    		ObtainSemaphore( &GetOBase(OOPBase)->ob_ServerListLock );
    
    		AddTail((struct List *)&GetOBase(OOPBase)->ob_ServerList
    			,(struct Node *)sn);
    		ReleaseSemaphore( & GetOBase(OOPBase)->ob_ServerListLock );
		
		ReturnBool ("OOP_AddServer", TRUE);

	    }
	    FreeMem(sn, sizeof (struct ServerNode));
    	}
	
    } /* if (valid parameters) */
    
    ReturnBool ("OOP_AddServer", FALSE);
    
    AROS_LIBFUNC_EXIT
} /* OOP_AddServer */
