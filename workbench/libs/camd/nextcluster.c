/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/



#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiCluster *, NextCluster,

/*  SYNOPSIS */
	AROS_LHA(struct MidiCluster *, last, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 21, Camd)

/*  FUNCTION
		Finds the next cluster in camds list of clusters.

    INPUTS
		last - cluster to start searching for.

    RESULT
		Next cluster in list, or first if 'last' is NULL.

    NOTES
		- CL_Linkages must be locked.

		- Often, a program wants to use this function for finding available
		  clusters a user can choose from. It is then recommended to also
		  let the user have the possibility to write in the name of a new cluster,
		  so that camd can make new clusters automatically to be used for
		  communication between various applications without having hardware-drivers
		  etc. interfere with the datastreams. Applications do
		  not need to make special concerns about how cluster works or
		  what they contain; that is all managed by camd.

    EXAMPLE

		#include <stdio.h>
		#include <proto/exec.h>
		#include <proto/camd.h>
		#include <midi/camd.h>

		int main(){

			APTR lock;
			struct MidiCluster *cluster;

			struct Library *CamdBase=OpenLibrary("camd.library",0L);
			if(CamdBase!=NULL){

				lock=LockCAMD(CD_Linkages);

				cluster=NextCluster(NULL);
				if(cluster==NULL){

					printf("No clusters available.\n");

				}else{

					do{
						printf("clustername: -%s-\n",cluster->mcl_Node.ln_Name);
						cluster=NextCluster(cluster);
					}while(cluster!=NULL);

				}

				UnlockCAMD(lock);
				CloseLibrary(CamdBase);

			}else{
				printf("Could not open camd.library.\n");
				return 1;
			}

			return 0;
		}


    BUGS

    SEE ALSO
		NextMidiLink, NextMidi, FindCluster

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	if(last==NULL){
		if(IsListEmpty(&CB(CamdBase)->midiclusters)){
			return NULL;
		}
		return (struct MidiCluster *)CB(CamdBase)->midiclusters.lh_Head;
	}
	if(last->mcl_Node.ln_Succ->ln_Succ==NULL) return NULL;

	return (struct MidiCluster *)last->mcl_Node.ln_Succ;

   AROS_LIBFUNC_EXIT
}

