/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Tell Workbench task that it shall open/close its windows.
*/

/******************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>
#include "intuition_intern.h"

/******************************************************************************/

ULONG TellWBTaskToOpenWindows(struct IntuitionBase *IntuitionBase)
{
    DEBUG_WORKBENCH(dprintf("TellWBTaskToOpenWindows: currenttask <%s>\n",
                            FindTask(NULL)->tc_Node.ln_Name));
    if
    (
           GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL 
        && GetPrivIBase(IntuitionBase)->WorkBenchMP->mp_SigTask != FindTask(NULL)
    )
    {
        struct IntuiMessage *imsg;

        if ((imsg = AllocIntuiMessage(NULL)))
        {
            /* Setup our message. */
            imsg->ExecMessage.mn_ReplyPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;
            imsg->Class                    = IDCMP_WBENCHMESSAGE;
            imsg->Code                     = WBENCHOPEN;

            DEBUG_WORKBENCH(dprintf("TellWBTaskToOpenWindows: Send Msg\n"));
            /* Sends it to the handler asynchron */
            PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP,
                    &imsg->ExecMessage);

            DEBUG_WORKBENCH(dprintf("TellWBTaskToOpenWindows: done\n"));
            return(TRUE);
        }
        else
        {
            DEBUG_WORKBENCH(dprintf("TellWBTaskToOpenWindows: no memory\n"));
        }
    }
    else
    {
        DEBUG_WORKBENCH(dprintf("TellWBTaskToOpenWindows: no Workbench port\n"));
    }
    return(FALSE);
}

#if 1

/******************************************************************************/

ULONG TellWBTaskToCloseWindows(struct IntuitionBase *IntuitionBase)
{
    DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: currenttask <%s>\n",
                            FindTask(NULL)->tc_Node.ln_Name));

    if
    (
           GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL
        && GetPrivIBase(IntuitionBase)->WorkBenchMP->mp_SigTask != FindTask(NULL)
    )
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

        DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: Send Msg\n"));
        /* Sends it to the handler and wait for the reply */
        PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP,
                (struct Message *) &imsg );
        WaitPort( &replymp );

        /* After leaving this block imsg and repymp will be automagically freed,
         * so we don't have to deallocate them ourselves. */
        DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: done\n"));
        return(TRUE);
    }
    else
    {
        DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: no Workbench port\n"));
    }
    return(FALSE);
}

/******************************************************************************/

#else
/******************************************************************************/

ULONG TellWBTaskToCloseWindows(struct IntuitionBase *IntuitionBase)
{
    DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: currenttask <%s>\n",
                            FindTask(NULL)->tc_Node.ln_Name));

    if( GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL )
    {
        struct IntuiMessage *imsg;

        if ((imsg = AllocIntuiMessage(NULL)))
        {
            /* Setup our message */
            imsg->ExecMessage.mn_ReplyPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;
            imsg->Class     	    	   = IDCMP_WBENCHMESSAGE;
            imsg->Code      	    	   = WBENCHCLOSE;

            DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: Send Msg\n"));
            /* Sends it to the handler asynchron */
            PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP,
                    &imsg->ExecMessage);
            DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: done\n"));
            return(TRUE);
        }
        else
        {
            DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: no memory\n"));
        }
    }
    else
    {
        DEBUG_WORKBENCH(dprintf("TellWBTaskToCloseWindows: no Workbench port\n"));
    }
    return(FALSE);
}

/******************************************************************************/
#endif
