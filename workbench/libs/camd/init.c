/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/camd.h>

#include "camd_intern.h"


BOOL InitCamd(struct CamdBase *CamdBase){
	struct FileInfoBlock fib;
	BPTR lock;
	char temp[256];

	CB(CamdBase)->CLSemaphore=AllocMem(sizeof(struct SignalSemaphore),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
	if(CB(CamdBase)->CLSemaphore==NULL){
		return FALSE;
	}
	InitSemaphore(CB(CamdBase)->CLSemaphore);
	NEWLIST(&CB(CamdBase)->mymidinodes);
	NEWLIST(&CB(CamdBase)->midiclusters);
	if(InitCamdTimer()==FALSE) return FALSE;
	lock=Lock("devs:Midi",ACCESS_READ);
	if(lock==NULL){
		return TRUE;
	}
	if(Examine(lock,&fib)==FALSE){
		UnLock(lock);
		return TRUE;
	}
	while(ExNext(lock,&fib)!=FALSE){
		mysprintf(CamdBase,temp,"devs:Midi/%s",fib.fib_FileName);
		LoadDriver(temp,CamdBase);
	}
	UnLock(lock);
	return TRUE;
}

void UninitCamd(struct CamdBase *CamdBase){
	struct Drivers *driver=CB(CamdBase)->drivers,*temp2;
	struct Node *node,*temp;
	struct MidiCluster *midicluster;
	struct DriverData *driverdata;

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

	UninitCamdTimer();
}

