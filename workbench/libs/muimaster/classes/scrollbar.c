/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

struct MUI_ScrollbarData
{
    Object *prop;
    Object *up_arrow;
    Object *down_arrow;
    int sb_pos;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Scrollbar_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ScrollbarData *data;
    //struct TagItem *tags,*tag;
    int horiz = GetTagData(MUIA_Group_Horiz, 0, msg->ops_AttrList);
    int usewinborder = GetTagData(MUIA_Prop_UseWinBorder, 0, msg->ops_AttrList);
    int sb_pos = GetTagData(MUIA_Scrollbar_Type, 0, msg->ops_AttrList);

    Object *prop = MUI_NewObject(MUIC_Prop, MUIA_Prop_Horiz, horiz, TAG_MORE, msg->ops_AttrList);

    obj = (Object *)DoSuperNew(cl, obj,
			       MUIA_Group_Spacing, 0,
			       MUIA_Background, MUII_BACKGROUND,
			       TAG_MORE, msg->ops_AttrList);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    data->prop = prop;
    data->sb_pos = sb_pos;

    D(bug("Scrollbar_New %p usewinborder=%d\n", obj, usewinborder));

    if (!usewinborder)
    {
	data->up_arrow = ImageObject,
	    MUIA_Background, MUII_ButtonBack,
	    horiz ? MUIA_Image_FreeVert : MUIA_Image_FreeHoriz, TRUE,
	    MUIA_Weight, 0,
	    ImageButtonFrame,
	    MUIA_InputMode, MUIV_InputMode_RelVerify,
	    MUIA_Image_Spec, horiz ? MUII_ArrowLeft : MUII_ArrowUp,
	    End;
	if (data->up_arrow)
	{
	    DoMethod(data->up_arrow, MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
		     (IPTR)prop, 2, MUIM_Prop_Decrease, 1);
        }

	data->down_arrow = ImageObject,
	    MUIA_Background, MUII_ButtonBack,
	    horiz ? MUIA_Image_FreeVert : MUIA_Image_FreeHoriz, TRUE,
	    MUIA_Weight, 0,
	    ImageButtonFrame,
	    MUIA_InputMode, MUIV_InputMode_RelVerify,
	    MUIA_Image_Spec, horiz ? MUII_ArrowRight : MUII_ArrowDown,
	    End;
	if (data->down_arrow)
	{
	    DoMethod(data->down_arrow, MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
		     (IPTR)prop, 2, MUIM_Prop_Increase, 1);
        }

	switch (sb_pos)
	{
	    case MUIV_Scrollbar_Type_Default:
	    case MUIV_Scrollbar_Type_Top:
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->prop);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->up_arrow);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->down_arrow);
		break;
	    case MUIV_Scrollbar_Type_Bottom:
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->up_arrow);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->down_arrow);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->prop);
		break;
	    case MUIV_Scrollbar_Type_Sym:
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->up_arrow);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->prop);
		DoMethod(obj, OM_ADDMEMBER, (IPTR)data->down_arrow);
		break;
	}
    }
    else
    {
	_flags(obj) |= MADF_BORDERGADGET;
	    DoMethod(obj, OM_ADDMEMBER, (IPTR)data->prop);
    }

    return (ULONG)obj;
}


static IPTR Scrollbar_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ScrollbarData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, msg))
	return FALSE;

    if (!(_flags(obj) & MADF_BORDERGADGET) && !data->sb_pos)
    {
	switch (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_arrangement)
	{
	    case SCROLLBAR_ARRANGEMENT_TOP:
		DoMethod(obj, MUIM_Group_Sort, (IPTR)data->prop,
			 (IPTR)data->up_arrow, (IPTR)data->down_arrow, (IPTR)NULL);
		break;
	    case SCROLLBAR_ARRANGEMENT_MIDDLE:
		DoMethod(obj, MUIM_Group_Sort, (IPTR)data->up_arrow,
			 (IPTR)data->prop, (IPTR)data->down_arrow, (IPTR)NULL);
		break;
	    case SCROLLBAR_ARRANGEMENT_BOTTOM:
		DoMethod(obj, MUIM_Group_Sort, (IPTR)data->up_arrow,
			 (IPTR)data->down_arrow, (IPTR)data->prop, (IPTR)NULL);
		break;	    
	}

	switch (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type)
	{
	    case SCROLLBAR_TYPE_STANDARD:
		break;
	    case SCROLLBAR_TYPE_NEWLOOK:
		break;
	    case SCROLLBAR_TYPE_CUSTOM:
		break;
	}
    }

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, Scrollbar_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Scrollbar_New(cl, obj, (struct opSet *) msg);
	case MUIM_Setup: return Scrollbar_Setup(cl, obj, msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Scrollbar_desc = { 
    MUIC_Scrollbar, 
    MUIC_Group, 
    sizeof(struct MUI_ScrollbarData), 
    (void*)Scrollbar_Dispatcher 
};
