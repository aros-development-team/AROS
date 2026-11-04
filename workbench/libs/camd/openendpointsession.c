/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"
#include "endpoint_registry.h"
#include "provider_contract.h"

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, OpenEndpointSession,

/*  SYNOPSIS */
	AROS_LHA(struct CAMDSessionRequestV1 *, request, A0),
	AROS_LHA(struct CAMDHandleV1 *, session, A1),
	AROS_LHA(struct CAMDSessionInfoV1 *, info, A2),

/*  LOCATION */
	struct CamdBase *, CamdBase, 49, Camd)

/*  FUNCTION
		Opens a session with an endpoint, in one direction and with
		one data format and protocol. CAMD converts nothing: the
		endpoint must carry that format and protocol itself.

    INPUTS
		request - the endpoint's ID and what the session needs.
		session - filled in with the session's handle.
		info - NULL, or Size and Version set by the caller; filled in
		    with what the session got.

    RESULT
		CAMD_REGISTRY_OK, CAMD_REGISTRY_UNSUPPORTED when the endpoint
		does not carry what was requested, or another CAMD_REGISTRY_
		value.

    NOTES
		New in V43 and provisional: the endpoint functions can still
		change. Nothing converts between MIDI 1.0
		and UMP: the request has to name what the endpoint carries.
		Existing clusters and links keep working on the
		same port, which is opened once. An input session gets what
		the port receives from then on; read it with
		GetEndpointMidi(). The task that opens an input session is
		the one signalled, with the request's Signal.
		Close every session before closing camd.library.

    EXAMPLE

    BUGS

    SEE ALSO
		CloseEndpointSession(), PutEndpointMidi(), PutEndpointSysEx(),
		GetEndpointMidi()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct CAMDProviderOpenRequestV1 open;
	struct CAMDRegistrySessionInfoV1 got;
	struct CAMDSessionInfoV1 full;
	struct CAMDHandleV1 handle;
	ULONG size;
	LONG result;

	if(session!=NULL){
		session->slot=0;
		session->generation=0;
	}
	if(request==NULL || session==NULL || request->Size!=sizeof(*request) ||
	   request->Version!=1){
		return CAMD_REGISTRY_INVALID;
	}
	if(info!=NULL && (info->Size<2*sizeof(ULONG) || info->Version!=1)){
		return CAMD_REGISTRY_INVALID;
	}
	if(request->Conversion!=CAMD_CONVERSION_NONE){
		return CAMD_REGISTRY_UNSUPPORTED;
	}

	open.Size=sizeof(open);
	open.Version=1;
	open.EndpointID=request->EndpointID;
	open.Direction=request->Direction;
	open.DataFormat=request->DataFormat;
	open.Protocol=request->Protocol;
	open.QueueCapacity=request->QueueCapacity;
	result=camd_registry_session_open(CB(CamdBase)->EndpointRegistry,&open,
		&handle);
	if(result!=CAMD_REGISTRY_OK) return result;

	got.Size=sizeof(got);
	got.Version=1;
	result=camd_registry_session_info(CB(CamdBase)->EndpointRegistry,
		handle,&got);
	if(result==CAMD_REGISTRY_OK && got.Direction==CAMD_DIRECTION_INPUT){
		result=StartEndpointReceiver(&handle,got.DataFormat,
			got.EffectiveQueueCapacity,got.MaxSysExBytes,request->Signal,
			CamdBase);
	}
	if(result!=CAMD_REGISTRY_OK){
		camd_registry_session_close(CB(CamdBase)->EndpointRegistry,handle);
		return result;
	}

	if(info!=NULL){
		full.Size=sizeof(full);
		full.Version=1;
		full.Direction=got.Direction;
		full.DataFormat=got.DataFormat;
		full.Protocol=got.Protocol;
		full.QueueCapacity=got.EffectiveQueueCapacity;
		full.MaxSysExBytes=got.MaxSysExBytes;
		size=info->Size<sizeof(full) ? info->Size : sizeof(full);
		CopyMem(&full,info,size);
		info->Size=sizeof(full);
	}
	*session=handle;

	return CAMD_REGISTRY_OK;

    AROS_LIBFUNC_EXIT
}
