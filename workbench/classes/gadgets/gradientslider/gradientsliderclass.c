/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS gradientslider gadget.
    Lang: english
*/

//#define USE_BOOPSI_STUBS
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <utility/tagitem.h>
#include <gadgets/gradientslider.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* abs() */
#include "gradientslider_intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 0

#include <aros/debug.h>

#define GradientSliderBase ((struct GradientSliderBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>


/***************************************************************************************************/

STATIC VOID notify_curval(Class *cl, Object *o, struct GadgetInfo *gi, BOOL interim, BOOL userinput)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct opUpdate		opu;
    struct TagItem		tags[] =
    {
        {GA_ID			, EG(o)->GadgetID	},
	{GA_UserInput		, userinput		},
        {GRAD_CurVal		, data->curval		},
	{TAG_DONE					}
    };
    
    opu.MethodID     = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo    = gi; 
    opu.opu_Flags    = interim ? OPUF_INTERIM : 0;
    
    DoMethodA(o, (Msg)&opu);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 		*tag, *tstate;
    IPTR 			retval = 0UL;
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    BOOL			disabled, redraw_all = FALSE;
    
    EnterFunc(bug("GradientSlider::Set()\n"));

    disabled = (EG(o)->Flags & GFLG_DISABLED) != 0L;
    
    if (msg->MethodID != OM_NEW) retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    tstate = msg->ops_AttrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	   	
    	switch (tag->ti_Tag)
    	{
	    case GA_Disabled:
	        if (disabled != tidata)
		{
		    retval += 1UL;
		    redraw_all = TRUE;
		    data->edgesOnly = FALSE;
		}
		break;
		
	    case GRAD_MaxVal:
	        if (tidata != 0)
		{
		    ULONG new_curval = (((ULONG)tidata) * data->curval) / data->maxval;
		    
		    if (tidata > 0xFFFF) tidata = 0xFFFF;
		    
	            data->maxval = (ULONG)tidata;

		    if (new_curval != data->curval)
		    {
		        data->curval = new_curval;
			notify_curval(cl, o, msg->ops_GInfo, FALSE, FALSE);
		    }
		    retval += 1UL;
		}
		break;
		
	    case GRAD_CurVal:
	        data->curval = (ULONG)tidata;
		notify_curval(cl, o, msg->ops_GInfo, FALSE, FALSE);
		retval += 1UL;
		break;
		
	    case GRAD_SkipVal:
	        data->skipval = (ULONG)tidata;
		break;
	
	    case GRAD_PenArray:
	        data->penarray = (UWORD *)tidata;
		data->numpens = 0;
		if (data->penarray)
		{
		    UWORD *pen = data->penarray;
		    
		    while(*pen++ != (UWORD)~0) data->numpens++;
		}		
		retval += 1UL;
		redraw_all = TRUE;
		break;
		
	    default:
	        break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */
    
    data->edgesOnly = (EG(o)->Flags & GFLG_DISABLED) ? FALSE : TRUE;
    
    if (retval)
    {
        struct RastPort *rp;
	
	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rp, redraw_all ? GREDRAW_REDRAW : GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
	    retval = 0L;
	}
    }
    
    ReturnPtr ("GradientSlider::Set", IPTR, retval);
}

/***************************************************************************************************/

STATIC Object *gradientslider_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("GradientSlider::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct GradientSliderData	*data = INST_DATA(cl, o);
	struct TagItem 			fitags[]=
	{
	   {IA_FrameType	, FRAME_BUTTON	},
           {TAG_DONE				}
	};
	
	if ((data->frame = NewObjectA(NULL, FRAMEICLASS, fitags)))
	{
	    data->maxval     = 0xFFFF;
	    data->curval     = 0;
	    data->skipval    = 0x1111;
	    data->knobpixels = GetTagData(GRAD_KnobPixels, 5, msg->ops_AttrList);
	    data->freedom    = GetTagData(PGA_Freedom, LORIENT_HORIZ, msg->ops_AttrList);
	    data->edgesOnly  = TRUE;
	    
    	    gradientslider_set(cl, o, msg);
	    
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    	   
    }
    ReturnPtr ("GradientSlider::New", Object *, o);
}

/***************************************************************************************************/

