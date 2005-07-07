/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>
#ifdef __amigaos4__
#  include <proto/CamdDriver.h>
#endif
#include "camd_intern.h"

#ifdef __AROS__
#  undef DEBUG
#  define DEBUG 1
#  include <aros/debug.h>
#endif

BOOL isPointerInSeglist(APTR pointer,BPTR seglist,ULONG minsize);

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

	struct Drivers *driver;

#ifndef __amigaos4__
	BPTR seglist,seg;
	struct MidiDeviceData *mididevicedata;


	STRPTR addr;
	ULONG size;

	seg=seglist=LoadSeg(name);

	if(seglist==NULL) return NULL;

// The code here is partly taken from AROS/rom/dos/lddemon.c - LDInit()

	while(seg!=NULL){
		addr=(STRPTR)((LONG)BADDR(seg)-sizeof(ULONG));
		size=*(ULONG *)addr;

		for(
			addr+=sizeof(BPTR)+sizeof(ULONG),
			  size-=sizeof(BPTR)+sizeof(ULONG);		// Is this a bug? (- -> + ?)
			size>=sizeof(struct MidiDeviceData);
			size-=AROS_PTRALIGN,addr+=AROS_PTRALIGN
		){
			mididevicedata=(struct MidiDeviceData *)addr;
			if
			  (
			   /* Do some tests to check that we have got a correct mididevicedata.
			      Its not failproof, but the chance for this to fail should be small.
			      */
			   mididevicedata->Magic==MDD_Magic && 		//Hopefully, this one should only succeed once.
			   mididevicedata->Name!=NULL &&
			   mididevicedata->Init!=NULL &&
			   isPointerInSeglist(mididevicedata->Init,seglist,4) &&			   
			   (((ULONG)(mididevicedata->Init)&(AROS_PTRALIGN-1))==0) &&
			   isPointerInSeglist(
					      mididevicedata->Name,
					      seglist,mystrlen(findonlyfilename(name))
					      )
			   &&
			   mystrcmp(findonlyfilename(name),mididevicedata->Name)==TRUE &&
			   (
			    mididevicedata->Expunge==NULL  ||
			    (
			     isPointerInSeglist(mididevicedata->Expunge,seglist,4) && 				  
			     (((ULONG)(mididevicedata->Expunge)&(AROS_PTRALIGN-1))==0)									       
			     )
			    )
			   &&
			   (
			    mididevicedata->OpenPort==NULL ||
			    (
			     (((ULONG)(mididevicedata->OpenPort)&(AROS_PTRALIGN-1))==0) &&
			     isPointerInSeglist(mididevicedata->OpenPort,seglist,4)
			     )
			    )
			   &&
			   (
			    mididevicedata->ClosePort==NULL ||
			    (
			     (((ULONG)(mididevicedata->ClosePort)&(AROS_PTRALIGN-1))==0) &&
			     isPointerInSeglist(mididevicedata->ClosePort,seglist,4)
			     )
			    )
			   &&
			   mididevicedata->IDString!=NULL &&
			   isPointerInSeglist(mididevicedata->IDString,seglist,4)

			   )
			  {

			    driver=AllocMem(sizeof(struct Drivers),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
			    if(driver==NULL){
			      UnLoadSeg(seglist);
			      return NULL;
			    }
			    driver->seglist=seglist;
			    driver->mididevicedata=mididevicedata;
			    
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
#else

    struct CamdDriverIFace  *ICamdDriver;
    struct Library *CamdDriverBase = (struct CamdDriverLibrary *) OpenLibrary( name, 0 );

    if( CamdDriverBase == NULL)
    {
//        DebugPrintF("Failed to load %s\n", name);
        return NULL;
    }
    
    if ((ICamdDriver = (struct CamdDriverIFace *) GetInterface((struct Library *) CamdDriverBase, "main", 1, NULL)) == NULL)
    {
//       DebugPrintF("Failed to load interface for %s\n", name);
       CloseLibrary(CamdDriverBase);
       return NULL;
    }

			    
                driver=AllocMem(sizeof(struct Drivers),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
			    if(driver==NULL){
                  CloseLibrary(CamdDriverBase);
			      return NULL;
			    }
			    driver->mididevicedata = AllocMem(sizeof(struct MidiDeviceData), MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
                driver->mididevicedata->Name = ICamdDriver->GetMidiDeviceData()->Name;
                driver->mididevicedata->IDString = ICamdDriver->GetMidiDeviceData()->IDString;
                driver->mididevicedata->Version = ICamdDriver->GetMidiDeviceData()->Version;
                driver->mididevicedata->Revision = ICamdDriver->GetMidiDeviceData()->Revision;
                driver->mididevicedata->NPorts = ICamdDriver->GetMidiDeviceData()->NPorts;
                driver->mididevicedata->Flags = ICamdDriver->GetMidiDeviceData()->Flags;
                driver->mididevicedata->CamdDriverBase = CamdDriverBase;
                driver->mididevicedata->ICamdDriver = ICamdDriver;
                

			    ObtainSemaphore(CB(CamdBase)->CLSemaphore);
			    driver->next=CB(CamdBase)->drivers;
			    CB(CamdBase)->drivers=driver;
			    ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
                
			    return driver->mididevicedata;
    

#endif

   AROS_LIBFUNC_EXIT

}

BOOL isPointerInSeglist(APTR pointer,BPTR seglist,ULONG minsize){
	STRPTR addr;
	ULONG size;

	while(seglist!=0){
		addr=(STRPTR)((LONG)BADDR(seglist)-sizeof(ULONG));
		size=*(ULONG *)addr;
		addr+=sizeof(BPTR)+sizeof(ULONG);
		size-=sizeof(BPTR)+sizeof(ULONG);
		if((STRPTR)pointer>=addr && (STRPTR)pointer<=addr+size-minsize){
			return TRUE;
		}
		seglist=*(BPTR *)BADDR(seglist);
	}

	return FALSE;
}


