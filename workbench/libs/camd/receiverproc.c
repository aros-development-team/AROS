/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#include <proto/utility.h>

#include "camd_intern.h"


struct DriverData *driverdatatemp=NULL;

SAVEDS void ReceiverFunc(void){
	ULONG sig;
	struct DriverData *driverdata=driverdatatemp;

	driverdata->ReceiverSig=AllocSignal(-1);
	if(driverdata->ReceiverSig==-1){
		driverdata->isReceiverProcessAlive=2;
		return;
	}

	driverdata->isReceiverProcessAlive=1;

	for(;;){
		sig=Wait(
			1L<<driverdata->ReceiverSig |
			SIGBREAKF_CTRL_C
		);

		if(sig&1L<<driverdata->ReceiverSig){
			while(driverdata->unpicked>0){
				Receiver_first(driverdata);
			}
		}
		if(sig&SIGBREAKF_CTRL_C) break;
	}

	FreeSignal(driverdata->ReceiverSig);

	driverdata->isReceiverProcessAlive=0;

}


/* CL_Linkages must be locked first (exclusive). */

BOOL CreateReceiverProc(
	struct DriverData *driverdata,
	char *inname,
	LONG portnum,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
){
	struct Process *process;
	char *name;

	name=AllocVec(mystrlen(inname)+70,MEMF_ANY | MEMF_PUBLIC);
	if(name==NULL){
		if(ErrorCode!=NULL){
			*ErrorCode=CME_NoMem;
		}
		return FALSE;
	}

	mysprintf(CamdBase,name,"CAMD ReceiverProc %s %ld",inname,portnum);

	
	driverdatatemp=driverdata;

	driverdata->isReceiverProcessAlive=0;

	process=CreateNewProcTags(
		NP_Entry, (IPTR) ReceiverFunc,
		NP_Name, (IPTR) name,
		NP_Priority,36,
		TAG_END
	);
	if(process==NULL){
		if(ErrorCode!=NULL){
			*ErrorCode=CME_NoMem;
		}
		FreeVec(name);
		return FALSE;
	}

	while(driverdata->isReceiverProcessAlive == 0){
		Delay(1);
	}

	if(driverdata->isReceiverProcessAlive==2){
		driverdata->isReceiverProcessAlive=0;
		if(ErrorCode!=NULL){
			*ErrorCode=CME_NoSignals;
		}
		FreeVec(name);
		return FALSE;
	}

	driverdata->ReceiverProc=process;
	driverdata->ReceiverProcName=name;

	return TRUE;
}

/* CL_Linkages must be locked first. */

void EndReceiverProc(
	struct DriverData *driverdata,
	struct CamdBase *CamdBase
){
	Signal((struct Task *)driverdata->ReceiverProc,SIGBREAKF_CTRL_C);
	while(driverdata->isReceiverProcessAlive==1) Delay(1);
	FreeVec(driverdata->ReceiverProcName);
}

