/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH4(LONG, GetEndpointFunctionBlock,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, snapshot, A0),
	AROS_LHA(ULONG, index, D0),
	AROS_LHA(ULONG, number, D1),
	AROS_LHA(struct CAMDFunctionBlockInfoV1 *, info, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 72, Camd)

/*  FUNCTION
		Copies the record of one Function Block of an endpoint in a
		snapshot.

    INPUTS
		snapshot - from ObtainEndpointSnapshot().
		index - the endpoint, as for GetEndpointInfo().
		number - 0 up to the endpoint's number of Function Block
		    records; this counts the records, it is not the block's
		    own number.
		info - Size and Version set by the caller. At most Size bytes
		    are written, and Size is then the size of the full record.

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_OUT_OF_RANGE past the last endpoint or
		the last block, CAMD_RESULT_STALE for a released snapshot, or
		another CAMD_RESULT_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointGroup(), GetEndpointInfo()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDEndpointSnapshot *taken;
	const struct CAMDEndpointInfoV1 *endpoint;
	const struct CAMDGroupInfoV1 *groups;
	const struct CAMDFunctionBlockInfoV1 *blocks;
	size_t groupcount,blockcount;
	ULONG size;
	LONG result;

	if(snapshot==NULL || info==NULL || info->Size<2*sizeof(ULONG) ||
	   info->Version!=1){
		return camd_public_result(CAMD_REGISTRY_INVALID);
	}

	ObtainSemaphoreShared(&CB(CamdBase)->EndpointClientSemaphore);
	taken=FindEndpointSnapshot(snapshot,CamdBase);
	if(taken==NULL){
		result=CAMD_REGISTRY_STALE;
	}else{
		result=camd_snapshot_endpoint(taken,index,&endpoint,&groups,
			&groupcount,&blocks,&blockcount);
		if(result==CAMD_REGISTRY_OK && number>=blockcount){
			result=CAMD_REGISTRY_RANGE;
		}
		if(result==CAMD_REGISTRY_OK){
			size=info->Size<sizeof(blocks[number]) ? info->Size : sizeof(blocks[number]);
			CopyMem((APTR)&blocks[number],info,size);
			info->Size=sizeof(blocks[number]);
		}
	}
	ReleaseSemaphore(&CB(CamdBase)->EndpointClientSemaphore);

	return camd_public_result(result);

    AROS_LIBFUNC_EXIT
}
