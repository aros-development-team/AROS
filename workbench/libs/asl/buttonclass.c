/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define OWN_INPUT_HANDLING 1


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

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL BUTTON CLASS **************************************************/

struct AslButtonData
{
    Object 		*frame;
    struct LayoutData 	*ld;
    struct CoolImage  	*coolimage;
    ULONG		*coolimagepal;
};

/***********************************************************************************/

static IPTR aslbutton_new(Class * cl, Object * o, struct opSet * msg)
{
    struct AslButtonData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE	   },
	{TAG_DONE, 0UL}
    };
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	data = INST_DATA(cl, o);
	
	/* {GA_Image, whatever} means, a frame shall be created */

	if (FindTagItem(GA_Image, msg->ops_AttrList))
	{
	    if (G(o)->GadgetText) fitags[1].ti_Tag = TAG_IGNORE;
	    data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	}
	
	data->coolimage = (struct CoolImage *)GetTagData(ASLBT_CoolImage, 0, msg->ops_AttrList);
	
	data->ld = (struct LayoutData *)GetTagData(GA_UserData, 0, msg->ops_AttrList);
	
	if (!data->ld)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
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
	
    } /* if (o) */

    return (IPTR)o;
}

/***********************************************************************************/

static IPTR aslbutton_dispose(Class * cl, Object * o, Msg msg)
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

static IPTR aslbutton_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    WORD gadx, gady, gadw, gadh;
    
    getgadgetcoords(G(o), msg->gpht_GInfo, &gadx, &gady, &gadw, &gadh);

    return ((msg->gpht_Mouse.X >= 0) &&
    	    (msg->gpht_Mouse.Y >= 0) &&
	    (msg->gpht_Mouse.X < gadw) &&
	    (msg->gpht_Mouse.Y < gadh)) ? GMR_GADGETHIT : 0;
}

/***********************************************************************************/

#if OWN_INPUT_HANDLING

/***********************************************************************************/

static IPTR aslbutton_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    struct GadgetInfo  	*gi = msg->gpi_GInfo;
    IPTR		retval = GMR_NOREUSE;

    if (gi)
    {
	struct RastPort *rp = ObtainGIRPort(gi);

	if (rp)
	{
	    EG(o)->Flags |= GFLG_SELECTED;

	    DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);

	    retval = GMR_MEACTIVE;
	}
    }
    
    return retval;
 }

/***********************************************************************************/

static IPTR aslbutton_handleinput(Class *cl, Object *o, struct gpInput *msg)
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
	        	if( EG(o)->Flags & GFLG_SELECTED )
			{
			    struct RastPort *rp;

			    /* mouse is over gadget */
			    EG(o)->Flags &= ~GFLG_SELECTED;

			    if ((rp = ObtainGIRPort(gi)))
			    {
				DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
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
			    if ( DoMethodA(o, (Msg)&gpht) == GMR_GADGETHIT )
			    {
				if ( (EG(o)->Flags & GFLG_SELECTED) == 0 )
				{
				    struct RastPort *rp;

				    /* mouse is over gadget */
				    EG(o)->Flags |= GFLG_SELECTED;

				    if ((rp = ObtainGIRPort(gi)))
				    {
					DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
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
					DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_UPDATE);
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

static IPTR aslbutton_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct GadgetInfo *gi = msg->gpgi_GInfo;

    EG(o)->Flags &= ~GFLG_SELECTED;
 
    if (gi)
    {
	struct RastPort *rp = ObtainGIRPort(gi);
	
	if (rp)
	{
	    DoMethod(o, GM_RENDER, (IPTR) gi, (IPTR) rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);
	}
    }

    return 0;
}

/***********************************************************************************/

#endif /* OWN_INPUT_HANDLING */

/***********************************************************************************/

static IPTR aslbutton_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslButtonData *data;
    char *text = (STRPTR)G(o)->GadgetText;
    struct TagItem im_tags[] =
    {
	{IA_Width	, 0	},
	{IA_Height	, 0	},
	{TAG_DONE		}
    };
    WORD x, y, w, h;

    getgadgetcoords(G(o), msg->gpr_GInfo, &x, &y, &w, &h);
    
    data = INST_DATA(cl, o);
    
    if (data->frame)
    {
	im_tags[0].ti_Data = w;
	im_tags[1].ti_Data = h;

	SetAttrsA(data->frame, im_tags);

	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       x,
		       y,
		       (!text || (G(o)->Flags & GFLG_SELECTED)) ? IDS_SELECTED : IDS_NORMAL,
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
			 data->ld->ld_Dri->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN],
			 0,
			 JAM1);
	}
	else
	{
    	#if AVOID_FLICKER
	    struct TextExtent te;
	    struct IBox obox, ibox;
	    
    	    getgadgetcoords(G(o), msg->gpr_GInfo, &obox.Left, &obox.Top, &obox.Width, &obox.Height);

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
			 data->ld->ld_Dri->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN],
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
	    
    	    getgadgetcoords(G(o), msg->gpr_GInfo, &fbox.Left, &fbox.Top, &fbox.Width, &fbox.Height);

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
		     data->ld->ld_Dri->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN],
		     0,
		     JAM1);
        
	RectFill(msg->gpr_RPort, x, y, x + w - 1, y + h - 1);
    }

    if (data->coolimage)
    {
        struct ColorMap *cm = msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap;
        ULONG bg[3];

	GetRGB32(cm, data->ld->ld_Dri->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN], 1, bg);
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

static IPTR aslbutton_layout(Class * cl, Object * o, struct gpLayout * msg)
{
    struct AslButtonData *data;
    IPTR retval;
    WORD innerwidth;
    WORD spacing;
    WORD x;
    
    data = INST_DATA(cl, o);
       
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    switch (G(o)->GadgetID)
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
 
	    x += (G(o)->GadgetID - ID_MAINBUTTON_OK) * (data->ld->ld_ButWidth * 16 + spacing);
	    G(o)->LeftEdge = x / 16;
	    break;
    }
   
    return retval;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslbuttonclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = aslbutton_new(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = aslbutton_dispose(cl, obj, msg);
	    break;
	
	case GM_HITTEST:
	    retval = aslbutton_hittest(cl, obj, (struct gpHitTest *)msg);
	    break;

#if OWN_INPUT_HANDLING
    	case GM_GOACTIVE:
	    retval = aslbutton_goactive(cl, obj, (struct gpInput *)msg);
	    break;
	    
	case GM_HANDLEINPUT:
	    retval = aslbutton_handleinput(cl, obj, (struct gpInput *)msg);
	    break;
	    
	case GM_GOINACTIVE:
	    retval = aslbutton_goinactive(cl, obj, (struct gpGoInactive *)msg);
	    break;
#endif	    
	case GM_RENDER:
	    retval = aslbutton_render(cl, obj, (struct gpRender *)msg);
	    break;
	
	case GM_LAYOUT:
	    retval = aslbutton_layout(cl, obj, (struct gpLayout *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslbuttonclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslbuttonclass)
	return AslBase->aslbuttonclass;

#if OWN_INPUT_HANDLING
    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslButtonData), 0UL);
#else
    cl = MakeClass(NULL, BUTTONGCLASS, NULL, sizeof(struct AslButtonData), 0UL);
#endif

    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslbuttonclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslbuttonclass = cl;

    return cl;
}

/***********************************************************************************/

