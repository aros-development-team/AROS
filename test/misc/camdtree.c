/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <proto/exec.h>
#include <proto/camd.h>
#include <midi/camd.h>

#define TABSIZE 4

struct Library *CamdBase=NULL;

#ifndef GetMidiLinkAttrs
ULONG GetMidiLinkAttrs(struct MidiLink *ml, Tag tag, ...){
	return GetMidiLinkAttrsA(ml, (struct TagItem *)&tag );
}
#endif

#ifndef GetMidiAttrs
ULONG GetMidiAttrs(struct MidiNode *ml, Tag tag, ...){
	return GetMidiAttrsA(ml, (struct TagItem *)&tag );
}
#endif

struct MidiLink *GetMidiLinkFromOwnerNode(struct MinNode *node){
	struct MidiLink dummy;
	return (struct MidiLink *)((char *)((char *)(node)-((char *)&dummy.ml_OwnerNode-(char *)&dummy)));
}

void printSpaces(int level){
	int lokke;
	for(lokke=0;lokke<level*TABSIZE;lokke++){
		printf(" ");
	}
}

void printLink_brancheNodes(struct MidiLink *midilink,int level,int maxlevel);
void printLink_brancheClusters(struct MidiLink *midilink,int level,int maxlevel);
void printCluster(struct MidiCluster *cluster,int level,int maxlevel);


void printNode(struct MidiNode *midinode,int level,int maxlevel){
	char *nodename=NULL;
	struct MinNode *node;

	if(level==maxlevel) return;

	GetMidiAttrs(midinode,MIDI_Name,(IPTR)&nodename,TAG_END);

	printSpaces(level);
	printf(
		"%p, -%s-\n",
		midinode,
		nodename
	);

	if(level+1==maxlevel) return;

	if( ! (IsListEmpty((struct List *)&midinode->mi_OutLinks))){
		printSpaces(level);
		printf("  -OutLinks:\n");
		node=midinode->mi_OutLinks.mlh_Head;
		while(node->mln_Succ!=NULL){
			printLink_brancheClusters(GetMidiLinkFromOwnerNode(node),level+1,maxlevel);
			node=node->mln_Succ;
		}
	}

	if( ! (IsListEmpty((struct List *)&midinode->mi_InLinks))){
		printSpaces(level);
		printf("  -InLinks:\n");
		node=midinode->mi_InLinks.mlh_Head;
		while(node->mln_Succ!=NULL){
			printLink_brancheClusters(GetMidiLinkFromOwnerNode(node),level+1,maxlevel);
			node=node->mln_Succ;
		}
	}
}


BOOL printLink(struct MidiLink *midilink){
	char *linkname=NULL;

	if(midilink->ml_Node.ln_Type==NT_USER-MLTYPE_Receiver || midilink->ml_Node.ln_Type==NT_USER-MLTYPE_Sender){
		GetMidiLinkAttrs(midilink,MLINK_Name,(IPTR)&linkname,TAG_END);
		printf(
			"%p, -%s-\n",
			midilink,
			linkname
		);
		return TRUE;
	}

	printf("%p, <driverdata> (private)\n",midilink);
	return FALSE;
}

void printLink_brancheNodes(struct MidiLink *midilink,int level,int maxlevel){
	struct MidiNode *midinode;

	if(level==maxlevel) return;

	printSpaces(level);
	if(printLink(midilink)==TRUE){
		midinode=midilink->ml_MidiNode;
		printSpaces(level);
		printf("  -Owner (MidiNode): \n");
		printNode(midinode,level+1,maxlevel);
	}
}

void printLink_brancheClusters(struct MidiLink *midilink,int level,int maxlevel){
	if(level==maxlevel) return;

	printSpaces(level);
	printLink(midilink);

	if(level+1==maxlevel) return;

	printSpaces(level);
	printf("  -Cluster: \n");
	printCluster(midilink->ml_Location,level+1,maxlevel);
}


void printCluster(struct MidiCluster *cluster,int level,int maxlevel){
	struct MidiLink *midilink;

	if(level==maxlevel) return;

	printSpaces(level);
	printf("clustername: -%s-\n",cluster->mcl_Node.ln_Name);

	if(level+1==maxlevel) return;

	if(!(IsListEmpty(&cluster->mcl_Receivers))){
		printSpaces(level);
		printf("  ");
		printf("-Receiver links:\n");

		midilink=(struct MidiLink *)cluster->mcl_Receivers.lh_Head;
		while(midilink->ml_Node.ln_Succ!=NULL){
			printLink_brancheNodes(midilink,level+1,maxlevel);
			midilink=(struct MidiLink *)midilink->ml_Node.ln_Succ;
		}
	}

	if(!(IsListEmpty(&cluster->mcl_Senders))){
		printSpaces(level);
		printf("  ");
		printf("-Sender links:\n");
		midilink=(struct MidiLink *)cluster->mcl_Senders.lh_Head;
		while(midilink->ml_Node.ln_Succ!=NULL){
			printLink_brancheNodes(midilink,level+1,maxlevel);
			midilink=(struct MidiLink *)midilink->ml_Node.ln_Succ;
		}
	}
}


int main(){

	APTR lock;
	struct MidiCluster *cluster;

	CamdBase=OpenLibrary("camd.library",40L);

	if(CamdBase!=NULL){

		lock=LockCAMD(CD_Linkages);

		cluster=NextCluster(NULL);
		if(cluster==NULL){

			printf("No clusters available.\n");

		}else{

			printf("-Clusters:\n\n");
			do{
				printCluster(cluster,1,6);
				printf("\n");
				cluster=NextCluster(cluster);
			}while(cluster!=NULL);

		}

		UnlockCAMD(lock);
		CloseLibrary(CamdBase);

	}else{
		printf("Could not open at least V40 of camd.library.\n");
		return 1;
	}

	return 0;
}


