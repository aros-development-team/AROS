/*
    (C) 1996-97 AROS - The Amiga Research OS
    $Id$

    Desc: AROS buttongclass implementation
    Lang: english

    Original version 10/26/96 by caldi@usa.nai.net
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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <proto/boopsi.h>
#include <aros/asmcall.h>
#include <proto/alib.h>
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

VOID notifypressed(Class *cl, Object *o, struct GadgetInfo *ginfo, ULONG flags)
{			

    struct TagItem ntags[] =
    {
    	{GA_ID,  0UL},
    	{TAG_DONE,}
    };
    
    ntags[0].ti_Data = ((EG(o)->Flags & GFLG_SELECTED) ? EG(o)->GadgetID : - EG(o)->GadgetID);

    DoMethod(o, OM_NOTIFY, ntags, ginfo, flags);
    
    return;
}

void buttong_render(Class *cl, Object *o, struct gpRender *msg)
{
    /* We will let the AROS gadgetclass test if it is safe to render */
    if ( DoSuperMethodA(cl, o, (Msg)msg) != 0)
    {
	UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
	struct RastPort *rp = msg->gpr_RPort;
	struct IBox container;

	GetGadgetIBox(o, msg->gpr_GInfo, &container);

	if (container.Width <= 1 || container.Height <= 1)
	    return;

#if 0 /* stegerg: ??? */
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
#endif

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
	    ULONG state;
	    
	    if (EG(o)->Activation & (GACT_LEFTBORDER |
	    			     GACT_TOPBORDER |
				     GACT_RIGHTBORDER |
				     GACT_BOTTOMBORDER))
	    {
	    	if (msg->gpr_GInfo->gi_Window->Flags & WFLG_WINDOWACTIVE)
		{
		    state = IDS_NORMAL;
		}
		else
		{
		    state = IDS_INACTIVENORMAL;
		}
	    }
	    else
	    {
	    	state = IDS_NORMAL;
	    }
	    
	    if ((EG(o)->SelectRender != NULL) &&
		(EG(o)->Flags & GFLG_SELECTED)) /* render selected image */
	    {
		/* center image position, we assume image top and left is 0 */
	        ULONG x = container.Left + ((container.Width / 2) -
		    (IM(EG(o)->SelectRender)->Width / 2));
		ULONG y = container.Top + ((container.Height / 2) -
		    (IM(EG(o)->SelectRender)->Height / 2));

		DrawImageState(rp,
		    IM(EG(o)->SelectRender),
		    x, y,
		    state + IDS_SELECTED,
		    msg->gpr_GInfo->gi_DrInfo );
	    }
	    else if ( EG(o)->GadgetRender != NULL ) /* render normal image */
	    {
	        /* center image position, we assume image top and left is 0 */
	        ULONG x = container.Left + ((container.Width / 2) -
		    (IM(EG(o)->GadgetRender)->Width / 2));
		ULONG y = container.Top + ((container.Height / 2) -
		    (IM(EG(o)->GadgetRender)->Height / 2));

		DrawImageState(rp,
		    IM(EG(o)->GadgetRender),
		    x, y,
		    state + ((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
		    msg->gpr_GInfo->gi_DrInfo);
	    }
	}

#warning Amiga buttongclass does not seem to render gadgetlabel at all

	/* print label */
	printgadgetlabel(cl, o, msg);

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
    }
}

IPTR buttong_handleinput(Class * cl, Object * o, struct gpInput * msg)
{
    IPTR retval = GMR_MEACTIVE;
    struct GadgetInfo *gi = msg->gpi_GInfo;

    if (gi)
    {
	struct InputEvent *ie = ((struct gpInput *)msg)->gpi_IEvent;

	switch( ie->ie_Class )
	{
	case IECLASS_RAWMOUSE:
	    switch( ie->ie_Code )
	    {
	    case SELECTUP:
	        if( EG(o)->Flags & GFLG_SELECTED )
		{
		    struct RastPort *rp;

		    /* mouse is over gadget */
		    EG(o)->Flags &= ~GFLG_SELECTED;

		    if ((rp = ObtainGIRPort(gi)))
		    {
			DoMethod(o, GM_RENDER, gi, rp, GREDRAW_UPDATE);
			ReleaseGIRPort(rp);
		    }
		    retval = GMR_NOREUSE | GMR_VERIFY;
		    *msg->gpi_Termination = IDCMP_GADGETUP;
		}
		else
		    retval = GMR_NOREUSE;
		    
		notifypressed(cl, o, gi, 0);
		break;

	    case IECODE_NOBUTTON:
	        {
		    struct gpHitTest gpht;

		    gpht.MethodID     = GM_HITTEST;
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
			    }
			}
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
			    }
			}
		    }
		    break;
		}

	    default:
	        retval = GMR_REUSE;
		*((struct gpInput *)msg)->gpi_Termination = 0UL;
		break;
	    }
	    break;

	case IECLASS_TIMER:
	    notifypressed(cl, o, msg->gpi_GInfo, OPUF_INTERIM);
	    break;
	}
    }
    else
        retval = GMR_NOREUSE;

    return retval;
}

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/* buttongclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_buttongclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    case GM_RENDER:
        buttong_render(cl, o, (struct gpRender *)msg);
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

		    notifypressed(cl, o, gi, OPUF_INTERIM);

		    retval = GMR_MEACTIVE;
		}
	    }
	}
	break;

    case GM_HANDLEINPUT:
        retval = buttong_handleinput(cl, o, (struct gpInput *)msg);
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

