/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifndef __MORPHOS__

struct ICData;
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#include "icclass.h"

#endif /* !__MORPHOS__ */

#define DEBUG_GADGET(x) ;

struct GadgetData
{
    struct ExtGadget    EG;
    struct ICData       IC;
};

/****************************************************************************/

/* Some handy transparent base class object casting defines.
*/
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define GD(o) ((struct GadgetData *)o)
#define IM(o) ((struct Image *)o)

/****************************************************************************/

#undef IntuitionBase

/* set gadget attributes
*/
static ULONG set_gadgetclass(Class *cl, Object *o, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (APTR)cl->cl_UserData;
    struct TagItem  	 *tstate = msg->ops_AttrList;
    struct TagItem  	 *tag;
    IPTR    	    	  tidata;
    ULONG   	    	  retval = 0UL; /* set to non-zero to signal visual changes */

    while ( (tag = NextTagItem(&tstate)) )
    {
        tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case GA_Left:
        	EG(o)->LeftEdge = (WORD)tidata;
        	//EG(o)->Flags &= ~GFLG_RELRIGHT;
        	retval = 1UL;
        	break;

            case GA_Top:
        	EG(o)->TopEdge = (WORD)tidata;
        	//EG(o)->Flags &= ~GFLG_RELBOTTOM;
        	retval = 1UL;
        	break;

            case GA_Width:
        	EG(o)->Width = (WORD)tidata;
        	EG(o)->Flags &= ~GFLG_RELWIDTH;
        	retval = 1UL;
        	break;

            case GA_Height:
        	EG(o)->Height = (WORD)tidata;
        	EG(o)->Flags &= ~GFLG_RELHEIGHT;
        	retval = 1UL;
        	break;

            case GA_RelRight:
        	EG(o)->LeftEdge = (WORD)tidata;
        	EG(o)->Flags |= GFLG_RELRIGHT;
        	retval = 1UL;
        	break;

            case GA_RelBottom:
        	EG(o)->TopEdge = (WORD)tidata;
        	EG(o)->Flags |= GFLG_RELBOTTOM;
        	retval = 1UL;
        	break;

            case GA_RelWidth:
        	EG(o)->Width = (WORD)tidata;
        	EG(o)->Flags |= GFLG_RELWIDTH;
        	retval = 1UL;
        	break;

            case GA_RelHeight:
        	EG(o)->Height = (WORD)tidata;
        	EG(o)->Flags |= GFLG_RELHEIGHT;
        	retval = 1UL;
        	break;

            case GA_RelSpecial:
        	if (tidata)
        	{
                    EG(o)->Flags |= GFLG_RELSPECIAL;
        	}
		else
		{
                    EG(o)->Flags &= ~GFLG_RELSPECIAL;
        	}
        	retval = 1UL;
        	break;

            case GA_Bounds:
        	if (tidata)
        	{
                    EG(o)->BoundsLeftEdge = ((struct IBox *)tidata)->Left;
                    EG(o)->BoundsTopEdge  = ((struct IBox *)tidata)->Top;
                    EG(o)->BoundsWidth    = ((struct IBox *)tidata)->Width;
                    EG(o)->BoundsHeight   = ((struct IBox *)tidata)->Height;
                    EG(o)->MoreFlags |= GMORE_BOUNDS;
        	}
        	retval = 1UL;
        	break;

            case GA_GadgetHelp:
        	if (tidata)
        	{
                    EG(o)->MoreFlags |= GMORE_GADGETHELP;
        	}
		else
		{
                    EG(o)->MoreFlags &= ~GMORE_GADGETHELP;
        	}
        	retval = 1UL;
        	break;

            case GA_Next:
        	EG(o)->NextGadget = (struct ExtGadget *)tidata;
        	break;

            case GA_Previous:
        	if( (tidata != 0L) && (msg->MethodID == OM_NEW) )
        	{
                    EG(o)->NextGadget = ((struct ExtGadget *)tidata)->NextGadget;
                    ((struct ExtGadget *)tidata)->NextGadget = EG(o);
        	}
        	break;

            case GA_IntuiText:
        	EG(o)->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    EG(o)->Flags &= ~GFLG_LABELMASK;
                    EG(o)->Flags |= GFLG_LABELITEXT;
        	}
        	retval = 1UL;
        	break;

            case GA_Text:
        	EG(o)->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    EG(o)->Flags &= ~GFLG_LABELMASK;
                    EG(o)->Flags |= GFLG_LABELSTRING;
        	}
        	retval = 1UL;
        	break;

            case GA_LabelImage:
        	EG(o)->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    EG(o)->Flags &= ~GFLG_LABELMASK;
                    EG(o)->Flags |= GFLG_LABELIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_Image:
        	EG(o)->GadgetRender = (APTR)tidata;
        	if (tidata)
        	{
                    EG(o)->Flags |= GFLG_GADGIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_Border:
        	EG(o)->GadgetRender = (APTR)tidata;
        	if (tidata)
        	{
                    EG(o)->Flags &= ~GFLG_GADGIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_SelectRender:
        	EG(o)->SelectRender = (APTR)tidata;
        	{
                    EG(o)->Flags |= (GFLG_GADGIMAGE & GFLG_GADGHIMAGE);
        	}
        	retval = 1UL;
        	break;

            case GA_SpecialInfo:
        	EG(o)->SpecialInfo = (APTR)tidata;
        	break;

            case GA_GZZGadget:
        	if ( tidata != FALSE )
		{
                    EG(o)->GadgetType |= GTYP_GZZGADGET;
		}
        	else
		{
                    EG(o)->GadgetType &= ~GTYP_GZZGADGET;
		}
        	break;

            case GA_SysGadget:
        	if ( tidata != FALSE )
		{
                    EG(o)->GadgetType |= GTYP_SYSGADGET;
		}
        	else
		{
                    EG(o)->GadgetType &= ~GTYP_SYSGADGET;
		}
        	break;

            case GA_Selected:
        	if ( tidata != FALSE )
		{
                    EG(o)->Flags |= GFLG_SELECTED;
		}
        	else
		{
                    EG(o)->Flags &= ~GFLG_SELECTED;
		}
        	retval = 1UL;
        	break;

            case GA_Disabled:
        	if ( tidata != FALSE )
		{
                    EG(o)->Flags |= GFLG_DISABLED;
		}
        	else
		{
                    EG(o)->Flags &= ~GFLG_DISABLED;
		}
        	retval = 1UL;
        	break;

            case GA_EndGadget:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_ENDGADGET;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_ENDGADGET;
		}
        	break;

            case GA_Immediate:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_IMMEDIATE;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_IMMEDIATE;
		}
        	break;

            case GA_RelVerify:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_RELVERIFY;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_RELVERIFY;
		}
        	break;

            case GA_FollowMouse:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_FOLLOWMOUSE;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_FOLLOWMOUSE;
		}
        	break;

            case GA_RightBorder:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_RIGHTBORDER;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_RIGHTBORDER;
		}
        	break;

            case GA_LeftBorder:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_LEFTBORDER;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_LEFTBORDER;
		}
        	break;

            case GA_TopBorder:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_TOPBORDER;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_TOPBORDER;
		}
        	break;

            case GA_BottomBorder:
        	if ( tidata != FALSE )
		{
                    EG(o)->Activation |= GACT_BOTTOMBORDER;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_BOTTOMBORDER;
		}
        	break;

            case GA_ToggleSelect:
        	if (tidata)
		{
                    EG(o)->Activation |= GACT_TOGGLESELECT;
		}
        	else
		{
                    EG(o)->Activation &= ~GACT_TOGGLESELECT;
		}
        	break;

            case GA_TabCycle:
        	if (tidata)
		{
                    EG(o)->Flags |= GFLG_TABCYCLE;
		}
        	else
		{
                    EG(o)->Flags &= ~GFLG_TABCYCLE;
		}
        	break;

            case GA_Highlight:
        	EG(o)->Flags &= ~GFLG_GADGHIGHBITS;
        	EG(o)->Flags |= tidata & GFLG_GADGHIGHBITS;
        	break;

            case GA_SysGType:
        	EG(o)->GadgetType &= ~GTYP_SYSTYPEMASK;
        	EG(o)->GadgetType |= tidata & GTYP_SYSTYPEMASK;
        	break;

            case GA_ID:
        	/* GA_ID should NOT be set if this is a OM_UPDATE.
        	** This is because gadgets should send their GA_ID
        	** when doing a OM_NOTIFY, so that the receiver
        	** might see who sent the message.
        	** But we surely don't want to change the GA_ID
        	** of the reciver to that of the sender.
        	*/
        	if (msg->MethodID != OM_UPDATE)
		{
                    EG(o)->GadgetID = tidata;
		}
        	break;

            case GA_UserData:
        	EG(o)->UserData = (APTR)tidata;
        	DEBUG_GADGET(dprintf("set_gadgetclass: UserData 0x%lx\n",tidata));
        	break;

            case ICA_TARGET:
        	GD(o)->IC.ic_Target = (Object *)tidata;
        	break;

            case ICA_MAP:
        	GD(o)->IC.ic_Mapping = (struct TagItem *)tidata;
        	break;

        } /* switch tag */

    } /* while NextTagItem */

