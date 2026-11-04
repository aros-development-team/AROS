/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, StartEndpointWatch,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDHandleV1 *, watch, A0),
	AROS_LHA(ULONG, signal, D0),
	AROS_LHA(struct CAMDGenerationV1 *, generation, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 55, Camd)

/*  FUNCTION
		Starts collecting the changes to CAMD's endpoints: endpoints
		added, updated, gone offline or retired.

    INPUTS
		watch - filled in with the watch's handle.
		signal - the number of the signal the calling task gets at
		    each change, or CAMD_SIGNAL_NONE.
		generation - NULL, or filled in with the generation of the
		    endpoints when the watch started.

    RESULT
		CAMD_REGISTRY_OK, or another CAMD_REGISTRY_ value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Start the watch first and take a
		snapshot afterwards, so that no change falls between the two.
		End every watch before closing camd.library.

    EXAMPLE

    BUGS

    SEE ALSO
		GetEndpointWatchEvent(), EndEndpointWatch(), ObtainEndpointSnapshot()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(watch==NULL) return CAMD_REGISTRY_INVALID;
	watch->slot=0;
	watch->generation=0;

	return StartEndpointWatcher(watch,signal,generation,CamdBase);

    AROS_LIBFUNC_EXIT
}
