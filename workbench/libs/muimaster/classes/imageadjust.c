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

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

extern struct Library *MUIMasterBase;

struct Imageadjust_DATA
{
    Object *bitmap_string;
    Object *bitmap_image;
    struct Hook bitmap_hook;

    Object *pattern_image[18];
    ULONG last_pattern_selected;
    struct Hook pattern_select_hook;

    Object *vector_image[24];
    ULONG last_vector_selected;
    struct Hook vector_select_hook;

    Object *color_group;

    Object *external_list;
    struct Hook external_display_hook;

    char *imagespec;
    LONG adjust_type;
};


static void Bitmap_Function(struct Hook *hook, Object *obj, APTR msg)
{
    struct Imageadjust_DATA *data = *(struct Imageadjust_DATA **)msg;
    char buf[255];
    STRPTR name;
    
    get(data->bitmap_string, MUIA_String_Contents, &name);
    if (name && strlen(name) > 0)
    {
	snprintf(buf, 255, "5:%s", name);
	set(data->bitmap_image, MUIA_Imagedisplay_Spec, (IPTR)buf);
    }
}


static VOID Pattern_Select_Function(struct Hook *hook, Object *obj, void **msg)
{
    struct Imageadjust_DATA *data = (struct Imageadjust_DATA *)hook->h_Data;
    int new_selected = (int)msg[0];

    if (data->last_pattern_selected != -1)
	set(data->pattern_image[data->last_pattern_selected],MUIA_Selected,FALSE);
    data->last_pattern_selected = new_selected;
}


static VOID Vector_Select_Function(struct Hook *hook, Object *obj, void **msg)
{
    struct Imageadjust_DATA *data = (struct Imageadjust_DATA *)hook->h_Data;
    int new_selected = (int)msg[0];

    if (data->last_vector_selected != -1)
	set(data->vector_image[data->last_vector_selected],MUIA_Selected,FALSE);
    data->last_vector_selected = new_selected;
}


static void Imageadjust_External_Display(struct Hook *h, char **strings, char *filename)
{
    if (filename) *strings = FilePart(filename);
}


/**************************************************************************
 Adds a directory to the list
**************************************************************************/
static int AddDirectory(Object *list, STRPTR dir, LONG parent)
{
    BPTR lock = Lock(dir,ACCESS_READ);
    struct ExAllControl *eac;
    struct ExAllData *ead, *entry;
    LONG more;
    int dir_len = strlen(dir);
    if (!lock) return 0;

    eac = (struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL,NULL);
    if (!eac)
    {
	UnLock(lock);
	return 0;
    }

    ead = AllocVec(1024,0);
    if (!ead)
    {
	FreeDosObject(DOS_EXALLCONTROL,eac);
	UnLock(lock);
	return 0;
    }

    eac->eac_LastKey = 0;

    do
    {
    	more = ExAll(lock,ead,1024,ED_TYPE,eac);
	if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) break;
	if (eac->eac_Entries == 0) continue;

	entry = ead;
	do
	{
	    int len = dir_len + strlen(ead->ed_Name) + 10;
	    char *buf = AllocVec(len,0);
	    if (buf)
	    {
	    	LONG num;
	    	int is_directory;

		if (ead->ed_Type > 0)
		{
		    is_directory = 1;
		    if (ead->ed_Type == ST_SOFTLINK)
		    {
		    	/* TODO: Special handling */
		    }
		} else is_directory = 0;

		strcpy(buf,dir);
		AddPart(buf,ead->ed_Name,len);

		num = DoMethod(list,MUIM_List_InsertSingleAsTree, (IPTR)buf, (IPTR)parent, MUIV_List_InsertSingleAsTree_Bottom,is_directory?MUIV_List_InsertSingleAsTree_List:0);

		if (num != -1 && is_directory)
		{
		    AddDirectory(list,buf,num);
		}
		FreeVec(buf);
	    }
	    ead = ead->ed_Next;
	}   while (ead);
    } while (more);

    FreeVec(ead);
    FreeDosObject(DOS_EXALLCONTROL,eac);
    UnLock(lock);
    return 1;
}



