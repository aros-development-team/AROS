/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

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
#include <proto/muimaster.h>

#include <string.h>

#define MYDEBUG 1
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "penspec.h"
#include "penadjust_private.h"

extern struct Library *MUIMasterBase;

static void UpdateState(Object *obj, struct Penadjust_DATA *data)
{  
    zune_pen_spec_to_intern(&data->penspec, &data->intpenspec);

    switch(data->intpenspec.p_type)
    {
    	case PST_MUI:
	    set(data->listobj, MUIA_List_Active, data->intpenspec.p_mui);
	    set(obj, MUIA_Group_ActivePage, 0);
	    break;
	    
    	case PST_CMAP:
	    set(data->sliderobj, MUIA_Numeric_Value, data->intpenspec.p_cmap);
	    set(obj, MUIA_Group_ActivePage, 1);
	    break;
	    
    	case PST_RGB:
	    set(data->coloradjobj, MUIA_Coloradjust_RGB, (IPTR) &data->intpenspec.p_rgb);
	    set(obj, MUIA_Group_ActivePage, 2);
	    break;

	case PST_SYS:
	    break;
    }  
}

static void UpdatePenspec(Object *obj, struct Penadjust_DATA *data)
{
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

IPTR Penadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    static const char * const register_labels[]  = {"MUI", "Colormap", "RGB", NULL};
    static const char * const lv_labels[]        = {"Shine", "Halfshine", "Background", "Halfshadow", "Shadow", "Text", "Fill", "Mark", NULL};
    static const struct Hook muipen_display_hook = { {NULL, NULL}, HookEntry,  MuipenDisplayFunc, NULL };

    struct Penadjust_DATA   *data;
    struct TagItem  	       *tag, *tags;
    Object  	    	       *listobj, *sliderobj, *coloradjobj;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
    	
        MUIA_Register_Titles, (IPTR) register_labels,
	Child, (IPTR) ListviewObject,
            MUIA_Listview_List, (IPTR) (listobj = ListObject,
                InputListFrame,
	        MUIA_List_SourceArray, (IPTR) lv_labels,
	        MUIA_List_Format,      (IPTR) ",,",
	        MUIA_List_DisplayHook, (IPTR) &muipen_display_hook,
            End),
        End,
	Child, (IPTR) (sliderobj = SliderObject,
	    MUIA_Slider_Horiz, TRUE,
	    MUIA_Numeric_Min,  -128,
	    MUIA_Numeric_Max,  127,
        End),  
	Child, (IPTR) (coloradjobj = ColoradjustObject, End),
	
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

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

    return (IPTR)obj;
}

IPTR Penadjust__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem  	     *tags,*tag;
    struct Penadjust_DATA *data = INST_DATA(cl, obj);
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

IPTR Penadjust__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Penadjust_DATA *data = INST_DATA(cl, obj);
    IPTR    	    	     *store = msg->opg_Storage;

    switch(msg->opg_AttrID)
    {
    	case MUIA_Penadjust_Spec:
	    UpdatePenspec(obj, data);
	    *store = (IPTR)&data->penspec;
	    break;
	    
	default:
    	    return DoSuperMethodA(cl, obj, (Msg) msg);
	
    }
    
    return TRUE;
}

#if ZUNE_BUILTIN_PENADJUST
BOOPSI_DISPATCHER(IPTR, Penadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Penadjust__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET: return Penadjust__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET: return Penadjust__OM_GET(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl, obj, msg);
    }    
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Penadjust_desc =
{ 
    MUIC_Penadjust, 
    MUIC_Register,
    sizeof(struct Penadjust_DATA), 
    (void*)Penadjust_Dispatcher 
};
#endif /* ZUNE_BUILTIN_PENADJUST */
