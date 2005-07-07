/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/camd.h>
#include <proto/utility.h>

#ifdef __amigaos4__
#  include <proto/CamdDriver.h>
#  include <exec/emulation.h>
#endif


#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

BOOL OpenDriver(struct DriverData *driverdata,ULONG *ErrorCode,struct CamdBase *CamdBase){
	if(
		CreateReceiverProc(
			driverdata,
			driverdata->mididevicedata->Name,
			driverdata->portnum,
			ErrorCode,
			CamdBase
		)==FALSE
	){
		return FALSE;
	}

#ifndef __amigaos4__
	driverdata->midiportdata=(*driverdata->mididevicedata->OpenPort)(
#else
	driverdata->midiportdata= driverdata->mididevicedata->ICamdDriver->OpenPort(
#endif
		driverdata->mididevicedata,
		driverdata->portnum,
		(ULONG (* ASM)(APTR REG(a2)))Transmitter,
		(void (* ASM)(UWORD REG(d0),APTR REG(a2))) Receiver,
		driverdata
	);

	if(driverdata->midiportdata==NULL){
		D(bug("camd.library: drivers.c/OpenDriver. (*OpenPort) failed...\n"));
		EndReceiverProc(driverdata,CamdBase);
		if(ErrorCode!=NULL){
			*ErrorCode=CME_NoUnit(driverdata->portnum);
		}
		return FALSE;
	}

	return TRUE;
}


void CloseDriver(struct DriverData *driverdata,struct CamdBase *CamdBase){
#ifndef __amigaos4__
	(*driverdata->mididevicedata->ClosePort)(
#else
        driverdata->mididevicedata->ICamdDriver->ClosePort(
#endif
                driverdata->mididevicedata,
		driverdata->portnum
	);
	EndReceiverProc(driverdata,CamdBase);
}

BOOL AllocDriverData(
	struct Drivers *driver,
	struct MidiDeviceData *mididevicedata,
	struct CamdBase *CamdBase
){
	ULONG nports;
	LONG lokke;

	struct DriverData *driverdata;

	struct MidiCluster *cluster;
	char nametemp[256];

	nports=driver->numports;

	driver->driverdatas=AllocMem((ULONG)(sizeof(struct DriverData *)*nports),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(driver->driverdatas==NULL){
		return FALSE;
	}

	for(lokke=0;lokke<nports;lokke++){
		driverdata=AllocMem(sizeof(struct DriverData),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
		if(driverdata==NULL){
			return FALSE;
		}
		driver->driverdatas[lokke]=driverdata;

		driverdata->innode.ln_Type=NT_USER-MLTYPE_NTypes;
		driverdata->innode.ln_Pri=127;					// Should be set lower?

		driverdata->outnode.ln_Type=NT_USER-MLTYPE_NTypes;
		driverdata->outnode.ln_Pri=127;					// Should be set lower?

		driverdata->mididevicedata=mididevicedata;

		driverdata->buffer=AllocVec(sizeof(ULONG)*OUTBUFFERSIZE,MEMF_ANY|MEMF_PUBLIC);
		if(driverdata->buffer==NULL){
			return FALSE;
		}

		driverdata->buffercurrsend=driverdata->buffer;
		driverdata->buffercurr=driverdata->buffer;
		driverdata->bufferend=driverdata->buffer+OUTBUFFERSIZE;
		driverdata->sendpos=0;

		driverdata->buffer_rt=AllocVec(sizeof(UBYTE)*OUTBUFFERSIZE_RT,MEMF_ANY|MEMF_PUBLIC);
		if(driverdata->buffer_rt==NULL){
			return FALSE;
		}
		driverdata->buffercurrsend_rt=driverdata->buffer_rt;
		driverdata->buffercurr_rt=driverdata->buffer_rt;
		driverdata->bufferend=driverdata->buffer+OUTBUFFERSIZE_RT;


		driverdata->status=0;
		driverdata->portnum=lokke;

		driverdata->Input_Treat=Receiver_init;

		driverdata->re_start=AllocVec((ULONG)(sizeof(UWORD)*RECEIVERPROCBUFFERSIZE),MEMF_ANY|MEMF_PUBLIC);
		if(driverdata->re_start==NULL){
			return FALSE;
		}
		driverdata->re_write=driverdata->re_start;
		driverdata->re_end=driverdata->re_start+RECEIVERPROCBUFFERSIZE;
		driverdata->re_read=driverdata->re_start;


		mysprintf(CamdBase,nametemp,"%s.in.%ld",mididevicedata->Name,lokke);
		if((cluster=NewCluster(nametemp,CamdBase))==FALSE){
			return FALSE;
		}
		driverdata->incluster=(struct MyMidiCluster *)cluster;

		mysprintf(CamdBase,nametemp,"%s.out.%ld",mididevicedata->Name,lokke);
		if((cluster=NewCluster(nametemp,CamdBase))==FALSE){
			return FALSE;
		}
		driverdata->outcluster=(struct MyMidiCluster *)cluster;

		InitSemaphore(&driverdata->sendsemaphore);
		InitSemaphore(&driverdata->sysexsemaphore);

		driverdata->isOutOpen=FALSE;
		driverdata->isInOpen=FALSE;
	}

	for(lokke=0;lokke<nports;lokke++){
		driverdata=driver->driverdatas[lokke];
		AddClusterSender(&driverdata->incluster->cluster,&driverdata->innode,NULL,CamdBase);
		AddClusterReceiver(&driverdata->outcluster->cluster,&driverdata->outnode,NULL,CamdBase);
	}

	return TRUE;
}


void FreeDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
){
	struct DriverData *driverdata;
	ULONG lokke;

	if(driver->driverdatas!=NULL){
		for(lokke=0;lokke<driver->numports;lokke++){
			driverdata=driver->driverdatas[lokke];
			RemoveCluster(&driverdata->incluster->cluster,CamdBase);
			RemoveCluster(&driverdata->outcluster->cluster,CamdBase);
		}
	}

	if(driver->driverdatas!=NULL){
		for(lokke=0;lokke<driver->numports;lokke++){
			driverdata=driver->driverdatas[lokke];
			if(driverdata!=NULL){
				if(driverdata->buffer!=NULL){
					FreeVec(driverdata->buffer);
				}
				if(driverdata->buffer_rt!=NULL){
					FreeVec(driverdata->buffer_rt);
				}
				if(driverdata->re_start!=NULL){
					FreeVec(driverdata->re_start);
				}
			}
		}
		FreeMem(driver->driverdatas,sizeof(struct DriverData *)*driver->numports);
	}
}

struct Drivers *FindPrevDriverForMidiDeviceData(
	struct MidiDeviceData *mididevicedata,
	struct CamdBase *CamdBase
){
	struct Drivers *driver,*temp=NULL;

	driver=CB(CamdBase)->drivers;

	while(driver->mididevicedata!=mididevicedata){
		temp=driver;
		driver=driver->next;
	}

	return temp;
}

#ifdef __amigaos4__
void LoadDriver(char *name,
	struct CamdIFace *ICamd
){
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)
#else
void LoadDriver(char *name,
	struct CamdBase *CamdBase
){
#endif
	struct Drivers *driver = NULL;
	struct MidiDeviceData *mididevicedata = NULL;

	D(bug("camd.library: drivers.c/LoadDriver - trying to open %s..\n",name));

	mididevicedata=OpenMidiDevice(name);
	D(bug("camd.library: drivers.c/LoadDriver - It was%s a success..\n",mididevicedata==NULL?" not":""));

	if(mididevicedata==NULL) return;

	if((mididevicedata->Flags&1)==0){
#ifdef __AROS__
		D(bug("%s: mididevicedata->Flags&1==0 is not not supported for AROS!\n",name));
		CloseMidiDevice(mididevicedata);
		return;
#else
		D(bug("%s: old camd driver format. (Supported)\n",name));
#endif
	}

#ifndef __amigaos4__
	if((*mididevicedata->Init)(SysBase)==FALSE){
		CloseMidiDevice(mididevicedata);
		return;
	}
#else
#endif

	//D(bug("%s %s %u %u\n", mididevicedata->Name, mididevicedata->IDString, mididevicedata->NPorts,mididevicedata->Flags));

	driver=FindPrevDriverForMidiDeviceData(mididevicedata,CamdBase);
	if(driver==NULL){
		driver=CB(CamdBase)->drivers;
	}else{
		driver=driver->next;
	}

	driver->numports=mididevicedata->NPorts;

	if(AllocDriverData(driver,mididevicedata,CamdBase)==FALSE){
		FreeDriverData(driver,CamdBase);
		CloseMidiDevice(mididevicedata);
		return;
	}

}


