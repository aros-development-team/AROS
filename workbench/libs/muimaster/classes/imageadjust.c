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

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ImageadjustData
{
    Object *bitmap_string;
    Object *pattern_image[18];
    int last_pattern_selected;
    struct Hook pattern_select_hook;
    char *imagespec;
};

#ifndef _AROS
static __asm VOID Pattern_Select_Function(register __a0 struct Hook *hook, register __a2 Object *obj, register __a1 void **msg)
#else
AROS_UFH3(VOID,Pattern_Selected_Function,
	AROS_UFHA(struct Hook *, hook,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(void **, msg,  A1))
#endif
{
    struct MUI_ImageadjustData *data = (struct MUI_ImageadjustData *)hook->h_Data;
    int new_selected = (int)msg[0];

    if (data->last_pattern_selected != -1) set(data->pattern_image[data->last_pattern_selected],MUIA_Selected,FALSE);
    data->last_pattern_selected = new_selected;
}

/**************************************************************************
 ...
**************************************************************************/
STATIC VOID Imageadjust_SetImagespec(Object *obj, struct MUI_ImageadjustData *data, char *spec)
{
    char *s;
    if (!spec) spec = "0:128";

    s = (char*)spec;

    switch (*s)
    {
	case	'0':
		{
		    LONG pat;
             	    StrToLong(s+2,&pat);
             	    pat -=  MUII_BACKGROUND;

             	    if (pat >= 0 && pat < 18)
             	    {
			set(data->pattern_image[pat],MUIA_Selected,TRUE);
			set(obj,MUIA_Group_ActivePage,0);
		    }
		}
		break;

	case	'2':
		{
		    ULONG r,g,b;
	     	    s += 2;
		    r = strtoul(s,&s, 16);
		    s++;
		    g = strtoul(s,&s, 16);
		    s++;
		    b = strtoul(s,&s, 16);
//		    return get_color_image_spec(r,g,b);
		}
		break;

	case	'5':
		set(data->bitmap_string,MUIA_String_Contents,s+2);
		set(obj,MUIA_Group_ActivePage,3);
		break;
		

	case    '6':
		{
		    LONG img;
             	    StrToLong(s+2,&img);

//		    if (img >= MUII_WindowBack && img <= MUII_ReadListBack)
//			return zune_imspec_copy(__zprefs.images[img]);
	        }
	        break;
    }
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Imageadjust_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ImageadjustData   *data;
    struct TagItem  	    *tag, *tags;
    static const char *labels[] = {"Pattern","Vector", "Color", "Bitmap", "External",NULL};
    Object *pattern_group;
    Object *bitmap_string;
    char *spec=NULL;
    int i;

    obj = (Object *)DoSuperNew(cl, obj,
    	MUIA_Register_Titles, labels,
	Child, pattern_group = ColGroup(6), End,
	Child, HVSpace,
	Child, HVSpace,
	Child, PopaslObject,
	    MUIA_Popstring_String, bitmap_string = StringObject, StringFrame, End,
	    MUIA_Popstring_Button, PopButton(MUII_PopFile),
	    End,
	Child, HVSpace,
	TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);

    data->last_pattern_selected = -1;
    data->pattern_select_hook.h_Data = data;
    data->pattern_select_hook.h_Entry = (HOOKFUNC)Pattern_Select_Function;

    for (i=0;i<18;i++)
    {
    	data->pattern_image[i] = ImageObject,
    	    ButtonFrame,
	    MUIA_Image_Spec, i + MUII_BACKGROUND,
	    MUIA_InputMode, MUIV_InputMode_Immediate,
	    MUIA_Image_FreeVert, TRUE,
	    MUIA_Image_FreeHoriz, TRUE,
	    End;

	if (data->pattern_image[i])
	{
	    DoMethod(pattern_group,OM_ADDMEMBER,data->pattern_image[i]);
	    DoMethod(data->pattern_image[i],MUIM_Notify,MUIA_Selected,TRUE,obj,3,MUIM_CallHook,&data->pattern_select_hook,i);
	}
    }

    data->bitmap_string = bitmap_string;
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Imageadjust_Spec:
		    spec = (char*)tag->ti_Data;
		    break;
	}
    }

    Imageadjust_SetImagespec(obj,data,spec);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
STATIC IPTR Imageadjust_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ImageadjustData *data = INST_DATA(cl, obj);

    if (data->imagespec) FreeVec(data->imagespec);

    DoSuperMethodA(cl,obj,msg);
    return 0;
}

/**************************************************************************
 OM_SET
**************************************************************************/
STATIC IPTR Imageadjust_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct MUI_ImageadjustData *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Imageadjust_Spec:
		    Imageadjust_SetImagespec(obj,data,(char*)tag->ti_Data);
		    break;
 	}
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Imageadjust_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ImageadjustData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    	case	MUIA_Imageadjust_Spec:
    		{
    		    LONG act;
		    if (data->imagespec)
		    {
		    	FreeVec(data->imagespec);
		    	data->imagespec = NULL;
		    }

		    get(obj,MUIA_Group_ActivePage,&act);
		    switch (act)
		    {
		    	case	0: /* Pattern */
				if ((data->imagespec = AllocVec(40,0)))
				{
				    if (data->last_pattern_selected != -1)
					sprintf(data->imagespec,"0:%ld",data->last_pattern_selected+128);
				}
		    		break;

			case    3: /* Bitmap */
				{
				    char *str;
				    get(data->bitmap_string,MUIA_String_Contents,&str);
				    if (str)
				    {
					if ((data->imagespec = AllocVec(strlen(str)+10,0)))
					    sprintf(data->imagespec,"5:%s",str);
				    }
				}
				break;
		    }
		    if (data->imagespec) *msg->opg_Storage = (ULONG)data->imagespec;
		    else *msg->opg_Storage = (ULONG)"0:128";
		}
		return 1;
    }

    if (DoSuperMethodA(cl, obj, (Msg) msg)) return 1;
    return 0;
}

#ifndef _AROS
__asm IPTR Imageadjust_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Imageadjust_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Imageadjust_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Imageadjust_Dispose(cl,obj,(APTR)msg);
	case OM_SET: return Imageadjust_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Imageadjust_Get(cl,obj,(APTR)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Imageadjust_desc = { 
    MUIC_Imageadjust, 
    MUIC_Register,
    sizeof(struct MUI_ImageadjustData), 
    (void*)Imageadjust_Dispatcher 
};

