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

	AROS_LH1(void, EndClusterNotify,

/*  SYNOPSIS */
	AROS_LHA(struct ClusterNotifyNode *, cn,A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 38, Camd)

/*  FUNCTION
        void EndClusterNotify(struct ClusterNotifyNode *)

    INPUTS
        pointer to previously added ClusterNotifyNode.

    RESULT
        void

    NOTES
        DO NOT call with a ClusterNotifyNode that has not been added.

    EXAMPLE

    BUGS
		None known

    SEE ALSO
        StartClusterNotify();

    INTERNALS

    HISTORY
	2001-01-12 ksvalast first created
    2005-05-07 Lyle Hazelwood first implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

    if(cn == NULL)   return;

    Remove((struct Node *)&cn->cnn_Node);
	return;

   AROS_LIBFUNC_EXIT
}

