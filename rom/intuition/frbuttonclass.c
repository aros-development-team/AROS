/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS frbuttonclass implementation
    Lang: english

    Based on buttongclass by caldi@usa.nai.net
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

#ifdef __AROS__
#include "intuition_intern.h"
#include "maybe_boopsi.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

void frbutton_render(Class *cl, Object *o, struct gpRender *msg)
{
    /* We will let the AROS gadgetclass test if it is safe to render */
#warning FIXME:
    /* FIXME: if ( DoSuperMethodA(cl, o, (Msg *)msg) != 0)
    { */
	UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
	struct RastPort *rp = msg->gpr_RPort;
	struct IBox container;

	GetGadgetIBox(o, msg->gpr_GInfo, &container);

	if (container.Width <= 1 || container.Height <= 1)
	    return;

	/* clear gadget */
	if (EG(o)->Flags & GFLG_SELECTED)
	    SetAPen(rp, pens[FILLPEN]);
	else
	    SetAPen(rp, pens[BACKGROUNDPEN]);
	SetDrMd(rp, JAM1);
	RectFill(rp,
	    container.Left,
	    container.Top,
	    container.Left + container.Width - 1,
	    container.Top + container.Height - 1);

	if ((EG(o)->Flags & GFLG_GADGIMAGE) == 0) /* not an image-button */
	{
	    /* draw border */
	    if ((EG(o)->SelectRender != NULL ) &&  (EG(o)->Flags & GFLG_SELECTED))
		DrawBorder(rp,
		    ((struct Border *)EG(o)->SelectRender),
		    container.Left,
		    container.Top);
	    else if (EG(o)->GadgetRender != NULL)
	        DrawBorder(rp,
		    ((struct Border *)EG(o)->GadgetRender),
		    container.Left,
		    container.Top);
	}
	else /* GFLG_GADGIMAGE set */
	{
	    struct TagItem image_tags[] =
	    {
	    	{IA_Width , EG(o)->Width },
		{IA_Height, EG(o)->Height},
		{TAG_DONE   	    	 }
	    };
	    
	    if ((EG(o)->SelectRender != NULL) &&
		(EG(o)->Flags & GFLG_SELECTED)) /* render selected image */
	    {
                ULONG x, y;

		/* center image position, we assume image top and left is 0 */
                SetAttrsA(EG(o)->SelectRender, image_tags);
 
 	        x = container.Left + (container.Width / 2) -
		    (IM(EG(o)->SelectRender)->Width / 2);
		y = container.Top + (container.Height / 2) -
		    (IM(EG(o)->SelectRender)->Height / 2);

		DrawImageState(rp,
		    IM(EG(o)->SelectRender),
		    x, y,
		    IDS_SELECTED,
		    msg->gpr_GInfo->gi_DrInfo );
	    }
	    else if ( EG(o)->GadgetRender != NULL ) /* render normal image */
	    {
                ULONG x, y;

	        /* center image position, we assume image top and left is 0 */
                SetAttrsA(EG(o)->GadgetRender, image_tags);

	        x = container.Left + (container.Width / 2) -
		    (IM(EG(o)->GadgetRender)->Width / 2);
		y = container.Top + (container.Height / 2) -
		    (IM(EG(o)->GadgetRender)->Height / 2);

		DrawImageState(rp,
		    IM(EG(o)->GadgetRender),
		    x, y,
		    ((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
		    msg->gpr_GInfo->gi_DrInfo);
	    }
	}

	/* print label */
	printgadgetlabel(cl, o, msg, IntuitionBase);

	if ( EG(o)->Flags & GFLG_DISABLED )
	{
	    UWORD pattern[] = { 0x8888, 0x2222 };

	    SetDrMd( rp, JAM1 );
	    SetAPen( rp, pens[SHADOWPEN] );
	    SetAfPt( rp, pattern, 1);

	    /* render disable pattern */
	    RectFill(rp,
		container.Left,
		container.Top,
		container.Left + container.Width - 1,
		container.Top + container.Height - 1 );
	}
	/* } FIXME */
}

/****************************************************************************/

AROS_UFH3S(IPTR, dispatch_frbuttonclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
	case GM_RENDER:
            frbutton_render(cl, o, (struct gpRender *)msg);
            break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);

	    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
		* because it would circumvent the subclass from fully overriding it.
		* The check of cl == OCLASS(o) should fail if we have been
		* subclassed, and we have gotten here via DoSuperMethodA().
		*/
	    if ( retval && ( (msg->MethodID != OM_UPDATE) || (cl == OCLASS(o)) ) )
	    {
		struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;
		if (gi)
		{
		    struct RastPort *rp = ObtainGIRPort(gi);
		    if (rp)
		    {
			DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
			ReleaseGIRPort(rp);
		    }
		}
	    }
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_frbuttonclass */


#undef IntuitionBase

/****************************************************************************/

struct IClass *InitFrButtonClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the frbuttonclass...
    */
    if ( (cl = MakeClass(FRBUTTONCLASS, BUTTONGCLASS, NULL, 0, 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_frbuttonclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

/****************************************************************************/