/**************************************************************************
 ...
**************************************************************************/
STATIC VOID Imageadjust_SetImagespec(Object *obj, struct Imageadjust_DATA *data, char *spec)
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

	case	'1':
		{
		    LONG vect;
		    StrToLong(s+2,&vect);

             	    if (vect >= 0 && vect < 24)
             	    {
			set(data->vector_image[vect],MUIA_Selected,TRUE);
			set(obj,MUIA_Group_ActivePage,1);
		    }

		}
		break;

	case	'2':
	    switch(data->adjust_type)
	    {
		case MUIV_Imageadjust_Type_All:
		case MUIV_Imageadjust_Type_Image:
		    nfset(obj, MUIA_Group_ActivePage, 2);
		    break;
		case MUIV_Imageadjust_Type_Background:
		    nfset(obj, MUIA_Group_ActivePage, 1);
		    break;
		default:
		    nfset(obj, MUIA_Group_ActivePage, 0);
		    break;
	    }
	    D(bug("imageadjust: setting color to %s\n", s));
	    set(data->color_group, MUIA_Penadjust_Spec, (IPTR)s+2);
		break;

	case	'5':
		set(data->bitmap_string,MUIA_String_Contents,(IPTR)s+2);
		Bitmap_Function(NULL, obj, &data);
		if (data->adjust_type == MUIV_Imageadjust_Type_All)
		    set(obj,MUIA_Group_ActivePage,4);
		else
		    set(obj,MUIA_Group_ActivePage,2);
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

