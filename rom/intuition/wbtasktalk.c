/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell Workbench task that it shall open/close its windows
    Lang: English
*/

/******************************************************************************/


#include <string.h>

#include "intuition_intern.h"
#include <proto/exec.h>

/******************************************************************************/

void TellWBTaskToOpenWindows(struct IntuitionBase *IntuitionBase)
{
    if (GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL)
    if (GetPrivIBase(IntuitionBase)->WorkBenchMP->mp_SigTask != FindTask(NULL))
    {
        struct MsgPort      replymp;
        struct IntuiMessage imsg;

        /* Setup our reply port. By doing this manually, we can use SIGB_SINGLE
         * and thus avoid allocating a signal (which may fail).*/
        memset( &replymp, 0, sizeof( replymp ) );

        replymp.mp_Node.ln_Type = NT_MSGPORT;
        replymp.mp_Flags        = PA_SIGNAL;
        replymp.mp_SigBit       = SIGB_SINGLE;
        replymp.mp_SigTask      = FindTask( NULL );
        NEWLIST( &replymp.mp_MsgList );

        /* Setup our message. */
    	imsg.ExecMessage.mn_ReplyPort = &replymp;	
        imsg.Class  	    	      = IDCMP_WBENCHMESSAGE;
        imsg.Code   	    	      = WBENCHOPEN;

    	SetSignal(0, SIGF_SINGLE);

        /* Sends it to the handler and wait for the reply. */
        PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP,
		(struct Message *) &imsg );
        WaitPort( &replymp );

        /* After leaving this block imsg and repymp will be automagically freed,
         * so we don't have to deallocate them ourselves. */
    }
}

/******************************************************************************/

void TellWBTaskToCloseWindows(struct IntuitionBase *IntuitionBase)
{
    if (GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL)
    if (GetPrivIBase(IntuitionBase)->WorkBenchMP->mp_SigTask != FindTask(NULL))
    {
        struct MsgPort      replymp;
        struct IntuiMessage imsg;

        /* Setup our reply port. By doing this manually, we can use SIGB_SINGLE
         * and thus avoid allocating a signal (which may fail). */
        memset( &replymp, 0, sizeof( replymp ) );

        replymp.mp_Node.ln_Type = NT_MSGPORT;
        replymp.mp_Flags        = PA_SIGNAL;
        replymp.mp_SigBit       = SIGB_SINGLE;
        replymp.mp_SigTask      = FindTask( NULL );
        NEWLIST( &replymp.mp_MsgList );

        /* Setup our message */
    	imsg.ExecMessage.mn_ReplyPort = &replymp;	
        imsg.Class = IDCMP_WBENCHMESSAGE;
        imsg.Code  = WBENCHCLOSE;

    	SetSignal(0, SIGF_SINGLE);
	
        /* Sends it to the handler and wait for the reply */
        PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP,
		(struct Message *) &imsg );
        WaitPort( &replymp );

        /* After leaving this block imsg and repymp will be automagically freed,
         * so we don't have to deallocate them ourselves. */
    }
}

/******************************************************************************/
