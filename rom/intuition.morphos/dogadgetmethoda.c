/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Implementation of DoGadgetMethodA
    Lang: english
*/
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include "intuition_intern.h"
#include "gadgets.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH4(IPTR, DoGadgetMethodA,

         /*  SYNOPSIS */
         AROS_LHA(struct Gadget    *, gad, A0),
         AROS_LHA(struct Window    *, win, A1),
         AROS_LHA(struct Requester *, req, A2),
         AROS_LHA(Msg               , msg, A3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 135, Intuition)

/*  FUNCTION
    Invokes a boopsi method on a object with a GadgetInfo derived from
    the supplied window or requester parameter.

    INPUTS
    gad - The gadget to work on
    win - The window which contains the gadget or the requester with
        the gadgets.
    req - If the gadget is in a requester, you must specify that one,
        too.
    message - Send this message to the gadget.

    RESULT
    The result depends on the contents of the message sent to the
    gadget.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    I have derived from a simular function from ClassAct where I have
    to "fake" the function which is not implemented under OS 2.04.
    There are likely a few differences between this routine and the
    real code, but this gets the job done.

    One thing to note, the Amiga Rom routinecauses some form of
    (layer?) locking. I presume the point of the lock is to avoid
    removing the gadget from the window durring a refresh, or to avoid
    resizing the window durring refresh, etc.

    This locking is fairly obvious within Workbench itself. When
    refreshing most any boopsi gadget(s) via RefreshGList() and you try
    to drag a Workbench icon you will get stuck in a layer lock.
    Workbench has a deadlock timer and is smart enough to release the
    lock and abort the drag. With this routine below this locking does
    not occur. Some might call it a good thing, however the issue
    should be revisited once more of Intuition has been implemented -
    if it hasn't been already?!. :)

    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
    25-10-96    calid   submitted the code

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct GadgetInfo   *gi = &GetPrivIBase(IntuitionBase)->DoGadgetMethodGI;
    struct RastPort *rp = &GetPrivIBase(IntuitionBase)->DoGadgetMethodRP;

    IPTR        ret = 0;

    DEBUG_DOGADGETMETHOD(dprintf("DoGadgetMethod[%x]: Gad %p Win %p Req %p Method %ld (0x%lx)\n",
                                 gi, gad, win, req, msg->MethodID, msg->MethodID));

    if (!win && req) win = req->RWindow;

    if (gad && (((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) || (gad->GadgetType & 0x100))) /* OS routines work with NULL objects */
    {

        /* Protect DoMethodA against race conditions between app task
        and input.device task (which executes Intuitions Inputhandler) */

        if (win)
        {
            if (IS_SCREEN_GADGET(gad))
            {
                if (((struct Screen*)win)->BarLayer) LockLayer(0,((struct Screen*)win)->BarLayer);
            } else {
                //jDc: we don't check to which rp the gadget belongs, so let's lock both layers
                LOCKWINDOWLAYERS(win);
            }
        }

        //jDc: lock above doesn't of course protect against simultaneous use of
        //our static rastport and gadgetinfo, let's use gadgetlock here, and ONLY here ;)
        //maybe we should have a static ginfo and rp per window?
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

        //jDc: SetupGInfo fails when there's no window (but it also clears gadgetinfo!)
        SetupGInfo(gi, win, req, gad, IntuitionBase);

        if (gi->gi_RastPort)
        {
            InitRastPort(rp);
            rp->Layer = gi->gi_RastPort->Layer;
            rp->BitMap = gi->gi_RastPort->BitMap;
            gi->gi_RastPort = rp;
            SetFont(rp, gi->gi_DrInfo->dri_Font);
        }

        switch (msg->MethodID)
        {
        case OM_NEW:
        case OM_SET:
        case OM_NOTIFY:
        case OM_UPDATE:
            ((struct opSet *)msg)->ops_GInfo = gi;
            ret = Custom_DoMethodA(gad, msg);
            break;

        case GM_LAYOUT:
            ((struct gpLayout *)msg)->gpl_GInfo = gi;
            ret = Custom_DoMethodA(gad, msg);
            break;

        case GM_RENDER:
            {
                struct RastPort *rport;

                /* Allocate a clone rastport derived from the GadgetInfo
                 * whose layer clipping information has been nulled out...
                 */
                rport = ObtainGIRPort(gi);

                if (rport)
                {
                    //init intuition's global dogadmethod rp with obtained data
                    CopyMem(rport,rp,sizeof (struct RastPort));

                    if (gi->gi_DrInfo)
                    {
                        SetFont(rp, gi->gi_DrInfo->dri_Font);
                    }

                    ((struct gpRender *)msg)->gpr_RPort = rp;
                    ((struct gpRender *)msg)->gpr_GInfo = gi;

                    ret = Custom_DoMethodA(gad, msg);

                    ReleaseGIRPort(rport);
                }
            }
            break;

        default:
            ((struct gpRender *)msg)->gpr_GInfo = gi;
            ret = Custom_DoMethodA (gad, msg);
            break;

        } /* switch (msg->MethodID) */

        if (gi->gi_DrInfo) FreeScreenDrawInfo (gi->gi_Screen, gi->gi_DrInfo );

        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

        if (win)
        {
            if (IS_SCREEN_GADGET(gad))
            {
                if (((struct Screen*)win)->BarLayer) UnlockLayer(((struct Screen*)win)->BarLayer);
            } else {
                UNLOCKWINDOWLAYERS(win);
            }
        }

    } /* if (gad) */

    DEBUG_DOGADGETMETHOD(dprintf("DoGadgetMethod[%x]: Return 0x%lx\n", gi, ret));

    return( (ULONG)ret );

    AROS_LIBFUNC_EXIT

} /* DoGadgetMethodA */
