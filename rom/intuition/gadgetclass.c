/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#endif /* !__MORPHOS__ */

#define DEBUG_GADGET(x) ;

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase ((struct IntuitionBase *)cl->cl_UserData)

/* set gadget attributes
*/
static ULONG set_gadgetclass(Class *cl, struct ExtGadget *eg, struct opSet *msg)
{
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
        	eg->LeftEdge = (WORD)tidata;
        	//eg->Flags &= ~GFLG_RELRIGHT;
        	retval = 1UL;
        	break;

            case GA_Top:
        	eg->TopEdge = (WORD)tidata;
        	//eg->Flags &= ~GFLG_RELBOTTOM;
        	retval = 1UL;
        	break;

            case GA_Width:
        	eg->Width = (WORD)tidata;
        	eg->Flags &= ~GFLG_RELWIDTH;
        	retval = 1UL;
        	break;

            case GA_Height:
        	eg->Height = (WORD)tidata;
        	eg->Flags &= ~GFLG_RELHEIGHT;
        	retval = 1UL;
        	break;

            case GA_RelRight:
        	eg->LeftEdge = (WORD)tidata;
        	eg->Flags |= GFLG_RELRIGHT;
        	retval = 1UL;
        	break;

            case GA_RelBottom:
        	eg->TopEdge = (WORD)tidata;
        	eg->Flags |= GFLG_RELBOTTOM;
        	retval = 1UL;
        	break;

            case GA_RelWidth:
        	eg->Width = (WORD)tidata;
        	eg->Flags |= GFLG_RELWIDTH;
        	retval = 1UL;
        	break;

            case GA_RelHeight:
        	eg->Height = (WORD)tidata;
        	eg->Flags |= GFLG_RELHEIGHT;
        	retval = 1UL;
        	break;

            case GA_RelSpecial:
        	if (tidata)
        	{
                    eg->Flags |= GFLG_RELSPECIAL;
        	}
		else
		{
                    eg->Flags &= ~GFLG_RELSPECIAL;
        	}
        	retval = 1UL;
        	break;

            case GA_Bounds:
        	if (tidata)
        	{
                    eg->BoundsLeftEdge = ((struct IBox *)tidata)->Left;
                    eg->BoundsTopEdge  = ((struct IBox *)tidata)->Top;
                    eg->BoundsWidth    = ((struct IBox *)tidata)->Width;
                    eg->BoundsHeight   = ((struct IBox *)tidata)->Height;
                    eg->MoreFlags |= GMORE_BOUNDS;
        	}
        	retval = 1UL;
        	break;

            case GA_GadgetHelp:
        	if (tidata)
        	{
                    eg->MoreFlags |= GMORE_GADGETHELP;
        	}
		else
		{
                    eg->MoreFlags &= ~GMORE_GADGETHELP;
        	}
        	retval = 1UL;
        	break;

            case GA_Next:
        	eg->NextGadget = (struct ExtGadget *)tidata;
        	break;

            case GA_Previous:
        	if( (tidata != 0L) && (msg->MethodID == OM_NEW) )
        	{
                    eg->NextGadget = ((struct ExtGadget *)tidata)->NextGadget;
                    ((struct ExtGadget *)tidata)->NextGadget = eg;
        	}
        	break;

            case GA_IntuiText:
        	eg->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELITEXT;
        	}
        	retval = 1UL;
        	break;

            case GA_Text:
        	eg->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELSTRING;
        	}
        	retval = 1UL;
        	break;

            case GA_LabelImage:
        	eg->GadgetText = (struct IntuiText *)tidata;
        	if (tidata)
        	{
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_Image:
        	eg->GadgetRender = (APTR)tidata;
        	if (tidata)
        	{
                    eg->Flags |= GFLG_GADGIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_Border:
        	eg->GadgetRender = (APTR)tidata;
        	if (tidata)
        	{
                    eg->Flags &= ~GFLG_GADGIMAGE;
        	}
        	retval = 1UL;
        	break;

            case GA_SelectRender:
        	eg->SelectRender = (APTR)tidata;
        	{
                    eg->Flags |= (GFLG_GADGIMAGE & GFLG_GADGHIMAGE);
        	}
        	retval = 1UL;
        	break;

            case GA_SpecialInfo:
        	eg->SpecialInfo = (APTR)tidata;
        	break;

            case GA_GZZGadget:
        	if ( tidata != FALSE )
		{
                    eg->GadgetType |= GTYP_GZZGADGET;
		}
        	else
		{
                    eg->GadgetType &= ~GTYP_GZZGADGET;
		}
        	break;

            case GA_SysGadget:
        	if ( tidata != FALSE )
		{
                    eg->GadgetType |= GTYP_SYSGADGET;
		}
        	else
		{
                    eg->GadgetType &= ~GTYP_SYSGADGET;
		}
        	break;

            case GA_Selected:
        	if ( tidata != FALSE )
		{
                    eg->Flags |= GFLG_SELECTED;
		}
        	else
		{
                    eg->Flags &= ~GFLG_SELECTED;
		}
        	retval = 1UL;
        	break;

            case GA_Disabled:
        	if ( tidata != FALSE )
		{
                    eg->Flags |= GFLG_DISABLED;
		}
        	else
		{
                    eg->Flags &= ~GFLG_DISABLED;
		}
        	retval = 1UL;
        	break;

            case GA_EndGadget:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_ENDGADGET;
		}
        	else
		{
                    eg->Activation &= ~GACT_ENDGADGET;
		}
        	break;

            case GA_Immediate:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_IMMEDIATE;
		}
        	else
		{
                    eg->Activation &= ~GACT_IMMEDIATE;
		}
        	break;

            case GA_RelVerify:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_RELVERIFY;
		}
        	else
		{
                    eg->Activation &= ~GACT_RELVERIFY;
		}
        	break;

            case GA_FollowMouse:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_FOLLOWMOUSE;
		}
        	else
		{
                    eg->Activation &= ~GACT_FOLLOWMOUSE;
		}
        	break;

            case GA_RightBorder:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_RIGHTBORDER;
		}
        	else
		{
                    eg->Activation &= ~GACT_RIGHTBORDER;
		}
        	break;

            case GA_LeftBorder:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_LEFTBORDER;
		}
        	else
		{
                    eg->Activation &= ~GACT_LEFTBORDER;
		}
        	break;

            case GA_TopBorder:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_TOPBORDER;
		}
        	else
		{
                    eg->Activation &= ~GACT_TOPBORDER;
		}
        	break;

            case GA_BottomBorder:
        	if ( tidata != FALSE )
		{
                    eg->Activation |= GACT_BOTTOMBORDER;
		}
        	else
		{
                    eg->Activation &= ~GACT_BOTTOMBORDER;
		}
        	break;

            case GA_ToggleSelect:
        	if (tidata)
		{
                    eg->Activation |= GACT_TOGGLESELECT;
		}
        	else
		{
                    eg->Activation &= ~GACT_TOGGLESELECT;
		}
        	break;

            case GA_TabCycle:
        	if (tidata)
		{
                    eg->Flags |= GFLG_TABCYCLE;
		}
        	else
		{
                    eg->Flags &= ~GFLG_TABCYCLE;
		}
        	break;

            case GA_Highlight:
        	eg->Flags &= ~GFLG_GADGHIGHBITS;
        	eg->Flags |= tidata & GFLG_GADGHIGHBITS;
        	break;

            case GA_SysGType:
        	eg->GadgetType &= ~GTYP_SYSTYPEMASK;
        	eg->GadgetType |= tidata & GTYP_SYSTYPEMASK;
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
                    eg->GadgetID = tidata;
		}
        	break;

            case GA_UserData:
        	eg->UserData = (APTR)tidata;
        	DEBUG_GADGET(dprintf("set_gadgetclass: UserData 0x%lx\n",tidata));
        	break;

            case ICA_TARGET:
        	((struct GadgetData *)eg)->IC.ic_Target = (Object *)tidata;
        	break;

            case ICA_MAP:
        	((struct GadgetData *)eg)->IC.ic_Mapping = (struct TagItem *)tidata;
        	break;

        } /* switch tag */

    } /* while NextTagItem */

