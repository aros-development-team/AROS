/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include "asl_intern.h"

/*****************************************************************************

    NAME */
#include <clib/asl_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>

	AROS_LH2(APTR, AllocAslRequest,

/*  SYNOPSIS */
	AROS_LHA(ULONG           , reqType, D0),
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct Library *, AslBase, 8, Asl)

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

    struct IntReq *intreq;
    APTR req;
	
    struct ReqNode *reqnode;
    struct AslReqInfo *reqinfo;

    /* Parameter check */
    if 
    ( 
    	(reqType != ASL_FileRequest) 
    && 
    	(reqType != ASL_FontRequest)
    &&
    	(reqType != ASL_ScreenModeRequest)
    )
	return (NULL);
	
    reqinfo = &(ASLB(AslBase)->ReqInfo[reqType]);
	
    /* Allocate memory for internal requester structure */
    intreq = AllocVec(reqinfo->IntReqSize, MEMF_ANY);
    if (intreq)
    {
	req = AllocVec(reqinfo->ReqSize, MEMF_ANY|MEMF_CLEAR);
	if (req)
	{
	    CopyMem(reqinfo->DefaultReq, intreq, reqinfo->IntReqSize);
			
	    if (tagList) /* If no taglist is supplied, we use default values */
	    {
		struct ParseTagArgs pta;
		
		ParseCommonTags(intreq, tagList, ASLB(AslBase));
		
		/* Parse tags specific for this type of requester */
		pta.pta_IntReq	= intreq;
		pta.pta_Req	= req;
		pta.pta_Tags	= tagList;
				
		CallHookPkt(&(reqinfo->ParseTagsHook), &pta, ASLB(AslBase));
				
				
	    } /* if (tagList) */
		
	    /* Add requester to internal list */
	    reqnode = AllocMem(sizeof (struct ReqNode), MEMF_ANY);
	    if (reqnode)
	    {
		reqnode->rn_Req		= req;
		reqnode->rn_IntReq	= intreq;
				
		ObtainSemaphore( &(ASLB(AslBase)->ReqListSem) );
		AddTail( (struct List*)&(ASLB(AslBase)->ReqList), (struct Node*)reqnode);
		ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));
				
		return (req);
				
	    }
	    FreeVec(req);
	    
	} /* if (Alloc public request structure) */
	FreeVec(intreq);
	
    } /* if (Alloc private request structure) */
	
    return (NULL);

    AROS_LIBFUNC_EXIT
} /* AllocAslRequest */
