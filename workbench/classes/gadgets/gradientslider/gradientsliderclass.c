/*
    (C) 1995-97 AROS - The Amiga Research OS
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

#define SDEBUG 0
#define DEBUG 1
#include <aros/debug.h>

#define GradientSliderBase ((struct GradientSliderBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>


/***************************************************************************************************/

STATIC VOID notify_curval(Class *cl, Object *o, struct GadgetInfo *gi, BOOL interim)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct opUpdate		opu;
    struct TagItem		tags[] =
    {
        {GRAD_CurVal	, data->curval	},
	{TAG_DONE			}
    };
    
    opu.MethodID     = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo    = gi; 
    opu.opu_Flags    = interim ? OPUF_INTERIM : 0;
    
    DoMethodA(o, &opu);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 		*tag, *tstate;
    IPTR 			retval;
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    EnterFunc(bug("GradientSlider::Set()\n"));

    retval = DoSuperMethod(cl, o, OM_SET, msg->ops_AttrList, msg->ops_GInfo);
    
    for (tstate = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
    	IPTR tidata = tag->ti_Data;
    	   	
    	switch (tag->ti_Tag)
    	{
	    case GRAD_MaxVal:
	        data->maxval = (ULONG)tidata;
		break;
		
	    case GRAD_CurVal:
	        data->curval = (ULONG)tidata;
		notify_curval(cl, o, msg->ops_GInfo, FALSE);
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
		break;
		
	    default:
	        break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */
    
    if (retval)
    {
        struct RastPort *rp;
	
	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    DoMethod(o, GM_RENDER, msg->ops_GInfo, rp, GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
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
           {IA_EdgesOnly	, FALSE		},
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

    	    gradientslider_set(cl, o, msg);
	    
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    	   
    }
    ReturnPtr ("GradientSlider::New", Object *, o);
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
    }
   
    return retval;
}

/***************************************************************************************************/

STATIC VOID gradientslider_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);    
    struct DrawInfo 		*dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort 		*rp = msg->gpr_RPort;
    struct IBox			gbox, sbox;
    
    EnterFunc(bug("GradientSlider::Render()\n"));    

    GetGadgetIBox(o, msg->gpr_GInfo, &gbox);

    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	    {
	        struct TagItem fitags[] =
		{
		    {IA_Width	, gbox.Width	},
		    {IA_Height	, gbox.Height	},
		    {TAG_DONE			}
		};
		
		SetAttrsA(data->frame, fitags);
		
		DrawImageState(rp, (struct Image *)data->frame, gbox.Left, gbox.Top, IDS_NORMAL, dri);
	    }
	    /* fall through */
		
    	case GREDRAW_UPDATE:
	    sbox = gbox;
	    sbox.Left   += FRAMESLIDERSPACINGX;
	    sbox.Top    += FRAMESLIDERSPACINGY;
	    sbox.Width  -= FRAMESLIDERSPACINGX * 2;
	    sbox.Height -= FRAMESLIDERSPACINGY * 2;
	    
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
		    DrawGradient(rp,
		    		 sbox.Left,
				 sbox.Top,
				 sbox.Left + sbox.Width - 1,
				 sbox.Top + sbox.Height - 1,
				 data->penarray,
				 data->numpens,
				 data->freedom,
				 GradientSliderBase);
		    
		} /* data->numpens >= 2 */
		
	    } /* if ((sbox.Width >= 2) && (sbox.Height >= 2)) */
	    
    	    break;
    	    
    	    
    } /* switch (redraw method) */
    
    if (EG(o)->Flags & GFLG_DISABLED)
    {
    	DrawDisabledPattern(rp, &gbox, dri->dri_Pens[SHADOWPEN], GradientSliderBase);
    }
        	
    ReturnVoid("GradientSlider::Render");
}

/***************************************************************************************************/

STATIC VOID gradientslider_dispose(Class *cl, Object *o, Msg msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    if (data->frame) DisposeObject(data->frame);
    
    DoSuperMethodA(cl, o, msg);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR 			retval = 0UL;
    struct GradientSliderData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("GradientSlider::GoActive()\n"));

    return GMR_NOREUSE;
    
    ReturnInt("GradientSlider::GoActive", IPTR, retval);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct GradientSliderData	*data = INST_DATA(cl, o);
    IPTR 			retval = 0UL;
    struct InputEvent 		*ie = msg->gpi_IEvent;
    
    EnterFunc(bug("GradientSlider::HandleInput\n"));
    
    retval = GMR_MEACTIVE;
        
    ReturnInt("GradientSlider::HandleInput", IPTR, retval);
}

/***************************************************************************************************/

STATIC VOID gradientslider_layout(Class *cl, Object *o, struct gpLayout *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("GradientSlider::Layout()\n"));
        
    ReturnVoid("GradientSlider::Layout");
}


/***************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_gradientsliderclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
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
	    
	case GM_LAYOUT:
	    gradientslider_layout(cl, o, (struct gpLayout *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = gradientslider_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = gradientslider_handleinput(cl, o, (struct gpInput *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)gradientslider_set(cl, o, (struct opSet *)msg);
	    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
	     * because it would circumvent the subclass from fully overriding it.
	     * The check of cl == OCLASS(o) should fail if we have been
	     * subclassed, and we have gotten here via DoSuperMethodA().
	     */
	    if ( retval && ((msg->MethodID != OM_UPDATE) || (cl == OCLASS(o))) )
	    {
	    	struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;

	    	if (gi)
	    	{
		    struct RastPort *rp = ObtainGIRPort(gi);

		    if (rp)
		    {		        
		        DoMethod(o, 
				 GM_RENDER,
				 gi,
				 rp,
				 FindTagItem(GA_Disabled, ((struct opSet *)msg)->ops_AttrList) ? GREDRAW_REDRAW : GREDRAW_UPDATE
				 );
				 
		        ReleaseGIRPort(rp);

		    } /* if */
		    
	    	} /* if */
		
	    } /* if */

	    break;

	case OM_GET:
	    retval = gradientslider_get(cl, o, (struct opGet *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return (retval);
    
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
