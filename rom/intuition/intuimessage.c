/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Functions for gaining getting and sending intuimessages.
    
    Lang: english
*/

#include <exec/memory.h>
#include <intuition/intuition.h>

#include "intuition_intern.h"

/* !!!!!!! VERY IMPORTANT NOTE !!!!!!!!!!!!
Because of possible race conditions these functions MUST ONLY be used
on the input.device's context. That generally means:
- From within the intuition inpthandler.
- Inside boopsi gadget dispatcher GM_GOACTIVE and GM_HANDLEINPUT methods.
(Window closegadgets use these to send IDCMP_CLOSEWINDOW messages)

*/

inline VOID send_intuimessage(struct IntuiMessage *imsg, struct Window *w, struct IntuitionBase *IntuitionBase)
{

    /* Mark the message as taken */    
    imsg->Class &= ~IDCMP_LONELYMESSAGE;

    /* Reply the message to intuition */
    imsg->ExecMessage.mn_ReplyPort = w->WindowPort;
    imsg->IDCMPWindow = w;
    
    PutMsg(w->UserPort, (struct Message *)imsg);
}


/* Gets a free intuimessage for the supplied window,
and fills in the replyport (intuitions msgport),
and  IDCMPWindow with the supplied window */

struct IntuiMessage *get_intuimessage(struct Window *w,  struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage	*imsg;
    
    imsg = w->MessageKey;

    /* Any reusable intuimessages ? */
    while( imsg && !(imsg->Class & IDCMP_LONELYMESSAGE) )
    {
	imsg = imsg->SpecialLink;
    }
    
    /* Nope. We must allocate a new one */
    if( !imsg )
    {
	imsg = AllocMem(sizeof(struct ExtIntuiMessage), MEMF_CLEAR|MEMF_PUBLIC);
	if( imsg != NULL )
	{
	    /* Add the newly created message to start of list of messages */
	    imsg->SpecialLink = w->MessageKey;
	    w->MessageKey = imsg;
	    
	}
    }
    return imsg;
}
