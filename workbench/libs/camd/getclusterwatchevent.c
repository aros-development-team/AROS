/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(BOOL, GetClusterWatchEvent,

/*  SYNOPSIS */
	AROS_LHA(APTR, handle, A0),
	AROS_LHA(struct ClusterWatchEvent *, event, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 45, Camd)

/*  FUNCTION
		Takes the oldest event from a cluster watch.

    INPUTS
		handle - from StartClusterWatchA().
		event - filled in with the event.

    RESULT
		TRUE if there was an event, FALSE if none is left.

    NOTES
		New in V42. Read events until it returns FALSE after each
		signal. When events were lost the next event returned is
		CWE_Lost, with an empty name. The old queue is discarded; rescan
		all clusters, then read normally again for later events.

    EXAMPLE
		while(GetClusterWatchEvent(watch,&event)){
			if(event.cwe_Type==CWE_Lost) rescan_all();
			else update(event.cwe_Type,event.cwe_Name);
		}

    BUGS

    SEE ALSO
		StartClusterWatchA()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct ClusterWatch *watch=(struct ClusterWatch *)handle;
	BOOL ret=FALSE;

	if(watch==NULL || event==NULL) return FALSE;

	ObtainSemaphore(&CB(CamdBase)->NotifySemaphore);
	if(watch->lost){
		watch->lost=FALSE;
		event->cwe_Type=CWE_Lost;
		event->cwe_Name[0]=0;
		ret=TRUE;
	}else if(watch->count>0){
		*event=watch->events[watch->first];
		watch->first=(watch->first+1)%CLUSTERWATCH_QUEUE;
		watch->count--;
		ret=TRUE;
	}
	ReleaseSemaphore(&CB(CamdBase)->NotifySemaphore);

	return ret;

    AROS_LIBFUNC_EXIT
}
