/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "scrollgroup_private.h"

extern struct Library *MUIMasterBase;


AROS_UFH3(ULONG,Scrollgroup_Layout_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(struct MUI_LayoutMsg *, lm,  A1))
{
    AROS_USERFUNC_INIT
    
    struct Scrollgroup_DATA *data = (struct Scrollgroup_DATA *)hook->h_Data;
    switch (lm->lm_Type)
    {
	case    MUILM_MINMAX:
		{
		    /* Calulate the minmax dimension of the group,
		    ** We only have a fixed number of children, so we need no NextObject()
		    */
		    WORD maxxxxwidth = 0;
		    WORD maxxxxheight = 0;

		    maxxxxwidth = _minwidth(data->contents) + _minwidth(data->vert);
		    if (_minwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _minwidth(data->horiz);
		    lm->lm_MinMax.MinWidth = maxxxxwidth;

		    maxxxxheight = _minheight(data->contents) + _minheight(data->horiz);
		    if (_minheight(data->vert) > maxxxxheight) maxxxxheight = _minheight(data->vert);
		    lm->lm_MinMax.MinHeight = maxxxxheight;

		    maxxxxwidth = _defwidth(data->contents) + _defwidth(data->vert);
		    if (_defwidth(data->horiz) > maxxxxwidth) maxxxxwidth = _defwidth(data->horiz);
		    lm->lm_MinMax.DefWidth = maxxxxwidth;

		    maxxxxheight = _defheight(data->contents) + _defheight(data->horiz);
		    if (_defheight(data->vert) > maxxxxheight) maxxxxheight = _defheight(data->vert);
		    lm->lm_MinMax.DefHeight = maxxxxheight;

		    lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
		    lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
		    return 0;
		}

		case MUILM_LAYOUT:
		{
		    /* Now place the objects between (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
		    */

		    LONG virt_width;
		    LONG virt_height;
		    LONG vert_width = _minwidth(data->vert);
		    LONG horiz_height = _minheight(data->horiz);
		    LONG lay_width = lm->lm_Layout.Width;
		    LONG lay_height = lm->lm_Layout.Height;
		    LONG cont_width;
		    LONG cont_height;

		    /* layout the virtual group a first time, to determine the virtual width/height */
		    MUI_Layout(data->contents,0,0,lay_width,lay_height,0);

		    get(data->contents, MUIA_Virtgroup_Width, &virt_width);
		    get(data->contents, MUIA_Virtgroup_Height, &virt_height);

		    virt_width -= _subwidth(data->contents);
		    virt_height += _subheight(data->contents);

		    if (virt_width > lay_width && virt_height > lay_height)
		    {
		    	/* We need all scrollbars and the button */
			set(data->vert, MUIA_ShowMe, TRUE); /* We could also overload MUIM_Show... */
			set(data->horiz, MUIA_ShowMe, TRUE);
			set(data->button, MUIA_ShowMe, TRUE);
			cont_width = lay_width - vert_width;
			cont_height = lay_height - horiz_height;
			MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
			MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
			MUI_Layout(data->button, cont_width, cont_height, vert_width, horiz_height, 0);
		    } else
		    {
		    	if (virt_height > lay_height)
		    	{
			    set(data->vert, MUIA_ShowMe, TRUE);
			    set(data->horiz, MUIA_ShowMe, FALSE);
			    set(data->button, MUIA_ShowMe, FALSE);

			    cont_width = lay_width - vert_width;
			    cont_height = lay_height;
			    MUI_Layout(data->vert, cont_width, 0, vert_width, cont_height,0);
		    	} else
		    	{
			    if (virt_width > lay_width)
			    {
				set(data->vert, MUIA_ShowMe, FALSE);
				set(data->horiz, MUIA_ShowMe, TRUE);
				set(data->button, MUIA_ShowMe, FALSE);

				cont_width = lay_width;
				cont_height = lay_height - horiz_height;
				MUI_Layout(data->horiz, 0, cont_height, cont_width, horiz_height, 0);
			    } else
			    {
				set(data->vert, MUIA_ShowMe, FALSE);
				set(data->horiz, MUIA_ShowMe, FALSE);
				set(data->button, MUIA_ShowMe, FALSE);

			    	cont_width = lay_width;
			    	cont_height = lay_height;
			    }
		    	}
		    }

		    /* Layout the group a second time, note that setting _mwidth() and _mheight() should be enough, or we invent a new flag */
		    MUI_Layout(data->contents,0,0,cont_width,cont_height,0);
		    return 1;
		}
    }
    return 0;

    AROS_USERFUNC_EXIT
}


AROS_UFH3(ULONG,Scrollgroup_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(APTR, dummy, A2),
	AROS_UFHA(void **, msg,  A1))
{
    AROS_USERFUNC_INIT

    struct Scrollgroup_DATA *data = (struct Scrollgroup_DATA *)hook->h_Data;
    int type = (int)msg[0];
    LONG val = (LONG)msg[1];

    switch (type)
    {
	case	1:
		{
		    get(data->vert,MUIA_Prop_First,&val);
		    SetAttrs(data->contents,MUIA_Virtgroup_Top, val, MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE);
		    break;
		}

	case	2:
		{
		    get(data->horiz,MUIA_Prop_First,&val);
		    SetAttrs(data->contents,MUIA_Virtgroup_Left, val, MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE);
		    break;
		}
	case	3: nnset(data->horiz, MUIA_Prop_First, val); break;
	case	4: nnset(data->vert, MUIA_Prop_First, val); break;
    }
    return 0;

    AROS_USERFUNC_EXIT
}

IPTR Scrollgroup__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scrollgroup_DATA *data;
    //struct TagItem *tags,*tag;
    Object *contents = (Object*)GetTagData(MUIA_Scrollgroup_Contents, NULL, msg->ops_AttrList);
    Object *vert,*horiz,*button,*group;

    struct Hook *layout_hook = mui_alloc_struct(struct Hook);
    if (!layout_hook) return NULL;

    layout_hook->h_Entry = (HOOKFUNC)Scrollgroup_Layout_Function;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
    	
        MUIA_Group_Horiz, FALSE,
    	Child, (IPTR) (group = GroupObject,
	    MUIA_Group_LayoutHook, (IPTR) layout_hook,
	    Child, (IPTR) contents,
	    Child, (IPTR) (vert   = ScrollbarObject, MUIA_Group_Horiz, FALSE, End),
	    Child, (IPTR) (horiz  = ScrollbarObject, MUIA_Group_Horiz, TRUE, End),
	    Child, (IPTR) (button = ScrollbuttonObject, End),
        End),
	
        TAG_DONE
    );

    if (!obj)
    {
    	mui_free(layout_hook);
	return NULL;
    }

    data = INST_DATA(cl, obj);
    data->vert = vert;
    data->horiz = horiz;
    data->button = button;
    data->contents = contents;

    data->hook.h_Entry = (HOOKFUNC)Scrollgroup_Function;
    data->hook.h_Data = data;
    data->layout_hook = layout_hook;
    layout_hook->h_Data = data;

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 1, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 2, MUIV_TriggerValue);
    DoMethod(contents, MUIM_Notify, MUIA_Virtgroup_Left, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 3, MUIV_TriggerValue);
    DoMethod(contents, MUIM_Notify, MUIA_Virtgroup_Top, MUIV_EveryTime, (IPTR)obj, 4, MUIM_CallHook, (IPTR)&data->hook, 4, MUIV_TriggerValue);

    D(bug("Scrollgroup_New(%lx)\n", obj));
    D(bug(" vert = %lx, horiz = %lx, button = %lx\n", vert, horiz, button));
    return (ULONG)obj;
}

