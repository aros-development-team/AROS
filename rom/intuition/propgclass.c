/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: AROS propgclass implementation.
    Lang: english
*/

#include <exec/types.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/boopsi.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <aros/asmcall.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "intuition_intern.h"
#include "propgadgets.h"

#undef IntuitionBase
#define IntuitionBase ((struct IntuitionBase *)(cl->cl_UserData))

#define EG(o)		((struct ExtGadget *)o)

/*****************************************************************************/

struct PropGData
{
    /* We use a propinfo structure, because we use the same routines
    for intuition propgadtets and in propgclass */
    
    struct PropInfo propinfo;
    
    /* A little kludge: since the HandleMouseMove function
    wants dx/dy and not absolute mouse coords, we
    have to remember the last ones */
    
    UWORD last_x;
    UWORD last_y;
    
    /* One only have to store total or visble, and use some other
    formulas than those in the RKRM:L, but for 
    code simplicity I store them all.  */
    UWORD top, visible, total;
};

#define PGD(x) ((struct PropGData *)x)

#define SetGadgetType(gad, type) \
	EG(gad)->GadgetType &= ~GTYP_GTYPEMASK; \
	EG(gad)->GadgetType |= type;

#undef MAX
#define MAX(a,b) \
 ((a) > (b)) ? (a) : (b);

static VOID FindScrollerValues
(
    UWORD   total,
    UWORD   visible,
    UWORD   top,
    WORD    overlap,
    UWORD   *body,
    UWORD   *pot
)
{
    UWORD hidden;
    
    hidden = MAX(total - visible, 0);
    
    if (top > hidden)
    	top = hidden;
   	
    *body = (hidden > 0) ?
    	(UWORD)(((ULONG)(visible - overlap) * MAXBODY) / (total - overlap)) :
    	MAXBODY;

    *pot = (hidden > 0) ? (UWORD)(((ULONG) top * MAXPOT) / hidden) : 0;
    
    return;
}

static UWORD FindScrollerTop(UWORD total, UWORD visible, UWORD pot)
{
    UWORD top, hidden;
    
    hidden = MAX(total - visible, 0);
    
    top = (((ULONG) hidden * pot) + (MAXPOT / 2)) >> 16;
    
    return (top);
}

static VOID NotifyTop(Class *cl, Object *o, struct GadgetInfo *gi, UWORD top, BOOL final)
{
    struct PropGData *data = INST_DATA(cl, o);
    
    if (data->top != top) /* A new value for top ? */
    {

    	struct opUpdate notifymsg;
    	struct TagItem notifyattrs[3];
	
	D(bug("NotifyTop(top=%d, final=%d)\n",
		top, final));
    	
    	notifyattrs[0].ti_Tag	= PGA_Top;
    	notifyattrs[0].ti_Data 	= top;
    	notifyattrs[1].ti_Tag	= GA_ID;
    	notifyattrs[1].ti_Data	= EG(o)->GadgetID;
    	notifyattrs[2].ti_Tag	= TAG_END;
	
    	notifymsg.MethodID	    = OM_NOTIFY;
    	notifymsg.opu_AttrList  = notifyattrs;
    	notifymsg.opu_GInfo	    = gi;
    	notifymsg.opu_Flags	    = (final != FALSE) ? 0 : OPUF_INTERIM;
	
    	DoSuperMethodA(cl, o, (Msg)&notifymsg);
    }
    
    return;
}

static VOID UpdateTop(Class *cl, Object *o, struct GadgetInfo *gi, BOOL final)
{
    /* Updates the PGA_Top attribute accordin to the Bofy/Pot vars.
    ** Also triggers notifcation if PGA_Top has changed.
    */

    UWORD top, pot;
    struct PropGData *data = (struct PropGData *)INST_DATA(cl, o);
    
    D(bug("UpdateTop()\n"));
    
    pot = (data->propinfo.Flags & FREEVERT) ? data->propinfo.VertPot :
       					      data->propinfo.HorizPot;
    	    					      
    top = FindScrollerTop(data->total, data->visible, pot);
    
    D(bug("Found scroller top: %d, old %d\n", top, data->top));
    	    
    /* PGA_Top changed by user ? */
    if (top != data->top)
    {
    	D(bug("top != data->top, calling NotifyTop\n"));
      	NotifyTop(cl, o, gi, top, final);
	
	/* Set this afterwards, since NotifyTop checks top != data->top */
    	data->top = top;
    }
    return;
}


