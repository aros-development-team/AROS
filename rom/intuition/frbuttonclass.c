/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif /* !__MORPHOS__ */

#define DEBUG_FRBUTTON(x)   ;

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

IPTR FrButtonClass__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    /* We will let the AROS gadgetclass test if it is safe to render */
#warning FIXME:
    /* FIXME: if ( DoSuperMethodA(cl, o, (Msg *)msg) != 0)
{ */
    UWORD   	    *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    struct RastPort *rp = msg->gpr_RPort;
    struct IBox      container;

    DEBUG_FRBUTTON(dprintf("frbutton_render: rp %p[%p] win %p[%p] req %p[%p] gi->rp %p[%p]\n",
                           rp, rp->Layer,
                           msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Window->WLayer,
                           msg->gpr_GInfo->gi_Requester, msg->gpr_GInfo->gi_Requester->ReqLayer,
                           msg->gpr_GInfo->gi_RastPort, msg->gpr_GInfo->gi_RastPort->Layer));
    SANITY_CHECK(rp)
    
    GetGadgetIBox(g, msg->gpr_GInfo, &container);

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
        struct TagItem image_tags[] =
        {
            {IA_Width , g->Width },
            {IA_Height, g->Height},
            {TAG_DONE        	     }
        };

        if ((g->SelectRender != NULL) &&
            (g->Flags & GFLG_SELECTED)) /* render selected image */
        {
            ULONG x, y;
	    struct Image *sr = g->SelectRender;
	    
            if(sr->Depth == CUSTOMIMAGEDEPTH)
            {
                // ONLY DO THIS FOR REAL IMAGE OBJECTS (cyfm 31/12/02)
                /* center image position, we assume image top and left is 0 */
                SetAttrsA(g->SelectRender, image_tags);
            }

            x = container.Left + (container.Width / 2) - (sr->Width / 2);
            y = container.Top + (container.Height / 2) - (sr->Height / 2);

            DrawImageState(rp,
                           sr,
                           x, y,
                           IDS_SELECTED,
                           msg->gpr_GInfo->gi_DrInfo );
        }
        else if ( g->GadgetRender != NULL ) /* render normal image */
        {
            ULONG x, y;
	    struct Image *gr = g->GadgetRender;
	    
            if(gr->Depth == CUSTOMIMAGEDEPTH)
            {
                // ONLY DO THIS FOR REAL IMAGE OBJECTS (cyfm 31/12/02)
                /* center image position, we assume image top and left is 0 */
                SetAttrsA(gr, image_tags);
            }

            x = container.Left + (container.Width / 2) - (gr->Width / 2);
            y = container.Top + (container.Height / 2) - (gr->Height / 2);

            DrawImageState(rp,
                           gr,
                           x, y,
                           ((g->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
                           msg->gpr_GInfo->gi_DrInfo);
        }
    }

    /* print label */
    SetABPenDrMd(rp, pens[TEXTPEN], 0, JAM1);
    
    printgadgetlabel(cl, (Object *)g, msg, IntuitionBase);

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
    /* } FIXME */
    
    return (IPTR)0;
}

/***********************************************************************************/

