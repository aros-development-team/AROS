/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function CloseWorkBench()
    Lang: english
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/*****************************************************************************

    NAME */

    AROS_LH0(LONG, CloseWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
    struct IntuitionBase *, IntuitionBase, 13, Intuition)

/*  FUNCTION
	Attempt to close the Workbench screen:
	- Check for open application windows. return FALSE if there are any
	- Clean up all special buffers
	- Close the Workbench screen
	- Make the Workbench program mostly inactive
	  (disk activity will still be monitored)
	- Return TRUE

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* If there is a Workbench process running, tell it to close it's windows. */
    if( GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL ) {
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
        PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP, (struct Message *) (&imsg) );
        WaitPort( &replymp );

        /* After leaving this block imsg and repymp will be automagically freed,
         * so we don't have to deallocate them ourselves. */
    }

    /* Try to close the Workbench screen, if there is any. */
    if( GetPrivIBase(IntuitionBase)->WorkBench != NULL ) {
        if( CloseScreen( GetPrivIBase(IntuitionBase)->WorkBench ) == TRUE ) {
            GetPrivIBase(IntuitionBase)->WorkBench = NULL;
            return TRUE;
        }
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* CloseWorkBench */