#if 0
    /* This seems to be wrong here. Instead buttongclass is where
       something like this happens, so look there (stegerg) */

    if ((msg->MethodID == OM_NEW) &&
            (EG(o)->Flags & GFLG_GADGIMAGE) &&
            (EG(o)->GadgetRender != NULL))
    {
        if (EG(o)->Width  == 0) EG(o)->Width  = IM(EG(o)->GadgetRender)->Width;
        if (EG(o)->Height == 0) EG(o)->Height = IM(EG(o)->GadgetRender)->Height;
    }
#endif 

    return retval;
}


/* get gadget attributes - gadgetclass really has no gettable
 * attributes, but we will implement some useful ones anyway. ;0
 */
static ULONG get_gadgetclass(Class *cl, Object *o, struct opGet *msg)
{
    ULONG retval = 1UL;

    switch (msg->opg_AttrID)
    {
	case GA_Left:
	case GA_RelRight:
            *msg->opg_Storage = (IPTR) EG(o)->LeftEdge;
            break;

	case GA_Top:
	case GA_RelBottom:
            *msg->opg_Storage = (IPTR) EG(o)->TopEdge;
            break;

	case GA_Width:
	case GA_RelWidth:
            *msg->opg_Storage = (IPTR) EG(o)->Width;
            break;

	case GA_Height:
	case GA_RelHeight:
            *msg->opg_Storage = (IPTR) EG(o)->Height;
            break;

	case GA_Selected:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_SELECTED) != 0);
            break;

	case GA_Disabled:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_DISABLED) != 0);
            break;

	case GA_ID:
            *msg->opg_Storage = (IPTR)EG(o)->GadgetID;
            break;

	case GA_UserData:
            *msg->opg_Storage = (IPTR)EG(o)->UserData;
            break;

	case GA_RelSpecial:
            *msg->opg_Storage = (IPTR)(EG(o)->Flags & GFLG_RELSPECIAL) ? TRUE : FALSE;
            break;

	case GA_GadgetHelp:
            *msg->opg_Storage = (IPTR)(EG(o)->MoreFlags & GMORE_GADGETHELP) ? TRUE : FALSE;
            break;

	case GA_Next:
            *msg->opg_Storage = (IPTR)EG(o)->NextGadget;
            break;

	case GA_IntuiText:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_LABELITEXT) ? EG(o)->GadgetText : 0);
            break;

	case GA_Text:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_LABELSTRING) ? EG(o)->GadgetText : 0);
            break;

	case GA_LabelImage:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_LABELIMAGE) ? EG(o)->GadgetText : 0);
            break;

	case GA_Image:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_GADGIMAGE) ? EG(o)->GadgetRender : 0);
            break;

	case GA_Border:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_GADGIMAGE) ? 0 : EG(o)->GadgetRender);
            break;

	case GA_SelectRender:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_GADGHIMAGE) ? EG(o)->SelectRender : 0);
            break;

	case GA_SpecialInfo:
            *msg->opg_Storage = (IPTR)EG(o)->SpecialInfo;
            break;

	case GA_GZZGadget:
            *msg->opg_Storage = (IPTR)((EG(o)->GadgetType & GTYP_GZZGADGET) ? TRUE : FALSE);
            break;

	case GA_SysGadget:
            *msg->opg_Storage = (IPTR)((EG(o)->GadgetType & GTYP_SYSGADGET) ? TRUE : FALSE);
            break;

	case GA_EndGadget:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_ENDGADGET) ? TRUE : FALSE);
            break;

	case GA_Immediate:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_IMMEDIATE) ? TRUE : FALSE);
            break;

	case GA_RelVerify:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_RELVERIFY) ? TRUE : FALSE);
            break;

	case GA_FollowMouse:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_FOLLOWMOUSE) ? TRUE : FALSE);
            break;

	case GA_RightBorder:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_RIGHTBORDER) ? TRUE : FALSE);
            break;

	case GA_LeftBorder:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_LEFTBORDER) ? TRUE : FALSE);
            break;

	case GA_TopBorder:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_TOPBORDER) ? TRUE : FALSE);
            break;

	case GA_BottomBorder:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_BOTTOMBORDER) ? TRUE : FALSE);
            break;

	case GA_ToggleSelect:
            *msg->opg_Storage = (IPTR)((EG(o)->Activation & GACT_TOGGLESELECT) ? TRUE : FALSE);
            break;

	case GA_TabCycle:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_TABCYCLE) ? TRUE : FALSE);
            break;

	case GA_Highlight:
            *msg->opg_Storage = (IPTR)((EG(o)->Flags & GFLG_GADGHIGHBITS) ? TRUE : FALSE);
            break;

	case GA_SysGType:
            *msg->opg_Storage = (IPTR)(EG(o)->GadgetType & GTYP_SYSTYPEMASK);
            break;

	case ICA_TARGET:
            *msg->opg_Storage = (IPTR)GD(o)->IC.ic_Target;
            break;

	case ICA_MAP:
            *msg->opg_Storage = (IPTR)GD(o)->IC.ic_Mapping;
            break;

	case GA_Bounds:
            if (msg->opg_Storage)
            {
        	struct IBox *ibox = (struct IBox *)msg->opg_Storage;

        	ibox->Left = EG(o)->BoundsLeftEdge;
        	ibox->Top = EG(o)->BoundsTopEdge;
        	ibox->Width = EG(o)->BoundsWidth;
        	ibox->Height = EG(o)->BoundsHeight;
            }
            break;


	default:
    	#if 0
            /* DONT DO THIS!! For example BGUI propclass relies on this not happening!! */

            *msg->opg_Storage = (IPTR)NULL;
            */
    	#endif
            retval = 0UL;
            break;

    } /* switch attrid) */

    return(retval);
}

