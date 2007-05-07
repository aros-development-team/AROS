/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/


#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiNode *, NextMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 11, Camd)

/*  FUNCTION
		Returns the next midinode in the list of midinodes, or NULL
		if midinode was the last one.

    INPUTS
		midinode - The midinode to begin searching from. If NULL,
		           returns the first midinode in the list.

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

	struct Node *node=CB(CamdBase)->mymidinodes.lh_Head;

	if(midinode==NULL){
		if(IsListEmpty(&CB(CamdBase)->mymidinodes)) return NULL;
		return (struct MidiNode *)node;
	}

	if(midinode->mi_Node.ln_Succ->ln_Succ==NULL) return NULL;

	return (struct MidiNode *)midinode->mi_Node.ln_Succ;

   AROS_LIBFUNC_EXIT
}

