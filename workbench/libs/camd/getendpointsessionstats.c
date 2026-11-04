/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetEndpointSessionStats,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, session, A0),
	AROS_LHA(struct CAMDSessionStatsV1 *, stats, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 65, Camd)

/*  FUNCTION
		Tells how many records a session queued, was refused,
		received and lost since it opened.

    INPUTS
		session - from OpenEndpointSession().
		stats - Size and Version set by the caller. At most Size bytes
		    are written, and Size is then the size of the full record.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_STALE for a closed session, or
		another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		OpenEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDSessionStatsV1 full;
	ULONG size;
	LONG result;

	if(session==NULL || stats==NULL || stats->Size<2*sizeof(ULONG) ||
	   stats->Version!=1){
		return CAMD_REGISTRY_INVALID;
	}
	result=camd_registry_session_stats(CB(CamdBase)->EndpointRegistry,
		*session,&full);
	if(result!=CAMD_REGISTRY_OK) return result;

	size=stats->Size<sizeof(full) ? stats->Size : sizeof(full);
	CopyMem(&full,stats,size);
	stats->Size=sizeof(full);

	return CAMD_REGISTRY_OK;

    AROS_LIBFUNC_EXIT
}
