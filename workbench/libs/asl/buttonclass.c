/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#if USE_SHARED_COOLIMAGES
#include <libraries/coolimages.h>
#else
#include "coolimages.h"
#endif

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL BUTTON CLASS **************************************************/

IPTR AslButton__OM_NEW(Class * cl, Object * o, struct opSet * msg)
{
    struct AslButtonData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE	   },
	{TAG_DONE, 0UL}
    };
    
    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);

    if (g)
    {
    	data = INST_DATA(cl, g);
	
	/* {GA_Image, whatever} means, a frame shall be created */

	if (FindTagItem(GA_Image, msg->ops_AttrList))
	{
	    if (g->GadgetText) fitags[1].ti_Tag = TAG_IGNORE;
	    data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	}
	
	data->coolimage = (struct CoolImage *)GetTagData(ASLBT_CoolImage, 0, msg->ops_AttrList);
	
	data->ld = (struct LayoutData *)GetTagData(GA_UserData, 0, msg->ops_AttrList);
	
	if (!data->ld)
	{
	    CoerceMethod(cl, (Object *)g, OM_DISPOSE);
	    g = NULL;
	} else {
	    if (data->coolimage && data->ld->ld_TrueColor && CyberGfxBase)
	    {
	    #if SHARED_COOLIMAGES_LIBRARY
	    	WORD numcols = data->coolimage->numcolors;
	    #else
	        WORD numcols = 1 << data->coolimage->depth;
	    #endif
	    
	        if ((data->coolimagepal = AllocVec(numcols * sizeof(ULONG), MEMF_PUBLIC)))
		{
		    ULONG *p = data->coolimagepal;
		    WORD i;
		    
		    for(i = 0; i < numcols; i++)
		    {
		        *p++ = (data->coolimage->pal[i * 3] << 16) |
			       (data->coolimage->pal[i * 3 + 1] << 8) |
			       (data->coolimage->pal[i * 3 + 2]);
		    }
		    
		} else {
		    data->coolimage = NULL;
		}
	    } else {
	        data->coolimage = NULL;
	    }
	}
	
    } /* if (g) */

    return (IPTR)g;
}

/***********************************************************************************/

IPTR AslButton__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct AslButtonData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    if (data->coolimagepal) FreeVec(data->coolimagepal);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

IPTR AslButton__GM_HITTEST(Class *cl, struct Gadget *g, struct gpHitTest *msg)
{
    WORD gadx, gady, gadw, gadh;
    
    getgadgetcoords(g, msg->gpht_GInfo, &gadx, &gady, &gadw, &gadh);

    return ((msg->gpht_Mouse.X >= 0) &&
    	    (msg->gpht_Mouse.Y >= 0) &&
	    (msg->gpht_Mouse.X < gadw) &&
	    (msg->gpht_Mouse.Y < gadh)) ? GMR_GADGETHIT : 0;
}

/***********************************************************************************/

#if BUTTON_OWN_INPUT_HANDLING

/***********************************************************************************/

IPTR AslButton__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct GadgetInfo  	*gi = msg->gpi_GInfo;
    IPTR		retval = GMR_NOREUSE;

    if (gi)
    {
	struct RastPort *rp = ObtainGIRPort(gi);

	if (rp)
	{
	    g->Flags |= GFLG_SELECTED;

	    DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);

	    retval = GMR_MEACTIVE;
	}
    }
    
    return retval;
 }

/***********************************************************************************/

IPTR AslButton__GM_HANDLEINPUT(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct GadgetInfo 	*gi = msg->gpi_GInfo;
    IPTR    	    	retval = GMR_MEACTIVE;

    if (gi)
    {
	struct InputEvent *ie = ((struct gpInput *)msg)->gpi_IEvent;

	switch(ie->ie_Class)
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
				DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
				ReleaseGIRPort(rp);
			    }
			    retval = GMR_NOREUSE | GMR_VERIFY;
			    *msg->gpi_Termination = IDCMP_GADGETUP;
			}
			else
			{
			    retval = GMR_NOREUSE;
    	    	    	}
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
					DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
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
					DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
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
		    
		} /* switch(ie->ie_Code) */
		break;
		
	} /* switch(ie->ie_Class) */
	
    } /* if (gi) */
    else
    {
        retval = GMR_NOREUSE;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR AslButton__GM_GOINACTIVE(Class *cl, struct Gadget *g, struct gpGoInactive *msg)
{
    struct GadgetInfo *gi = msg->gpgi_GInfo;

    g->Flags &= ~GFLG_SELECTED;
 
    if (gi)
    {
	struct RastPort *rp = ObtainGIRPort(gi);
	
	if (rp)
	{
	    DoMethod((Object *)g, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);
	}
    }

    return 0;
}

/***********************************************************************************/

#else /* BUTTON_OWN_INPUT_HANDLING */

IPTR AslButton__GM_GOACTIVE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}
IPTR AslButton__GM_HANDLEINPUT(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}
IPTR AslButton__GM_GOINACTIVE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

#endif /* BUTTON_OWN_INPUT_HANDLING */

/***********************************************************************************/

