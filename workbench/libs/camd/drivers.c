/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "camd_intern.h"


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

	driverdata->midiportdata=(*driverdata->mididevicedata->OpenPort)(
		driverdata->mididevicedata,
		driverdata->portnum,
		Transmitter,
		Receiver,
		driverdata
	);
	if(driverdata->midiportdata==NULL){
		EndReceiverProc(driverdata,CamdBase);
		if(ErrorCode!=NULL){
			*ErrorCode=CME_NoUnit(driverdata->portnum);
		}
		return FALSE;
	}

	return TRUE;
}


void CloseDriver(struct DriverData *driverdata,struct CamdBase *CamdBase){
	D(bug("closing driver %lx\n",driverdata));
	(*driverdata->mididevicedata->ClosePort)(
		driverdata->mididevicedata,
		driverdata->portnum
	);
	EndReceiverProc(driverdata,CamdBase);
	D(bug("finished closing driver %lx\n",driverdata));
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

	driver->mididevicedata=mididevicedata;

	nports=mididevicedata->NPorts;
	if(nports==0) return FALSE;

	driver->numports=nports;

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
		D(bug("isOutOpen=FALSE\n"));
		driverdata->isInOpen=FALSE;
	}

	for(lokke=0;lokke<nports;lokke++){
		driverdata=driver->driverdatas[lokke];
		D(bug("driverdata: %lx\n",driverdata));
		AddClusterSender(&driverdata->incluster->cluster,&driverdata->innode,NULL,CamdBase);
		AddClusterReceiver(&driverdata->outcluster->cluster,&driverdata->outnode,NULL,CamdBase);
	}

	return TRUE;
}


void FreeDriver(struct Drivers *driver,
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
	UnLoadSeg(driver->seglist);
	FreeMem(driver,sizeof(struct Drivers));
}

void LoadDriver(char *name,
	struct CamdBase *CamdBase
){
	BPTR seglist;
	struct SegmentSak *myseglist;
	struct Drivers *driver;
	struct MidiDeviceData *mididevicedata;
	seglist=LoadSeg(name);

	myseglist=BADDR(seglist);

	mididevicedata=(struct MidiDeviceData *)&myseglist->mididevicedata;

	if(mididevicedata->Magic!=MDD_Magic){
		UnLoadSeg(seglist);
		return;
	}

	driver=AllocMem(sizeof(struct Drivers),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
	if(driver==NULL){
		UnLoadSeg(seglist);
		return;
	}

	driver->seglist=seglist;

	if(AllocDriverData(driver,mididevicedata,CamdBase)==FALSE){
		FreeDriver(driver,CamdBase);
		return;
	}

	if(CB(CamdBase)->drivers!=NULL){
		driver->num=CB(CamdBase)->drivers->num+1;
	}else{
		driver->num=0;
	}

	driver->next=CB(CamdBase)->drivers;
	CB(CamdBase)->drivers=driver;

	(*mididevicedata->Init)();

}


