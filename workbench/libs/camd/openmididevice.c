/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include <aros/debug.h>

BOOL isPointerInSeglist(APTR pointer,BPTR seglist);

/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiDeviceData *, OpenMidiDevice,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 34, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		CloseMidiDevice

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	BPTR seglist,seg;
	struct MidiDeviceData *mididevicedata;
	struct Drivers *driver;

	STRPTR addr;
	ULONG size;

	seg=seglist=LoadSeg(name);

	D(bug("seglist loaded okey? %lx\n",seg));

	if(seglist==NULL) return NULL;

	D(bug("seglist loaded okey\n"));

// The code here is partly taken from AROS/rom/dos/lddemon.c - LDInit()

	while(seg!=NULL){
	  D(bug("checking a new seglist\n"));
		addr=(STRPTR)((LONG)BADDR(seg)-sizeof(ULONG));
		size=*(ULONG *)addr;

		for(
			addr+=sizeof(BPTR)+sizeof(ULONG),
			  size-=sizeof(BPTR)-sizeof(ULONG);		// Is this a bug? (- -> + ?)
			size>=sizeof(struct MidiDeviceData);
			size-=AROS_PTRALIGN,addr+=AROS_PTRALIGN
		){
			mididevicedata=(struct MidiDeviceData *)addr;
			if(
				/* Do some tests to check that we have got a correct mididevicedata.
				   Its not failproof, but the chance for this to fail should be small.
				*/
				mididevicedata->Magic==MDD_Magic && 		//Hopefully, this one should only succeed once.
				mididevicedata->Name!=NULL &&
				mididevicedata->Init!=NULL &&
				mididevicedata->Expunge!=NULL &&
				mididevicedata->OpenPort!=NULL &&
				mididevicedata->ClosePort!=NULL &&
				(((ULONG)(mididevicedata->Init)&(AROS_PTRALIGN-1))==0) &&
				(((ULONG)(mididevicedata->Expunge)&(AROS_PTRALIGN-1))==0) &&
				(((ULONG)(mididevicedata->OpenPort)&(AROS_PTRALIGN-1))==0) &&
				(((ULONG)(mididevicedata->ClosePort)&(AROS_PTRALIGN-1))==0) &&
//				mididevicedata->NPorts>0 &&	// No, the driver must have the possibility to set number of ports at the init-routine.
				isPointerInSeglist(mididevicedata->Name,seglist) &&
				isPointerInSeglist(mididevicedata->Init,seglist) &&
				isPointerInSeglist(mididevicedata->Expunge,seglist) &&
				isPointerInSeglist(mididevicedata->OpenPort,seglist) &&
				isPointerInSeglist(mididevicedata->ClosePort,seglist) &&
				(
					mididevicedata->IDString==NULL ||
					isPointerInSeglist(mididevicedata->Name,seglist)
				)
				
			){
				driver=AllocMem(sizeof(struct Drivers),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
				if(driver==NULL){
					UnLoadSeg(seglist);
					return NULL;
				}
				driver->seglist=seglist;
				driver->mididevicedata=mididevicedata;
				driver->numports=mididevicedata->NPorts;

				ObtainSemaphore(CB(CamdBase)->CLSemaphore);
					driver->next=CB(CamdBase)->drivers;
					CB(CamdBase)->drivers=driver;
				ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
				return mididevicedata;
			}
		}
		seg=*(BPTR *)BADDR(seg);
	}

	UnLoadSeg(seglist);
	return NULL;

   AROS_LIBFUNC_EXIT

}

BOOL isPointerInSeglist(APTR pointer,BPTR seglist){
	STRPTR addr;
	ULONG size;

	while(seglist!=NULL){
		addr=(STRPTR)((LONG)BADDR(seglist)-sizeof(ULONG));
		size=*(ULONG *)addr;
		addr+=sizeof(BPTR)+sizeof(ULONG);
		size-=sizeof(BPTR)-sizeof(ULONG);
		if((STRPTR)pointer>=addr && (STRPTR)pointer<=addr+size){
			return TRUE;
		}
		seglist=*(BPTR *)BADDR(seglist);
	}

	return FALSE;
}