/* test if we should try to activate this gadget...
 */
static ULONG hittest_gadgetclass(Class *cl, Object *o, struct gpHitTest *gpht)
{
    return GMR_GADGETHIT;
}


/* gadgetclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_gadgetclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    struct IntuitionBase *IntuitionBase = (APTR)cl->cl_UserData;
    IPTR    	    	  retval = 0UL;

    DEBUG_GADGET(dprintf("dispatch_gadgetclass: Cl 0x%lx o 0x%lx msg 0x%lx\n",cl,o,msg));

    switch(msg->MethodID)
    {
	case GM_RENDER:
            retval = 1UL;
            break;

	case GM_LAYOUT:
	case GM_DOMAIN:
	case GM_GOINACTIVE:
            /* our subclasses handle these methods */
            break;

	case GM_GOACTIVE:
	case GM_HANDLEINPUT:
            /* our subclasses handle these methods */
            /* but still return GMR_NOREUSE, for those that don't. */
            retval = GMR_NOREUSE;
            break;

	case GM_HITTEST:
            retval = (IPTR)hittest_gadgetclass(cl, o, (struct gpHitTest *)msg);
            break;

	case GM_HELPTEST:
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: GM_HELPTEST\n"));
            retval = GMR_HELPHIT;
            break;

	case OM_NEW:
            retval = DoSuperMethodA(cl, o, msg);

            if (retval)
            {
        	/* set some defaults */
        	/*
        	  The instance object is cleared memory!
        	  memset (EG(retval), 0, sizeof(struct GadgetData));
        	*/
        	EG(retval)->Flags         = GFLG_EXTENDED;
        	EG(retval)->GadgetType    = GTYP_CUSTOMGADGET;
        	EG(retval)->MutualExclude = (LONG)&((Class *)o)->cl_Dispatcher;

        	/* Handle our special tags - overrides defaults */
        	set_gadgetclass(cl, (Object*)retval, (struct opSet *)msg);
            }
            break;

	case OM_SET:
	case OM_UPDATE:
            retval = DoSuperMethodA(cl, o, msg);
            retval += (IPTR)set_gadgetclass(cl, o, (struct opSet *)msg);
            break;

	case OM_GET:
            retval = (IPTR)get_gadgetclass(cl, o, (struct opGet *)msg);
            break;

	case OM_NOTIFY:
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY\n"));
            DoNotify(cl, o, &(GD(o)->IC), (struct opUpdate *)msg);
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY done\n"));
            break;

	case OM_DISPOSE:
            FreeICData((struct ICData *)(&(GD(o)->IC)));
            retval = DoSuperMethodA(cl, o, (Msg)msg);
            break;

	case ICM_SETLOOP:
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_SETLOOP\n"));
            GD(o)->IC.ic_LoopCounter += 1UL;
            break;

	case ICM_CLEARLOOP:
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CLEARLOOP\n"));
            GD(o)->IC.ic_LoopCounter -= 1UL;
            break;

	case ICM_CHECKLOOP:
            DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CHECKLOOP\n"));
            retval = GD(o)->IC.ic_LoopCounter;
            break;

	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    DEBUG_GADGET(dprintf("dispatch_gadgetclass: retval 0x%lx\n",retval));

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_gadgetclass */


#undef IntuitionBase

/****************************************************************************/

/* Initialize our image class. */
struct IClass *InitGadgetClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the gadgetclass...
     */
    if ((cl = MakeClass(GADGETCLASS, ROOTCLASS, NULL, sizeof(struct GadgetData), 0)))
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_gadgetclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

