/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <graphics/clip.h>
#include <aros/asmcall.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "intuition_intern.h"
#include "inputhandler.h"
#include "propgadgets.h"

#define DEBUG_PROP(x)   ;

#ifdef PROPHACK
void PropRefreshTask(struct IntuitionBase *IntuitionBase,struct Gadget *gadget,struct Window *window,struct Requester *requester)
{
    ULONG signals;

    for (;;)
    {
        signals = Wait(PSIG_REFRESHALL|PSIG_DIE);

        //go_inactive refreshes the gadget on it's own, so we do not need to care about queued refreshes!
        if (signals & PSIG_DIE)
        {
            Forbid();
            ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask = 0;
            Permit();
            Signal(((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask,SIGF_INTUITION);
            return;
        }

        if (signals & PSIG_REFRESHALL)
        {
            RefreshPropGadget (gadget, window, requester, IntuitionBase);
        }

      //no need to refresh the knob when whole gadget was refreshed already
      //if ((signals & PSIG_REFRESHKNOB) && (!(signals & PSIG_REFRESHALL)))
      //{
      //}

    }
}
#endif

/*****************************************************************************************
 
stegerg: Tested behaviour under AmigaOS:
 
propgclass object:
    renders itself during OM_SET(PGA_Top)    : yes, method retval = 0
    renders itself during OM_UPDATE(PGA_Top) : yes, method retval = 0
    sends OM_NOTIFY during OM_SET(PGA_Top)   : no
    sends OM_NOTIFY during OM_UPDATE(PGA_Top): no
    sends OM_NOTIFY when user drags knob     : yes
 
    -> only when user uses gadget, OM_NOTIFY is sent
 
propgclass subclass object:
    renders itself during OM_SET(PGA_Top)    : no, method retval = 1 [*]
    renders itself during OM_UPDATE(PGA_Top) : no, method retval = 1 [*]
    sends OM_NOTIFY during OM_SET(PGA_Top)   : no
    sends OM_NOTIFY during OM_UPDATE(PGA_Top): no
    sends OM_NOTIFY when user drags knob     : yes
 
Here [*], another weird behaviour is, that the ~internal (??) PGA_Top value
stays at the old value, so you can click at the knob where you see it
(although because of the real PGA_Top it could in reality be in some
completely different place) and use it. Only when the gadget is re-rendered
(completely?), or you click somewhere in the prop box to cause a ~one page knob
move, the ~internal PGA_Top gets updated to the real value. Note, that
GetAttr(PGA_Top) will always be correct, so when I say ~internal PGA_Top,
this must be some internal variable only for inputhandling of gadget!!
BTW: With PGA_Total (and most likely also PGA_Visible) it's exactly the
same: imagine before the OM_SET/OM_UPDATE the prop knob was 10 % of the
size of the prop box. And after the OM_SET/OM_UPDATE(PGA_Total) the prop knob
would be 50 % of the size of the prop box -> again - before a rerendering of
the gadget happens, or a jump-one-page click by user - the gadget behaves
like it was still having the old PGA_Total value (-> knob size == 10 % of
prop box).
 
AROS propgclass at the moment does not copy this behaviour!!!!
 
*****************************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

#define EG(o)       	((struct ExtGadget *)o)
#define G(o)        	((struct Gadget *)o)

/****************************************************************************************/

struct PropGData
{
    /* We use a propinfo structure, because we use the same routines
    for intuition propgadtets and in propgclass */

    struct PropInfo  propinfo;

    /* A little kludge: since the HandleMouseMove function
    wants dx/dy and not absolute mouse coords, we
    have to remember the last ones */

    UWORD            last_x;
    UWORD            last_y;

    /* One only have to store total or visble, and use some other
    formulas than those in the RKRM:L, but for
    code simplicity I store them all.  */
    UWORD            top, visible, total;

    UWORD            flags;
    struct BBox     *old_knobbox;
};

/****************************************************************************************/

#define PGD(x)      	    	    ((struct PropGData *)x)

#define SetGadgetType(gad, type)    EG(gad)->GadgetType &= ~GTYP_GTYPEMASK; \
    	    	    	    	    EG(gad)->GadgetType |= type;

#define PRIVFLAG_NICERENDER 	    1
#define PRIVFLAG_NICENOTIFY 	    2

/****************************************************************************************/

static VOID FindScrollerValues(UWORD total, UWORD visible, UWORD top,
    	    	    	       WORD overlap, UWORD *body, UWORD *pot)
{
    UWORD hidden;

    if (total > visible)
        hidden = total - visible;
    else
        hidden = 0;

    if (top > hidden)
        top = hidden;

    *body = (hidden > 0) ?
            (UWORD)(((ULONG)(visible - overlap) * MAXBODY) / (total - overlap)) :
            MAXBODY;

    *pot = (hidden > 0) ? (UWORD)(((ULONG) top * MAXPOT) / hidden) : 0;

    return;
}

/****************************************************************************************/

static UWORD FindScrollerTop(UWORD total, UWORD visible, UWORD pot)
{
    UWORD top, hidden;

    if (total > visible)
        hidden = total - visible;
    else
        hidden = 0;

    top = (((ULONG) hidden * pot) + (MAXPOT / 2)) >> 16;

    return (top);
}

/****************************************************************************************/

static VOID NotifyTop(Class *cl, Object *o, struct GadgetInfo *gi, BOOL final)
{
    struct PropGData *data = INST_DATA(cl, o);
    struct opUpdate   notifymsg;
    struct TagItem    notifyattrs[3];

    D(bug("PropGClass: NotifyTop(top=%d, final=%d)\n",
          data->top, final));

    notifyattrs[0].ti_Tag   = PGA_Top;
    notifyattrs[0].ti_Data  = data->top;
    notifyattrs[1].ti_Tag   = GA_ID;
    notifyattrs[1].ti_Data  = EG(o)->GadgetID;
    notifyattrs[2].ti_Tag   = TAG_END;

    notifymsg.MethodID      = OM_NOTIFY;
    notifymsg.opu_AttrList  = notifyattrs;
    notifymsg.opu_GInfo     = gi;
    notifymsg.opu_Flags     = (final != FALSE) ? 0 : OPUF_INTERIM;

    DoSuperMethodA(cl, o, (Msg)&notifymsg);

    return;
}

/****************************************************************************************/

static VOID UpdateTop(Class *cl, Object *o, struct GadgetInfo *gi, BOOL final)
{
    /* Updates the PGA_Top attribute accordin to the Bofy/Pot vars.
    ** Also triggers notifcation if PGA_Top has changed.
    */

    struct PropGData	*data = (struct PropGData *)INST_DATA(cl, o);
    UWORD           	 top, pot;

    D(bug("PropGClass: UpdateTop()\n"));

    pot = (data->propinfo.Flags & FREEVERT) ? data->propinfo.VertPot :
          data->propinfo.HorizPot;

    top = FindScrollerTop(data->total, data->visible, pot);

    D(bug("PropGClass: Found scroller top: %d, old %d\n", top, data->top));

    /* PGA_Top changed by user ? */
    if ((top != data->top))
    {
        D(bug("PropGClass: top != data->top, calling NotifyTop\n"));

        data->top = top;

        NotifyTop(cl, o, gi, final);
    }
    
    return;
}

/****************************************************************************************/

#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)            \
    	flagvar |= flag;    \
    else                    \
    	flagvar &= ~flag;

/****************************************************************************************/

static IPTR set_propgclass(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem  	*tag, *tstate;
    struct PropGData    *data;
    struct BBox     	 old_knobbox;
    struct opSet    	 method;
    UWORD     	    	 newtop;
    BOOL            	 set_flag = FALSE;
    BOOL            	 was_disabled = FALSE;
    BOOL            	 full_redraw = FALSE;
    BOOL            	 old_knobbox_ok = FALSE;
    IPTR            	 retval;

    data = INST_DATA(cl, o);
    tstate = msg->ops_AttrList;

    was_disabled = (G(o)->Flags & GFLG_DISABLED) ? TRUE : FALSE;

    method.MethodID = OM_SET;
    method.ops_AttrList = msg->ops_AttrList;
    method.ops_GInfo = msg->ops_GInfo;
    retval = DoSuperMethodA(cl, o, (Msg)&method);

    if ( ((G(o)->Flags & GFLG_DISABLED) ? TRUE : FALSE) != was_disabled ) full_redraw = TRUE;

    if (msg->ops_GInfo)
    {
        CalcBBox (msg->ops_GInfo->gi_Window, msg->ops_GInfo->gi_Requester, G(o), &old_knobbox);
        old_knobbox_ok = CalcKnobSize(G(o), &old_knobbox);
    }

    newtop = data->top; /* !! */

    /* Set to 1 to signal visual changes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case PGA_Top:
            newtop = (UWORD)tag->ti_Data;
            /* This will be poked into data->top later below, because the
               value might have to be adjusted. It depends on PGA_Total
               and PGA_Visible which might be also set later in the taglist */
            set_flag= TRUE;
            retval += 1UL;
            break;

        case PGA_Visible:
            data->visible = tag->ti_Data;
            set_flag = TRUE;
            retval += 1UL;
            break;

        case PGA_Total:
            data->total = tag->ti_Data;
            set_flag = TRUE;
            retval += 1UL;
            break;

            /* When one of the four next ones is set, what should then happen
            with PGA_Top, Total and Visible ?
            For example decreasing Body could mean both a decrease of
            Visible or an increase in Total. Which of them it is,
            we cannot know. So we say that the PGA_xxxPot/Body
            attrs should not be used along with Top, Total and Visible. */

        case PGA_HorizPot:
            data->propinfo.HorizPot = (UWORD)tag->ti_Data;
            retval += 1UL;
            break;

        case PGA_HorizBody:
            data->propinfo.HorizBody= (UWORD)tag->ti_Data;
            retval += 1UL;
            break;

        case PGA_VertPot:
            data->propinfo.VertPot  = (UWORD)tag->ti_Data;
            retval += 1UL;
            break;

        case PGA_VertBody:
            data->propinfo.VertBody = (UWORD)tag->ti_Data;
            retval += 1UL;
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

    /* Top, Visible or Total set? */

    if (set_flag)
    {
        UWORD *bodyptr, *potptr;

        /* fix top  value if necessary */

        if (data->total > data->visible)
        {
            if (newtop > (data->total - data->visible))
                newtop = data->total - data->visible;
        }
        else
        {
            newtop = 0;
        }

        if (data->top != newtop)
        {
            data->top = newtop;
            if (data->flags & PRIVFLAG_NICENOTIFY)
            {
                NotifyTop(cl, o, msg->ops_GInfo, TRUE);
            }
        }

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
    if ((retval || full_redraw) && (NULL != msg->ops_GInfo) &&
            ((OCLASS(o) == cl) || (data->flags & PRIVFLAG_NICERENDER)))
    {
        struct RastPort *rp;

        rp = ObtainGIRPort(msg->ops_GInfo);
        if (NULL != rp)
        {
            struct gpRender method;

            data->old_knobbox = old_knobbox_ok ? &old_knobbox : 0;

            method.MethodID   = GM_RENDER;
            method.gpr_GInfo  = msg->ops_GInfo;
            method.gpr_RPort  = rp;
            method.gpr_Redraw = full_redraw ? GREDRAW_REDRAW : GREDRAW_UPDATE;
	    
            DoMethodA(o, (Msg)&method);

            ReleaseGIRPort(rp);
        }
        if (!(data->flags & PRIVFLAG_NICERENDER))
        {
            /* retval of 1 indicates that user needs to rerender gadget
               manually, while 0 means he does not need to do so */
            retval = 0;
        }
    }

    return (retval);
}

/****************************************************************************************/

static IPTR new_propgclass(Class *cl, Object *o, struct opSet *msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    D(bug("PropGClass: new %p\n", o));

    if (o)
    {
        struct PropGData *data = INST_DATA(cl, o);

        /* A hack to make the functions in propgadgets.c work
        with this class */
        EG(o)->SpecialInfo = &(data->propinfo);

        /* Set some default values in the propinfo structure */
        /*
          The instance object is cleared memory!
          memset(&(data->propinfo), 0, sizeof (struct PropInfo));
        */
        data->propinfo.Flags    = PROPNEWLOOK|AUTOKNOB|FREEVERT;
        data->propinfo.VertPot  = 0;
        data->propinfo.VertBody = MAXBODY;
        data->propinfo.HorizPot = 0;
        data->propinfo.HorizBody = MAXBODY;
        data->top     = 0;
        data->visible = 1;
        data->total   = 1;

        if (GetTagData(PGA_NotifyBehaviour, PG_BEHAVIOUR_COMPATIBLE, msg->ops_AttrList) ==
                PG_BEHAVIOUR_NICE)
        {
            data->flags |= PRIVFLAG_NICENOTIFY;
        }

        if (GetTagData(PGA_RenderBehaviour, PG_BEHAVIOUR_COMPATIBLE, msg->ops_AttrList) ==
                PG_BEHAVIOUR_NICE)
        {
            data->flags |= PRIVFLAG_NICERENDER;
        }

        /* Handle our special tags - overrides defaults */
        set_propgclass(cl, o, msg);

    } /* if (object created) */

    return ((IPTR)o);
}

/****************************************************************************************/

static IPTR get_propgclass(Class *cl, Object *o,struct opGet *msg)
{
    struct PropGData *data;
    IPTR              retval = 1UL;

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

    case PGA_Freedom:
        *(msg->opg_Storage) = data->propinfo.Flags & (FREEHORIZ|FREEVERT);
        break;

    default:
        retval = DoSuperMethodA(cl, o, (Msg)msg);
        break;
    }
    return (retval);
}

/****************************************************************************************/

static IPTR goactive_propgclass(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = GMR_NOREUSE;

    /* Was GOACTIVE caused by an input event ? */
    if (msg->gpi_IEvent)
    {
        struct PropGData *data = INST_DATA(cl, o);

        /* Fake a standard intuition prop gadget.
        Of course this is a hack. */
        SetGadgetType(o, GTYP_PROPGADGET);

        /* Handle SelectDown event */
        HandlePropSelectDown
        (
            G(o),
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

            /* enable buffering to speed up refresh */
            ((struct IntWindow *)(msg->gpi_GInfo->gi_Window))->specialflags |= SPFLAG_WANTBUFFER;

    	#ifdef PROPHACK
            ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask =
                NewCreateTask(TASKTAG_CODETYPE, CODETYPE_PPC, TASKTAG_PC, (ULONG)PropRefreshTask,
                                TASKTAG_PRI,0,
                                TASKTAG_PPC_ARG1,(ULONG)IntuitionBase,
                                TASKTAG_PPC_ARG2,(ULONG)o,
                                TASKTAG_PPC_ARG3,(ULONG)msg->gpi_GInfo->gi_Window,
                                TASKTAG_PPC_ARG4,(ULONG)msg->gpi_GInfo->gi_Requester,
                                TAG_DONE);
    	#endif
        
        } /* if not knob was hit */

    } /* if gadget was activated by an input event */

    return (retval);
}

/****************************************************************************************/

static IPTR handleinput_propgclass(Class *cl, Object *o, struct gpInput *msg)
{
    struct InputEvent   *ie;
    struct PropGData    *data = INST_DATA(cl, o);

    /* Default: stay active */
    IPTR        retval = GMR_MEACTIVE;

    ie = msg->gpi_IEvent;
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        switch (ie->ie_Code)
        {
        case IECODE_NOBUTTON:
            if ((msg->gpi_Mouse.X != data->last_x) || (msg->gpi_Mouse.Y != data->last_y))
            {
                 /* Fake a standard intuition prop gadget */
                SetGadgetType(o, GTYP_PROPGADGET);
                HandlePropMouseMove
                (
                    G(o),
                    msg->gpi_GInfo->gi_Window,
                    msg->gpi_GInfo->gi_Requester,
                    msg->gpi_Mouse.X, /* - data->last_x, */
                    msg->gpi_Mouse.Y, /* - data->last_y, */
                    IntuitionBase
                );

                SetGadgetType(o, GTYP_CUSTOMGADGET);

                data->last_x = msg->gpi_Mouse.X;
                data->last_y = msg->gpi_Mouse.Y;

                D(bug("PropGClass: Calling UpdateTop\n"));

                /* Update PGA_Top. Interim update. */
                UpdateTop(cl, o, msg->gpi_GInfo, FALSE);

            }
            break;

        case SELECTUP:
            /* User has released the knob. Refresh knob */

    #ifdef PROPHACK
        if (((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask)
        {
            ULONG   	 oldSignals = SetSignal(0,SIGF_INTUITION);
            struct Task *task = ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask;
            ULONG   	 signals = 0;

            Forbid();
            ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask = 0;
            Permit();

            if (task) //small protection
            {
                Signal(task,PSIG_DIE);

                signals = Wait(SIGF_INTUITION);
            }

            SetSignal(oldSignals | (signals & ~SIGF_INTUITION),0xFFFFFFFF);
        }
    #endif

            SetGadgetType(o, GTYP_PROPGADGET);

            HandlePropSelectUp
            (
                G(o),
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

/****************************************************************************************/

static IPTR render_propgclass(Class *cl, Object *o, struct gpRender *msg)
{
    struct PropGData *data = INST_DATA(cl, o);

    DEBUG_PROP(dprintf("render_propgclass:\n"));

#ifdef PROPHACK
    Forbid();
    if ((((struct Gadget *)o) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->ActiveGadget) && (FindTask(0) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask) && ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask)
    {
        Signal(((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask,PSIG_REFRESHALL);
        return (1UL);
    }
    Permit();
#endif

    /* Fake a standard intuition prop gadget */
    SetGadgetType(o, GTYP_PROPGADGET);

    if ((msg->gpr_Redraw == GREDRAW_UPDATE) && (data->old_knobbox))
    {
        struct BBox new_knobbox;

        CalcBBox (msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Requester, G(o), &new_knobbox);

        DEBUG_PROP(dprintf("render_propgclass: BBox Left %ld Top %ld Width %ld Height %ld\n",
                           new_knobbox.Left,
                           new_knobbox.Width,
                           new_knobbox.Top,
                           new_knobbox.Height));

        if (CalcKnobSize(G(o), &new_knobbox))
        {
    	#if PROP_RENDER_OPTIMIZATION
            RefreshPropGadgetKnob (G(o), data->old_knobbox, &new_knobbox, msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Requester, IntuitionBase);
    	#else
            struct BBox gbox;

            CalcBBox (msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Requester, G(o), &gbox);
            RefreshPropGadgetKnob (G(o), &gbox, &new_knobbox, msg->gpr_GInfo->gi_Window, msg->gpr_GInfo->gi_Requester, IntuitionBase);
    	#endif
        }

        data->old_knobbox = 0;
    }
    else
    {
        /* Redraw the whole gadget */
        RefreshPropGadget
        (
            G(o),
            msg->gpr_GInfo->gi_Window,
            msg->gpr_GInfo->gi_Requester,
            IntuitionBase
        );
    }

    SetGadgetType(o, GTYP_CUSTOMGADGET);

    return (1UL);

}

/****************************************************************************************/

static IPTR goinactive_propgclass(Class *cl, Object *o, struct gpGoInactive *msg)
{
    /* Gadget cancelled by intuition ? */

    if (msg->gpgi_Abort == 1)
    {

    #ifdef PROPHACK
        if (((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask)
        {
            ULONG   	 oldSignals = SetSignal(0,SIGF_INTUITION);
            struct Task *task = ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask;
            ULONG   	 signals = 0;

            Forbid();
            ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->PropTask = 0;
            Permit();

            if (task) //small protection
            {
                Signal(task,PSIG_DIE);

                signals = Wait(SIGF_INTUITION);
            }

            SetSignal(oldSignals | (signals & ~SIGF_INTUITION),0xFFFFFFFF);
        }
    #endif

        SetGadgetType(o, GTYP_PROPGADGET);

        HandlePropSelectUp
        (
            G(o),
            msg->gpgi_GInfo->gi_Window,
            msg->gpgi_GInfo->gi_Requester,
            IntuitionBase
        );

        SetGadgetType(o, GTYP_CUSTOMGADGET);

        ((struct IntWindow *)(msg->gpgi_GInfo->gi_Window ))->specialflags &= ~SPFLAG_WANTBUFFER;

        /* Update PGA_Top. Final update */
        UpdateTop(cl, o, msg->gpgi_GInfo, TRUE);
    }
    return (0UL);
}

/****************************************************************************************/

AROS_UFH3S(IPTR, dispatch_propgclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    DEBUG_PROP(dprintf("dispatch_propgclass: Cl 0x%lx o 0x%lx msg 0x%lx\n",cl,o,msg));

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
            retval = (IPTR)set_propgclass(cl, o, (struct opSet *)msg);
            break;


	case OM_GET:
            retval = (IPTR)get_propgclass(cl, o, (struct opGet *)msg);
            break;

	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    DEBUG_PROP(dprintf("dispatch_propgclass: retval 0x%lx\n",retval));

    return (retval);

    AROS_USERFUNC_EXIT

}  /* dispatch_propgclass */

/****************************************************************************************/

#undef IntuitionBase

/****************************************************************************************/

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
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/****************************************************************************************/

