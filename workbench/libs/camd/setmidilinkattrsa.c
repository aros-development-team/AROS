/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/utility.h>
#include <proto/exec.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

/*****************************************************************************

    NAME */

	AROS_LH2(BOOL, SetMidiLinkAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 16, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES
		- If the midilink is not owned by yourself, please lock
		  camd to ensure it wont go away.

		- Allthough you are able to modify midilinks owned by
		  others, please avoid it, its normally "non of your buziness",
		  and may lead to crashes and other "unexpected" behaviours.
		  However, if you have full control of the owner of the
		  midilink (f.ex when both you and the owner belongs to the
		  same probram and you are absolutely shure you know what
		  you are doing), there is no problem.

		- Warning! If another task have locked Camd and is waiting
		  for you to finish, there will be a deadlock if you try
		  to change priority or change/set cluster.

    EXAMPLE

    BUGS

    SEE ALSO
		GetMidiLinkAttrsA

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct TagItem *tag;

	const struct TagItem *tstate=tags;

	BOOL ret=TRUE;
	char *clustername;
	int type=midilink->ml_Node.ln_Type;
	struct MyMidiCluster *mycluster;
	struct DriverData *driverdata=NULL;

	ULONG *ErrorCode = (ULONG *)GetTagData(MLINK_ErrorCode, (IPTR) NULL,tags);

	while((tag=NextTagItem(&tstate))){
		switch(tag->ti_Tag){
			case MLINK_Name:
				midilink->ml_Node.ln_Name=(char *)tag->ti_Data;
				break;
			case MLINK_Location:
				break;
			case MLINK_ChannelMask:
				midilink->ml_ChannelMask=tag->ti_Data;
				break;
			case MLINK_EventMask:
				midilink->ml_EventTypeMask=tag->ti_Data;
				break;
			case MLINK_UserData:
				midilink->ml_UserData=(void *)tag->ti_Data;
				break;
			case MLINK_Comment:
// FixME!
//				if(midilink
				break;
			case MLINK_PortID:
				midilink->ml_PortID=tag->ti_Data;
				break;
			case MLINK_Private:
				midilink->ml_Flags|=MLF_PrivateLink*tag->ti_Data;
				break;
			case MLINK_Priority:
				mycluster=(struct MyMidiCluster *)midilink->ml_Location;
				if(mycluster!=NULL){
					ObtainSemaphore(CB(CamdBase)->CLSemaphore);
					if(type==NT_USER-MLTYPE_Receiver){
						ObtainSemaphore(&mycluster->semaphore);
					}

						Remove(&midilink->ml_Node);
						midilink->ml_Node.ln_Pri=tag->ti_Data;
						if(type==NT_USER-MLTYPE_Receiver){
							Enqueue(&midilink->ml_Location->mcl_Receivers,&midilink->ml_Node);
						}else{
							Enqueue(&midilink->ml_Location->mcl_Senders,&midilink->ml_Node);
						}
					if(type==NT_USER-MLTYPE_Receiver){
						ReleaseSemaphore(&mycluster->semaphore);
					}
					ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
				}else{
					midilink->ml_Node.ln_Pri=tag->ti_Data;
				}
				break;
			case MLINK_SysExFilter:
				midilink->ml_SysExFilter.sxf_Packed=0xffffff & tag->ti_Data;
				break;
			case MLINK_SysExFilterX:
				midilink->ml_SysExFilter.sxf_Packed=0x01000000 | tag->ti_Data;
				break;
			case MLINK_Parse:
				driverdata=AllocMem(sizeof(struct DriverData),MEMF_ANY|MEMF_CLEAR|MEMF_PUBLIC);
				if(driverdata==NULL){
					if(ErrorCode!=NULL){
						*ErrorCode=CME_NoMem;
					}
					ret=FALSE;
					break;
				}
				driverdata->Input_Treat=Receiver_init;
				break;
			case MLINK_ErrorCode:
				break;
			default:
				break;
		}
	}

	if(ret!=FALSE){
		clustername = (char *) GetTagData(MLINK_Location, (IPTR) NULL, tags);
		if(clustername!=NULL){
			ObtainSemaphore(CB(CamdBase)->CLSemaphore);
				if(SetClusterForLink(midilink,clustername,ErrorCode,CamdBase)==FALSE){
					ret=FALSE;
				}
			ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
		}
	}

	if(driverdata!=NULL && midilink->ml_Location!=NULL){
		driverdata->incluster=(struct MyMidiCluster *)midilink->ml_Location;
		midilink->ml_ParserData=driverdata;
	}else{
		midilink->ml_ParserData=NULL;
	}

	return ret;

   AROS_LIBFUNC_EXIT
}



