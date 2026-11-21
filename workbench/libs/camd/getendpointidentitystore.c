/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, GetEndpointIdentityStore,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDIdentityStoreInfoV1 *, info, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 74, Camd)

/*  FUNCTION
		Tells how the store of endpoint IDs was found when camd.library
		started, how many IDs it holds and how many it can hold.

    INPUTS
		info - Size and Version set by the caller. At most Size bytes
		    are written, and Size is then the size of the full record.

    RESULT
		CAMD_RESULT_OK, or CAMD_RESULT_INVALID.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.
		With CAMD_IDENTITY_STORE_UNREADABLE every endpoint has a
		temporary ID and the files in ENVARC:Sys are left untouched,
		for whoever wants to look at them. Removing them gives a new
		store, and new IDs, the next time the library starts.

    EXAMPLE

    BUGS

    SEE ALSO
		GetStoredEndpointIdentity(), ForgetEndpointIdentity()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDIdentityStoreInfoV1 full;
	ULONG size;

	if(info==NULL || info->Size<2*sizeof(ULONG) || info->Version!=1){
		return camd_public_result(CAMD_REGISTRY_INVALID);
	}
	ReadIdentityStore(&full,CamdBase);
	size=info->Size<sizeof(full) ? info->Size : sizeof(full);
	CopyMem(&full,info,size);
	info->Size=sizeof(full);

	return camd_public_result(CAMD_REGISTRY_OK);

    AROS_LIBFUNC_EXIT
}
