/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "penspec.h"

extern struct Library *MUIMasterBase;

struct MUI_PenadjustData
{
    struct MUI_PenSpec      	penspec;
    struct MUI_PenSpec_intern	intpenspec;
    struct Hook     	    	inputhook;
    Object  	    	    	*listobj;
    Object  	    	    	*sliderobj;
    Object  	    	    	*coloradjobj;
};

static void UpdateState(Object *obj, struct MUI_PenadjustData *data)
{  
    zune_pen_spec_to_intern(&data->penspec, &data->intpenspec);

    switch(data->intpenspec.p_type)
    {
    	case PST_MUI:
	    nnset(data->listobj, MUIA_List_Active, data->intpenspec.p_mui);
	    nnset(obj, MUIA_Group_ActivePage, 0);
	    break;
	    
    	case PST_CMAP:
	    nnset(data->sliderobj, MUIA_Numeric_Value, data->intpenspec.p_cmap);
	    nnset(obj, MUIA_Group_ActivePage, 1);
	    break;
	    
    	case PST_RGB:
	    SetAttrs(data->coloradjobj, MUIA_NoNotify, TRUE,
		     MUIA_Coloradjust_RGB, &data->intpenspec.p_rgb, TAG_DONE);
	    
	    nnset(obj, MUIA_Group_ActivePage, 2);
	    break;
    }  
}

static void InputFunc(struct Hook *hook, Object *obj, APTR msg)
{
    struct MUI_PenadjustData *data = (struct MUI_PenadjustData *)hook->h_Data;
    IPTR    	    	      val;

    get(obj, MUIA_Group_ActivePage, &val);

    switch(val)
    {
    	case 0:
	    data->intpenspec.p_type = PST_MUI;

	    get(data->listobj, MUIA_List_Active, &val);
	    data->intpenspec.p_mui = (LONG)val;
	    break;
	    
	case 1:
	    data->intpenspec.p_type = PST_CMAP;

	    get(data->sliderobj, MUIA_Numeric_Value, &val);
	    data->intpenspec.p_cmap = (LONG)val;
	    break;
	    
	case 2:
	    data->intpenspec.p_type = PST_RGB;
	    
	    get(data->coloradjobj, MUIA_Coloradjust_Red, &val);
	    data->intpenspec.p_rgb.red = (ULONG)val;
	    get(data->coloradjobj, MUIA_Coloradjust_Green, &val);
	    data->intpenspec.p_rgb.green = (ULONG)val;
	    get(data->coloradjobj, MUIA_Coloradjust_Blue, &val);
	    data->intpenspec.p_rgb.blue = (ULONG)val;
    }
    
    zune_pen_intern_to_spec(&data->intpenspec, &data->penspec);

    D(bug(" ## penspec now %s\n", &data->penspec));
}

static IPTR MuipenDisplayFunc(struct Hook *hook, char **array, char *entry)
{
    LONG line;
    static char buf[16];

    line = (LONG)array[-1];
    if (line < 0 || line > 7)
	line = 0;
    snprintf(buf, sizeof(buf), "\33I[2:m%ld]", line);

    *array++ = buf;
    *array++ = "";
    *array = entry;

    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Penadjust_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    static const char *register_labels[] = {"MUI", "Colormap", "RGB", NULL};
    static const char *lv_labels[] = {"Shine", "Halfshine", "Background", "Halfshadow", "Shadow", "Text", "Fill", "Mark", NULL};
    static const struct Hook          muipen_display_hook = { {NULL, NULL}, HookEntry,  MuipenDisplayFunc, NULL };

    struct MUI_PenadjustData   *data;
    struct TagItem  	       *tag, *tags;
    Object  	    	       *listobj, *sliderobj, *coloradjobj;

    obj = (Object *)DoSuperNew(cl, obj,
    	MUIA_Register_Titles, register_labels,
	Child, ListviewObject,
	   MUIA_Listview_List, listobj = ListObject,
	       InputListFrame,
	       MUIA_List_SourceArray, lv_labels,
	       MUIA_List_Format, ",,",
	       MUIA_List_DisplayHook, &muipen_display_hook,
   	       End,
	    End,
	Child, sliderobj = SliderObject,
	    MUIA_Slider_Horiz, TRUE,
	    MUIA_Numeric_Min, -128,
	    MUIA_Numeric_Max, 127,
	    End,  
	Child, coloradjobj = ColoradjustObject,
	    End,
	TAG_MORE, msg->ops_AttrList);

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    data->inputhook.h_Entry = HookEntry;
    data->inputhook.h_SubEntry = (HOOKFUNC)InputFunc;
    data->inputhook.h_Data = data;
    
    data->listobj   	= listobj;
    data->sliderobj 	= sliderobj;
    data->coloradjobj 	= coloradjobj;
    
    strcpy(data->penspec.ps_buf, "m5");
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Penadjust_Spec:
	    	data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
		break;		
   	}
    }
        
    UpdateState(obj, data);

    DoMethod(obj, MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->inputhook);
    DoMethod(listobj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->inputhook);
    DoMethod(sliderobj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->inputhook);
    DoMethod(coloradjobj, MUIM_Notify, MUIA_Coloradjust_RGB, MUIV_EveryTime,
    	     (IPTR)obj, 2, MUIM_CallHook, (IPTR)&data->inputhook);
	    
    return (IPTR)obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC IPTR Penadjust_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem  	     *tags,*tag;
    struct MUI_PenadjustData *data = INST_DATA(cl, obj);
    BOOL    	    	      update = FALSE;
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Penadjust_Spec:
	    	data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
		update = TRUE;
		break;
 	}
    }

    if (update) UpdateState(obj, data);
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Penadjust_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_PenadjustData *data = INST_DATA(cl, obj);
    IPTR    	    	     *store = msg->opg_Storage;

    switch(msg->opg_AttrID)
    {
    	case MUIA_Penadjust_Spec:
	    *store = (IPTR)&data->penspec;
	    break;
	    
	default:
    	    return DoSuperMethodA(cl, obj, (Msg) msg);
	
    }
    
    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, Penadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Penadjust_New(cl, obj, (struct opSet *)msg);
	case OM_SET: return Penadjust_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Penadjust_Get(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Penadjust_desc = { 
    MUIC_Penadjust, 
    MUIC_Register,
    sizeof(struct MUI_PenadjustData), 
    (void*)Penadjust_Dispatcher 
};

