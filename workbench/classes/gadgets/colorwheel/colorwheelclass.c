/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: AROS colorwheel gadget.
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
#include <gadgets/colorwheel.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* abs() */
#include "colorwheel_intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define ColorWheelBase ((struct ColorWheelBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>


/***************************************************************************************************/

STATIC IPTR colorwheel_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 		*tag, *tstate;
    IPTR 			retval = 0UL;
    struct ColorWheelData 	*data = INST_DATA(cl, o);
    
    EnterFunc(bug("ColorWheel::Set()\n"));
    
    if (msg->MethodID == OM_SET) retval = DoSuperMethodA(cl, o, msg);
    
    for (tstate = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
    	IPTR tidata = tag->ti_Data;
    	   	
    	switch (tag->ti_Tag)
    	{
	    default:
	        break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */
    
    ReturnPtr ("ColorWheel::Set", IPTR, retval);
}

/***************************************************************************************************/

STATIC Object *colorwheel_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("ColorWheel::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct ColorWheelData *data = INST_DATA(cl, o);
    	
	data->scr = (struct Screen *)GetTagData(WHEEL_Screen, 0, msg->ops_AttrList);
	
	if (data->scr)
	{
    	    colorwheel_set(cl, o, msg);
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    	   
    }
    ReturnPtr ("ColorWheel::New", Object *, o);
}

/***************************************************************************************************/

STATIC IPTR colorwheel_get(Class *cl, Object *o, struct opGet *msg)
{
    struct ColorWheelData 	*data = INST_DATA(cl, o);
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

STATIC VOID colorwheel_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct ColorWheelData 	*data = INST_DATA(cl, o);    
    struct DrawInfo 		*dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort 		*rp = msg->gpr_RPort;
    struct IBox			gbox;
    
    EnterFunc(bug("ColorWheel::Render()\n"));    

    GetGadgetIBox(o, msg->gpr_GInfo, &gbox);
    
    data->dri = dri;
    
    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	    RenderWheel(data, rp, &gbox, ColorWheelBase);
	    break;
	    
    	case GREDRAW_UPDATE:    	 
    	    break;
    	    
    	    
    } /* switch (redraw method) */
    
    if (EG(o)->Flags & GFLG_DISABLED)
    {
    	//DrawDisabledPattern(rp, gbox, dri->dri_Pens[SHADOWPEN], ColorWheelBase);
    }
        	
    ReturnVoid("ColorWheel::Render");
}

/***************************************************************************************************/

STATIC VOID colorwheel_dispose(Class *cl, Object *o, Msg msg)
{
    struct ColorWheelData 	*data = INST_DATA(cl, o);
    
    if (data->rgblinebuffer) FreeVec(data->rgblinebuffer);
    
    DoSuperMethodA(cl, o, msg);
}

/***************************************************************************************************/

STATIC IPTR colorwheel_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR 			retval = 0UL;
    struct ColorWheelData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("ColorWheel::GoActive()\n"));
    
    retval = GMR_NOREUSE;
    
    ReturnInt("ColorWheel::GoActive", IPTR, retval);
}

/***************************************************************************************************/

STATIC IPTR colorwheel_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    struct ColorWheelData	*data = INST_DATA(cl, o);
    IPTR 			retval = 0UL;
    struct InputEvent 		*ie = msg->gpi_IEvent;
    
    EnterFunc(bug("ColorWheel::HandleInput \n"));

    retval = GMR_MEACTIVE;
        
    ReturnInt("ColorWheel::HandleInput", IPTR, retval);
}

/***************************************************************************************************/

STATIC VOID colorwheel_layout(Class *cl, Object *o, struct gpLayout *msg)
{
    struct ColorWheelData 	*data = INST_DATA(cl, o);

    EnterFunc(bug("ColorWheel::Layout()\n"));
        
    ReturnVoid("ColorWheel::Layout");
}


/***************************************************************************************************/

AROS_UFH3S(IPTR, dispatch_colorwheelclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    
    switch(msg->MethodID)
    {
	case OM_NEW:
	    retval = (IPTR)colorwheel_new(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    colorwheel_dispose(cl, o, msg);
	    break;
	    
	case GM_RENDER:
	    colorwheel_render(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_LAYOUT:
	    colorwheel_layout(cl, o, (struct gpLayout *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = colorwheel_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = colorwheel_handleinput(cl, o, (struct gpInput *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)colorwheel_set(cl, o, (struct opSet *)msg);
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
	    retval = colorwheel_get(cl, o, (struct opGet *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return (retval);
    
}  /* dispatch_colorwheelclass */


#undef ColorWheelBase

/***************************************************************************************************/

struct IClass *InitColorWheelClass (struct ColorWheelBase_intern * ColorWheelBase)
{
    struct IClass *cl = NULL;

    if ((cl = MakeClass("colorwheel.gadget", GADGETCLASS, NULL, sizeof(struct ColorWheelData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_colorwheelclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)ColorWheelBase;

	AddClass (cl);
    }

    return (cl);
}

/***************************************************************************************************/