#if 0
    /* This seems to be wrong here. Instead buttongclass is where
       something like this happens, so look there (stegerg) */

    if ((msg->MethodID == OM_NEW) &&
            (eg->Flags & GFLG_GADGIMAGE) &&
            (eg->GadgetRender != NULL))
    {
        if (eg->Width  == 0) eg->Width  = ((struct Image *)eg->GadgetRender)->Width;
        if (eg->Height == 0) eg->Height = ((struct Image *)eg->GadgetRender)->Height;
    }
#endif 

    return retval;
}


IPTR GadgetClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct ExtGadget *eg = (struct ExtGadget *)DoSuperMethodA(cl, o, (Msg)msg);

    if (eg)
    {
	/* set some defaults */
	/*
	 * The instance object is cleared memory!
	 * memset (eg, 0, sizeof(struct GadgetData));
	 */
	eg->Flags         = GFLG_EXTENDED;
	eg->GadgetType    = GTYP_CUSTOMGADGET;
	eg->MutualExclude = (LONG)&((Class *)o)->cl_Dispatcher;

	/* Handle our special tags - overrides defaults */
	set_gadgetclass(cl, eg, msg);
    }
    
    return (IPTR)eg;
}

IPTR GadgetClass__OM_SET(Class *cl, struct ExtGadget *eg, struct opSet *msg)
{
    return DoSuperMethodA(cl, (Object *)eg, (Msg)msg) + set_gadgetclass(cl, eg, (struct opSet *)msg);
}

