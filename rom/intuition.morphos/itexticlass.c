/*
    (C) 1997-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc: AROS itexticlass implementation
    Lang: english
*/

/****************************************************************************/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <string.h>

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include "maybe_boopsi.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif /* !__MORPHOS__ */

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

LONG itext_draw(Class *cl, Object *o, struct impDraw *msg)
{
    struct RastPort *rp = msg->imp_RPort;
    LONG retval = 0;

    if (rp)
    {
        struct IntuiText *iText = (struct IntuiText *)IM(o)->ImageData;
        int leftOffset = msg->imp_Offset.X + IM(o)->LeftEdge;
        int topOffset = msg->imp_Offset.Y + IM(o)->TopEdge;
        struct TextFont *newfont = NULL;
        struct TextFont *font;
        UBYTE style;

        font = rp->Font;
        style = rp->AlgoStyle;

        SetDrMd(rp, JAM1);
        SetAPen(rp, IM(o)->PlanePick);

        /* For all borders... */
        for ( ; iText; iText=iText->NextText)
        {
            if (iText->ITextFont)
            {
                newfont = OpenFont (iText->ITextFont);

                if (newfont)
                    SetFont (rp, newfont);

                SetSoftStyle(rp, iText->ITextFont->ta_Style, AskSoftStyle(rp));
            }

            /* Move to initial position */
            Move (rp
                  , iText->LeftEdge + leftOffset
                  , iText->TopEdge + topOffset + rp->Font->tf_Baseline
                 );
            Text (rp, iText->IText, strlen (iText->IText));

            if (iText->ITextFont)
            {
                if (newfont)
                {
                    SetFont (rp, font);
                    CloseFont (newfont);
                }
            }
        }
        retval = 1;

        rp->AlgoStyle = style;
    }

    return retval;
}

/****************************************************************************/

AROS_UFH3S(IPTR, dispatch_itexticlass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    case IM_DRAW:
        retval = itext_draw(cl, o, (struct impDraw *)msg);
        break;

#warning itexticlass/IM_DRAWFRAME not implemented
#if 0
    case IM_DRAWFRAME:
        itext_drawframe(cl, o, (struct impDraw *)msg);
        break;
#endif

    default:
        retval = DoSuperMethodA(cl, o, msg);
        break;

    } /* switch */

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_itexticlass */


#undef IntuitionBase

/****************************************************************************/

struct IClass *InitITextIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the itexticlass...
    */
    if ( (cl = MakeClass(ITEXTICLASS, IMAGECLASS, NULL, 0, 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_itexticlass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData          = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/****************************************************************************/
