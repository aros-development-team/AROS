/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/camd.h>

#include "camd_intern.h"

#undef CreateMidiA

/*****************************************************************************

    NAME */

	AROS_LH1(struct MidiNode *, CreateMidiA,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 7, Camd)

/*  FUNCTION

    INPUTS
		tags - Tag-values supplied to SetMidiAttrs.

    RESULT
		NULL if failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct MyMidiNode *midinode;


	midinode=AllocMem(sizeof(struct MyMidiNode),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
	if(midinode==NULL) return NULL;

	NEWLIST((struct List *)&midinode->midinode.mi_OutLinks);
	NEWLIST((struct List *)&midinode->midinode.mi_InLinks);

	midinode->midinode.mi_Node.ln_Name="unnamed";

	midinode->midinode.mi_SigTask=FindTask(0);

	midinode->midinode.mi_ReceiveSigBit=-1;
	midinode->midinode.mi_ParticipantSigBit=-1;

	midinode->midinode.mi_TimeStamp=&midinode->dummytimestamp;

	InitSemaphore(&midinode->receiversemaphore);
	InitSemaphore(&midinode->sysexsemaphore);
	InitSemaphore(&midinode->sysexsemaphore2);

	if(!SetMidiAttrsA(&midinode->midinode,tags))
	  {
		FreeMem(midinode,sizeof(struct MyMidiNode));
		return NULL;
	  }

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);
		AddHead(&CB(CamdBase)->mymidinodes,&midinode->midinode.mi_Node);
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	return (struct MidiNode *)midinode;
    AROS_LIBFUNC_EXIT

}




