/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/dos.h>

#include "camd_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH2(void, PutMidi,

/*  SYNOPSIS */
	AROS_LHA(struct MidiLink *, link, A0),
	AROS_LHA(ULONG, msg, D0),

/*  LOCATION */
	struct CamdBase *, CamdBase, 23, Camd)

/*  FUNCTION
		Puts a midimessage to hardware and all sender-links that belongs
		to the midilink's cluster. Does only wait if a hardware send-
		buffer is full, and then tries again and again until the message
		is sent. Else, the function should return immediately.

    INPUTS
		link - pointer to the midilink to send to.
		msg  - The complete message to send. A message can not hold more
		       than 3 bytes, so it fits fine in an ULONG integer. See NOTES
		       to see how a message is built up.

    RESULT

    NOTES
		Sending an illegal message may have serious consequences. If you for
		some reason are not completely shure whether your message is legal,
		you could do the following test:

		if((msg>>24)<0x80 || (msg>>24)==0xf0 || (msg>>24)==0xf7 || (msg>>16&0xff)>0x7f || (msg>>8&0xff)>0x7f){
			debug("Warning, illegal midimessage: %x\n",msg);
		}else{
			PutMidi(midilink,msg);
		}

    EXAMPLE

    BUGS

    SEE ALSO
		PutMidiMsg

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	struct DriverData *driverdata;

	driverdata=GoodPutMidi(link,msg,OUTBUFFERSIZE-1,CamdBase);

	if(driverdata!=NULL){
		while(Midi2Driver(driverdata,msg,OUTBUFFERSIZE-1)==NULL) CamdWait();
	}

   AROS_LIBFUNC_EXIT
}







