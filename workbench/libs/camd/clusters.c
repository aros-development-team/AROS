/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"


#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

/*
 *  CLSemaphore must be obtained and the clusters mutex must
 *  not be exclusive locked.
 */
void RemoveCluster(struct MidiCluster *cluster,struct CamdBase *CamdBase){
  if(cluster==NULL) return;
  
  Remove(&cluster->mcl_Node);
  
  if(cluster->mcl_Node.ln_Name!=NULL){
    FreeVec(cluster->mcl_Node.ln_Name);
  }
  FreeMem(cluster,sizeof(struct MyMidiCluster));
}


/*
 * REQUIREMENTS
 * 	CLSemaphore must be obtained and the clusters mutex must
 *  	not be exclusive locked. The link must not be freed before calling.
 * NOTE
 *  	A cluster can never have both a hardware receiver and sender.
 */
void LinkHasBeenRemovedFromCluster(struct MidiCluster *cluster,struct CamdBase *CamdBase){
  struct Node *node;
  struct DriverData *driverdata;
  
  if( ! (IsListEmpty(&cluster->mcl_Receivers))){
    node=cluster->mcl_Receivers.lh_Head;
    
    while(node->ln_Succ!=NULL){
      if(node->ln_Type==NT_USER-MLTYPE_NTypes){
	
	/* We now know that the cluster has a hardware-receiver. */
	
	if(IsListEmpty(&cluster->mcl_Senders)){
	  
	  /* And we now know that the cluster has no senders. */
	  
	  driverdata=(struct DriverData *)cluster->mcl_Receivers.lh_Head;
	  
	  /* We mark the hardware-receiver not to be in use. */
	  
	  if(driverdata->isOutOpen==TRUE){
	    driverdata->isOutOpen=FALSE;
	    
	    /* And we close it if the hardware-sender is not in use either. */
	    
	    if(driverdata->isInOpen==FALSE){
	      CloseDriver(driverdata,CamdBase);
	    }
	  }
	  
	}
	break;
      }
      node=node->ln_Succ;
    }
    
  }
  
  
  if( ! (IsListEmpty(&cluster->mcl_Senders))){
    
    node=cluster->mcl_Senders.lh_Head;
    
    while(node->ln_Succ!=NULL){
      if(node->ln_Type==NT_USER-MLTYPE_NTypes){
	
	/* We now now that the cluster only has a hardware-sender. */
	
	if(IsListEmpty(&cluster->mcl_Receivers)){
	  
	  /* And we now know that the cluster has no senders. */
	  
	  driverdata=(struct DriverData *)cluster->mcl_Senders.lh_Head;
	  driverdata=(struct DriverData *)(((char *)driverdata-sizeof(struct Node)));
	  /* We mark the hardware-sender not to be in use. */
	  
	  if(driverdata->isInOpen==TRUE){
	    driverdata->isInOpen=FALSE;
	    
	    /* And we close it if the hardware-receiver is not in use either. */
	    
	    if(driverdata->isOutOpen==FALSE){
	      CloseDriver(driverdata,CamdBase);
	    }
	  }
	}
	break;
      }
      node=node->ln_Succ;
    }
    
    
    return;
  }
  
  if(
     (IsListEmpty(&cluster->mcl_Receivers)) &&
     (IsListEmpty(&cluster->mcl_Senders))
     ){
    RemoveCluster(cluster,CamdBase);
  }
}


/*
	CLSemaphore must be obtained first.
*/

struct MidiCluster *NewCluster(char *name,struct CamdBase *CamdBase){
	struct MyMidiCluster *mymidicluster;
	struct MidiCluster *midicluster;


	mymidicluster=AllocMem(sizeof(struct MyMidiCluster),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);

	if(mymidicluster==NULL) return NULL;

	InitCamdMutEx(&mymidicluster->mutex);

	midicluster=&mymidicluster->cluster;

	midicluster->mcl_Node.ln_Name=AllocVec(mystrlen(name) + 1,MEMF_ANY|MEMF_PUBLIC);

	if(midicluster->mcl_Node.ln_Name==NULL){
		FreeMem(midicluster,sizeof(struct MyMidiCluster));
		return NULL;
	}

	mysprintf(CamdBase,midicluster->mcl_Node.ln_Name,"%s",name);

	NEWLIST(&midicluster->mcl_Receivers);
	NEWLIST(&midicluster->mcl_Senders);

	AddTail(&CB(CamdBase)->midiclusters,&midicluster->mcl_Node);


	return midicluster;
}