IPTR AslButton__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct AslButtonData *data;
    char *text = (STRPTR)g->GadgetText;
    struct TagItem im_tags[] =
    {
	{IA_Width	, 0	},
	{IA_Height	, 0	},
	{TAG_DONE		}
    };
    WORD x, y, w, h;

    getgadgetcoords(g, msg->gpr_GInfo, &x, &y, &w, &h);
    
    data = INST_DATA(cl, g);
    
    if (data->frame)
    {
	im_tags[0].ti_Data = w;
	im_tags[1].ti_Data = h;

	SetAttrsA(data->frame, im_tags);

	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       x,
		       y,
		       (!text || (g->Flags & GFLG_SELECTED)) ? IDS_SELECTED : IDS_NORMAL,
		       msg->gpr_GInfo->gi_DrInfo);
    }
    
    if (text)
    {
	WORD len = strlen(text);
	WORD tx = x, ty = y;
	
	SetFont(msg->gpr_RPort, data->ld->ld_Font);

	if (data->coolimage)
	{
	    tx += BORDERIMAGESPACINGX + data->coolimage->width + BORDERIMAGESPACINGX;
	    w  -= (BORDERIMAGESPACINGX + data->coolimage->width + BORDERIMAGESPACINGX + BORDERIMAGESPACINGX);
	}
	
	tx += (w - TextLength(msg->gpr_RPort, text, len)) / 2; 
	ty += (h - msg->gpr_RPort->TxHeight) / 2 + msg->gpr_RPort->TxBaseline;
	
	if (data->frame)
	{
	    SetABPenDrMd(msg->gpr_RPort,
			 data->ld->ld_Dri->dri_Pens[(g->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN],
			 0,
			 JAM1);
	}
	else
	{
    	#if AVOID_FLICKER
	    struct TextExtent te;
	    struct IBox obox, ibox;
	    
    	    getgadgetcoords(g, msg->gpr_GInfo, &obox.Left, &obox.Top, &obox.Width, &obox.Height);

	    TextExtent(msg->gpr_RPort, text, len, &te);

	    ibox.Width  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
	    ibox.Height = te.te_Extent.MaxY - te.te_Extent.MinY + 1;
    	    ibox.Left   = te.te_Extent.MinX + tx;
	    ibox.Top	= te.te_Extent.MinY + ty;
	    
	    PaintBoxFrame(msg->gpr_RPort,
	    	    	  &obox,
			  &ibox,
			  data->ld->ld_Dri->dri_Pens[BACKGROUNDPEN],
			  AslBase);
	        
	#endif	
	    SetABPenDrMd(msg->gpr_RPort,
			 data->ld->ld_Dri->dri_Pens[(g->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN],
			 data->ld->ld_Dri->dri_Pens[BACKGROUNDPEN],
			 JAM2);
	    
	}
	
	Move(msg->gpr_RPort, tx, ty);
	Text(msg->gpr_RPort, text, len);
    }
    else
    {
    	x += 3; w -= 6;
	y += 3; h -= 6;
    
    #if AVOID_FLICKER
    	if (data->frame)
	{
	    struct IBox ibox, fbox;
	    
    	    getgadgetcoords(g, msg->gpr_GInfo, &fbox.Left, &fbox.Top, &fbox.Width, &fbox.Height);

	    ibox.Left = x;
	    ibox.Top = y;
	    ibox.Width = w;
	    ibox.Height = h;
	    
	    PaintInnerFrame(msg->gpr_RPort,
	    	    	    data->ld->ld_Dri,
			    data->frame,
			    &fbox,
			    &ibox,
			    data->ld->ld_Dri->dri_Pens[BACKGROUNDPEN],
			    AslBase);
	}
	
    #endif
    		 
	SetABPenDrMd(msg->gpr_RPort,
		     data->ld->ld_Dri->dri_Pens[(g->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN],
		     0,
		     JAM1);
        
	RectFill(msg->gpr_RPort, x, y, x + w - 1, y + h - 1);
    }

    if (data->coolimage)
    {
        struct ColorMap *cm = msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap;
        ULONG bg[3];

	GetRGB32(cm, data->ld->ld_Dri->dri_Pens[(g->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN], 1, bg);
	data->coolimagepal[0] = ((bg[0] & 0xFF000000) >> 8) + ((bg[1] & 0xFF000000) >> 16) + ((bg[2] & 0xFF000000) >> 24);
		
	WriteLUTPixelArray((APTR)data->coolimage->data,
			    0,
			    0,
			    data->coolimage->width,
			    msg->gpr_RPort,
			    data->coolimagepal,
			    x + BORDERIMAGESPACINGX,
			    y + (h - data->coolimage->height) / 2,
			    data->coolimage->width,
			    data->coolimage->height,
			    CTABFMT_XRGB8);
        
    }
    	    
    return 0;
}

/***********************************************************************************/

IPTR AslButton__GM_LAYOUT(Class * cl, struct Gadget * g, struct gpLayout * msg)
{
    struct AslButtonData *data;
    IPTR retval;
    WORD innerwidth;
    WORD spacing;
    WORD x;
    
    data = INST_DATA(cl, g);
       
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    switch (g->GadgetID)
    {
        case ID_MAINBUTTON_OK:
	case ID_MAINBUTTON_MIDDLELEFT:
	case ID_MAINBUTTON_MIDDLERIGHT:
	case ID_MAINBUTTON_CANCEL:

	    /* multiply width 16 for sub-pixel accuracy */

 	    x = (data->ld->ld_WBorLeft + OUTERSPACINGX) * 16 + 8;
	    
    	    innerwidth = msg->gpl_GInfo->gi_Window->Width - 
	    		 msg->gpl_GInfo->gi_Window->BorderLeft -
			 msg->gpl_GInfo->gi_Window->BorderRight -
			 OUTERSPACINGX * 2;
	
	    /* multiply width 16 for sub-pixel accuracy */
	    	 
    	    spacing = (innerwidth - data->ld->ld_ButWidth * data->ld->ld_NumButtons) * 16 /
	    	      (data->ld->ld_NumButtons - 1);
 
	    x += (g->GadgetID - ID_MAINBUTTON_OK) * (data->ld->ld_ButWidth * 16 + spacing);
	    g->LeftEdge = x / 16;
	    break;
    }
   
    return retval;
}

/***********************************************************************************/

