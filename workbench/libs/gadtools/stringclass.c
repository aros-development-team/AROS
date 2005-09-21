/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal GadTools string class.
    Lang: English
*/
 

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h> /* memset() */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define GadToolsBase ((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

IPTR GTString__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 	*tag, tags[] =
    {
    	{TAG_IGNORE	, 0UL	},  /* 0 STRINGA_TextVal  */
	{TAG_IGNORE	, 0UL	},  /* 1 STRINGA_LongVal  */
    	{TAG_IGNORE	, 0UL	},  /* 2 STRINGA_MaxChars */
    	{TAG_IGNORE	, 0UL	},  /* 3 STRINGA_EditHook */
    	{TAG_MORE	, 0UL	}
    };
    const struct TagItem *tstate = msg->ops_AttrList;
    
    LONG 		labelplace = GV_LabelPlace_Left;
    struct DrawInfo 	*dri = NULL;
    struct TextAttr 	*tattr = NULL;
    LONG 		gadgetkind = STRING_KIND;
    
    struct Gadget	*retval = NULL;

    EnterFunc(bug("String::SetNew()\n"));
    
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
	    case GTA_GadgetKind:
		gadgetkind = tidata;
	        break;
	    
    	    case GTST_String:
	    	tags[0].ti_Tag = STRINGA_TextVal;		
	    	tags[0].ti_Data = tidata;
		break;
		
    	    case GTIN_Number:
	    	tags[1].ti_Tag = STRINGA_LongVal;
	    	tags[1].ti_Data = tidata;
		break;
    	    
    	    /* Another weird inconsistency of AmigaOS GUI objects:
    	    ** For intuition and strgclass gadgets, MaxChars includes trailing
    	    ** zero, but this is NOT true for gadtools string gadgets
    	    */
    	    case GTIN_MaxChars:
    	    case GTST_MaxChars:
	    	tags[2].ti_Tag = STRINGA_MaxChars;
	    	tags[2].ti_Data = ((WORD)tidata) + 1;
		break;
		
/*    	    case GTIN_EditHook:  Duplicate case value */
    	    case GTST_EditHook:
	    	tags[3].ti_Tag = STRINGA_EditHook;
	    	tags[3].ti_Data = tidata;
		break;
    	    
    	    case GA_LabelPlace:
    	    	labelplace = tidata;
    	    	break;
                
    	    case GA_DrawInfo:
    	    	dri = (struct DrawInfo *)tidata;
    	    	break;
    	    case GA_TextAttr:
    	    	tattr = (struct TextAttr *)tidata;
    	    	break;

    	}
    }
    
    tags[4].ti_Data = (IPTR)msg->ops_AttrList;

    retval = (struct Gadget *)DoSuperMethod(cl, o, msg->MethodID, (IPTR) tags, (IPTR) msg->ops_GInfo);
   
    D(bug("Returned from supermethod: %p\n", retval));
    
    if ((msg->MethodID == OM_NEW) && (retval != NULL))
    {
    	struct StringData *data = INST_DATA(cl, retval);
    	struct TagItem fitags[] =
    	{
	    {IA_Width		, 0UL		},
	    {IA_Height		, 0UL		},
	    {IA_Resolution	, 0UL		},
	    {IA_FrameType	, FRAME_RIDGE	},
	    {IA_EdgesOnly	, TRUE		},
	    {TAG_DONE		, 0UL		}
    	};
    	
	fitags[0].ti_Data = retval->Width;
    	fitags[1].ti_Data = retval->Height;
    	fitags[2].ti_Data = (dri->dri_Resolution.X << 16) + dri->dri_Resolution.Y;
    	
    	data->labelplace = labelplace;
    	data->frame = NULL;
    	data->font  = NULL;
	data->gadgetkind = gadgetkind;
	
    	D(bug("Creating frame image"));
    	
    	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);

    	D(bug("Created frame image: %p", data->frame));
    	if (!data->frame)
    	{
    	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
    	    
    	    retval = (IPTR)NULL;
    	}

    	if (tattr)
    	{
    	    data->font = OpenFont(tattr);

    	    if (data->font)
    	    {
    	    	struct TagItem sftags[] = {{STRINGA_Font, (IPTR)NULL}, {TAG_DONE, }};
    	    	
    	    	sftags[0].ti_Data = (IPTR)data->font;

    	    	DoSuperMethod(cl, (Object *)retval, OM_SET, (IPTR) sftags, NULL);
    	    }
    	}
    }
    
    ReturnPtr ("String::SetNew", IPTR, (IPTR)retval);

}

/**********************************************************************************************/

IPTR GTString__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct StringData	*data = INST_DATA(cl, o);
    struct opGet 	cloned_msg = *msg;
    IPTR		retval = 0;
    
    switch(cloned_msg.opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = data->gadgetkind;
	    retval = 1UL;
	    break;

	case GTST_String:
	    cloned_msg.opg_AttrID = STRINGA_TextVal;
	    break;

	case GTIN_Number:
	    cloned_msg.opg_AttrID = STRINGA_LongVal;
	    break;

	default:
	    break;
    }

    if (!retval) retval = DoSuperMethodA(cl, o, (Msg)&cloned_msg);
    
    return retval;
}

/**********************************************************************************************/

IPTR GTString__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    IPTR retval;
    
    EnterFunc(bug("String::Render()\n"));
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    D(bug("Superclass render OK\n"));
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
    	struct StringData *data = INST_DATA(cl, g);
    	
	WORD x, y;
	    
	struct TagItem itags[] =
	{
	    {IA_Width,	0L},
	    {IA_Height,	0L},
	    {TAG_DONE,}
	};
	
	D(bug("Full redraw\n"));

	/* center image position, we assume image top and left is 0 */
	itags[0].ti_Data = g->Width + BORDERSTRINGSPACINGX * 2;
	itags[1].ti_Data = g->Height + BORDERSTRINGSPACINGY * 2;

	SetAttrsA((Object *)data->frame, itags);
	
	x = g->LeftEdge - BORDERSTRINGSPACINGX; 
	y = g->TopEdge - BORDERSTRINGSPACINGY;
	    
	DrawImageState(msg->gpr_RPort,
		(struct Image *)data->frame,
		x, y,
		IDS_NORMAL,
		msg->gpr_GInfo->gi_DrInfo);
   
   	/* render label */
   	renderlabel(GadToolsBase, g, msg->gpr_RPort, data->labelplace);
   	
    } /* if (whole gadget should be redrawn) */
    
    ReturnInt ("String::Render", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTString__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct StringData *data = INST_DATA(cl, o);
    	    
    if (data->frame) DisposeObject(data->frame);
    if (data->font) CloseFont(data->font);

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/**********************************************************************************************/

