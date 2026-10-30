/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, GetClusterAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiCluster *, cluster, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 41, Camd)

/*  FUNCTION
		Reads attributes of a cluster. Each tag's ti_Data points to an
		IPTR that receives the value.

    INPUTS
		cluster - from FindCluster() or NextCluster(), with CAMD locked
		          by LockCAMD() so that it cannot go away.
		tags - MCLA_Name (STRPTR)
		       MCLA_Comment (STRPTR) - the cluster's comment, NULL if none
		       MCLA_Participants (ULONG) - links in the cluster; a driver
		           port is not a link
		       MCLA_PublicParticipants (ULONG) - those not MLINK_Private
		       MCLA_HasDriver (BOOL) - a DEVS:Midi driver port is in it
		       MCLA_Overflows (ULONG) - messages dropped because a
		           receiver's queue was full
		       MCLA_SysExDropped (ULONG) - SysEx messages dropped because
		           a receiver's SysEx buffer was full
		       MCLA_RecvErrors (ULONG) - errors and overruns of the
		           driver port receiving into the cluster

    RESULT
		The number of tags understood.

    NOTES
		New in V42. The counters are incremented atomically and start at
		0 when the cluster is created. Each read is a diagnostic snapshot;
		several counters are not sampled as one transaction.

    EXAMPLE

    BUGS

    SEE ALSO
		GetMidiLinkAttrsA(), NextCluster()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct MyMidiCluster *mycluster=(struct MyMidiCluster *)cluster;
	struct TagItem *tag;
	struct TagItem *tstate=tags;
	IPTR *where;
	ULONG ret=0;

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);

	while((tag=NextTagItem(&tstate))){
		ret++;
		where=(IPTR *)tag->ti_Data;
		switch(tag->ti_Tag){
			case MCLA_Name:
				*where=(IPTR)cluster->mcl_Node.ln_Name;
				break;
			case MCLA_Comment:
				*where=(IPTR)(mycluster->comment[0]!=0 ? mycluster->comment : NULL);
				break;
			case MCLA_Participants:
				*where=(IPTR)(UWORD)cluster->mcl_Participants;
				break;
			case MCLA_PublicParticipants:
				*where=(IPTR)(UWORD)cluster->mcl_PublicParticipants;
				break;
			case MCLA_HasDriver:
				*where=(IPTR)(FindSenderDriverInCluster(cluster)!=NULL ||
				              FindReceiverDriverInCluster(cluster)!=NULL);
				break;
			case MCLA_Overflows:
				*where=(IPTR)mycluster->overflows;
				break;
			case MCLA_SysExDropped:
				*where=(IPTR)mycluster->sysexdropped;
				break;
			case MCLA_RecvErrors:
				*where=(IPTR)mycluster->recverrors;
				break;
			default:
				ret--;
				break;
		}
	}

	ReleaseSemaphore(CB(CamdBase)->CLSemaphore);

	return ret;

    AROS_LIBFUNC_EXIT
}
