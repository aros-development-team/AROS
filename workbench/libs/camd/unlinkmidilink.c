/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"


/* 
	CLSemaphore must be exlusive obtained first.
*/

void UnlinkMidiLink(
	struct MidiLink *midilink,
	BOOL unlinkfromnode,
	struct CamdBase *CamdBase
){
	struct MidiCluster *cluster=midilink->ml_Location;
	int type=midilink->ml_Node.ln_Type;
	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)cluster;

	if(cluster!=NULL){
		if(type==NT_USER-MLTYPE_Receiver){
			ObtainSemaphore(&mycluster->semaphore);
		}
		Remove(&midilink->ml_Node);
	}

	if(unlinkfromnode==TRUE){
		Remove((struct Node *)&midilink->ml_OwnerNode);
	}

	if(cluster!=NULL){
		if(type==NT_USER-MLTYPE_Receiver){
			ReleaseSemaphore(&mycluster->semaphore);
		}
		LinkHasBeenRemovedFromCluster(cluster,CamdBase);
	}

}

