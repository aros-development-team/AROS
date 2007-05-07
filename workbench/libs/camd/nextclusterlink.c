/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH3(struct MidiLink *, NextClusterLink,

/*  SYNOPSIS */
	AROS_LHA(struct MidiCluster *, cluster, A0),
	AROS_LHA(struct MidiLink *, midilink, A1),
	AROS_LHA(LONG, type, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 18, Camd)

/*  FUNCTION
		Finds the next midilink of a specified type in a midicluster.

    INPUTS
		cluster - Pointer to the midicluster that the midilink belongs to.
		midilink - Pointer to the midilink to begin searching from.
		type - Either MLTYPE_Receiver or MLTYPE_Sender

    RESULT
		Returns the next MidiLink of a spevified type, or NULL if the last
		in the list. If midilink is NULL, returns the first.

    NOTES
		CL_Linkages must be locked.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct Node *node;

	if(type==MLTYPE_Receiver){
		node=cluster->mcl_Receivers.lh_Head;
	}else{
		node=cluster->mcl_Senders.lh_Head;
	}


	while(node->ln_Succ!=NULL){
		if(node->ln_Type!=NT_USER-MLTYPE_NTypes){
			if(midilink==NULL){
				return (struct MidiLink *)node;
			}else{
				if(node==&midilink->ml_Node){
					if(node->ln_Succ->ln_Succ!=NULL){
						return (struct MidiLink *)node->ln_Succ;
					}else{
						return NULL;
					}
				}
			}
		}
		node=node->ln_Succ;
	}

	return NULL;

   AROS_LIBFUNC_EXIT
}


