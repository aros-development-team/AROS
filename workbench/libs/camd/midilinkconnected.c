/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/


#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH1(BOOL, MidiLinkConnected,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 20, Camd)

/*  FUNCTION
		If midilink is a sender, returns FALSE if the cluster has no
		receivers. If midilink is a receiver, returns FALSE if the
		cluster has no senders. Else TRUE.

    INPUTS
		midilink - pointer to midilink we want to check.

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
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)


	ULONG lock;
	BOOL ret=TRUE;
	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)midilink->ml_Location;
	int type=midilink->ml_Node.ln_Type;

	if(type==NT_USER-MLTYPE_Sender){
		lock=ObtainSharedSem(&mycluster->mutex);
			if(IsListEmpty(&midilink->ml_Location->mcl_Receivers)) ret=FALSE;
		ReleaseSharedSem(&mycluster->mutex,lock);
	}else{
		ObtainSemaphore(CB(CamdBase)->CLSemaphore);
			if(IsListEmpty(&midilink->ml_Location->mcl_Senders)) ret=FALSE;
		ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
	}

	return ret;

   AROS_LIBFUNC_EXIT
}


