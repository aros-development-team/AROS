/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetPublishedEndpointStats,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDPublishedStatsV1 *, stats, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 73, Camd)

/*  FUNCTION
		Tells how many records a published endpoint's publisher took
		and sent, and how many were lost on the way to it and from it.

    INPUTS
		endpoint - from PublishEndpoint().
		stats - Size and Version set by the caller. At most Size bytes
		    are written, and Size is then the size of the full record.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_STALE for a withdrawn endpoint, or
		another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		PublishEndpoint(), GetEndpointSessionStats()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDPublishedStatsV1 full;
	ULONG size;
	LONG result;

	if(endpoint==NULL || stats==NULL || stats->Size<2*sizeof(ULONG) ||
	   stats->Version!=1){
		return CAMD_REGISTRY_INVALID;
	}
	result=GetAppEndpointStats(endpoint,&full,CamdBase);
	if(result!=CAMD_REGISTRY_OK) return result;

	size=stats->Size<sizeof(full) ? stats->Size : sizeof(full);
	CopyMem(&full,stats,size);
	stats->Size=sizeof(full);

	return CAMD_REGISTRY_OK;

    AROS_LIBFUNC_EXIT
}