/*
	CLSemaphore must have been obtained before calling.
*/
struct DriverData *FindReceiverDriverInCluster(struct MidiCluster *cluster){
	struct Node *node;

	if( ! (IsListEmpty(&cluster->mcl_Receivers))){
		node=cluster->mcl_Receivers.lh_Head;

		while(node->ln_Succ!=NULL){
			if(node->ln_Type==NT_USER-MLTYPE_NTypes) return (struct DriverData *)node;
			node=node->ln_Succ;
		}
	}
	return NULL;
}

/*
	CLSemaphore must have been obtained before calling.
*/

struct DriverData *FindSenderDriverInCluster(struct MidiCluster *cluster){
	struct Node *node;

	if( ! (IsListEmpty(&cluster->mcl_Senders))){
		node=cluster->mcl_Senders.lh_Head;

		while(node->ln_Succ!=NULL){
			if(node->ln_Type==NT_USER-MLTYPE_NTypes){
				return (struct DriverData *)(((char *)node-sizeof(struct Node)));
			}
			node=node->ln_Succ;
		}
	}
	return NULL;
}


/*
	CLSemaphore must have been obtained before calling.
	The clusters exclusive mutex must not have be obtained.
*/

BOOL AddClusterReceiver(
	struct MidiCluster *cluster,
	struct Node *node,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
){
	struct MidiLink *midilink;
	struct DriverData *driverdata;
	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)cluster;

	if(node->ln_Type!=NT_USER-MLTYPE_NTypes){

	  driverdata=FindSenderDriverInCluster(cluster);

	  if(driverdata!=NULL){
			if(driverdata->isInOpen==FALSE && driverdata->isOutOpen==FALSE){
				if(OpenDriver(driverdata,ErrorCode,CamdBase)==FALSE){
					return FALSE;
				}
	  
				driverdata->isInOpen=TRUE;
			}
		}
		midilink=(struct MidiLink *)node;
	  
		ObtainExclusiveSem(&mycluster->mutex);
	  		midilink->ml_Location=cluster;
	}else{
		/* The receiver is a hardware-receiver, not a midilink. */
		ObtainExclusiveSem(&mycluster->mutex);
	}

	Enqueue(&cluster->mcl_Receivers,node);
	ReleaseExclusiveSem(&mycluster->mutex);

	return TRUE;
}


/*
	CLSemaphore must have been obtained before calling.
*/

BOOL AddClusterSender(
	struct MidiCluster *cluster,
	struct Node *node,
	ULONG *ErrorCode,
	struct CamdBase *CamdBase
){
	struct MidiLink *midilink;
	struct DriverData *driverdata;

	if(node->ln_Type!=NT_USER-MLTYPE_NTypes){
		driverdata=FindReceiverDriverInCluster(cluster);
		if(driverdata!=NULL){
			if(driverdata->isInOpen==FALSE && driverdata->isOutOpen==FALSE){
				if(OpenDriver(driverdata,ErrorCode,CamdBase)==FALSE){
					return FALSE;
				}
				driverdata->isOutOpen=TRUE;
				D(bug("isOutOpen=TRUE\n"));
			}
		}
		midilink=(struct MidiLink *)node;
		midilink->ml_Location=cluster;
	}

	D(bug("enqueueing Sen cluster: %lx, node:%lx\n",cluster,node));
	Enqueue(&cluster->mcl_Senders,node);
	return TRUE;
}



/*
	CLSemaphore must have been obtained before calling.
*/

BOOL SetClusterForLink(
		       struct MidiLink *midilink,
		       char *name,
		       ULONG *ErrorCode,
		       struct CamdBase *CamdBase
		       ){
  struct MidiCluster *cluster;
  ULONG type=midilink->ml_Node.ln_Type;
  
  
  UnlinkMidiLink(midilink,FALSE,CamdBase);
  
  midilink->ml_Location=NULL;
  

  cluster=(struct MidiCluster *)FindName(&CB(CamdBase)->midiclusters,name);
  
  if(cluster==NULL){
    
    cluster=NewCluster(name,CamdBase);

    if(cluster==NULL){
      if(ErrorCode!=NULL){
	*ErrorCode=CME_NoMem;
      }
      return FALSE;
    }
  }
  
  if(type==NT_USER-MLTYPE_Receiver){
    
    if(AddClusterReceiver(cluster,&midilink->ml_Node,ErrorCode,CamdBase)==FALSE){
      return FALSE;
    }
  }else{
    
    if(AddClusterSender(cluster,&midilink->ml_Node,ErrorCode,CamdBase)==FALSE){
      return FALSE;
    }
  }
  
  return TRUE;
}