#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)			\
    	flagvar |= flag;		\
    else				\
    	flagvar &= ~flag;

static IPTR set_propgclass(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = 0UL;
    
    struct TagItem *tag, *tstate;
    struct PropGData *data;
    BOOL set_flag = FALSE;
    BOOL rerender = FALSE;
    
    data = INST_DATA(cl, o);
    tstate = msg->ops_AttrList;
    
    /* Set to 1 to signal visual changes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
    	switch (tag->ti_Tag)
    	{
    	    case PGA_Top:
		NotifyTop(cl, o, msg->ops_GInfo, tag->ti_Data, TRUE);
		/* Set this afterwards, since NotifyTop checks top != data->top */
		data->top = tag->ti_Data;
 	    	set_flag= TRUE; rerender = TRUE;
		retval = 1UL;
		break;
    	
    	    case PGA_Visible:
		data->visible = tag->ti_Data;
		set_flag = TRUE; rerender = TRUE;
		retval = 1UL;
		break;
    	    
	    case PGA_Total:
		data->total = tag->ti_Data;
    		set_flag = TRUE; rerender = TRUE;
		retval = 1UL;

		break;

	    /* When one of the four next ones is set, what should then happen
	    with PGA_Top, Total and Visible ?
	    For example decreasing Body could mean both a decrease of
	    Visible or an increase in Total. Which of them it is,
	    we cannot know. So we say that the PGA_xxxPot/Body
	    attrs should not be used along with Top, Total and Visible. */
	    
	    case PGA_HorizPot:
	    	data->propinfo.HorizPot	= (UWORD)tag->ti_Data;
		rerender = TRUE;
	    	retval = 1UL;
	    	break;

	    case PGA_HorizBody:
	    	data->propinfo.HorizBody= (UWORD)tag->ti_Data;
		rerender = TRUE;
	    	retval = 1UL;
	    	break;

	    case PGA_VertPot:
	    	data->propinfo.VertPot	= (UWORD)tag->ti_Data;
		rerender = TRUE;
	    	retval = 1UL;
	    	break;

	    case PGA_VertBody:
	    	data->propinfo.VertBody = (UWORD)tag->ti_Data;
		rerender = TRUE;
	    	retval = 1UL;
	    	break;
		
	    case PGA_Freedom:
	    	data->propinfo.Flags &= ~(FREEHORIZ|FREEVERT);
	        data->propinfo.Flags |= tag->ti_Data;
	        break;
	    
	    case PGA_NewLook:
	        SETFLAG(data->propinfo.Flags, tag->ti_Data, PROPNEWLOOK);
	        break;
	        
	    case PGA_Borderless:
	        SETFLAG(data->propinfo.Flags, tag->ti_Data, PROPBORDERLESS);
	        break;
	        
	    case GA_Image:
	        break;
	    
	    case GA_Border:
	        break;
	        
	    case GA_Highlight:
	        /* Convert GADGHBOX to GADGHCOMP */
	    	if (tag->ti_Data & GFLG_GADGHBOX)
	    	{
	    	    /* Clear GADCHBOX */
	    	    tag->ti_Data &= ~GFLG_GADGHBOX;
	    	    /* Set GADGHCOMP */
	    	    tag->ti_Data |= GFLG_GADGHCOMP;
	    	}
	        break;
	        
		
	    default:
	    	break;
	    	
    	} /* switch (tag->ti_Tag) */
    	
    } /* while ((tag = NextTagItem(&tstate)) != NULL) */
    
    /* Top, Visible or Total set */
    if (set_flag)
    {
    	UWORD *bodyptr, *potptr;
    	
    	if (data->propinfo.Flags & FREEVERT)
    	{
    	    bodyptr = &(data->propinfo.VertBody);
    	    potptr  = &(data->propinfo.VertPot);
    	}
    	else
    	{
    	    bodyptr = &(data->propinfo.HorizBody);
    	    potptr  = &(data->propinfo.HorizPot);
    	}
    	    
    	FindScrollerValues
    	(
    	    data->total,
    	    data->visible,
    	    data->top,
    	    0,
    	    bodyptr,
    	    potptr
    	);
    }
    
    /* The two last tests below may be redundant */
    if (rerender && (OM_NEW != msg->MethodID) && (NULL != msg->ops_GInfo))
    {
        struct RastPort *rp;
	
	rp = ObtainGIRPort(msg->ops_GInfo);
    	if (NULL != rp)
	{
	    DoMethod(o, GM_RENDER, msg->ops_GInfo, rp, GREDRAW_REDRAW);
	    ReleaseGIRPort(rp);
	}
    }

    return (retval);
}

