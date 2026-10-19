/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: 
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH0(LONG, RethinkCAMD,

/*  SYNOPSIS */

/*  LOCATION */
	struct CamdBase *, CamdBase, 36, Camd)

/*  FUNCTION
		Load newly installed drivers from DEVS:Midi.

    INPUTS

    RESULT
		0 on success.

    NOTES
		Must be called from a process.

		Drivers already loaded stay loaded, also when their file has gone.
		A new driver's ports join clusters of the same name that clients
		made before it was installed, and open at once for the links
		waiting there.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
	struct FileInfoBlock fib;
	struct Drivers *driver;
	BPTR lock;
	char path[256];
	BOOL loaded;

	ObtainSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	lock = Lock((CONST_STRPTR)"DEVS:Midi", ACCESS_READ);
	if(!lock) {
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return 0;
	}
	if(!Examine(lock, &fib)) {
		UnLock(lock);
		ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);
		return -1;
	}
	while(ExNext(lock, &fib)) {
		if(fib.fib_DirEntryType > 0)
			continue;
		loaded = FALSE;
		ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
		for(driver = CB(CamdBase)->drivers; driver; driver = driver->next) {
			if(mystrcmp((char *)fib.fib_FileName,
			            driver->mididevicedata->Name)) {
				loaded = TRUE;
				break;
			}
		}
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		if(!loaded) {
			mysprintf(CamdBase, path, "DEVS:Midi/%s", fib.fib_FileName);
			LoadDriver(path, CamdBase);
		}
	}
	UnLock(lock);
	ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);

	return 0;

   AROS_LIBFUNC_EXIT
}