IPTR GadgetClass__OM_NOTIFY(Class *cl, struct GadgetData *gd, struct opUpdate *msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY\n"));
    DoNotify(cl, (Object *)gd, &(gd->IC), msg);
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY done\n"));

    return (IPTR)0;
}

IPTR GadgetClass__OM_DISPOSE(Class *cl, struct GadgetData *gd, Msg msg)
{
    FreeICData((struct ICData *)&gd->IC);
    return DoSuperMethodA(cl, (Object *)gd, (Msg)msg);
}

/* get gadget attributes - gadgetclass really has no gettable
 * attributes, but we will implement some useful ones anyway. ;0
 */
IPTR GadgetClass__OM_GET(Class *cl, struct ExtGadget *eg, struct opGet *msg)
{
    ULONG retval = 1UL;

    switch (msg->opg_AttrID)
    {
	case GA_Left:
	case GA_RelRight:
            *msg->opg_Storage = (IPTR) eg->LeftEdge;
            break;

	case GA_Top:
	case GA_RelBottom:
            *msg->opg_Storage = (IPTR) eg->TopEdge;
            break;

	case GA_Width:
	case GA_RelWidth:
            *msg->opg_Storage = (IPTR) eg->Width;
            break;

	case GA_Height:
	case GA_RelHeight:
            *msg->opg_Storage = (IPTR) eg->Height;
            break;

	case GA_Selected:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_SELECTED) != 0);
            break;

	case GA_Disabled:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_DISABLED) != 0);
            break;

	case GA_ID:
            *msg->opg_Storage = (IPTR)eg->GadgetID;
            break;

	case GA_UserData:
            *msg->opg_Storage = (IPTR)eg->UserData;
            break;

	case GA_RelSpecial:
            *msg->opg_Storage = (IPTR)(eg->Flags & GFLG_RELSPECIAL) ? TRUE : FALSE;
            break;

	case GA_GadgetHelp:
            *msg->opg_Storage = (IPTR)(eg->MoreFlags & GMORE_GADGETHELP) ? TRUE : FALSE;
            break;

	case GA_Next:
            *msg->opg_Storage = (IPTR)eg->NextGadget;
            break;

	case GA_IntuiText:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_LABELITEXT) ? eg->GadgetText : 0);
            break;

	case GA_Text:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_LABELSTRING) ? eg->GadgetText : 0);
            break;

	case GA_LabelImage:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_LABELIMAGE) ? eg->GadgetText : 0);
            break;

	case GA_Image:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_GADGIMAGE) ? eg->GadgetRender : 0);
            break;

	case GA_Border:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_GADGIMAGE) ? 0 : eg->GadgetRender);
            break;

	case GA_SelectRender:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_GADGHIMAGE) ? eg->SelectRender : 0);
            break;

	case GA_SpecialInfo:
            *msg->opg_Storage = (IPTR)eg->SpecialInfo;
            break;

	case GA_GZZGadget:
            *msg->opg_Storage = (IPTR)((eg->GadgetType & GTYP_GZZGADGET) ? TRUE : FALSE);
            break;

	case GA_SysGadget:
            *msg->opg_Storage = (IPTR)((eg->GadgetType & GTYP_SYSGADGET) ? TRUE : FALSE);
            break;

	case GA_EndGadget:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_ENDGADGET) ? TRUE : FALSE);
            break;

	case GA_Immediate:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_IMMEDIATE) ? TRUE : FALSE);
            break;

	case GA_RelVerify:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_RELVERIFY) ? TRUE : FALSE);
            break;

	case GA_FollowMouse:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_FOLLOWMOUSE) ? TRUE : FALSE);
            break;

	case GA_RightBorder:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_RIGHTBORDER) ? TRUE : FALSE);
            break;

	case GA_LeftBorder:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_LEFTBORDER) ? TRUE : FALSE);
            break;

	case GA_TopBorder:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_TOPBORDER) ? TRUE : FALSE);
            break;

	case GA_BottomBorder:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_BOTTOMBORDER) ? TRUE : FALSE);
            break;

	case GA_ToggleSelect:
            *msg->opg_Storage = (IPTR)((eg->Activation & GACT_TOGGLESELECT) ? TRUE : FALSE);
            break;

	case GA_TabCycle:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_TABCYCLE) ? TRUE : FALSE);
            break;

	case GA_Highlight:
            *msg->opg_Storage = (IPTR)((eg->Flags & GFLG_GADGHIGHBITS) ? TRUE : FALSE);
            break;

	case GA_SysGType:
            *msg->opg_Storage = (IPTR)(eg->GadgetType & GTYP_SYSTYPEMASK);
            break;

	case ICA_TARGET:
            *msg->opg_Storage = (IPTR)((struct GadgetData *)eg)->IC.ic_Target;
            break;

	case ICA_MAP:
            *msg->opg_Storage = (IPTR)((struct GadgetData *)eg)->IC.ic_Mapping;
            break;

	case GA_Bounds:
            if (msg->opg_Storage)
            {
        	struct IBox *ibox = (struct IBox *)msg->opg_Storage;

        	ibox->Left = eg->BoundsLeftEdge;
        	ibox->Top = eg->BoundsTopEdge;
        	ibox->Width = eg->BoundsWidth;
        	ibox->Height = eg->BoundsHeight;
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
IPTR GadgetClass__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *gpht)
{
    return (IPTR)GMR_GADGETHIT;
}

/* Methods follows that only need to return a value because they should be handled by
 * the subclasses
 */
IPTR GadgetClass__One(Class *cl, Object *o, Msg msg)
{
     return (IPTR)1;
}

IPTR GadgetClass__Zero(Class *cl, Object *o, Msg msg)
{
    return (IPTR)0;
}

IPTR GadgetClass_NoReuse(Class *cl, Object *o, Msg msg)
{
    return (IPTR)GMR_NOREUSE;
}

IPTR GadgetClass__GM_HELPTEST(Class *cl, Object *o, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: GM_HELPTEST\n"));
    return (IPTR)GMR_HELPHIT;
}

IPTR GadgetClass__ICM_SETLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_SETLOOP\n"));
    gd->IC.ic_LoopCounter += 1UL;
    
    return (IPTR)0;
}

IPTR GadgetClass__ICM_CLEARLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CLEARLOOP\n"));
    gd->IC.ic_LoopCounter -= 1UL;

    return (IPTR)0;
}

IPTR GadgetClass__ICM_CHECKLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CHECKLOOP\n"));
    return (IPTR)gd->IC.ic_LoopCounter;
}
