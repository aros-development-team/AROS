/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(BOOL, RemoveMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(struct MidiDeviceData *, mididevicedata, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 78, Camd)

/*  FUNCTION
		Removes a driver added with AddMidiDevice(). Its endpoint provider
		is retired, open ports are closed, its driver nodes leave the
		clusters, and Expunge() is called. Client links remain and may wait
		for a driver of the same name to return.

    INPUTS
		mididevicedata - the same pointer passed to AddMidiDevice().

    RESULT
		TRUE if the in-memory driver was removed. FALSE if it was not
		registered by AddMidiDevice(), or an endpoint session still holds
		the driver; close that session and call RemoveMidiDevice() again.

    NOTES
		New in V43. ClosePort() must stop all callbacks into CAMD before it
		returns. The driver structure and code may be released after this
		function returns TRUE.

		A driver loaded from DEVS:Midi cannot be removed with this call.
		Call from a task, not from an interrupt.

    EXAMPLE

    BUGS

    SEE ALSO
		AddMidiDevice(), CloseEndpointSession()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Drivers *driver,*previous;

	if(mididevicedata==NULL) return FALSE;

	ObtainSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
	driver=FindDriverForMidiDeviceData(mididevicedata,CamdBase);
	if(driver==NULL || !driver->inMemory){
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	if(!UnpublishDriverEndpoints(driver,CamdBase)){
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);
	previous=NULL;
	for(driver=CB(CamdBase)->drivers;driver;driver=driver->next){
		if(driver->mididevicedata==mididevicedata) break;
		previous=driver;
	}
	if(driver==NULL || !driver->inMemory){
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}
	if(previous!=NULL) previous->next=driver->next;
	else CB(CamdBase)->drivers=driver->next;
	DetachDriverData(driver,CamdBase);
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	FreeDriverData(driver,CamdBase);
	if(mididevicedata->Expunge!=NULL) (*mididevicedata->Expunge)();
	FreeMem(driver,sizeof(struct Drivers));
	ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	return TRUE;

    AROS_LIBFUNC_EXIT
}
