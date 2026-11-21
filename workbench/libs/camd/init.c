/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: 
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/camd.h>
#include <proto/utility.h>

#include "camd_intern.h"


#ifdef __amigaos4__
BOOL InitCamd(struct CamdIFace *ICamd){
    CAMD_LIBBASE_DECL(struct CamdBase *,CamdBase)
#else
BOOL InitCamd(struct CamdBase *CamdBase){
#endif
	struct FileInfoBlock fib;
	BPTR lock;
	char temp[256];

	CB(CamdBase)->CLSemaphore=AllocMem(sizeof(struct SignalSemaphore),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(CB(CamdBase)->CLSemaphore==NULL){
		return FALSE;
	}
	InitSemaphore(CB(CamdBase)->CLSemaphore);
	InitSemaphore(&CB(CamdBase)->DriverScanSemaphore);
	InitSemaphore(&CB(CamdBase)->NotifySemaphore);
#ifndef __amigaos4__
	NEWLIST(&CB(CamdBase)->mymidinodes);
	NEWLIST(&CB(CamdBase)->midiclusters);
	NEWLIST(&CB(CamdBase)->clusnotifynodes);
	NEWLIST((struct List *)&CB(CamdBase)->clusterwatches);
#else // ???
	NEWLIST(CB(CamdBase)->mymidinodes);
	NEWLIST(CB(CamdBase)->midiclusters);
	NEWLIST(CB(CamdBase)->clusnotifynodes);
	NEWLIST(CB(CamdBase)->clusterwatches);
#endif
#ifndef __amigaos4__
	InitDriverWatch(CamdBase);
#endif
	if(InitCamdTimer()==FALSE) goto fail;
	if(InitCamdClock()==FALSE){
		UninitCamdTimer();
		goto fail;
	}
	lock=Lock("devs:Midi",ACCESS_READ);
	if(lock==0){
		return TRUE;
	}
	if(Examine(lock,&fib)==FALSE){
		UnLock(lock);
		return TRUE;
	}
	while(ExNext(lock,&fib)!=FALSE){
		mysprintf(CamdBase,temp,"devs:Midi/%s",fib.fib_FileName);
#ifdef __amigaos4__
		LoadDriver(temp,ICamd);
#else
		LoadDriver(temp,CamdBase);
#endif
	}
	UnLock(lock);
	return TRUE;

fail:
#ifndef __amigaos4__
	StopDriverWatch();
#endif
	FreeMem(CB(CamdBase)->CLSemaphore,sizeof(struct SignalSemaphore));
	CB(CamdBase)->CLSemaphore=NULL;
	return FALSE;
}

#ifdef __amigaos4__
void UninitCamd(struct CamdIFace *ICamd){
    CAMD_LIBBASE_DECL(struct CamdBase *,CamdBase)
#else
void UninitCamd(struct CamdBase *CamdBase){
#endif
	struct Drivers *driver=CB(CamdBase)->drivers,*temp2;
	struct Node *node,*temp;
	struct MidiCluster *midicluster;
	struct DriverData *driverdata;

#ifndef __amigaos4__
	StopDriverWatch();
#endif

	if( ! IsListEmpty(&CB(CamdBase)->midiclusters)){
		node=CB(CamdBase)->midiclusters.lh_Head;
		while(node->ln_Succ!=NULL){
			temp=node->ln_Succ;
			midicluster=(struct MidiCluster *)node;
			driverdata=FindSenderDriverInCluster(midicluster);
			if(driverdata==NULL){
				driverdata=FindReceiverDriverInCluster(midicluster);
			}
			if(driverdata!=NULL){
				if(driverdata->isInOpen==TRUE || driverdata->isOutOpen==TRUE){
					driverdata->isInOpen=FALSE;
					driverdata->isOutOpen=FALSE;
					CloseDriver(driverdata,CamdBase);
				}
			}else{
				RemoveCluster((struct MidiCluster *)node,CamdBase);	//Clients should have do this, but..
			}
			node=temp;
		}
	}

	while(driver!=NULL){
		temp2=driver->next;
		(*driver->mididevicedata->Expunge)();
		FreeDriverData(driver,CamdBase);
		CloseMidiDevice(driver->mididevicedata);
		driver=temp2;
	}

	if(CB(CamdBase)->CLSemaphore!=NULL)
	  FreeMem(CB(CamdBase)->CLSemaphore,sizeof(struct SignalSemaphore));

	/* Watches a client did not end. */
	while((node=RemHead((struct List *)&CB(CamdBase)->clusterwatches))){
		FreeMem(node,sizeof(struct ClusterWatch));
	}

	UninitCamdClock();
	UninitCamdTimer();
}

