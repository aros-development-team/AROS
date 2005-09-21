/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    AROS buttongclass implementation.
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

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif /* !__MORPHOS__ */

#define DEBUG_BUTTON(x) ;
/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/***********************************************************************************/

static VOID notifypressed(Class *cl, struct Gadget *g, struct GadgetInfo *ginfo, ULONG flags)
{
    struct opUpdate method;

    struct TagItem  ntags[] =
    {
        { GA_ID	    , 0UL   },
        { TAG_DONE   	    }
    };

    method.MethodID 	= OM_NOTIFY;
    method.opu_AttrList = ntags;
    method.opu_GInfo 	= ginfo;
    method.opu_Flags 	= flags;

    ntags[0].ti_Data = ((g->Flags & GFLG_SELECTED) ? g->GadgetID : - g->GadgetID);

    DoMethodA((Object *)g, (Msg)&method);
}

/***********************************************************************************/

IPTR ButtonGClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem *ti;
    struct Gadget *g;
        
    g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);
    if (g)
    {
	ti = FindTagItem(GA_Image, msg->ops_AttrList);
	if (ti)
	{
    	    struct Image *im = (struct Image *)g->GadgetRender;

    	    if (im)
	    {
		g->Width  = im->Width;
		g->Height = im->Height;
	    }
	};  
    }
    
    return (IPTR)g;
}

/***********************************************************************************/

IPTR ButtonGClass__OM_SET(Class *cl, struct Gadget *g, struct opSet *msg)
{
    struct TagItem *ti;
    IPTR    	    retval;
        
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);

    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	* because it would circumvent the subclass from fully overriding it.
	* The check of cl == OCLASS(o) should fail if we have been
	* subclassed, and we have gotten here via DoSuperMethodA().
	*/

    ti = FindTagItem(GA_Image, msg->ops_AttrList);
    if (ti)
    {
    	struct Image *im = (struct Image *)g->GadgetRender;
		
    	if (im)
	{
	    g->Width  = im->Width;
	    g->Height = im->Height;
	}
    };
    
    if ( retval && ( (msg->MethodID != OM_UPDATE) || (cl == OCLASS(g)) ) )
    {
    	struct GadgetInfo *gi = msg->ops_GInfo;
	
	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);
	    
	    if (rp)
	    {
		DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
		ReleaseGIRPort(rp);
	    }
	} 
    } 

    return retval;
}

/***********************************************************************************/

