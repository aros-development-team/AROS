/* AROS buttongclass implementation
 * (frameless button, no window border support yet)
 * 10/26/96 caldi@usa.nai.net
 */

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

#ifdef _SASC
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#elif __GNUC__
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/utility_protos.h>
#endif

#ifdef _AROS
#include <aros/asmcall.h>
#include <clib/alib_protos.h>
#include "intuition_intern.h"
#include "gadgets.h"
#endif

/****************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/****************************************************************************/


#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/* buttongclass boopsi dispatcher
 */
AROS_UFH3(static IPTR, dispatch_buttongclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    case GM_RENDER:
	/* We will let the AROS gadgetclass test if it is safe to render */
	if ( DoSuperMethodA(cl, o, msg) != 0)
	{
	    UWORD *pens = ((struct gpRender *)msg)->gpr_GInfo->gi_DrInfo->dri_Pens;
	    struct RastPort *rp = ((struct gpRender *)msg)->gpr_RPort;
	    struct IBox container;

	    GetGadgetIBox(o, ((struct gpRender *)msg)->gpr_GInfo, &container);

	    if (container.Width <= 1 || container.Height <= 1)
		return(retval);

	    if ( EG(o)->Flags & GFLG_SELECTED )
	    {
		SetAPen( rp, pens[FILLPEN] );
	    }
	    else
	    {
		SetAPen( rp, pens[BACKGROUNDPEN] );
	    } /* if */

	    SetDrMd( rp, JAM1 );

	    RectFill( rp,
		container.Left,
		container.Top,
		container.Left + container.Width - 1,
		container.Top + container.Height - 1
	    );

	    if ( (EG(o)->Flags & GFLG_GADGIMAGE) == 0 )
	    {
		if ( ( EG(o)->SelectRender != NULL ) && ( EG(o)->Flags & GFLG_SELECTED ) )
		{
		    DrawBorder(rp, ((struct Border *)EG(o)->SelectRender), container.Left, container.Top );
		}
		else if ( EG(o)->GadgetRender != NULL )
		{
		    DrawBorder(rp, ((struct Border *)EG(o)->GadgetRender), container.Left, container.Top );
		}
	    }
	    else if ( EG(o)->Flags & GFLG_GADGIMAGE )
	    {
		if ( ( EG(o)->SelectRender != NULL ) && ( EG(o)->Flags & GFLG_SELECTED ) )
		{
		    /* center image position, we assume image top and left is 0 */
		    ULONG x = container.Left + ((container.Width / 2) - (IM(EG(o)->SelectRender)->Width / 2));
		    ULONG y = container.Top + ((container.Height / 2) - (IM(EG(o)->SelectRender)->Height / 2));

		    DrawImageState( rp,
			IM(EG(o)->SelectRender),
			x, y,
			( (EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
			((struct gpRender *)msg)->gpr_GInfo->gi_DrInfo );
		}
		else if ( EG(o)->GadgetRender != NULL )
		{
		    /* center image position, we assume image top and left is 0 */
		    ULONG x = container.Left + ((container.Width / 2) - (IM(EG(o)->GadgetRender)->Width / 2));
		    ULONG y = container.Top + ((container.Height / 2) - (IM(EG(o)->GadgetRender)->Height / 2));

		    DrawImageState( rp,
			IM(EG(o)->GadgetRender),
			x, y,
			( (EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
			((struct gpRender *)msg)->gpr_GInfo->gi_DrInfo );
		}
	    }

	    switch (EG(o)->Flags & GFLG_LABELMASK)
	    {
		case GFLG_LABELITEXT:
		    PrintIText( rp, EG(o)->GadgetText, container.Left, container.Top );
		    break;

		case GFLG_LABELSTRING:
		    if( EG(o)->GadgetText != NULL )
		    {
			ULONG len;

			if ((len = strlen ((STRPTR) EG(o)->GadgetText)) > 0UL)
			{
			    ULONG x;
			    ULONG y;

			    x = container.Left + (container.Width / 2);
			    x -= LabelWidth (rp, (STRPTR) EG(o)->GadgetText, len, IntuitionBase) / 2;

			    y = container.Top + (container.Height / 2) + rp->Font->tf_Baseline;
			    y -= rp->Font->tf_YSize / 2;

			    SetAPen (rp, pens[TEXTPEN] );

			    Move (rp, x, y );
			    RenderLabel (rp, (STRPTR) EG(o)->GadgetText, len, IntuitionBase);
			}
		    }
		    break;

		case GFLG_LABELIMAGE:
		    {
			/* center image position, we assume image top and left is 0 */
			ULONG x = container.Left + ((container.Width / 2) - (IM(EG(o)->GadgetText)->Width / 2));
			ULONG y = container.Top + ((container.Height / 2) - (IM(EG(o)->GadgetText)->Height / 2));

			DrawImageState( rp,
			    IM(EG(o)->GadgetText),
			    x, y,
			    ( (EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
			    ((struct gpRender *)msg)->gpr_GInfo->gi_DrInfo );
		    }
		    break;
	    }

	    if ( EG(o)->Flags & GFLG_DISABLED )
	    {
		UWORD pattern[] = { 0x8888, 0x2222 };

		SetDrMd( rp, JAM1 );
		SetAPen( rp, pens[SHADOWPEN] );
		SetAfPt( rp, pattern, 1);

		/* render disable pattern */
		RectFill( rp,
		    container.Left,
		    container.Top,
		    container.Left + container.Width - 1,
		    container.Top + container.Height - 1 );
	    } /* if */
	} /* if */
	break;

    case GM_LAYOUT:
	break;

    case GM_DOMAIN:
	break;

    case GM_GOACTIVE:
	{
	    struct GadgetInfo *gi = ((struct gpInput *)msg)->gpi_GInfo;

	    if (gi)
	    {
		struct RastPort *rp = ObtainGIRPort(gi);
		if (rp)
		{
		    EG(o)->Flags |= GFLG_SELECTED;

		    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
		    ReleaseGIRPort(rp);

		    DoMethod(o, OM_NOTIFY, gi, NULL, OPUF_INTERIM);

		    retval = GMR_MEACTIVE;
		} /* if */
	    } /* if */
	} /* if */
	break;

    case GM_HANDLEINPUT:
	{
	    struct GadgetInfo *gi = ((struct gpInput *)msg)->gpi_GInfo;

	    if (gi)
	    {
		struct InputEvent *ie = ((struct gpInput *)msg)->gpi_IEvent;

		retval = GMR_MEACTIVE;

		switch( ie->ie_Class )
		{
		case IECLASS_RAWMOUSE:
		    switch( ie->ie_Code )
		    {
		    case MENUDOWN:
			retval = GMR_REUSE;
			*((struct gpInput *)msg)->gpi_Termination = 0UL;
			break;

		    case SELECTUP:
			if( EG(o)->Flags & GFLG_SELECTED )
			{
			    retval = GMR_NOREUSE | GMR_VERIFY;
			    *((struct gpInput *)msg)->gpi_Termination = 1UL;
			}
			else
			{
			    retval = GMR_NOREUSE;
			    *((struct gpInput *)msg)->gpi_Termination = 0UL;
			} /* if */
			break;

		    case IECODE_NOBUTTON: {
			struct gpHitTest gpht;

			gpht.MethodID	  = GM_HITTEST;
			gpht.gpht_GInfo   = gi;
			gpht.gpht_Mouse.X = ((struct gpInput *)msg)->gpi_Mouse.X;
			gpht.gpht_Mouse.Y = ((struct gpInput *)msg)->gpi_Mouse.Y;

			/*
			    This case handles selection state toggling when the
			    left button is depressed and the mouse is moved
			    around on/off the gadget bounds.
			*/
			if ( DoMethodA(o, (Msg)&gpht) == GMR_GADGETHIT )
			{
			    if ( (EG(o)->Flags & GFLG_SELECTED) == 0 )
			    {
				struct RastPort *rp;

				/* mouse is over gadget */
				EG(o)->Flags |= GFLG_SELECTED;

				if ((rp = ObtainGIRPort(gi)))
				{
				    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_UPDATE);
				    ReleaseGIRPort(rp);
				} /* if */
			    } /* if */
			}
			else
			{
			    if ( (EG(o)->Flags & GFLG_SELECTED) != 0 )
			    {
				struct RastPort *rp;

				/* mouse is not over gadget */
				EG(o)->Flags &= ~GFLG_SELECTED;

				if ((rp = ObtainGIRPort(gi)))
				{
				    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_UPDATE);
				    ReleaseGIRPort(rp);
				} /* if */
			    } /* if */
			} /* if */
			break; }

		    } /* switch */

		    break;

		case IECLASS_TIMER:
		    if (EG(o)->Flags & GFLG_SELECTED)
		    {
			DoMethod(o, OM_NOTIFY, NULL, gi, OPUF_INTERIM);
		    } /* if */
		    break;
		} /* switch */
	    }
	    else
	    {
		/* if we get here something is *really* wrong */
		retval = GMR_NOREUSE;
	    } /* if */
	} /* if */
	break;

    case GM_GOINACTIVE:
	EG(o)->Flags &= ~GFLG_SELECTED;
	{
	    struct GadgetInfo *gi = ((struct gpGoInactive *)msg)->gpgi_GInfo;

	    if (gi)
	    {
		struct RastPort *rp = ObtainGIRPort(gi);
		if (rp)
		{
		    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
		    ReleaseGIRPort(rp);

		    DoMethod(o, OM_NOTIFY, NULL, gi, 0);
		} /* if */
	    } /* if */
	} /* if */
	break;

    case OM_NEW:
	retval = DoSuperMethodA(cl, o, msg);

	if (retval)
	{
	    /* Handle our special tags - overrides defaults */
	    /* set_buttongclass(cl, (Object *)retval, (struct opSet *)msg); */
	} /* if */
	break;

    case OM_SET:
    case OM_UPDATE:
	retval = DoSuperMethodA(cl, o, msg);

	/* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	    * because it would circumvent the subclass from fully overriding it.
	    * The check of cl == OCLASS(o) should fail if we have been
	    * subclassed, and we have gotten here via DoSuperMethodA().
	    */
	if ( retval && ( msg->MethodID == OM_UPDATE ) && ( cl == OCLASS(o) ) )
	{
	    struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;
	    if (gi)
	    {
		struct RastPort *rp = ObtainGIRPort(gi);
		if (rp)
		{
		    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
		    ReleaseGIRPort(rp);
		} /* if */
	    } /* if */
	} /* if */
	break;

    default:
	retval = DoSuperMethodA(cl, o, msg);
	break;
    } /* switch */

    return retval;
}  /* dispatch_buttongclass */


#undef IntuitionBase

/****************************************************************************/

/* Initialize our image class. */
struct IClass *InitButtonGClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the buttongclass...
    */
    if ( (cl = MakeClass(BUTTONGCLASS, GADGETCLASS, NULL, 0, 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_buttongclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