STATIC VOID gradientslider_dispose(Class *cl, Object *o, Msg msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    if (data->frame) DisposeObject(data->frame);
    if (data->savebm)
    {
        WaitBlit();
	FreeBitMap(data->savebm);
    }
    DoSuperMethodA(cl, o, msg);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_get(Class *cl, Object *o, struct opGet *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    IPTR 			retval = 1UL;
   
    switch(msg->opg_AttrID)
    {
        case GRAD_MaxVal:
	    *msg->opg_Storage = data->maxval;
	    break;
	    
	case GRAD_CurVal:
	    *msg->opg_Storage = data->curval;
	    break;
	    
	case GRAD_SkipVal:
	    *msg->opg_Storage = data->skipval;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
	    
    } /* switch(msg->opg_AttrID) */
   
    return retval;
}

/***************************************************************************************************/

STATIC VOID gradientslider_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);    
    struct DrawInfo 		*dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort 		*rp = msg->gpr_RPort;
    struct IBox			gbox, sbox, kbox;
    LONG			redraw = msg->gpr_Redraw;
    
    EnterFunc(bug("GradientSlider::Render()\n"));    

    GetGadgetIBox(o, msg->gpr_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    GetKnobBox(data, &sbox, &kbox);
    
    if (data->savebm && ((sbox.Width != data->savebmwidth) || (sbox.Height != data->savebmheight)))
    {
        redraw = GREDRAW_REDRAW;
    }
    
    switch (redraw)
    {
    	case GREDRAW_REDRAW:
    	    {
	        struct TagItem fitags[] =
		{
		    {IA_Width		, gbox.Width		},
		    {IA_Height		, gbox.Height		},
		    {IA_EdgesOnly	, data->edgesOnly	},
		    {TAG_DONE					}
		};
		
		SetAttrsA(data->frame, fitags);
		
		/* Draw slider background */
		
		if ((sbox.Width >= 2) && (sbox.Height >= 2))
		{
	            if (data->numpens < 2)
		    {
			WORD pen = dri->dri_Pens[BACKGROUNDPEN];

			if (data->penarray && (data->numpens == 1))
			{
		            pen = data->penarray[0];
			}

			SetDrMd(rp, JAM1);
			SetAPen(rp, pen);
			RectFill(rp, sbox.Left, sbox.Top, sbox.Left + sbox.Width - 1, sbox.Top + sbox.Height - 1);

		    } /* ff (data->numpens < 2) */
		    else
		    {
		        struct RastPort trp;
			
			if (data->savebm)
			{
			    if ((sbox.Width  != data->savebmwidth) ||
			        (sbox.Height != data->savebmheight))
			    {
			        WaitBlit();
				FreeBitMap(data->savebm);
				data->savebm = NULL;
			    }
			}

		    	if ( (data->savebm != NULL) ||
			     ((data->savebm = AllocBitMap(sbox.Width,
	    			   			  sbox.Height,
				       			  GetBitMapAttr(rp->BitMap, BMA_DEPTH),
				       			  BMF_MINPLANES,
				       			  rp->BitMap))) )
			{						
			    InitRastPort(&trp);
		    	
			    trp.BitMap = data->savebm;
		    		
			    DrawGradient(&trp,
		    		     	 0,
					 0,
					 sbox.Width - 1,
					 sbox.Height - 1,
					 data->penarray,
					 data->numpens,
					 data->freedom,
					 msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap,
					 GradientSliderBase);
		
			    DeinitRastPort( &trp );
		
			    BltBitMapRastPort( data->savebm, 0,0, rp, sbox.Left, sbox.Top, sbox.Width, sbox.Height, 0xc0 );
			
			}

		    } /* data->numpens >= 2 */

		} /* if ((sbox.Width >= 2) && (sbox.Height >= 2)) */
	    				
		/* Backup area over which knob will be drawn */
		
		if ((kbox.Width > 0) && (kbox.Height > 0) &&
		    (kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height))
		{
		    data->savefromx	= kbox.Left;
		    data->savefromy	= kbox.Top;
		    data->savebmwidth	= sbox.Width;
		    data->savebmheight	= sbox.Height;
		    		    
		} /* if ((kbox.Width > 0) && (kbox.Height > 0) && (kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height)) */
		
		/* Render knob */
                
		if ((kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height))
		{
		    DrawKnob(data, rp, dri, &kbox, 0, GradientSliderBase);
		}

		/* Draw frame */
		
		DrawImageState(rp, (struct Image *)data->frame, gbox.Left, gbox.Top,
				   (EG(o)->Flags & GFLG_DISABLED) ? IDS_DISABLED : IDS_NORMAL, dri);
			
		if (data->edgesOnly == FALSE) data->edgesOnly = TRUE;    		
				
	    }
	    break;
	    		
    	case GREDRAW_UPDATE:
	    if ((kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height))
	    {
		if ((kbox.Left != data->savefromx) || (kbox.Top != data->savefromy))
		{
	            if (data->savebm)
		    {
			/* Restore old area behind knob */

			BltBitMapRastPort(data->savebm,
					  data->savefromx - sbox.Left,
					  data->savefromy - sbox.Top,
					  rp,
					  data->savefromx,
					  data->savefromy,
					  kbox.Width,
					  kbox.Height,
					  192);		
					  				
		    } /* if (data->savebm) */
		    else
		    {
		    	WORD pen = dri->dri_Pens[BACKGROUNDPEN];

			if (data->penarray && (data->numpens == 1))
			{
			    pen = data->penarray[0];
			}

			SetDrMd(rp, JAM1);
			SetAPen(rp, pen);
			RectFill(rp, data->savefromx, data->savefromy,
				     (data->savefromx + kbox.Width) - 1, (data->savefromy + kbox.Height) - 1);
		    }	

		    data->savefromx = kbox.Left;
		    data->savefromy = kbox.Top;

                    DrawKnob(data, rp, dri, &kbox, 0, GradientSliderBase);

		} /* if (!data->savebm || (kbox.Left != data->savefromx) || (kbox.Top != data->savefromy)) */

	    } /* if ((kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height)) */
 	    
    	    break;
    	    
    	    
    } /* switch (redraw) */
            	
    ReturnVoid("GradientSlider::Render");
}

