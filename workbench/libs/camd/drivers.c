/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: 
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


		InitSemaphore(&driverdata->sendsemaphore);
		InitSemaphore(&driverdata->sysexsemaphore);

		driverdata->isOutOpen=FALSE;
		driverdata->isInOpen=FALSE;
	}

	return TRUE;
}


/*
	Finds the cluster called name, or creates it.
	CLSemaphore must be exclusive obtained.
*/
static struct MyMidiCluster *GetDriverCluster(char *name,struct CamdBase *CamdBase){
	struct MidiCluster *cluster;

	cluster=(struct MidiCluster *)FindName(&CB(CamdBase)->midiclusters,name);
	if(cluster==NULL){
		cluster=NewCluster(name,CamdBase);
	}
	return (struct MyMidiCluster *)cluster;
}


/*
	Removes a port's nodes from its clusters, closes the port, and removes
	the clusters no client is linked to any more. A client's link stays in
	its cluster and waits there, as it would for any other cluster.
	CLSemaphore must be exclusive obtained.
*/
static void DetachPort(struct DriverData *driverdata,struct CamdBase *CamdBase){
	struct MyMidiCluster *incluster=driverdata->incluster;
	struct MyMidiCluster *outcluster=driverdata->outcluster;

	if(driverdata->isInOpen==TRUE || driverdata->isOutOpen==TRUE){
		driverdata->isInOpen=FALSE;
		driverdata->isOutOpen=FALSE;
		CloseDriver(driverdata,CamdBase);
	}

	if(incluster!=NULL){
		Remove(&driverdata->innode);
		if(IsListEmpty(&incluster->cluster.mcl_Receivers) &&
		   IsListEmpty(&incluster->cluster.mcl_Senders)){
			RemoveCluster(&incluster->cluster,CamdBase);
		}
	}
	if(outcluster!=NULL){
		ObtainSemaphore(&outcluster->semaphore);
		Remove(&driverdata->outnode);
		ReleaseSemaphore(&outcluster->semaphore);
		if(IsListEmpty(&outcluster->cluster.mcl_Receivers) &&
		   IsListEmpty(&outcluster->cluster.mcl_Senders)){
			RemoveCluster(&outcluster->cluster,CamdBase);
		}
	}

	driverdata->incluster=NULL;
	driverdata->outcluster=NULL;
	driverdata->isAttached=FALSE;
}


/*
	Puts the driver's ports in their <name>.in.<n> and <name>.out.<n>
	clusters. A driver loaded by RethinkCAMD() can find a cluster of that
	name already there, made by a client that linked to it before the
	driver was installed. The port joins that cluster, and is opened at
	once if a client is linked to it: such a client does not link again.
	CLSemaphore must be exclusive obtained.
*/
BOOL AttachDriverData(
	struct Drivers *driver,
	struct CamdBase *CamdBase
){
	struct DriverData *driverdata;
	char nametemp[256];
	ULONG lokke;

	for(lokke=0;lokke<driver->numports;lokke++){
		driverdata=driver->driverdatas[lokke];

		mysprintf(CamdBase,nametemp,"%s.in.%ld",driver->mididevicedata->Name,lokke);
		driverdata->incluster=GetDriverCluster(nametemp,CamdBase);
		if(driverdata->incluster==NULL){
			DetachDriverData(driver,CamdBase);
			return FALSE;
		}
		AddClusterSender(&driverdata->incluster->cluster,&driverdata->innode,NULL,CamdBase);

		mysprintf(CamdBase,nametemp,"%s.out.%ld",driver->mididevicedata->Name,lokke);
		driverdata->outcluster=GetDriverCluster(nametemp,CamdBase);
		if(driverdata->outcluster==NULL){
			DetachPort(driverdata,CamdBase);
			DetachDriverData(driver,CamdBase);
			return FALSE;
		}
		AddClusterReceiver(&driverdata->outcluster->cluster,&driverdata->outnode,NULL,CamdBase);

		driverdata->isAttached=TRUE;
	}

	for(lokke=0;lokke<driver->numports;lokke++){
		driverdata=driver->driverdatas[lokke];

		/* Receivers in the in cluster and senders in the out cluster can
		   only be clients' links. If the port does not open now, they keep
		   waiting, and the next link to the cluster tries again. */
		if(IsListEmpty(&driverdata->incluster->cluster.mcl_Receivers)==FALSE ||
		   IsListEmpty(&driverdata->outcluster->cluster.mcl_Senders)==FALSE){
			if(OpenDriver(driverdata,NULL,CamdBase)==TRUE){
				driverdata->isInOpen=!IsListEmpty(&driverdata->incluster->cluster.mcl_Receivers);
				driverdata->isOutOpen=!IsListEmpty(&driverdata->outcluster->cluster.mcl_Senders);
			}else{
				D(bug("camd.library: %s port %ld did not open for its waiting links\n",
				      driver->mididevicedata->Name,(long)lokke));
			}
		}
	}

	return TRUE;
}


/*
	CLSemaphore must be exclusive obtained, or CAMD be expunging.
*/
void DetachDriverData(
	struct Drivers *driver,
	struct CamdBase *CamdBase
){
	struct DriverData *driverdata;
	ULONG lokke;

	if(driver->driverdatas==NULL) return;

	for(lokke=0;lokke<driver->numports;lokke++){
		driverdata=driver->driverdatas[lokke];
		if(driverdata!=NULL && driverdata->isAttached==TRUE){
			DetachPort(driverdata,CamdBase);
		}
	}
}


void FreeDriverData(struct Drivers *driver,
	struct CamdBase *CamdBase
){
	struct DriverData *driverdata;
	ULONG lokke;

	DetachDriverData(driver,CamdBase);

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
				FreeMem(driverdata,sizeof(struct DriverData));
			}
		}
		FreeMem(driver->driverdatas,sizeof(struct DriverData *)*driver->numports);
		driver->driverdatas=NULL;
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
    CAMD_LIBBASE_DECL(struct CamdBase *,CamdBase)
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

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);
	driver=FindPrevDriverForMidiDeviceData(mididevicedata,CamdBase);
	if(driver==NULL){
		driver=CB(CamdBase)->drivers;
	}else{
		driver=driver->next;
	}
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	driver->numports=mididevicedata->NPorts;

	/* Clients may be walking or changing the clusters while RethinkCAMD()
	   loads a driver, so the ports join them under the lock. */
	if(AllocDriverData(driver,mididevicedata,CamdBase)==TRUE){
		BOOL attached;

		ObtainSemaphore(CB(CamdBase)->CLSemaphore);
		attached=AttachDriverData(driver,CamdBase);
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		if(attached==TRUE) return;
	}

	FreeDriverData(driver,CamdBase);
#ifndef __amigaos4__
	if(mididevicedata->Expunge!=NULL) (*mididevicedata->Expunge)();
#endif
	CloseMidiDevice(mididevicedata);
}


