/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#ifndef __amigaos4__
#  include <proto/camd.h>
#endif

#include "camd_intern.h"
#  undef DEBUG
#  define DEBUG 1
#ifndef __amigaos4__
#  include AROS_DEBUG_H_FILE
#endif

#undef AddMidiLinkA

/*****************************************************************************

    NAME */

	AROS_LH3(struct MidiLink *, AddMidiLinkA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiNode *, midinode, A0),
	AROS_LHA(LONG, type, D0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 14, Camd)

/*  FUNCTION
		Adds a midilink to a midinode.

    INPUTS
		midinode - Owner.
		type - MLTYPE_Receiver or MLTYPE_Sender
		tags - Tag-values supplied to SetMidiLinkAttrs.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
		CreateMidi, SetMidiLinkAttrsA

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)
	struct MidiLink *midilink;

	midilink=AllocMem(sizeof(struct MidiLink),MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);

	if(midilink==NULL) return NULL;

	midilink->ml_MidiNode=midinode;

	midilink->ml_Node.ln_Name="noname";
	midilink->ml_Node.ln_Type=NT_USER-type;

	midilink->ml_EventTypeMask=(ULONG)~0;
	midilink->ml_ChannelMask=(UWORD)~0;

	ObtainSemaphore(CB(CamdBase)->CLSemaphore);
		if(type==MLTYPE_Receiver){
			AddHead((struct List *)&midinode->mi_InLinks,(struct Node *)&midilink->ml_OwnerNode);
		}else{
			AddHead((struct List *)&midinode->mi_OutLinks,(struct Node *)&midilink->ml_OwnerNode);
		}
	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

#ifndef __amigaos4__
	if(SetMidiLinkAttrsA(midilink,tags)==FALSE)
#else
	if(SetMidiLinkAttrsA(ICamd, midilink,tags)==FALSE)
#endif
	{

		ObtainSemaphore(CB(CamdBase)->CLSemaphore);
			Remove((struct Node *)&midilink->ml_OwnerNode);
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		FreeMem(midilink,sizeof(struct MidiLink));
		return NULL;
	}


	return midilink;

   AROS_LIBFUNC_EXIT
}



#ifdef __amigaos4__
#include <stdarg.h>
struct MidiLink * VARARGS68K AddMidiLink(
	struct CamdIFace *Self,
	struct MidiNode * mi,
	LONG type,
	...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, type);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	AddMidiLinkA(Self,
		mi,
		type,
		varargs);
}

#endif

