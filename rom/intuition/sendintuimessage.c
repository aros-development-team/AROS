/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/timer.h>
#include "intuition_intern.h"

#define DEBUG_SENDINTUIMESSAGE(x)   ;
/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, SendIntuiMessage,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct IntuiMessage *, imsg, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 151, Intuition)

/*  FUNCTION
    Private: send an IntuiMessage to an Intuition window
 
    INPUTS
    window - The window to which the IntuiMessage shall be sent
        imsg   - The IntuiMessage to send, which must have been allocated with
         AllocIntuiMessage.
 
    RESULT
        none
 
    NOTES
        The caller of this function should first check himself
    whether window->UserPort is NULL. And in this case do not
    call this function at all.
 
    If inside this function the window->UserPort turns out to
    be NULL, then what happens is, that the IntuiMessage is
    immediately ReplyMessage()ed in here, just like if this was
    done by the app whose window was supposed to get the
    IntuiMessage.
 
    The protection with Forbid() is necessary, because of the
    way shared window userports are handled, when one of this
    windows is closed, where there is also just a protection with
    Forbid() when stripping those IntuiMessages from the port
    which belong to the window which is going to be closed.
 
    This function does not check whether the window to which
    the IntuiMessage is supposed to be sent, really wants to
    get the IDCMP in question, that is, whether the corresponding
    flag in window->IDCMPFLags is set.
 
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ASSERT_VALID_PTR(window);
    ASSERT_VALID_PTR(imsg);

    DEBUG_SENDINTUIMESSAGE(dprintf("SendIntuiMessage: Window 0x%lx Port 0x%lx Msg 0x%lx\n",
                                   window, window->UserPort, imsg));

    SANITY_CHECK(window)
    SANITY_CHECK(imsg)

    Forbid();

#ifdef __MORPHOS__
    if (window->UserPort)
    {
        struct Task *apptask = window->UserPort->mp_SigTask;
	
        if (apptask && (!apptask->tc_SigWait) && (apptask->tc_State == TS_WAIT))
        {
            //task is DEAD!
            imsg->IDCMPWindow = 0;
            imsg->Code = 0;
            imsg->Qualifier = 0;
            ReplyMsg(&imsg->ExecMessage);
	    
            if (IW(window)->messagecache)
            {
                IW(window)->messagecache->IDCMPWindow = 0;
                IW(window)->messagecache->Code = 0;
                IW(window)->messagecache->Qualifier = 0;
                ReplyMsg(&IW(window)->messagecache->ExecMessage);
                IW(window)->messagecache = 0;
            }
            Permit();

            //give some visual feedback to the user
            IW(window)->specialflags |= SPFLAG_IAMDEAD;

            int_refreshwindowframe(window,REFRESHGAD_TOPBORDER,0,IntuitionBase);

            return;
        }
	else
	{
            IW(window)->specialflags &= ~SPFLAG_IAMDEAD;
        }
    }
#endif

    if (imsg->Qualifier & IEQUALIFIER_REPEAT)
    {
        IW(window)->num_repeatevents++;
    }

#if USE_IDCMPUPDATE_MESSAGECACHE
    if (imsg->Class == IDCMP_IDCMPUPDATE)
    {
        if (IW(window)->num_idcmpupdate && !IW(window)->messagecache)
        {
kprintf("======= setting messagecache\n");
            IW(window)->messagecache = imsg;
            Permit();
            return;
        }

        //reduce number of messages if possible (prop updates only!)
        if (IW(window)->num_idcmpupdate && IW(window)->messagecache)
        {
            if (imsg->Code == IW(window)->messagecache->Code && imsg->Qualifier == IEQUALIFIER_REPEAT)
            {
                IW(window)->messagecache->MouseX = imsg->MouseX;
                IW(window)->messagecache->MouseY = imsg->MouseY;
                IW(window)->messagecache->Seconds = imsg->Seconds;
                IW(window)->messagecache->Micros = imsg->Micros;
                imsg->IDCMPWindow = 0;
                imsg->Code = 0;
                imsg->Qualifier = 0;
                ReplyMsg(&imsg->ExecMessage);
                Permit();
                return;
            }
        }

        IW(window)->num_idcmpupdate++;
    }
#endif

    Forbid();
    GetSysTime(&((struct IntWindow *)(window))->lastmsgsent);
    Permit();

    if (window->UserPort)
    {
        if (imsg->Class == IDCMP_INTUITICKS)
        {
            window->Flags |= WFLG_WINDOWTICKED;
        }
        DEBUG_SENDINTUIMESSAGE(dprintf("SendIntuiMessage: Class 0x%lx Code 0x%lx Qual 0x%lx Mouse %d,%d IAddress 0x%lx\n", imsg->Class, imsg->Code, imsg->Qualifier, imsg->MouseX, imsg->MouseY, imsg->IAddress));

        PutMsg(window->UserPort, &imsg->ExecMessage);

        /* Help sucky programs... (die DigiBooster, die!) */
        window->MessageKey = imsg;
    }
    else
    {
        ReplyMsg(&imsg->ExecMessage);
    }

    Permit();

    AROS_LIBFUNC_EXIT
}
