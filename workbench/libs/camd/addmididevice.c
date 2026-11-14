/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(BOOL, AddMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(struct MidiDeviceData *, mididevicedata, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 77, Camd)

/*  FUNCTION
		Adds a CAMD driver which is already in memory. Its ports appear in
		the same <name>.in.<n> and <name>.out.<n> clusters, and in the same
		endpoint registry, as a driver loaded from DEVS:Midi.

    INPUTS
		mididevicedata - the driver's MidiDeviceData. The structure, its
			strings, callbacks and callback code must remain valid until
			RemoveMidiDevice() returns or camd.library is expunged.

    RESULT
		TRUE if the driver was initialized and all ports were added. FALSE
		for an invalid driver, a duplicate pointer or name, a name already
		used by a published endpoint, failed Init(), or insufficient memory.

    NOTES
		New in V43. The caller must keep camd.library open and keep the
		driver's code resident while it is registered. Init() is called
		before this function returns TRUE. Existing links waiting for the
		driver's clusters are connected just as with RethinkCAMD().

		The driver's name is also its endpoint identity key. Replacing a
		DEVS:Midi driver of the same name therefore preserves endpoint IDs.

		Only the AROS-supported new driver format (Flags bit 0) is accepted.
		Call from a task, not from an interrupt.

    EXAMPLE

    BUGS

    SEE ALSO
		RemoveMidiDevice(), RethinkCAMD(), OpenMidiDevice()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Drivers *driver=NULL;
	BOOL attached=FALSE;

	if(mididevicedata==NULL || mididevicedata->Magic!=MDD_Magic ||
	   mididevicedata->Name==NULL || mididevicedata->Name[0]==0 ||
	   mididevicedata->IDString==NULL || mididevicedata->Init==NULL ||
	   mididevicedata->OpenPort==NULL || mididevicedata->ClosePort==NULL ||
	   (mididevicedata->Flags&1)==0 || mididevicedata->NPorts==0)
		return FALSE;

	ObtainSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
	if(FindDriverForMidiDeviceData(mididevicedata,CamdBase)!=NULL ||
	   MidiDeviceNameIsLoaded(mididevicedata->Name,CamdBase)){
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
	if(AppEndpointNamed(mididevicedata->Name,CamdBase)){
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}

	if((*mididevicedata->Init)(SysBase)==FALSE){
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return FALSE;
	}

	driver=AllocMem(sizeof(struct Drivers),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(driver!=NULL){
		driver->numports=mididevicedata->NPorts;
		driver->mididevicedata=mididevicedata;
		driver->inMemory=TRUE;
		if(AllocDriverData(driver,mididevicedata,CamdBase)){
			ObtainSemaphore(CB(CamdBase)->CLSemaphore);
			driver->next=CB(CamdBase)->drivers;
			CB(CamdBase)->drivers=driver;
			attached=AttachDriverData(driver,CamdBase);
			if(!attached) CB(CamdBase)->drivers=driver->next;
			ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		}
	}

	if(attached){
		PublishDriverEndpoints(driver,mididevicedata->Name,CamdBase);
	}else{
		if(driver!=NULL){
			FreeDriverData(driver,CamdBase);
			FreeMem(driver,sizeof(struct Drivers));
		}
		if(mididevicedata->Expunge!=NULL) (*mididevicedata->Expunge)();
	}
	ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	return attached;

    AROS_LIBFUNC_EXIT
}
