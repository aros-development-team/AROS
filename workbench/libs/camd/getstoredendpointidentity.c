/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, GetStoredEndpointIdentity,

/*  SYNOPSIS */
	AROS_LHA(ULONG, index, D0),
	AROS_LHA(struct CAMDStoredIdentityV1 *, identity, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 75, Camd)

/*  FUNCTION
		Reads one of the stored endpoint IDs and what it is stored
		for: a published endpoint's name, or a driver's file name and
		port.

    INPUTS
		index - from 0; see GetEndpointIdentityStore() for how many.
		identity - Size and Version set by the caller. At most Size
		    bytes are written, and Size is then the size of the full
		    record.

    RESULT
		CAMD_RESULT_OK, CAMD_RESULT_OUT_OF_RANGE past the last one, or
		CAMD_RESULT_INVALID.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change.
		The order means nothing and changes when an ID is forgotten
		or added: read them all in one go, and forget by ID.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointIdentityStore(), ForgetEndpointIdentity()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDStoredIdentityV1 full;
	ULONG size;
	LONG result;

	if(identity==NULL || identity->Size<2*sizeof(ULONG) ||
	   identity->Version!=1){
		return camd_public_result(CAMD_REGISTRY_INVALID);
	}
	result=ReadStoredIdentity(index,&full,CamdBase);
	if(result==CAMD_REGISTRY_OK){
		size=identity->Size<sizeof(full) ? identity->Size : sizeof(full);
		CopyMem(&full,identity,size);
		identity->Size=sizeof(full);
	}

	return camd_public_result(result);

    AROS_LIBFUNC_EXIT
}
