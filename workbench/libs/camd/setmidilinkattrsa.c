/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
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

    EXAMPLE

    BUGS

    SEE ALSO
		GetMidiLinkAttrsA

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct TagItem *tag;

	//If your compiler complains about the const qualifier, change your protos file,
	//not this file.
	const struct TagItem *tstate=tags;

	BOOL ret=TRUE;
	char *clustername;
	int type=midilink->ml_Node.ln_Type;
	struct MyMidiCluster *mycluster;
	struct DriverData *driverdata=NULL;

	ULONG *ErrorCode=(ULONG *)GetTagData(MLINK_ErrorCode,NULL,tags);

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
						ObtainExclusiveSem(&mycluster->mutex);
					}

						Remove(&midilink->ml_Node);
						midilink->ml_Node.ln_Pri=tag->ti_Data;
						if(type==NT_USER-MLTYPE_Receiver){
							Enqueue(&midilink->ml_Location->mcl_Receivers,&midilink->ml_Node);
						}else{
							Enqueue(&midilink->ml_Location->mcl_Senders,&midilink->ml_Node);
						}
					if(type==NT_USER-MLTYPE_Receiver){
						ReleaseExclusiveSem(&mycluster->mutex);
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
		clustername=(char *)GetTagData(MLINK_Location,NULL,tags);
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



