/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/
#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, CloseMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(struct MidiDeviceData *, mididevicedata, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 35, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		OpenMidiDevice()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Drivers *driver,*temp;

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);

	driver=FindPrevDriverForMidiDeviceData(mididevicedata,CamdBase);

	if(driver==NULL){
		driver=CB(CamdBase)->drivers;
		CB(CamdBase)->drivers=CB(CamdBase)->drivers->next;
	}else{
		temp=driver->next;
		driver->next=driver->next->next;
		driver=temp;
	}

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	UnLoadSeg(driver->seglist);
	FreeMem(driver,sizeof(struct Drivers));

   AROS_LIBFUNC_EXIT
}

