/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, ObtainEndpointSnapshot,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, snapshot, A0),
	AROS_LHA(ULONG *, count, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 46, Camd)

/*  FUNCTION
		Takes an unchanging copy of the endpoints CAMD knows now.

    INPUTS
		snapshot - filled in with the snapshot's handle.
		count - filled in with the number of endpoints, or NULL.

    RESULT
		CAMD_REGISTRY_OK, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Release every snapshot before
		closing camd.library.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointInfo(), ReleaseEndpointSnapshot()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDEndpointSnapshot *taken;
	LONG result;

	if(snapshot==NULL) return CAMD_REGISTRY_INVALID;
	snapshot->slot=0;
	snapshot->generation=0;
	if(count!=NULL) *count=0;

	result=camd_registry_snapshot(CB(CamdBase)->EndpointRegistry,&taken);
	if(result!=CAMD_REGISTRY_OK) return result;

	result=AddEndpointSnapshot(taken,snapshot,CamdBase);
	if(result!=CAMD_REGISTRY_OK){
		camd_snapshot_destroy(taken);
		return result;
	}
	if(count!=NULL) *count=(ULONG)camd_snapshot_endpoint_count(taken);

	return CAMD_REGISTRY_OK;

    AROS_LIBFUNC_EXIT
}
