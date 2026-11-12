/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, EndClusterWatch,

/*  SYNOPSIS */
	AROS_LHA(APTR, handle, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 44, Camd)

/*  FUNCTION
		Stops watching clusters and frees the handle with the events it
		still holds.

    INPUTS
		handle - from StartClusterWatchA(), or NULL.

    RESULT

    NOTES
		New in V42. The task is not signalled for this handle once this
		returns; a signal sent before may still be pending.

    EXAMPLE

    BUGS

    SEE ALSO
		StartClusterWatchA()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct ClusterWatch *watch=(struct ClusterWatch *)handle;

	if(watch==NULL) return;

	ObtainSemaphore(&CB(CamdBase)->NotifySemaphore);
	Remove((struct Node *)&watch->node);
	ReleaseSemaphore(&CB(CamdBase)->NotifySemaphore);

	FreeMem(watch,sizeof(struct ClusterWatch));

    AROS_LIBFUNC_EXIT
}
