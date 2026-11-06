/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(APTR, StartClusterWatchA,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 43, Camd)

/*  FUNCTION
		Starts watching every cluster. Unlike StartClusterNotify(), which
		only signals that clusters came or went, each change is kept as an
		event that says what happened and to which cluster: read them
		with GetClusterWatchEvent() after the signal.

    INPUTS
		tags - CWA_SigBit (BYTE) - the signal to send; required
		       CWA_Task (struct Task *) - the task to signal; default
		           the caller

    RESULT
		A handle for GetClusterWatchEvent() and EndClusterWatch(), or
		NULL without memory or without CWA_SigBit.

    NOTES
		New in V42. The events are:

		CWE_Added - a cluster was created
		CWE_Removed - a cluster was removed
		CWE_Participants - a link or a driver port joined or left
		CWE_Lost - more events came than the handle can keep; some are
		    lost, so look at every cluster again

		Unlike StartClusterNotify() it may be called while holding
		LockCAMD().

    EXAMPLE

    BUGS

    SEE ALSO
		EndClusterWatch(), GetClusterWatchEvent(), StartClusterNotify()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct ClusterWatch *watch;
	LONG sigbit=(LONG)GetTagData(CWA_SigBit,(IPTR)-1,tags);

	if(sigbit<0 || sigbit>31) return NULL;

	watch=AllocMem(sizeof(struct ClusterWatch),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(watch==NULL) return NULL;

	watch->task=(struct Task *)GetTagData(CWA_Task,(IPTR)FindTask(NULL),tags);
	if(watch->task==NULL){
		FreeMem(watch,sizeof(struct ClusterWatch));
		return NULL;
	}
	watch->sigbit=(BYTE)sigbit;

	ObtainSemaphore(&CB(CamdBase)->NotifySemaphore);
	AddTail((struct List *)&CB(CamdBase)->clusterwatches,(struct Node *)&watch->node);
	ReleaseSemaphore(&CB(CamdBase)->NotifySemaphore);

	return watch;

    AROS_LIBFUNC_EXIT
}
