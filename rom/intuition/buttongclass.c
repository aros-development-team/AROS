/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include "intuition_intern.h"
#include "maybe_boopsi.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif

/***********************************************************************************/

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

#undef IntuitionBase
#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

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

/***********************************************************************************/

IPTR buttong_new(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem *ti;
    IPTR    	    retval;
        
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
	ti = FindTagItem(GA_Image, msg->ops_AttrList);
	if (ti)
	{
    	    struct Image *im = (struct Image *)G(retval)->GadgetRender;

    	    if (im)
	    {
		G(retval)->Width  = im->Width;
		G(retval)->Height = im->Height;
	    }
	};  
    }
    
    return retval;
}

/***********************************************************************************/

IPTR buttong_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem *ti;
    IPTR    	    retval;
        
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	* because it would circumvent the subclass from fully overriding it.
	* The check of cl == OCLASS(o) should fail if we have been
	* subclassed, and we have gotten here via DoSuperMethodA().
	*/

    ti = FindTagItem(GA_Image, msg->ops_AttrList);
    if (ti)
    {
    	struct Image *im = (struct Image *)G(o)->GadgetRender;
		
    	if (im)
	{
	    G(o)->Width  = im->Width;
	    G(o)->Height = im->Height;
	}
    };
    
    if ( retval && ( (msg->MethodID != OM_UPDATE) || (cl == OCLASS(o)) ) )
    {
    	struct GadgetInfo *gi = msg->ops_GInfo;
	
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

    return retval;
}

/***********************************************************************************/

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
	    	/* No centering of the image inside the gadget to be done! */
		
		DrawImageState(rp,
		    IM(EG(o)->SelectRender),
		    container.Left, container.Top,
		    state + IDS_SELECTED,
		    msg->gpr_GInfo->gi_DrInfo );
	    }
	    else if ( EG(o)->GadgetRender != NULL ) /* render normal image */
	    {
	    	/* No centering of the image inside the gadget to be done! */
		
		DrawImageState(rp,
		    IM(EG(o)->GadgetRender),
		    container.Left, container.Top,
		    state + ((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
		    msg->gpr_GInfo->gi_DrInfo);
	    }
	}

	#if 0
	/*#warning Amiga buttongclass does not seem to render gadgetlabel at all*/

	/* print label */
	printgadgetlabel(cl, o, msg, IntuitionBase);
	#endif
	
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

/***********************************************************************************/

IPTR buttong_hittest(Class *cl, Object * o, struct gpHitTest * msg)
{
    Object *image = (Object *)EG(o)->GadgetRender;
    
    IPTR retval = GMR_GADGETHIT;
    
    if (image)
    {
        if (((struct Image *)image)->Depth == CUSTOMIMAGEDEPTH)
	{
            struct impHitTest imph;

	    imph.MethodID    = IM_HITTEST;
	    imph.imp_Point.X = msg->gpht_Mouse.X;
	    imph.imp_Point.Y = msg->gpht_Mouse.Y;

	    retval = DoMethodA(image, (Msg)&imph) ? GMR_GADGETHIT : 0;
	}
    }
    
    return retval;
}

/***********************************************************************************/

IPTR buttong_goactive(Class * cl, Object * o, struct gpInput * msg)
{
    struct GadgetInfo  	*gi = msg->gpi_GInfo;
    IPTR		retval = GMR_NOREUSE;

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
    
    return retval;
    
}

/***********************************************************************************/

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
		
	} /* switch( ie->ie_Class ) */
	
    } /* if (gi) */
    else
        retval = GMR_NOREUSE;

    return retval;
}

/***********************************************************************************/

IPTR buttong_goinactive(Class * cl, Object * o, struct gpGoInactive * msg)
{
    struct GadgetInfo *gi = msg->gpgi_GInfo;

    EG(o)->Flags &= ~GFLG_SELECTED;
 
    if (gi)
    {
	struct RastPort *rp = ObtainGIRPort(gi);
	
	if (rp)
	{
	    DoMethod(o, GM_RENDER, gi, rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);

	    DoMethod(o, OM_NOTIFY, NULL, gi, 0);
	}
    }

    return 0;
}

/***********************************************************************************/



/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_buttongclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    	case OM_NEW:
	    retval = buttong_new(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_SET:
	case OM_UPDATE:
	    retval = buttong_set(cl, o, (struct opSet *)msg);
	    break;

	case GM_RENDER:
            buttong_render(cl, o, (struct gpRender *)msg);
	    break;

	case GM_HITTEST:
	    retval = buttong_hittest(cl, o, (struct gpHitTest *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = buttong_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
            retval = buttong_handleinput(cl, o, (struct gpInput *)msg);
	    break;

	case GM_GOINACTIVE:
	    retval = buttong_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return retval;

    AROS_USERFUNC_EXIT

}  /* dispatch_buttongclass */

/***********************************************************************************/

#undef IntuitionBase

/***********************************************************************************/

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

/***********************************************************************************/
