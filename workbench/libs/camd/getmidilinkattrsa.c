/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/utility.h>
#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, GetMidiLinkAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, midilink, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct CamdBase *, CamdBase, 17, Camd)

/*  FUNCTION
		Remind me to fill in things here later.

    INPUTS

    RESULT

    NOTES
		If you are not the owner of the midilink, you should lock
		Camd before calling to ensure that it wont go away.
		Theres no point in locking if you know it wont go away.

    EXAMPLE

    BUGS

    SEE ALSO
		SetMidiLinkAttrsA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct TagItem *tag;
	struct TagItem *tstate=tags;
	IPTR *where;
	ULONG ret=0;

	ObtainSemaphoreShared(CB(CamdBase)->CLSemaphore);

	while((tag=NextTagItem(&tstate))){
		ret++;
		where=(IPTR *)tag->ti_Data;
		switch(tag->ti_Tag){
			case MLINK_Name:
				*where=(IPTR)midilink->ml_Node.ln_Name;
				break;
			case MLINK_Location:
				*where=(IPTR)midilink->ml_Location->mcl_Node.ln_Name;
				break;
			case MLINK_ChannelMask:
				*where=(IPTR)midilink->ml_ChannelMask;
				break;
			case MLINK_EventMask:
				*where=(IPTR)midilink->ml_EventTypeMask;
				break;
			case MLINK_UserData:
				*where=(IPTR)midilink->ml_UserData;
				break;
			case MLINK_Comment:
				ret--;
				break;
			case MLINK_PortID:
				*where=(IPTR)midilink->ml_PortID;
				break;
			case MLINK_Private:
				*where=(IPTR)midilink->ml_Flags&MLF_PrivateLink;
				break;
			case MLINK_Priority:
				*where=(IPTR)midilink->ml_Node.ln_Pri;
				break;
			case MLINK_SysExFilter:
				*where=(IPTR)midilink->ml_SysExFilter.sxf_Packed;
				break;
			case MLINK_SysExFilterX:
				*where=(IPTR)midilink->ml_SysExFilter.sxf_Packed;
				break;
			case MLINK_Parse:
				*where=(IPTR)midilink->ml_ParserData==0?FALSE:TRUE;
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


#ifdef __amigaos4__
#include <stdarg.h>
ULONG VARARGS68K GetMidiLinkAttrs(
	struct CamdIFace *Self,
	struct MidiLink * ml,
	...
)
{
	va_list ap;
	struct TagItem * varargs;
	va_startlinear(ap, ml);
	varargs = va_getlinearva(ap, struct TagItem *);
	return	Self->GetMidiLinkAttrsA(
		ml,
		varargs);
}
#endif
