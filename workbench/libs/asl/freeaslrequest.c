/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include "asl_intern.h"

/*****************************************************************************

    NAME */
#include <clib/asl_protos.h>

	AROS_LH1(void, FreeAslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),

/*  LOCATION */
	struct Library *, AslBase, 9, Asl)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,AslBase)

    /* Here We should also free WBArg when multiselection etc. etc. */
    struct ReqNode *reqnode;
		
    if ((reqnode = FindReqNode(requester, ASLB(AslBase))) != NULL)
    {
    	/* Strip requester specific stuff */
    	StripRequester(requester, reqnode->rn_IntReq->ir_ReqType, ASLB(AslBase));
    	
	FreeVec(requester);
	FreeVec(reqnode->rn_IntReq);
	
	ObtainSemaphore( &(ASLB(AslBase)->ReqListSem) );
	Remove( (struct Node *)reqnode );
	ReleaseSemaphore( &(ASLB(AslBase)->ReqListSem) );
		
	FreeMem(reqnode, sizeof (struct ReqNode));
		
    }
    return;

    AROS_LIBFUNC_EXIT
} /* FreeAslRequest */