/***************************************************************************************************/

STATIC IPTR gradientslider_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    struct IBox			gbox, sbox;
    WORD			mousex, mousey;
    IPTR 			retval = 0UL;

    EnterFunc(bug("GradientSlider::HitTest()\n"));

    GetGadgetIBox(o, msg->gpht_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);

    if ((sbox.Width > 2) && (sbox.Height > 2))
    {
	/* calc mouse coords relative to slider box */

	mousex = msg->gpht_Mouse.X - (sbox.Left - gbox.Left);
	mousey = msg->gpht_Mouse.Y - (sbox.Top  - gbox.Top);

	if ((mousex >= 0) && (mousey >= 0) &&
    	    (mousex < sbox.Width) && (mousey < sbox.Height)) retval = GMR_GADGETHIT;
    }
    
    ReturnInt("GradientSlider::HitTest", IPTR, retval);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct IBox			gbox, sbox, kbox;
    WORD			mousex, mousey;
    ULONG			old_curval = data->curval;
    ULONG			new_curval;
    BOOL			knobhit = TRUE;
    IPTR 			retval = 0UL;

    EnterFunc(bug("GradientSlider::GoActive()\n"));

    if (!msg->gpi_IEvent) return GMR_NOREUSE;

    GetGadgetIBox(o, msg->gpi_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    GetKnobBox(data, &sbox, &kbox);

    mousex = msg->gpi_Mouse.X + gbox.Left;
    mousey = msg->gpi_Mouse.Y + gbox.Top;
    
    data->clickoffsetx = mousex - kbox.Left + (sbox.Left - gbox.Left);
    data->clickoffsety = mousey - kbox.Top  + (sbox.Top - gbox.Top);
    
    if ( ((data->freedom == LORIENT_HORIZ) && (mousex < kbox.Left)) ||
         ((data->freedom == LORIENT_VERT)  && (mousey < kbox.Top)) )
    {
  	new_curval = old_curval - data->skipval;
	if ((LONG)new_curval < 0) new_curval = 0;
	knobhit = FALSE;	
    }
    else if ( ((data->freedom == LORIENT_HORIZ) && (mousex >= kbox.Left + kbox.Width)) ||
              ((data->freedom == LORIENT_VERT)  && (mousey >= kbox.Top + kbox.Height)) )

    {
        new_curval = old_curval + data->skipval;
	if (new_curval > data->maxval) new_curval = (LONG)data->maxval;
	knobhit = FALSE;
    }    
    
    if (!knobhit)
    {
   	struct RastPort	*rp;
   		
   	data->curval = new_curval;   		
   	notify_curval(cl, o, msg->gpi_GInfo, FALSE, TRUE);
   		
   	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
   	{
   	    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE );
   	    ReleaseGIRPort( rp );
   	}
   
	retval = GMR_VERIFY | GMR_NOREUSE;
	/* original AmigaOS gradientslider.gadget does not seem to place any meaningful
	   value into gpi_Termination :-\ */
	*msg->gpi_Termination = data->curval;
		
    } else {
        retval = GMR_MEACTIVE;
    }
    
    ReturnInt("GradientSlider::GoActive", IPTR, retval);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct GradientSliderData	*data = INST_DATA(cl, o);
    struct IBox			gbox, sbox, kbox;
    struct InputEvent 		*ie = msg->gpi_IEvent;
    LONG			new_curval; 			/* use LONG instead of ULONG for easier checks against < 0 */
    WORD			mousex, mousey;
    IPTR 			retval = GMR_MEACTIVE;
    
    //EnterFunc(bug("GradientSlider::HandleInput\n"));
    
    GetGadgetIBox(o, msg->gpi_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    GetKnobBox(data, &sbox, &kbox);
    
    switch(ie->ie_Class)
    {
        case IECLASS_RAWMOUSE:
	    switch(ie->ie_Code)
	    {
	        case SELECTUP:
		    retval = GMR_VERIFY | GMR_NOREUSE;
		    /* original AmigaOS gradientslider.gadget does not seem to place any meaningful
		       value into gpi_Termination :-\ */
		    *msg->gpi_Termination = data->curval;
		    
		    notify_curval(cl, o, msg->gpi_GInfo, FALSE, TRUE);

		    break; /* SELECTUP */
		    
		case IECODE_NOBUTTON:
		    mousex = msg->gpi_Mouse.X - data->clickoffsetx;
		    mousey = msg->gpi_Mouse.Y - data->clickoffsety;
		    
		    if (data->freedom == LORIENT_HORIZ)
		    {
		        if (sbox.Width != data->knobpixels) /* avoid div by 0 */
			{
			    new_curval = mousex * ((LONG)data->maxval) / (sbox.Width - (LONG)data->knobpixels);
			}
		    } else {
		        if (sbox.Height != data->knobpixels) /* avoid div by 0 */
			{
			    new_curval = mousey * ((LONG)data->maxval) / (sbox.Height - (LONG)data->knobpixels);
			}
		    }
		    
		    if (new_curval < 0)
		    {
		        new_curval = 0;
		    }
		    else if (new_curval > data->maxval)
		    {
		        new_curval = (LONG)data->maxval;
		    }
		    
		    if ((ULONG)new_curval != data->curval)
		    {
		        struct RastPort *rp;
			
			data->curval = (ULONG)new_curval;
			
			if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
			{
	    		    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
			    ReleaseGIRPort(rp);
			}
			
			notify_curval(cl, o, msg->gpi_GInfo, TRUE, TRUE);
		    }
		    break; /* IECODE_NOBUTTON */
		    
	    } /* switch(ie->ie_Code) */
	    
	    break; /* IECLASS_RAWMOUSE */
	    
    } /* switch(ie->ie_Class) */
    
    ReturnInt("GradientSlider::HandleInput", IPTR, retval);
}


/***************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_gradientsliderclass,
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
	    retval = (IPTR)gradientslider_new(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    gradientslider_dispose(cl, o, msg);
	    break;
	    
	case GM_RENDER:
	    gradientslider_render(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_HITTEST:
	    retval = gradientslider_hittest(cl, o, (struct gpHitTest *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = gradientslider_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = gradientslider_handleinput(cl, o, (struct gpInput *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = (IPTR)gradientslider_set(cl, o, (struct opSet *)msg);
	    break;

	case OM_GET:
	    retval = gradientslider_get(cl, o, (struct opGet *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return (retval);

    AROS_USERFUNC_EXIT
}  /* dispatch_gradientsliderclass */


#undef GradientSliderBase

/***************************************************************************************************/

struct IClass *InitGradientSliderClass (struct GradientSliderBase_intern * GradientSliderBase)
{
    struct IClass *cl = NULL;

    if ((cl = MakeClass("gradientslider.gadget", GADGETCLASS, NULL, sizeof(struct GradientSliderData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_gradientsliderclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)GradientSliderBase;

	AddClass (cl);
    }

    return (cl);
}

/***************************************************************************************************/
