/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "inputhandler.h"
#include <intuition/gadgetclass.h>

struct RemoveGListActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct Gadget   	    *gadget;
    LONG    	    	     numGad;
    UWORD   	    	     count;
    BOOL    	    	     success;
};

static VOID int_removeglist(struct RemoveGListActionMsg *msg,
                            struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH3(UWORD, RemoveGList,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, remPtr, A0),
         AROS_LHA(struct Gadget *, gadget, A1),
         AROS_LHA(LONG           , numGad, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 74, Intuition)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Gadget   *pred;
    struct Gadget   *last;
    struct IIHData  *iihdata;
    LONG             numGad2;
    UWORD            count;
    BOOL             done = TRUE;

    EXTENDWORD(numGad);

    DEBUG_REMOVEGLIST(dprintf("RemoveGList: Window 0x%lx Gadgets 0x%lx Num %ld\n",
                              remPtr, gadget, numGad));

    if (!numGad) return ~0;
    if (!gadget) return ~0;
    if (!remPtr) return ~0;

#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(remPtr);
#endif

    iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    pred    = (struct Gadget *)&remPtr->FirstGadget;
    count   = 0;
    numGad2 = numGad;

    while (pred->NextGadget && pred->NextGadget != gadget)
    {
        pred = pred->NextGadget;
        count ++;
    }

    if (pred->NextGadget)
    {
        /* Check if one of the gadgets to be removed is the active gadget.
           If it is, then make it inactive! */


        for (last = gadget; last && numGad2--; last = last->NextGadget)
        {
            if ((iihdata->ActiveGadget == last) || (iihdata->ActiveSysGadget == last))
            {
                done = FALSE;
                break;
            }
        }

        if (done)
        {
            for (last = gadget; last->NextGadget && --numGad; last = last->NextGadget) ;

            pred->NextGadget = last->NextGadget;

            /* stegerg: don't do this. DOpus for example relies on gadget->NextGadget
               not being touched */
            /* Emm: but the autodocs say it is done for V36 ??? */
    	#if 0
            last->NextGadget = NULL;
    	#endif
        }


    } /* if (pred->NextGadget) */
    else
    {
        count = ~0;
    }
 
#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(remPtr);
#endif

    /* We tried to remove the active gadget. This must be delayed until LMB
     * is released.
     */
    if (!done)
    {
        struct RemoveGListActionMsg msg;

        msg.window = remPtr;
        msg.gadget = gadget;
        msg.numGad = numGad;

        do
        {
            DEBUG_REMOVEGLIST(dprintf("RemoveGList: trying to remove the active gadget.\n"));
            DoSyncAction((APTR)int_removeglist, &msg.msg, IntuitionBase);
        }
        while (!msg.success);

        count = msg.count;
    }

    DEBUG_REMOVEGLIST(dprintf("RemoveGList: removed %ld gadgets\n",count));

    return count;

    AROS_LIBFUNC_EXIT

} /* RemoveGList */

static VOID int_removeglist(struct RemoveGListActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    struct Window   *remPtr = msg->window;
    struct Gadget   *gadget = msg->gadget;
    LONG    	     numGad = msg->numGad;
    struct Gadget   *pred;
    struct Gadget   *last;
    struct IIHData  *iihdata;
    LONG             numGad2;
    UWORD            count;

    DEBUG_REMOVEGLIST(dprintf("IntRemoveGList: Window 0x%lx Gadgets 0x%lx Num %ld\n",
                              remPtr, gadget, numGad));

    iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    /* Don't remove the gadget until the LMB is released. */
    if (iihdata->ActQualifier & IEQUALIFIER_LEFTBUTTON)
    {
        DEBUG_REMOVEGLIST(dprintf("IntRemoveGList: LMB down\n"));
        msg->success = FALSE;
        return;
    }

#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(remPtr);
#endif

    pred    = (struct Gadget *)&remPtr->FirstGadget;

    count   = 0;
    numGad2 = numGad;

    while (pred->NextGadget && pred->NextGadget != gadget)
    {
        pred = pred->NextGadget;
        count ++;
    }

    if (pred->NextGadget)
    {
        /* Check if one of the gadgets to be removed is the active gadget.
           If it is, then make it inactive! If we got here, one of them was
           the active gadget at the time of RemoveGList, but this may have
           changed. */

        for (last = gadget; last && numGad2--; last = last->NextGadget)
        {
            if ((iihdata->ActiveGadget == last) || (iihdata->ActiveSysGadget == last))
            {
                switch(last->GadgetType & GTYP_GTYPEMASK)
                {
                    case GTYP_CUSTOMGADGET:
                    {
                        struct gpGoInactive gpgi;

                        gpgi.MethodID   = GM_GOINACTIVE;
                        gpgi.gpgi_GInfo = NULL;
                        gpgi.gpgi_Abort = 1;

                        DoGadgetMethodA(last, remPtr, NULL, (Msg)&gpgi);

                        if (SYSGADGET_ACTIVE)
                        {
                            if (IS_BOOPSI_GADGET(iihdata->ActiveSysGadget))
                            {
                                DoGadgetMethodA(iihdata->ActiveSysGadget, remPtr, NULL, (Msg)&gpgi);
                            }
                            iihdata->ActiveSysGadget = NULL;
                        }

                        break;
                    }
                }

                last->Activation &= ~GACT_ACTIVEGADGET;
                iihdata->ActiveGadget = NULL;
            }
        }

        for (last = gadget; last->NextGadget && --numGad; last = last->NextGadget) ;

        pred->NextGadget = last->NextGadget;

        /* stegerg: don't do this. DOpus for example relies on gadget->NextGadget
           not being touched */
        /* Emm: but the autodocs say it is done for V36 ??? */
#if 0
        last->NextGadget = NULL;
#endif

    } /* if (pred->NextGadget) */
    else
    {
        count = ~0;
    }

#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(remPtr);
#endif

    DEBUG_REMOVEGLIST(dprintf("IntRemoveGList: done\n"));
    
    msg->count = count;
    msg->success = TRUE;
}
