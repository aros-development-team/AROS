/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Add a server to the list of public servers
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include <oop/root.h>
#include <string.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH2(BOOL, AddServer,

/*  SYNOPSIS */
	AROS_LHA(Object  *, serverPtr,	A0),
	AROS_LHA(STRPTR	  , serverID, 	A1),

/*  LOCATION */
	struct Library *, OOPBase, 12, OOP)

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
    
    EnterFunc(bug("AddServer(server=%p, serverID=%s)\n",
    	serverPtr, serverID));
    
    if (serverPtr && serverID)
    {
    	struct ServerNode *sn;
	
	sn = AllocMem(sizeof (struct ServerNode), MEMF_PUBLIC);
    	if (sn)
	{
	    sn->sn_Node.ln_Name = AllocVec(strlen (serverID) + 1, MEMF_ANY);
	    if (sn->sn_Node.ln_Name)
	    {
	    	sn->sn_Server = serverPtr;
		
		strcpy(sn->sn_Node.ln_Name, serverID);
		
    		ObtainSemaphore( &GetOBase(OOPBase)->ob_ServerListLock );
    
    		AddTail((struct List *)&GetOBase(OOPBase)->ob_ServerList
    			,(struct Node *)sn);
    		ReleaseSemaphore( & GetOBase(OOPBase)->ob_ServerListLock );
		
		ReturnBool ("AddServer", TRUE);

	    }
	    FreeMem(sn, sizeof (struct ServerNode));
    	}
	
    } /* if (valid parameters) */
    
    ReturnBool ("AddServer", FALSE);
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
