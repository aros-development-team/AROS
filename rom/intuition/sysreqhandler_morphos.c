/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/keymap.h>

#include "intuition_intern.h"
#include "intuition_customizesupport.h"
#include "requesters.h"

#include <ctype.h>

#define DEBUG_SYSREQHANDLER(x)  ;

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

AROS_LH3(LONG, SysReqHandler,

         /*  SYNOPSIS */
         AROS_LHA(struct Window  *, window, A0),
         AROS_LHA(ULONG   *,        IDCMPFlagsPtr, A1),
         AROS_LHA(BOOL            , WaitInput, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 100, Intuition)

/*  FUNCTION
    Handles a requester, which was opened with BuildSysRequest() or
    BuildEasyRequestArgs(). When this function is called all outstanding
    IDCMP requests are processed. If an IDCMP request that would close
    a normal EasyRequestArgs() is encountered, SysReqHandler() returns
    with a return code equally to the return code EasyRequestArgs()
    would have returned. You may call this function in synchronous or
    asynchronous mode, by setting the WaitInput parameter.
 
    INPUTS
    Window - The window pointer returned by either BuildSysRequest() or
         BuildEasyRequestArgs().
    IDCMPFlagsPtr - Pointer to a ULONG to store the IDCMP flag that was
            received by the window. This will be set if you
            provided additional IDCMP flags to BuildSysRequest() or
            BuildEasyRequest(). You may set this to NULL. You must
            initialize the pointed to ULONG every time you call
            SysReqHandler().
    WaitInput - Set this to TRUE, if you want this function to wait for
            the next IDCMP request, if there is none at the moment
            the function is called.
 
    RESULT
    -2, if the requester was not satisfied. Normally you want to call
        this function at least until this function returns something
        different than -2.
    -1, if one of the IDCMP flags of idcmpPTR was set.
     0, if the rightmost button was clicked or an error occured.
     n, if the n-th button from the left was clicked.
 
    NOTES
 
    EXAMPLE
 
    BUGS
    Gadget placing is still untidy.
    Does not support BuildSysRequest() requesters, yet.
 
    SEE ALSO
    BuildSysRequest(), BuildEasyRequestArgs()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    LONG result;
    struct IntuiMessage *msg;
    struct IntRequestUserData *udata = (struct IntRequestUserData *)window->UserData;

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: window 0x%lx IDCMPPtr 0x%lx WaitInput 0x%lx\n",
                                (ULONG) window,
                                (ULONG) IDCMPFlagsPtr,
                                (ULONG) WaitInput));

    if (window == 0)
    {
        result = 0;
    }
    else if (window == (struct Window *)1)
    {
        result = 1;
    }
    else
    {
        result = -2;

        if (WaitInput)
        {
            WaitPort(window->UserPort);
        }
        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)))
        {
            DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: msg 0x%lx class 0x%lx\n", (ULONG) msg, msg->Class));
            switch (msg->Class)
            {
            /* we don't use VANILLA (filtered from useridcmp!) to get
            all events we need */
            case IDCMP_RAWKEY:
            {
#define RKBUFLEN 1
                struct InputEvent ie;
                char rawbuffer[RKBUFLEN];

                if (msg->Code & IECODE_UP_PREFIX) break;
                
                //hilight support
                if (udata->NumGadgets > 1)
                {
                    if (((msg->Code == 66) && (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))) ||
                         (msg->Code == 79))
                    {
                        intrequest_hilightgadget(udata,udata->ActiveGad,FALSE,IntuitionBase);
                        if (udata->ActiveGad > 0)
                        {
                            udata->ActiveGad --;
                        } else {
                            udata->ActiveGad = udata->NumGadgets - 1;
                        }
                        intrequest_hilightgadget(udata,udata->ActiveGad,TRUE,IntuitionBase);
                    } else if ((msg->Code == 66) || (msg->Code == 78))
                    {
                        intrequest_hilightgadget(udata,udata->ActiveGad,FALSE,IntuitionBase);
                        if (udata->ActiveGad < udata->NumGadgets - 1)
                        {
                            udata->ActiveGad ++;
                        } else {
                            udata->ActiveGad = 0;
                        }
                        intrequest_hilightgadget(udata,udata->ActiveGad,TRUE,IntuitionBase);
                    }
                }

                ie.ie_Class = IECLASS_RAWKEY;
                ie.ie_SubClass = 0;
                ie.ie_Code = msg->Code;
                ie.ie_Qualifier = 0;
                ie.ie_EventAddress = (APTR *) *((ULONG *)msg->IAddress);

                if (KeymapBase && MapRawKey(&ie,rawbuffer,RKBUFLEN,0))
                {
                    if (rawbuffer[0] == 13) //enter key
                    {
                        if (udata->ActiveGad != udata->NumGadgets - 1)
                        {
                            result = udata->ActiveGad + 1;
                        } else {
                            result = 0;
                        }
                    }

                    if (rawbuffer[0] == 27) //escape key
                    {
                        result = 0;
                    }
                }

                ie.ie_Qualifier = msg->Qualifier;
                if (MatchHotkey(&ie,IA_REQUESTERCANCEL,IntuitionBase)) result = 0;

                if (MatchHotkey(&ie,IA_REQUESTEROK,IntuitionBase))
                {
                    if (((struct IntRequestUserData *)window->UserData)->NumGadgets > 1)
                    {
                        result = 1;
                    } else {
                        result = 0;
                    }
                }
            }
                break;

            case IDCMP_GADGETUP:
                result = ((struct Gadget *)msg->IAddress)->GadgetID;
                break;

            case IDCMP_REFRESHWINDOW:
                BeginRefresh(window);
                intrequest_drawrequester(udata,IntuitionBase);
                EndRefresh(window,TRUE);
                break;

            default:
                DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: unknown IDCMP\n"));
                if (result == -2)
                {
                    if (msg->Class &
                            ((struct IntRequestUserData *)window->UserData)->IDCMP)
                    {
                        if (IDCMPFlagsPtr)
                            *IDCMPFlagsPtr = msg->Class;
                        result = -1;
                    }
                }
                break;
            }
            ReplyMsg((struct Message *)msg);

        } /* while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) */

    }

    DEBUG_SYSREQHANDLER(dprintf("SysReqHandler: Result 0x%lx\n",result));

    return result;
    AROS_LIBFUNC_EXIT
} /* SysReqHandler() */
