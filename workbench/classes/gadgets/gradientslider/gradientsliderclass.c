/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: AROS gradientslider gadget.
    Lang: english
*/

#define USE_BOOPSI_STUBS
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>
#include <gadgets/gradientslider.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* abs() */
#include "gradientslider_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define GradientSliderBase ((struct GradientSliderBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>


/***************************************************************************************************/

STATIC IPTR gradientslider_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 		*tag, *tstate;
    IPTR 			retval = 0UL;
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    EnterFunc(bug("GradientSlider::Set()\n"));
    
    for (tstate = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
    	IPTR tidata = tag->ti_Data;
    	   	
    	switch (tag->ti_Tag)
    	{
	    default:
	        break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */
    
    ReturnPtr ("GradientSlider::Set", IPTR, retval);
}

/***************************************************************************************************/

STATIC Object *gradientslider_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("GradientSlider::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct GradientSliderData *data = INST_DATA(cl, o);
    	
    	gradientslider_set(cl, o, msg);
    	   
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
    
    EnterFunc(bug("GradientSlider::Render()\n"));    

    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	     
    	case GREDRAW_UPDATE:    	 
    	    break;
    	    
    	    
    } /* switch (redraw method) */
    
    if (EG(o)->Flags & GFLG_DISABLED)
    {
    	//DrawDisabledPattern(rp, gbox, dri->dri_Pens[SHADOWPEN], GradientSliderBase);
    }
        	
    ReturnVoid("GradientSlider::Render");
}

/***************************************************************************************************/

STATIC VOID gradientslider_dispose(Class *cl, Object *o, Msg msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    DoSuperMethodA(cl, o, msg);
}

/***************************************************************************************************/

STATIC IPTR gradientslider_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR 			retval = 0UL;
    struct GradientSliderData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("GradientSlider::GoActive()\n"));

    
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