IPTR Scrollgroup__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Scrollgroup_DATA *data = INST_DATA(cl, obj);
    mui_free(data->layout_hook);
    return DoSuperMethodA(cl,obj,msg);
}

IPTR Scrollgroup__MUIM_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct Scrollgroup_DATA *data = INST_DATA(cl, obj);
    LONG top,left,width,height;

    get(data->contents, MUIA_Virtgroup_Left, &left);
    get(data->contents, MUIA_Virtgroup_Top, &top);
    get(data->contents, MUIA_Virtgroup_Width, &width);
    get(data->contents, MUIA_Virtgroup_Height, &height);

    SetAttrs(data->horiz, MUIA_Prop_First, left,
			  MUIA_Prop_Entries, width,
			  MUIA_Prop_Visible, _mwidth(data->contents),
			  TAG_DONE);


    SetAttrs(data->vert,  MUIA_Prop_First, top,
			  MUIA_Prop_Entries, height,
			  MUIA_Prop_Visible, _mheight(data->contents),
			  TAG_DONE);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

#if ZUNE_BUILTIN_SCROLLGROUP
BOOPSI_DISPATCHER(IPTR, Scrollgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Scrollgroup__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Scrollgroup__OM_DISPOSE(cl, obj, msg);
	case MUIM_Show: return Scrollgroup__MUIM_Show(cl, obj, (struct MUIP_Show*)msg);
        default: return DoSuperMethodA(cl, obj, msg);
    }
}

const struct __MUIBuiltinClass _MUI_Scrollgroup_desc =
{ 
    MUIC_Scrollgroup, 
    MUIC_Group, 
    sizeof(struct Scrollgroup_DATA), 
    (void*)Scrollgroup_Dispatcher 
};
#endif /* ZUNE_BUILTIN_SCROLLGROUP */