IPTR ButtonGClass__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    /* We will let the AROS gadgetclass test if it is safe to render */
    if ( DoSuperMethodA(cl, (Object *)g, (Msg)msg) != 0 && msg->gpr_GInfo)
    {
        UWORD 	    	*pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
        struct RastPort *rp = msg->gpr_RPort;
        struct IBox 	 container;

        SANITY_CHECK(msg->gpr_RPort)

        /*dprintf("button_render: rp %p[%p] win %p[%p] req %p[%p] gi->rp %p[%p]\n",
                rp, rp->Layer,
                msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Window->WLayer,
                msg->gpr_GInfo->gi_Requester, msg->gpr_GInfo->gi_Requester->ReqLayer,
                msg->gpr_GInfo->gi_RastPort, msg->gpr_GInfo->gi_RastPort->Layer);*/

        GetGadgetIBox((struct Gadget *)g, msg->gpr_GInfo, &container);

        if (container.Width <= 1 || container.Height <= 1)
            return (IPTR)0;

        if ((g->Flags & GFLG_GADGIMAGE) == 0) /* not an image-button */
        {
            /* draw border */
            if ((g->SelectRender != NULL ) &&  (g->Flags & GFLG_SELECTED))
                DrawBorder(rp,
                       ((struct Border *)g->SelectRender),
                       container.Left,
                       container.Top);
            else if (g->GadgetRender != NULL)
                DrawBorder(rp,
                       ((struct Border *)g->GadgetRender),
                       container.Left,
                       container.Top);

        }
        else /* GFLG_GADGIMAGE set */
        {
            ULONG state;

            if (g->Activation & (GACT_LEFTBORDER |
                         GACT_TOPBORDER |
                         GACT_RIGHTBORDER |
                         GACT_BOTTOMBORDER |
                         GACT_BORDERSNIFF))
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

            if ((g->SelectRender != NULL) &&
                (g->Flags & GFLG_SELECTED)) /* render selected image */
            {
	    	/* No centering of the image inside the gadget to be done! */

                DrawImageState(rp,
                           (struct Image *)g->SelectRender,
                           container.Left, container.Top,
                           state + IDS_SELECTED,
                           msg->gpr_GInfo->gi_DrInfo );
            }
            else if ( g->GadgetRender != NULL ) /* render normal image */
            {
	    	/* No centering of the image inside the gadget to be done! */
		
                DrawImageState(rp,
                           (struct Image *)g->GadgetRender,
                           container.Left, container.Top,
                           state + ((g->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
                           msg->gpr_GInfo->gi_DrInfo);
            }
        }

#if 0
        /*#warning Amiga buttongclass does not seem to render gadgetlabel at all*/

        /* print label */
        printgadgetlabel(cl, (Object *)g, msg, IntuitionBase);
#endif

        if ( g->Flags & GFLG_DISABLED )
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
    
    return (IPTR)0;
}

/***********************************************************************************/

IPTR ButtonGClass__GM_HITTEST(Class *cl, struct Gadget *g, struct gpHitTest * msg)
{
    struct Image *image = (struct Image *)g->GadgetRender;

    IPTR retval = GMR_GADGETHIT;

    if (image)
    {
        if (image->Depth == CUSTOMIMAGEDEPTH)
        {
            struct impHitTest imph;

            imph.MethodID    = IM_HITTEST;
            imph.imp_Point.X = msg->gpht_Mouse.X;
            imph.imp_Point.Y = msg->gpht_Mouse.Y;

            retval = DoMethodA((Object *)image, (Msg)&imph) ? GMR_GADGETHIT : 0;
        }
        else
        {
                DEBUG_BUTTON(dprintf("buttong_hittest without customimage called\n");)
        }
    }

    return retval;
}

/***********************************************************************************/

IPTR ButtonGClass__GM_GOACTIVE(Class * cl, struct Gadget * g, struct gpInput * msg)
{
    struct GadgetInfo *gi = msg->gpi_GInfo;
    IPTR    	       retval = GMR_NOREUSE;


    if (gi)
    {
        struct RastPort *rp = ObtainGIRPort(gi);
        if (rp)
        {
            struct gpRender method;

            g->Flags ^= GFLG_SELECTED;

            method.MethodID   = GM_RENDER;
            method.gpr_GInfo  = gi;
            method.gpr_RPort  = rp;
            method.gpr_Redraw = GREDRAW_REDRAW;


            DoMethodA((Object *)g, (Msg)&method);
            ReleaseGIRPort(rp);

            /* Let's support toggleselect buttons too, for SnoopDos (DIE!),
             * which creates gadtools buttons and sets the toggleselect flag
             * afterwards (spit).
             */
            if (!(g->Activation & GACT_TOGGLESELECT))
            {
                notifypressed(cl, g, gi, OPUF_INTERIM);
                retval = GMR_MEACTIVE;
            }
            else
            {
                notifypressed(cl, g, gi, 0);
                retval = GMR_NOREUSE | GMR_VERIFY;
            }
        }
    }

    return retval;

}

/***********************************************************************************/

IPTR ButtonGClass__GM_HANDLEINPUT(Class * cl, struct Gadget * g, struct gpInput * msg)
{
    struct GadgetInfo  *gi = msg->gpi_GInfo;
    IPTR    	    	retval = GMR_MEACTIVE;

    if (gi)
    {
        struct InputEvent *ie = ((struct gpInput *)msg)->gpi_IEvent;

        switch( ie->ie_Class )
        {
            case IECLASS_RAWMOUSE:
        	switch( ie->ie_Code )
        	{
        	case SELECTUP:
                    if( g->Flags & GFLG_SELECTED )
                    {
                	struct RastPort *rp;

                	/* mouse is over gadget */
                	g->Flags &= ~GFLG_SELECTED;

                	if ((rp = ObtainGIRPort(gi)))
                	{
                            struct gpRender method;
			    
                            method.MethodID   = GM_RENDER;
                            method.gpr_GInfo  = gi;
                            method.gpr_RPort  = rp;
                            method.gpr_Redraw = GREDRAW_UPDATE;
			    
                            DoMethodA((Object *)g, (Msg)&method);
			    
                            ReleaseGIRPort(rp);
                	}
			
                	retval = GMR_NOREUSE | GMR_VERIFY;
                	/* This doesn't really make sense, as Termination will
                	 * be sent in msg->Code. The original intuition puts
                	 * junk here.
                	 */
                	*msg->gpi_Termination = IDCMP_GADGETUP;
                    }
                    else
                	retval = GMR_NOREUSE;

                    notifypressed(cl, g, gi, 0);
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
                	if ( DoMethodA((Object *)g, (Msg)&gpht) == GMR_GADGETHIT )
                	{
                            if ( (g->Flags & GFLG_SELECTED) == 0 )
                            {
                        	struct RastPort *rp;

                        	/* mouse is over gadget */
                        	g->Flags |= GFLG_SELECTED;

                        	if ((rp = ObtainGIRPort(gi)))
                        	{
                                    struct gpRender method;
				    
                                    method.MethodID   = GM_RENDER;
                                    method.gpr_GInfo  = gi;
                                    method.gpr_RPort  = rp;
                                    method.gpr_Redraw = GREDRAW_UPDATE;
				    
                                    DoMethodA((Object *)g, (Msg)&method);
				    
                                    ReleaseGIRPort(rp);
                        	}
                            }
                	}
                	else
                	{
                            if ( (g->Flags & GFLG_SELECTED) != 0 )
                            {
                        	struct RastPort *rp;

                        	/* mouse is not over gadget */
                        	g->Flags &= ~GFLG_SELECTED;

                        	if ((rp = ObtainGIRPort(gi)))
                        	{
                                    struct gpRender method;
				    
                                    method.MethodID   = GM_RENDER;
                                    method.gpr_GInfo  = gi;
                                    method.gpr_RPort  = rp;
                                    method.gpr_Redraw = GREDRAW_UPDATE;
				    
                                    DoMethodA((Object *)g, (Msg)&method);
				    
                                    ReleaseGIRPort(rp);
                        	}
                            }
                	}
                	break;
			
                    } /**/

        	default:
                    retval = GMR_REUSE;
                    *((struct gpInput *)msg)->gpi_Termination = 0UL;
                    break;
		    
        	} /* switch( ie->ie_Code ) */
        	break;

            case IECLASS_TIMER:
        	notifypressed(cl, g, msg->gpi_GInfo, OPUF_INTERIM);
        	break;

        } /* switch( ie->ie_Class ) */

    } /* if (gi) */
    else
    {
        retval = GMR_NOREUSE;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR ButtonGClass__GM_GOINACTIVE(Class * cl, struct Gadget *g, struct gpGoInactive * msg)
{
    struct GadgetInfo *gi = msg->gpgi_GInfo;

    if (!(g->Activation & GACT_TOGGLESELECT))
    {
        g->Flags &= ~GFLG_SELECTED;

        if (gi)
        {
            struct RastPort *rp = ObtainGIRPort(gi);

            if (rp)
            {
                struct gpRender method;
                struct opUpdate method2;

                method.MethodID   = GM_RENDER;
                method.gpr_GInfo  = gi;
                method.gpr_RPort  = rp;
                method.gpr_Redraw = GREDRAW_REDRAW;
		
                DoMethodA((Object *)g, (Msg)&method);

                ReleaseGIRPort(rp);

                method2.MethodID     = OM_NOTIFY;
                method2.opu_AttrList = NULL;
                method2.opu_GInfo    = gi;
                method2.opu_Flags    = 0;
		
                DoMethodA((Object *)g, (Msg)&method2);
            }
        }
    }

    return (IPTR)0;
}

/***********************************************************************************/