void frbutton_setsize(Class *cl, struct Gadget *g, struct opSet *msg)
{
    struct Image *image = (struct Image *)g->GadgetRender;

    DEBUG_FRBUTTON(dprintf("frbutton_setsize: o %p\n", o));

    if ((FindTagItem(GA_Width, msg->ops_AttrList) == NULL ||
            FindTagItem(GA_Height, msg->ops_AttrList) == NULL) &&
            image && g->Flags & GFLG_GADGIMAGE)
    {
        struct IBox 	 contents, frame;
        struct DrawInfo *dri = msg->ops_GInfo ? msg->ops_GInfo->gi_DrInfo : NULL;
        BOOL 	    	 do_framebox = TRUE;
	
        DEBUG_FRBUTTON(dprintf("frbutton_setsize: image %p flags 0x%lx\n", image,g->Flags));

        dri = (APTR)GetTagData(GA_DrawInfo, (IPTR)dri, msg->ops_AttrList);
        contents.Left = 0;
        contents.Top = 0;

        switch (g->Flags & GFLG_LABELMASK)
        {
          //case GFLG_LABELITEXT:
          //break;

            case GFLG_LABELSTRING:
        	if (dri)
        	{
                    struct RastPort rp;
                    STRPTR  	    text = (STRPTR)g->GadgetText;

                    InitRastPort(&rp);
                    SetFont(&rp, dri->dri_Font);

                    contents.Height = dri->dri_Font->tf_YSize;
                    contents.Width = LabelWidth(&rp, text, strlen(text), IntuitionBase);

		    DeinitRastPort(&rp);
        	}
        	else
                    do_framebox = FALSE;
        	break;

            case GFLG_LABELIMAGE:
        	contents.Width  = ((struct Image *)g->GadgetText)->Width;
        	contents.Height = ((struct Image *)g->GadgetText)->Height;
        	break;

            default:
        	do_framebox = FALSE;
        	break;
        }

        DEBUG_FRBUTTON(dprintf("frbutton_setsize: do_framebox %d contents %d %d %d %d\n", do_framebox,
                               contents.Left,contents.Top, contents.Width,contents.Height));
        if (do_framebox)
        {
            struct impFrameBox method;
            int     	       width, height;

            method.MethodID 	   = IM_FRAMEBOX;
            method.imp_ContentsBox = &contents;
            method.imp_FrameBox    = &frame;
            method.imp_DrInfo      = dri;
            method.imp_FrameFlags  = 0;

            if (DoMethodA((Object *)image, (Msg)&method))
            {
                width = frame.Width;
                height = frame.Height;
                DEBUG_FRBUTTON(dprintf("frbutton_setsize: ok, w=%d h=%d l=%d t=%d\n", width, height, frame.Left, frame.Top));
            }
            else
            {
                width = image->Width;
                height = image->Height;
                DEBUG_FRBUTTON(dprintf("frbutton_setsize: bad, w=%d h=%d\n", width, height));
            }

            g->Width = width;
            g->Height = height;
        }
    }
}

/***********************************************************************************/

IPTR FrButtonClass__GM_HITTEST(Class *cl, struct Gadget * g, struct gpHitTest * msg)
{
    struct Image *image = (struct Image *)g->GadgetRender;

    IPTR retval = GMR_GADGETHIT;

    if (image)
    {
        if (image->Depth == CUSTOMIMAGEDEPTH)
        {
            struct impHitTest imph;

            imph.MethodID    = IM_HITFRAME;
            imph.imp_Point.X = msg->gpht_Mouse.X;
            imph.imp_Point.Y = msg->gpht_Mouse.Y;

            retval = DoMethodA((Object *)image, (Msg)&imph) ? GMR_GADGETHIT : 0;
        }
    }

    return retval;
}

/****************************************************************************/

IPTR FrButtonClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);
    if (g)
    {
	frbutton_setsize(cl, g, msg);
    }
    return (IPTR)g;
}

/***********************************************************************************/

IPTR FrButtonClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
     * because it would circumvent the subclass from fully overriding it.
     * The check of cl == OCLASS(o) should fail if we have been
     * subclassed, and we have gotten here via DoSuperMethodA().
     */
    if ( retval && ( (msg->MethodID != OM_UPDATE) || (cl == OCLASS(o)) ) )
    {
	struct GadgetInfo *gi = msg->ops_GInfo;
		
	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);
		    
	    if (rp)
	    {
		struct gpRender method;
			
		method.MethodID   = GM_RENDER;
		method.gpr_GInfo  = gi;
		method.gpr_RPort  = rp;
		method.gpr_Redraw = GREDRAW_REDRAW;
			
		DoMethodA(o, (Msg)&method);
			
		ReleaseGIRPort(rp);
	    }
	}
    }
    
    return retval;
}

/****************************************************************************/
