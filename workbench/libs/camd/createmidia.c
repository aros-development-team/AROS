/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/utility.h>
#include <proto/exec.h>
#ifndef __amigaos4__
#  include <proto/camd.h>
#endif

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

#ifndef __amigaos4__
	NEWLIST((struct List *)&midinode->midinode.mi_OutLinks);
	NEWLIST((struct List *)&midinode->midinode.mi_InLinks);
#else
	NEWMINLIST(midinode->midinode.mi_OutLinks);
	NEWMINLIST(midinode->midinode.mi_InLinks);
#endif

	midinode->midinode.mi_Node.ln_Name="unnamed";

	midinode->midinode.mi_SigTask=FindTask(0);

	midinode->midinode.mi_ReceiveSigBit=-1;
	midinode->midinode.mi_ParticipantSigBit=-1;

	midinode->midinode.mi_TimeStamp=&midinode->dummytimestamp;

	InitSemaphore(&midinode->receiversemaphore);
	InitSemaphore(&midinode->sysexsemaphore);
	InitSemaphore(&midinode->sysexsemaphore2);

#ifndef __amigaos4__
	if(!SetMidiAttrsA(&midinode->midinode,tags))
#else
	if(!SetMidiAttrsA(ICamd, &midinode->midinode,tags))
#endif
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



#ifdef __amigaos4__
#include <stdarg.h>
struct MidiNode * VARARGS68K CreateMidi(
	struct CamdIFace *Self,
	...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, Self);
	varargs = va_getlinearva(ap, struct TagItem *);
    
	return	CreateMidiA(Self,
		varargs);
}
#endif

