/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeleteMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 8, Camd)

/*  FUNCTION
		First deletes all midilinks attached to the midinode, then
		frees all buffers, before it frees itself.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiNode *mymidinode=(struct MyMidiNode *)midinode;
	struct MinNode *node,*temp;
	struct MidiLink *midilink;

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);
		if( ! (IsListEmpty((struct List *)&midinode->mi_OutLinks))){
			node=midinode->mi_OutLinks.mlh_Head;
			while(node->mln_Succ!=NULL){
				temp=node->mln_Succ;

				midilink=GetMidiLinkFromOwnerNode(node);
				UnlinkMidiLink(midilink,FALSE,CamdBase);

				if(midilink->ml_ParserData!=NULL) FreeMem(midilink->ml_ParserData,sizeof(struct DriverData));

				FreeMem(midilink,sizeof(struct MidiLink));
				node=temp;
			}
		}
		if( ! (IsListEmpty((struct List *)&midinode->mi_InLinks))){
			node=midinode->mi_InLinks.mlh_Head;
			while(node->mln_Succ!=NULL){
				temp=node->mln_Succ;

				midilink=GetMidiLinkFromOwnerNode(node);

				UnlinkMidiLink(midilink,FALSE,CamdBase);

				if(midilink->ml_ParserData!=NULL) FreeMem(midilink->ml_ParserData,sizeof(struct DriverData));

				FreeMem(midilink,sizeof(struct MidiLink));
				node=temp;
			}
		}

		Remove(&midinode->mi_Node);

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);


	if(mymidinode->in_start!=NULL){
		FreeVec(mymidinode->in_start);
	}
	if(mymidinode->sysex_start!=NULL){
		FreeVec(mymidinode->sysex_start);
	}
	FreeMem(midinode,sizeof(struct MyMidiNode));

   AROS_LIBFUNC_EXIT

}



