/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH3(struct MidiLink *, NextMidiLink,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(struct MidiLink *, midilink, A1),
	AROS_LHA(LONG, type, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 19, Camd)

/*  FUNCTION
		Returns the next MidiLink of a specified type that belongs
		to a midinode. Or NULL if midilink was the first. If midilink
		is NULL, returns the first one.

    INPUTS
		type - MLTYPE_Sender or MLTYPE_Receiver.

    RESULT

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

	struct MinNode *node;

	if(type==MLTYPE_Receiver){
		node=midinode->mi_InLinks.mlh_Head;
	}else{
		node=midinode->mi_OutLinks.mlh_Head;
	}

	while(node->mln_Succ!=NULL){
		if(midilink==NULL){
			return (struct MidiLink *)node;
		}else{
			if(node==(struct MinNode *)midilink){
				if(node->mln_Succ->mln_Succ!=NULL){
					return GetMidiLinkFromOwnerNode(node->mln_Succ);
				}else{
					return NULL;
				}
			}
		}
		node=node->mln_Succ;
	}

	return NULL;

   AROS_LIBFUNC_EXIT

}