static IPTR new_propgclass(Class *cl, Object *o, struct opSet *msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	struct PropGData *data;
    	data = INST_DATA(cl, o);

    	/* A hack to make the functions in propgadgets.c work
    	with this class */
    	EG(o)->SpecialInfo = &(data->propinfo);
	    	
   	/* Set some default values in the propinfo structure */
	memset(&(data->propinfo), 0, sizeof (struct PropInfo));
    	data->propinfo.Flags	= PROPNEWLOOK|AUTOKNOB|FREEVERT;
    	data->propinfo.VertPot	= MAXPOT;
    	data->propinfo.VertBody = 20000;
	    	
    	/* Handle our special tags - overrides defaults */
   	set_propgclass(cl, o, msg);
	    
    } /* if (object created) */
    return ((IPTR)o);
}

static IPTR get_propgclass(Class *cl, Object *o,struct opGet *msg)
{
    IPTR retval = 1UL;
    struct PropGData *data;
    
    data = INST_DATA(cl, o);
    
    switch (msg->opg_AttrID)
    {
    	case PGA_Top:
    	    *(msg->opg_Storage) = data->top;
	    break;
	    
	case PGA_Total:
    	    *(msg->opg_Storage) = data->total;
	    break;

	case PGA_Visible:
    	    *(msg->opg_Storage) = data->visible;
	    break;
	    
	case PGA_HorizPot:
	    *(msg->opg_Storage) = data->propinfo.HorizPot;
	    break;
	    
	case PGA_HorizBody:
	    *(msg->opg_Storage) = data->propinfo.HorizBody;
	    break;

	case PGA_VertPot:
	    *(msg->opg_Storage) = data->propinfo.VertPot;
	    break;

	case PGA_VertBody:
	    *(msg->opg_Storage) = data->propinfo.VertBody;
	    break;
	    
    	default:
    	    retval = DoSuperMethodA(cl, o, (Msg)msg);
    	    break;
    }
    return (retval);    
}

static IPTR goactive_propgclass(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = GMR_NOREUSE;
	    
    /* Was GOACTIVE caused by an input event ? */
    if (msg->gpi_IEvent)
    {
    	struct PropGData *data;
       	data = INST_DATA(cl, o);

   	/* Fake a standard intuition prop gadget.
    	Of course this is a hack. */
    	SetGadgetType(o, GTYP_PROPGADGET);

    	/* Handle SelectDown event */
    	HandlePropSelectDown
    	(
            (struct Gadget *)o,
    	    msg->gpi_GInfo->gi_Window,
            msg->gpi_GInfo->gi_Requester,
            msg->gpi_Mouse.X,
    	    msg->gpi_Mouse.Y,
            IntuitionBase
        );
	SetGadgetType(o, GTYP_CUSTOMGADGET);
    	    	
    	if (!(data->propinfo.Flags & KNOBHIT))
    	{
    	    /* If the knob was not hit, swallow hit-event.
    	    (Gadget has allready been updated) */

    	    /* Update PGA_Top. Final update. */
    	    UpdateTop(cl, o, msg->gpi_GInfo, TRUE);

    	    *(msg->gpi_Termination) = data->top;
    	    retval = GMR_NOREUSE|GMR_VERIFY;
    	}
    	else
    	{
    	     /* We must remember mousepos for use in GM_HANDLEINPUT */
    	     data->last_x = msg->gpi_Mouse.X;
    	     data->last_y = msg->gpi_Mouse.Y;
    	    	     
    	     retval = GMR_MEACTIVE; /* Stay active */
    	} /* if not knob was hit */
    	    			
    } /* if gadget was activated by an input event */
	    
    return (retval);
} 