IPTR Imageadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Imageadjust_DATA   *data;
    struct TagItem  	    *tag, *tags;
    static const char *labels_all[] = {"Pattern", "Vector", "Color", "External", "Bitmap", NULL};
    static const char *labels_image[] = {"Pattern", "Vector", "Color", "External", NULL};
    static const char *labels_bg[] = {"Pattern", "Color", "Bitmap", NULL};
    static const char *labels_color[] = {"Color", NULL};
    Object *pattern_group = NULL;
    Object *vector_group = NULL;
    Object *bitmap_string = NULL;
    Object *bitmap_image = NULL;
    Object *bitmap_popasl = NULL;
    Object *external_list = NULL;
    char *spec = NULL;
    LONG i;
    LONG adjust_type;
    Object *color_group = NULL;
    Object *external_group = NULL;
    Object *bitmap_group = NULL;

    adjust_type = GetTagData(MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
			     msg->ops_AttrList);

    color_group = MUI_NewObject(MUIC_Penadjust, TAG_DONE);

    if (adjust_type == MUIV_Imageadjust_Type_All ||
	adjust_type == MUIV_Imageadjust_Type_Image)
    {
	external_group = ListviewObject,
	    MUIA_Listview_List, (IPTR)external_list = ListObject,
	        InputListFrame,
	        MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
	        MUIA_List_DestructHook, MUIV_List_DestructHook_String,
	        End,
	    End;
    }
    
    if (adjust_type == MUIV_Imageadjust_Type_All ||
	adjust_type == MUIV_Imageadjust_Type_Background)
    {
	bitmap_group = VGroup,
	    Child, (IPTR)bitmap_image = ImagedisplayObject,
	        TextFrame,
	        InnerSpacing(0,0),
	        MUIA_Imagedisplay_FreeHoriz, TRUE,
	        MUIA_Imagedisplay_FreeVert, TRUE,
	        End,
	    Child, (IPTR)bitmap_popasl = PopaslObject,
	        MUIA_Popstring_String, (IPTR)bitmap_string = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                    End,
	        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
	        End,
	    End;
    }

    switch (adjust_type)
    {
	case MUIV_Imageadjust_Type_All:
	    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				       MUIA_Register_Titles, (IPTR)labels_all,
				       Child, (IPTR)HCenter((pattern_group = ColGroup(6), End)),
				       Child, (IPTR)HCenter((vector_group = ColGroup(6), End)),
				       Child, (IPTR)color_group,
				       Child, (IPTR)external_group,
				       Child, (IPTR)bitmap_group,
				       TAG_MORE, (IPTR)msg->ops_AttrList);
	    break;
	case MUIV_Imageadjust_Type_Background:
	    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				       MUIA_Register_Titles, (IPTR)labels_bg,
				       Child, (IPTR)HCenter((pattern_group = ColGroup(6), End)),
				       Child, (IPTR)color_group,
				       Child, (IPTR)bitmap_group,
				       TAG_MORE, (IPTR)msg->ops_AttrList);
	    break;
	case MUIV_Imageadjust_Type_Image:
	    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				       MUIA_Register_Titles, (IPTR)labels_image,
				       Child, (IPTR)HCenter((pattern_group = ColGroup(6), End)),
				       Child, (IPTR)HCenter((vector_group = ColGroup(6), End)),
				       Child, (IPTR)color_group,
				       Child, (IPTR)external_group,
				       TAG_MORE, (IPTR)msg->ops_AttrList);
	    break;
	case MUIV_Imageadjust_Type_Pen:
	    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				       MUIA_Register_Titles, (IPTR)labels_color,
				       Child, (IPTR)color_group,
				       TAG_MORE, (IPTR)msg->ops_AttrList);
	    break;
    }

    if (!obj) return FALSE;

    data = INST_DATA(cl, obj);
    data->adjust_type = adjust_type;
    data->color_group = color_group;

    if (adjust_type != MUIV_Imageadjust_Type_Pen)
    {
	data->last_pattern_selected = -1;
	data->pattern_select_hook.h_Data = data;
	data->pattern_select_hook.h_Entry = HookEntry;
	data->pattern_select_hook.h_SubEntry = (HOOKFUNC)Pattern_Select_Function;

	for (i=0;i<18;i++)
	{
	    data->pattern_image[i] = ImageObject,
		ButtonFrame,
		MUIA_CycleChain, 1,
		InnerSpacing(4,4),
		MUIA_Image_Spec, i + MUII_BACKGROUND,
		MUIA_InputMode, MUIV_InputMode_Immediate,
		MUIA_Image_FreeHoriz, TRUE,
		MUIA_Image_FreeVert, TRUE,
		MUIA_FixWidth, 16,
		MUIA_FixHeight, 16,
		End;

	    if (data->pattern_image[i])
	    {
		DoMethod(pattern_group,OM_ADDMEMBER,(IPTR)data->pattern_image[i]);
		DoMethod(data->pattern_image[i],MUIM_Notify,MUIA_Selected,TRUE,(IPTR)obj,3,MUIM_CallHook,(IPTR)&data->pattern_select_hook,i);
	    }
	}

	if (adjust_type != MUIV_Imageadjust_Type_Background)
	{
	    data->last_vector_selected = -1;
	    data->vector_select_hook.h_Data = data;
	    data->vector_select_hook.h_Entry = HookEntry;
	    data->vector_select_hook.h_SubEntry = (HOOKFUNC)Vector_Select_Function;

	    for (i=0;i<24;i++)
	    {
		char spec[10];

		snprintf(spec, sizeof(spec), "1:%ld", i);
		data->vector_image[i] = ImageObject,
		    ButtonFrame,
		    InnerSpacing(4,4),
		    MUIA_CycleChain, 1,
		    MUIA_Image_Spec, (IPTR)spec,
		    MUIA_InputMode, MUIV_InputMode_Immediate,
		    MUIA_Weight, 0,
		    End;

		if (data->vector_image[i])
		{
		    DoMethod(vector_group,OM_ADDMEMBER,(IPTR)data->vector_image[i]);
		    DoMethod(data->vector_image[i],MUIM_Notify,MUIA_Selected,TRUE,(IPTR)obj,3,MUIM_CallHook,(IPTR)&data->vector_select_hook,i);
		}
	    }
	} /* if (adjust_type != MUIV_Imageadjust_Type_Background) */

	if (adjust_type != MUIV_Imageadjust_Type_Image)
	{
	    data->bitmap_string = bitmap_string;
	    data->bitmap_image = bitmap_image;
	    data->bitmap_hook.h_Entry = HookEntry;
	    data->bitmap_hook.h_SubEntry = (HOOKFUNC)Bitmap_Function;
	    DoMethod(bitmap_popasl, MUIM_Notify, MUIA_Popasl_Active, FALSE,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->bitmap_hook, (IPTR)data);
	    DoMethod(bitmap_string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->bitmap_hook, (IPTR)data);
	}
    } /* if (adjust_type != MUIV_Imageadjust_Type_Pen) */

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

    if (adjust_type != MUIV_Imageadjust_Type_Background &&
	adjust_type != MUIV_Imageadjust_Type_Pen)
    {
	data->external_list = external_list;
	data->external_display_hook.h_Entry = HookEntry;
	data->external_display_hook.h_SubEntry = (HOOKFUNC)Imageadjust_External_Display;
	set(data->external_list,MUIA_List_DisplayHook, (IPTR)&data->external_display_hook);
    }
    /* Because we have many childs, we disable the forwarding of the notify method */
    DoMethod(obj, MUIM_Group_DoMethodNoForward, MUIM_Notify, MUIA_Group_ActivePage, 4, (IPTR)obj, 1, MUIM_Imageadjust_ReadExternal);

    Imageadjust_SetImagespec(obj,data,spec);
    return (IPTR)obj;
}

IPTR Imageadjust__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Imageadjust_DATA *data = INST_DATA(cl, obj);

    if (data->imagespec) FreeVec(data->imagespec);

    DoSuperMethodA(cl,obj,msg);
    return 0;
}

