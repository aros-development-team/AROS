/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: 
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>

#include "camd_intern.h"

static BOOL DriverIsLoaded(struct CamdBase *CamdBase, CONST_STRPTR name){
	struct Drivers *driver;
	BOOL loaded=FALSE;

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
	for(driver=CB(CamdBase)->drivers;driver;driver=driver->next){
		if(mystrcmp((char *)name,driver->mididevicedata->Name)){
			loaded=TRUE;
			break;
		}
	}
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
	return loaded;
}

/*
	Loads the drivers in DEVS:Midi that are not loaded yet.
	Must run in a process.
*/
LONG ScanDrivers(struct CamdBase *CamdBase){
	struct FileInfoBlock fib;
	BPTR lock;
	char path[256];
	UBYTE ignored[40];
	LONG result=0;

	if(ParsePatternNoCase((CONST_STRPTR)"#?.(info|dbg)",ignored,sizeof(ignored))<0)
		return -1;

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
		if(fib.fib_DirEntryType > 0 ||
			MatchPatternNoCase(ignored,(CONST_STRPTR)fib.fib_FileName))
			continue;
		if(!DriverIsLoaded(CamdBase,(CONST_STRPTR)fib.fib_FileName)) {
			mysprintf(CamdBase, path, "DEVS:Midi/%s", fib.fib_FileName);
			LoadDriver(path, CamdBase);
			if(!DriverIsLoaded(CamdBase,(CONST_STRPTR)fib.fib_FileName))
				result=-1;
		}
	}
	UnLock(lock);
	ReleaseSemaphore(&CB(CamdBase)->DriverScanSemaphore);

	return result;
}

#ifndef __amigaos4__
/*
	CAMD loads new drivers by itself. The wait process is told when
	DEVS:Midi changes and starts a short-lived scanner, so a class that
	writes a driver for a device (USB, Bluetooth) never opens camd.library.
	The wait process does not scan itself: callers of CamdWait() may hold
	the locks a scan needs.
*/
static struct SignalSemaphore scansemaphore;
static struct CamdBase *scanbase;
static volatile BOOL scanrunning, scanagain, scanclosing;
static volatile ULONG scannerid;

static SAVEDS void ScannerProc(void){
	struct CamdBase *CamdBase=scanbase;
	BOOL again;
	LONG result;
	ULONG attempts=20;

	ObtainSemaphore(&scansemaphore);
	scannerid=GetETaskID(FindTask(NULL));
	ReleaseSemaphore(&scansemaphore);

	do{
		Delay(25);		/* let whoever writes the driver finish the file */
		ObtainSemaphore(&scansemaphore);
		scanagain=FALSE;
		ReleaseSemaphore(&scansemaphore);

		result=ScanDrivers(CamdBase);

		ObtainSemaphore(&scansemaphore);
		/* A creation notification may precede the last write, and not
		   every handler sends a closing notification. Retry failures for
		   ten seconds; a new notification starts a fresh retry window.
		   Invalid files do not keep a scanner resident indefinitely. */
		if(scanagain) attempts=20;
		else attempts--;
		again=(!scanclosing && (scanagain || (result!=0 && attempts!=0)));
		if(!again) scanrunning=FALSE;
		ReleaseSemaphore(&scansemaphore);
	}while(again);
}

void InitDriverWatch(struct CamdBase *CamdBase){
	InitSemaphore(&scansemaphore);
	scanbase=CamdBase;
	scanrunning=FALSE;
	scanagain=FALSE;
	scanclosing=FALSE;
	scannerid=0;
}

/* Called by the wait process when DEVS:Midi changes. */
void DriversChanged(void){
	struct Process *process;

	ObtainSemaphore(&scansemaphore);
	if(scanclosing==FALSE){
		if(scanrunning){
			scanagain=TRUE;
		}else{
			scanrunning=TRUE;
			process=CreateNewProcTags(
				NP_Entry, (IPTR) ScannerProc,
				NP_Name,  (IPTR) "CAMD driver scan",
				NP_Priority,0,
				TAG_END
			);
			if(process==NULL) scanrunning=FALSE;
		}
	}
	ReleaseSemaphore(&scansemaphore);
}

/*
	No new scan starts, and a running one is waited for: it loads drivers
	into lists UninitCamd() is about to free, and runs this library's code.
	Exec removes the scanner only after it has left that code.
*/
void StopDriverWatch(void){
	ObtainSemaphore(&scansemaphore);
	scanclosing=TRUE;
	ReleaseSemaphore(&scansemaphore);

	while(scanrunning) Delay(1);
	while(scannerid!=0 && FindTaskByPID(scannerid)!=NULL) Delay(1);
}
#endif

/*****************************************************************************

    NAME */

	AROS_LH0(LONG, RethinkCAMD,

/*  SYNOPSIS */

/*  LOCATION */
	struct CamdBase *, CamdBase, 36, Camd)

/*  FUNCTION
		Load newly installed drivers from DEVS:Midi.
		Directories, icons (*.info) and debug files (*.dbg) are ignored.

    INPUTS

    RESULT
		0 when the scan completed and every candidate loaded; -1 if a
		directory scan or a driver load failed.

    NOTES
		Must be called from a process.

		Drivers already loaded stay loaded, also when their file has gone.
		A new driver's ports join clusters of the same name that clients
		made before it was installed, and open at once for the links
		waiting there.

		Where the filesystem supports notification, CAMD does this by
		itself shortly after DEVS:Midi changes; calling this is then only
		needed to load a driver at once.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	return ScanDrivers(CamdBase);

   AROS_LIBFUNC_EXIT
}