static IPTR handleinput_propgclass(Class *cl, Object *o, struct gpInput *msg)
{
    struct InputEvent *ie;
	    
    /* Default: stay active */
    IPTR retval = GMR_MEACTIVE;
    struct PropGData *data = INST_DATA(cl, o);
	    
    ie = msg->gpi_IEvent;
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
    	switch (ie->ie_Code)
    	{
    	    case IECODE_NOBUTTON:
    	    {
    	    	struct PropGData *data;
	    	    
    	    	data = INST_DATA(cl, o);
	    	    
    	    	/* Fake a standard intuition prop gadget */
    	    	SetGadgetType(o, GTYP_PROPGADGET);
   	    	HandlePropMouseMove
    	    	(
    	   	    (struct Gadget *)o,
    	    	    msg->gpi_GInfo->gi_Window,
    	    	    msg->gpi_GInfo->gi_Requester,
    	    	    msg->gpi_Mouse.X, /* - data->last_x, */
    	    	    msg->gpi_Mouse.Y, /* - data->last_y, */
    	    	    IntuitionBase
    		);
	    	    	
    	    	SetGadgetType(o, GTYP_CUSTOMGADGET);	    	    	
	    	    
    	    	data->last_x = msg->gpi_Mouse.X;
    	    	data->last_y = msg->gpi_Mouse.Y;
		
		D(bug("Calling UpdateTop\n"));
    	    	
    	    	/* Update PGA_Top. Interim update. */
    	    	UpdateTop(cl, o, msg->gpi_GInfo, FALSE);
    	    	
	    } break;
	    	    
	    case SELECTUP:
    	    	/* User has released the knob. Refresh knob */
	    	    	
    	    	SetGadgetType(o, GTYP_PROPGADGET);
    	    	
    	    	HandlePropSelectUp
    	    	(
    	    	    (struct Gadget *)o,
    	    	    msg->gpi_GInfo->gi_Window,
    	    	    msg->gpi_GInfo->gi_Requester,
    	    	    IntuitionBase
    	    	);
	    	    	    
    	    	SetGadgetType(o, GTYP_CUSTOMGADGET);
    	    	
    	    	/* Update PGA_Top. Final update. */
    	    	UpdateTop(cl, o, msg->gpi_GInfo, TRUE);
    	    
    	    	*(msg->gpi_Termination) = data->top;
    	    	retval = GMR_NOREUSE|GMR_VERIFY;
    	    	break;
	    	
    	} /* switch (ie->ie_Code) */
	    	
    } /* if (ie->ie_Class == IECLASS_RAWMOUSE) */

    return (retval);    
}

static IPTR render_propgclass(Class *cl, Object *o, struct gpRender *msg)
{
    	
    /* Fake a standard intuition prop gadget */
    SetGadgetType(o, GTYP_PROPGADGET);
    	    	    
    /* Redraw the whole gadget */
    RefreshPropGadget
    (
    	(struct Gadget *)o,
   	msg->gpr_GInfo->gi_Window,
    	IntuitionBase
    );
    
    SetGadgetType(o, GTYP_CUSTOMGADGET);

    return (1UL);

}

static IPTR goinactive_propgclass(Class *cl, Object *o, struct gpGoInactive *msg)
{

    /* Gadget cancelled by intuition ? */
    if (msg->gpgi_Abort == 1)
    {
    	SetGadgetType(o, GTYP_PROPGADGET);
    	    	
    	HandlePropSelectUp
    	(
    	    (struct Gadget *)o,
    	    msg->gpgi_GInfo->gi_Window,
    	    msg->gpgi_GInfo->gi_Requester,
    	    IntuitionBase
    	);
	    	    	    
	SetGadgetType(o, GTYP_CUSTOMGADGET);
	
	/* Update PGA_Top. Final update */
    	UpdateTop(cl, o, msg->gpgi_GInfo, TRUE);
    }
    return (0UL);   	    	
}


/* propgclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_propgclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
    	case GM_RENDER:
    	    retval = render_propgclass(cl, o, (struct gpRender *)msg);
    	    break;
	    
	case GM_GOACTIVE:
	    retval = goactive_propgclass(cl, o, (struct gpInput *)msg);
	    break;

	case GM_HANDLEINPUT:
	    retval = handleinput_propgclass(cl, o, (struct gpInput *)msg);
	    break;
	    	    
	case GM_GOINACTIVE:
	    retval = goinactive_propgclass(cl, o, (struct gpGoInactive *)msg);
	    break;

	case OM_NEW:
	    retval = new_propgclass(cl, o, (struct opSet *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = DoSuperMethodA(cl, o, msg);
	    retval += (IPTR)set_propgclass(cl, o, (struct opSet *)msg);

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


	case OM_GET:
	    retval = (IPTR)get_propgclass(cl, o, (struct opGet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    } /* switch */

    return (retval);
}  /* dispatch_propgclass */


#undef IntuitionBase

/****************************************************************************/

/* Initialize our propg class. */
struct IClass *InitPropGClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the gadgetclass...
     */
    if ((cl = MakeClass(PROPGCLASS, GADGETCLASS, NULL, sizeof(struct PropGData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_propgclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

