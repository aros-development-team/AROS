/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH5(LONG, SetPublishedEndpointTopology,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, endpoint, A0),
	AROS_LHA(struct CAMDGroupInfoV1 *, groups, A1),
	AROS_LHA(ULONG, groupcount, D0),
	AROS_LHA(struct CAMDFunctionBlockInfoV1 *, blocks, A2),
	AROS_LHA(ULONG, blockcount, D1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 70, Camd)

/*  FUNCTION
		Replaces a published endpoint's Groups and Function Blocks as
		a whole. Clients see the old set or the new one in a snapshot,
		never a mixture, and their watches get an update.

    INPUTS
		endpoint - from PublishEndpoint().
		groups - groupcount records with Size, Version, Group and Name
		    set; EndpointID is filled in. NULL with a count of 0.
		groupcount - the number of Groups.
		blocks - blockcount records with Size, Version, Number,
		    FirstGroup, GroupCount and Name set; EndpointID is filled
		    in. NULL with a count of 0.
		blockcount - the number of Function Blocks.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_INVALID for a Group past 15 or
		a block that spans past it, CAMD_REGISTRY_DUPLICATE for a
		Group or block number given twice, or another CAMD_REGISTRY_
		value. A refused set leaves the earlier one.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. CAMD checks the ranges only; it
		does not route by Group.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointGroup(), GetEndpointFunctionBlock()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(endpoint==NULL) return CAMD_REGISTRY_INVALID;

	return SetAppEndpointTopology(endpoint,groups,groupcount,blocks,
		blockcount,CamdBase);

    AROS_LIBFUNC_EXIT
}
