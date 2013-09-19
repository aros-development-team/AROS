/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "asl_intern.h"

/*****************************************************************************

    NAME */
#include <proto/asl.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>


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

    struct IntReq        *intreq;
    APTR                req;

    struct ReqNode      *reqnode;
    struct AslReqInfo   *reqinfo;

    SetIoErr(0);

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

            if (intreq->ir_MemPoolPuddle)
            {
                intreq->ir_MemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
                                                intreq->ir_MemPoolPuddle,
                                                intreq->ir_MemPoolThresh);
            }

            if (!intreq->ir_MemPoolPuddle || intreq->ir_MemPool)
            {
                if (tagList) /* If no taglist is supplied, we use default values */
                {
                    struct ParseTagArgs pta;

                    ParseCommonTags(intreq, tagList, ASLB(AslBase));

                    /* Parse tags specific for this type of requester */
                    pta.pta_IntReq      = intreq;
                    pta.pta_Req         = req;
                    pta.pta_Tags        = tagList;

                    CallHookPkt(&(reqinfo->ParseTagsHook), &pta, ASLB(AslBase));


                } /* if (tagList) */

                /* Add requester to internal list */
                reqnode = AllocMem(sizeof (struct ReqNode), MEMF_ANY);
                if (reqnode)
                {
                    reqnode->rn_Req       = req;
                    reqnode->rn_IntReq    = intreq;
                    reqnode->rn_ReqWindow = NULL;
                    ObtainSemaphore( &(ASLB(AslBase)->ReqListSem) );
                    AddTail( (struct List*)&(ASLB(AslBase)->ReqList), (struct Node*)reqnode);
                    ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));

                    SetIoErr(0);

                    return (req);

                }

                if (intreq->ir_MemPool) DeletePool(intreq->ir_MemPool);

            } /* if no mempool needed or mempool creation ok */
            FreeVec(req);

        } /* if (Alloc public request structure) */
        FreeVec(intreq);

    } /* if (Alloc private request structure) */

    SetIoErr(ERROR_NO_FREE_STORE);

    return (NULL);

    AROS_LIBFUNC_EXIT
} /* AllocAslRequest */
