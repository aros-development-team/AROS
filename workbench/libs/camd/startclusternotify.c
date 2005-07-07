/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, StartClusterNotify,

/*  SYNOPSIS */
	AROS_LHA(struct ClusterNotifyNode *,cn,A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 37, Camd)

/*  FUNCTION
        void StartClusterNotify(struct ClusterNotifyNode *cn)

    INPUTS
        pointer to initialized ClusterNotifyNode structure

    RESULT
        void

    NOTES
        ClusterNotifyNode structure must remain valid until EndClusterNotify();
        Will only signal added and removed clusters, not internal state changes.

    EXAMPLE
        struct ClusterNotifyNode cnn;
        
        cnn.cnn_Task=IExec->FindTask(NULL);
        cnn.cnn_SigBit=IExec->AllocSignal(-1);
        StartClusterNotify(&cnn);
        
                    somewhere down the line...
        
        Wait(1L<<cnn.cnn_SigBit)
            printf("Cluster Changes have happened\n");

    BUGS

    SEE ALSO
        EndClusterNotify();

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created
    2005-05-07 Lyle Hazelwood first implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

    if(cn == NULL)    return;
    AddTail(&CB(CamdBase)->clusnotifynodes,(struct Node *)&cn->cnn_Node);
	return;

   AROS_LIBFUNC_EXIT
}