IPTR Imageadjust__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags,*tag;
    struct Imageadjust_DATA *data = INST_DATA(cl, obj);

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

IPTR Imageadjust__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Imageadjust_DATA *data = INST_DATA(cl, obj);
    struct pages {
	LONG type;
	LONG pos[5];
    };

    static struct pages pages_per_type[] = 
    {
	{ MUIV_Imageadjust_Type_Pen, { 2, -1, -1, -1, -1} },
	{ MUIV_Imageadjust_Type_Background, { 0, 2, 4, -1, -1} },
	{ MUIV_Imageadjust_Type_Image, { 0, 1, 2, 3, -1} },
	{ MUIV_Imageadjust_Type_All, { 0, 1, 2, 3, 4} },
    };

    switch (msg->opg_AttrID)
    {
    	case	MUIA_Imageadjust_Spec:
    		{
		    int i;

    		    LONG act;
		    if (data->imagespec)
		    {
		    	FreeVec(data->imagespec);
		    	data->imagespec = NULL;
		    }

		    get(obj,MUIA_Group_ActivePage,&act);
		    
		    for (i = 0; i < 4; i++)
		    {
			if (pages_per_type[i].type == data->adjust_type)
			    break;
		    }

		    act = pages_per_type[i].pos[act];

		    switch (act)
		    {
		    	case	0: /* Pattern */
				if ((data->imagespec = AllocVec(40,0)))
				{
				    if (data->last_pattern_selected != -1)
					snprintf(data->imagespec, 40, "0:%ld",
						 data->last_pattern_selected+128);
				    else
					strcpy(data->imagespec,"0:128");
				}
		    		break;

			case	1:
				if ((data->imagespec = AllocVec(20,0)))
				{
				    if (data->last_vector_selected != -1)
					snprintf(data->imagespec, 20, "1:%ld",
						 data->last_vector_selected);
				    else
					strcpy(data->imagespec,"0:128");
				}
				break;

			case 2:
			{
			    struct MUI_PenSpec *penspec;

			    get(data->color_group, MUIA_Penadjust_Spec, &penspec);
			    if (penspec)
			    {
				LONG len;
				D(bug("imageadjust: penspec = %s\n", penspec));
				len = strlen((STRPTR)penspec) + 3;
				if ((data->imagespec = AllocVec(len, 0)))
				    snprintf(data->imagespec, len, "2:%s", penspec->ps_buf);
			    }
			}
			break;

			case    4: /* Bitmap */
				{
				    char *str;
				    get(data->bitmap_string,MUIA_String_Contents,&str);
				    if (str)
				    {
					LONG len;
					len = strlen(str) + 10;
					if ((data->imagespec = AllocVec(len, 0)))
					    snprintf(data->imagespec, len, "5:%s", str);
				    }
				}
				break;
		    }
		    if (data->imagespec) *msg->opg_Storage = (ULONG)data->imagespec;
		    else *msg->opg_Storage = (ULONG)"0:128";
		}
		return TRUE;
    }

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

IPTR Imageadjust__MUIM_Imageadjust_ReadExternal(struct IClass *cl, Object *obj, Msg msg)
{
    struct Imageadjust_DATA *data = INST_DATA(cl, obj);
    DoMethod(data->external_list,MUIM_List_Clear);
    AddDirectory(data->external_list,"MUI:Images",-1);
    return 0;
}

#if ZUNE_BUILTIN_IMAGEADJUST
BOOPSI_DISPATCHER(IPTR, Imageadjust_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
            return Imageadjust__OM_NEW(cl, obj, (struct opSet *)msg);
	
        case OM_DISPOSE:
            return Imageadjust__OM_DISPOSE(cl,obj,(APTR)msg);
	
        case OM_SET:
            return Imageadjust__OM_SET(cl, obj, (struct opSet *)msg);
	
        case OM_GET:
            return Imageadjust__OM_GET(cl,obj,(APTR)msg);
        
        case MUIM_Imageadjust_ReadExternal: 
            return Imageadjust__MUIM_Imageadjust_ReadExternal(cl,obj,(APTR)msg);
        
        default:
            return DoSuperMethodA(cl, obj, msg);
    }   
}

const struct __MUIBuiltinClass _MUI_Imageadjust_desc =
{ 
    MUIC_Imageadjust, 
    MUIC_Register,
    sizeof(struct Imageadjust_DATA), 
    (void*)Imageadjust_Dispatcher 
};
#endif /* ZUNE_BUILTIN_IMAGEADJUST */
