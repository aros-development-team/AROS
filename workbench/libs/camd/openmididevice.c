/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: 
*/

#include <aros/macros.h>
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
		CloseMidiDevice()

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Drivers *driver;

#ifndef __amigaos4__
	BPTR seglist,seg;
	struct MidiDeviceData *mididevicedata;


	STRPTR addr;
	ULONG size;
	ULONG skip;

	seg=seglist=LoadSeg(name);

	if(seglist==BNULL) return NULL;

// The code here is partly taken from AROS/rom/dos/lddemon.c - LDInit()

	while(seg!=BNULL){
		addr=AROS_SEG_DATA(seg);
		size=AROS_SEG_DATALEN(seg);

		/* A module loaded into one arena (ELF_MODULE_ARENA) has hunks whose
		   stored size is 0: their memory belongs to the container hunk at
		   the end of the list, which is scanned instead. */
		if(size==0){
			seg=AROS_SEG_NEXT(seg);
			continue;
		}

		/* MidiDeviceData is aligned in memory, but a hunk's payload need
		   not hold it at the start: the arena container's data holds
		   sections placed at aligned addresses. Stepping from an unaligned
		   start would never meet the structure, so start at the first
		   aligned address. */
		skip=AROS_ROUNDUP2((IPTR)addr,AROS_PTRALIGN)-(IPTR)addr;
		if(size<skip){
			seg=AROS_SEG_NEXT(seg);
			continue;
		}
		addr+=skip;
		size-=skip;

		while(size>=sizeof(struct MidiDeviceData)){
			mididevicedata=(struct MidiDeviceData *)addr;
			if
			  (
			   /* Do some tests to check that we have got a correct mididevicedata.
			      It's not failproof, but the chance for this to fail should be small.
			      */
			   mididevicedata->Magic==MDD_Magic && 		//Hopefully, this one should only succeed once.
			   mididevicedata->Name!=NULL &&
			   mididevicedata->Init!=NULL &&
			   isPointerInSeglist(mididevicedata->Init,seglist,4) &&			   
    	    	    #ifndef __AROS__
			   (((ULONG)(mididevicedata->Init)&(AROS_PTRALIGN-1))==0) &&
    	    	    #endif
			   isPointerInSeglist(
					      mididevicedata->Name,
					      seglist,mystrlen(findonlyfilename(name))
					      )
			   &&
			   mystrcmp(findonlyfilename(name),mididevicedata->Name)==TRUE &&
			   (
			    mididevicedata->Expunge==NULL  ||
			    (
			     isPointerInSeglist(mididevicedata->Expunge,seglist,4)
    	    	    #ifndef __AROS__			     
			     && (((ULONG)(mididevicedata->Expunge)&(AROS_PTRALIGN-1))==0)									       
    	    	    #endif			     
			     )
			    )
			   &&
			   (
			    mididevicedata->OpenPort==NULL ||
			    (
    	    	    #ifndef __AROS__			    
			     (((ULONG)(mididevicedata->OpenPort)&(AROS_PTRALIGN-1))==0) &&
		    #endif
			     isPointerInSeglist(mididevicedata->OpenPort,seglist,4)
			     )
			    )
			   &&
			   (
			    mididevicedata->ClosePort==NULL ||
			    (
    	    	    #ifndef __AROS__			    
			     (((ULONG)(mididevicedata->ClosePort)&(AROS_PTRALIGN-1))==0) &&
		    #endif
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
			size-=AROS_PTRALIGN;
			addr+=AROS_PTRALIGN;
		}
		seg=AROS_SEG_NEXT(seg);
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
		addr=AROS_SEG_DATA(seglist);
		size=AROS_SEG_DATALEN(seglist);
		if(size<=minsize){
			/* an arena hunk (size 0) or too small: the container covers it */
			seglist=AROS_SEG_NEXT(seglist);
			continue;
		}
		if((STRPTR)pointer>=addr && (STRPTR)pointer<=addr+size-minsize){
			return TRUE;
		}
		seglist=AROS_SEG_NEXT(seglist);
	}

	return FALSE;
}


