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

#include "imspec_intern.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "imageadjust_private.h"

extern struct Library *MUIMasterBase;

STATIC STRPTR StrDupPooled(APTR pool, CONST_STRPTR str)
{
    char *newstr;
    if (!str) return NULL;
    newstr = AllocPooled(pool,strlen(str)+1);
    if (newstr) strcpy(newstr,str);
    return newstr;
}

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


static void Gradient_Function(struct Hook *hook, Object *obj, APTR msg)
{
    struct Imageadjust_DATA *data = *(struct Imageadjust_DATA **)msg;
    struct MUI_RGBcolor *start_rgb; 
    struct MUI_RGBcolor *end_rgb;
    int angle = XGET(data->gradient_angle_slider, MUIA_Numeric_Value);
    int is_tiled = XGET(data->gradient_type_cycle, MUIA_Cycle_Active);

    start_rgb = (struct MUI_RGBcolor*)XGET(data->gradient_start_poppen,
					   MUIA_Pendisplay_RGBcolor);
    end_rgb = (struct MUI_RGBcolor*)XGET(data->gradient_end_poppen,
					 MUIA_Pendisplay_RGBcolor);

    snprintf(data->gradient_imagespec,sizeof(data->gradient_imagespec),
	     "%s:%d,%08lx,%08lx,%08lx-%08lx,%08lx,%08lx",
	     is_tiled ? "8" : "7",
                 angle,
                 start_rgb->red,start_rgb->green,start_rgb->blue,
                 end_rgb->red,end_rgb->green,end_rgb->blue);

    set(data->gradient_imagedisplay, MUIA_Imagedisplay_Spec, data->gradient_imagespec);
}

static void GradientSwap_Function(struct Hook *hook, Object *obj, APTR msg)
{
    struct Imageadjust_DATA *data = *(struct Imageadjust_DATA **)msg;
    struct MUI_RGBcolor *start_rgb; 
    struct MUI_RGBcolor *end_rgb;
    struct MUI_RGBcolor tmp;
    
    start_rgb = (struct MUI_RGBcolor*)XGET(data->gradient_start_poppen,
					   MUIA_Pendisplay_RGBcolor);
    end_rgb = (struct MUI_RGBcolor*)XGET(data->gradient_end_poppen,
					 MUIA_Pendisplay_RGBcolor);

    tmp = *start_rgb;
    set(data->gradient_start_poppen, MUIA_Pendisplay_RGBcolor, (IPTR)end_rgb);
    set(data->gradient_end_poppen, MUIA_Pendisplay_RGBcolor, (IPTR)&tmp);
    Gradient_Function(NULL,obj,&data);
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


struct ExternalListEntry {
    STRPTR reldir;
    ULONG dirlen;
    STRPTR filename;
    ULONG namelen;
    LONG type;
};

static void Imageadjust_External_Display(struct Hook *h, char **strings,
					 struct ExternalListEntry *entry)
{
    static char buf[256];

    if (entry->filename)
    {
	int len;
	*strings = buf;
	snprintf(buf, 256, "%s", FilePart(entry->filename));
	buf[255] = 0;
	len = strlen(buf);
	if (len > 4 && !strcmp(buf + len - 4, ".mf0"))
	    buf[len - 4] = 0;
	else if (len > 4 && !strcmp(buf + len - 4, ".mb0"))
	    buf[len - 4] = 0;
	else if (len > 4 && !strcmp(buf + len - 4, ".mbr"))
	    buf[len - 4] = 0;
	else if (len > 6 && !strcmp(buf + len - 6, ".image"))
	    buf[len - 6] = 0;
    }
}

static struct ExternalListEntry *
Imageadjust_External_Construct(struct Hook *h,
			       APTR pool,
			       struct ExternalListEntry *ele)
{
    struct ExternalListEntry *entry = NULL;

    if (NULL == ele)
	return NULL;

    if (NULL != ele->filename)
    {
	entry = AllocPooled(pool, sizeof(struct ExternalListEntry));
	if (NULL != entry)
	{
	    *entry = *ele;
	    entry->filename = StrDupPooled(pool, entry->filename);
	    entry->reldir = StrDupPooled(pool, entry->reldir);
	    return entry;
	}
    }
    return NULL;
}

static void Imageadjust_External_Destruct(struct Hook *h, APTR pool,
					  struct ExternalListEntry *entry)
{
    if (entry != NULL)
    {
	if (entry->filename != NULL)
	    FreePooled(pool, entry->filename, entry->namelen + 1);
	if (entry->reldir != NULL)
	    FreePooled(pool, entry->reldir, entry->dirlen + 1);
	FreePooled(pool, entry, sizeof(struct ExternalListEntry));
    }
}


/**************************************************************************
 Adds a directory to the list
**************************************************************************/
static int AddDirectory(Object *list, STRPTR dir, LONG parent)
{
    BPTR lock = Lock(dir,ACCESS_READ);
    struct ExAllControl *eac;
    struct ExAllData *ead, *EAData;
    LONG more;
    int dir_len = strlen(dir);
    if (!lock) return 0;

    //bug("AddDirectory: locked %s\n", dir);

    eac = (struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL, NULL);
    if (!eac)
    {
	UnLock(lock);
	return 0;
    }

    EAData = AllocVec(1024, 0);
    if (!EAData)
    {
	FreeDosObject(DOS_EXALLCONTROL, eac);
	UnLock(lock);
	return 0;
    }

    eac->eac_LastKey = 0;

    do
    {
	//DoMethod(_app(list), MUIM_Application_InputBuffered);

    	more = ExAll(lock, EAData, 1024, ED_TYPE, eac);
	if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) break;
	if (eac->eac_Entries == 0) continue;

	ead = EAData;
	do
	{
	    size_t namelen = strlen(ead->ed_Name);
	    int len = dir_len + namelen + 10;
	    char *buf;

	    //DoMethod(_app(list), MUIM_Application_InputBuffered);

	    buf = AllocVec(len,0);
	    if (buf)
	    {
	    	LONG num;
	    	int is_directory;
		BOOL add_me = TRUE;

		if (ead->ed_Type > 0)
		{
		    is_directory = 1;
		    if (ead->ed_Type == ST_SOFTLINK)
		    {
		    	/* TODO: Special handling */
		    }
		} else is_directory = 0;

		strcpy(buf, dir);
		AddPart(buf, ead->ed_Name, len);

		if (!is_directory && namelen > 4
		    && !strcmp(ead->ed_Name + namelen - 4, ".mf1"))
		    add_me = FALSE;
		if (!is_directory && namelen > 4
		    && !strcmp(ead->ed_Name + namelen - 4, ".mb1"))
		    add_me = FALSE;

		if (add_me)
		{
		    struct ExternalListEntry ele;
		    //bug("AddDirectory: adding image %s\n", buf);
		    

		    ele.reldir = dir + strlen(IMSPEC_EXTERNAL_PREFIX);
		    ele.dirlen = strlen(ele.reldir);
		    ele.filename = ead->ed_Name;
		    ele.namelen = strlen(ele.filename);
		    ele.type = ead->ed_Type;

		    num = DoMethod(list, MUIM_List_InsertSingleAsTree,
				   (IPTR)&ele,
				   (IPTR)parent,
				   MUIV_List_InsertSingleAsTree_Bottom,
				   is_directory ?
				   MUIV_List_InsertSingleAsTree_List : 0);
		}
#warning "FIXME: where does num's value come from here?"
		if (num != -1 && is_directory)
		{
		    AddDirectory(list, buf, num);
		}
		FreeVec(buf);
	    }
	    ead = ead->ed_Next;
	}   while (ead);
    } while (more);

    FreeVec(EAData);
    FreeDosObject(DOS_EXALLCONTROL, eac);
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

	case '3':
	case '4':
	{
	    struct ExternalListEntry *entry;
	    LONG entries;
	    int i;

	    set(obj, MUIA_Group_ActivePage, 3);

	    get(data->external_list, MUIA_List_Entries, &entries);
	    for (i = 0; i < entries; i++)
	    {
		DoMethod(data->external_list, MUIM_List_GetEntry, i,
			 (IPTR)&entry);
		if (entry != NULL
		    && entry->reldir != NULL
		    && entry->filename != NULL)
		{
		    STRPTR file = FilePart(s+2);
		    //bug("entry->reldir = %s, s + 2 = %s, len=%d\n",
		    //entry->reldir, s + 2, strlen(s + 2) - strlen(file) - 1);
		    if (!strncmp(entry->reldir, s + 2,
				 strlen(s + 2) - strlen(file) - 1))
		    {
			//bug("entry->filename = %s, file = %s\n",
			//entry->filename, file);
			if (!strcmp(entry->filename, file))
			{
			    set(data->external_list, MUIA_List_Active, i);
			    break;
			}
		    }
		}
	    }
	} 
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
	        }
	        break;

	case	'7':
	case	'8':
		{
		    struct MUI_ImageSpec_intern spec;
		    if (zune_gradient_string_to_intern(s+2,&spec))
		    {
			struct MUI_RGBcolor col;
			col.red = spec.u.gradient.start_rgb[0]*0x01010101;
			col.green = spec.u.gradient.start_rgb[1]*0x01010101;
			col.blue = spec.u.gradient.start_rgb[2]*0x01010101;

			nnset(data->gradient_start_poppen, MUIA_Pendisplay_RGBcolor, &col);

			col.red = spec.u.gradient.end_rgb[0]*0x01010101;
			col.green = spec.u.gradient.end_rgb[1]*0x01010101;
			col.blue = spec.u.gradient.end_rgb[2]*0x01010101;

			nnset(data->gradient_end_poppen, MUIA_Pendisplay_RGBcolor, &col);

			nnset(data->gradient_angle_slider, MUIA_Numeric_Value, spec.u.gradient.angle);
		       
			set(data->gradient_type_cycle, MUIA_Cycle_Active, *s == '7' ? 0 : 1);
		    }

		    Gradient_Function(NULL,obj,&data);
		    if (data->adjust_type == MUIV_Imageadjust_Type_All)
		        set(obj,MUIA_Group_ActivePage,5);
		    else
		        set(obj,MUIA_Group_ActivePage,3);
		}
		break;
    }
}

IPTR Imageadjust__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Imageadjust_DATA   *data;
    struct TagItem  	    *tag, *tags;
    static const char * const labels_all[] = {"Pattern", "Vector", "Color", "External", "Bitmap", "Gradient", NULL};
    static const char * const labels_image[] = {"Pattern", "Vector", "Color", "External", NULL};
    static const char * const labels_bg[] = {"Pattern", "Color", "Bitmap", "Gradient", NULL};
    static const char * const labels_color[] = {"Color", NULL};
    static const char * const gradient_type_entries[] = {"Scaled", "Tiled", NULL};

    Object *pattern_group = NULL;
    Object *vector_group = NULL;
    Object *external_list = NULL;
    Object *bitmap_string = NULL;
    Object *bitmap_image = NULL;
    Object *bitmap_popasl = NULL;
    Object *gradient_imagedisplay = NULL;
    Object *gradient_start_poppen = NULL;
    Object *gradient_end_poppen = NULL;
    Object *gradient_angle_slider = NULL;
    Object *gradient_type_cycle = NULL;
    Object *gradient_horiz_button = NULL;
    Object *gradient_vert_button = NULL;
    Object *gradient_swap_button = NULL;
    char *spec = NULL;
    LONG i;
    LONG adjust_type;
    Object *color_group = NULL;
    Object *external_group = NULL;
    Object *bitmap_group = NULL;
    Object *gradient_group = NULL;

    adjust_type = GetTagData(MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
			     msg->ops_AttrList);

    color_group = MUI_NewObject(MUIC_Penadjust, TAG_DONE);

    if (adjust_type == MUIV_Imageadjust_Type_All ||
	adjust_type == MUIV_Imageadjust_Type_Image)
    {
	external_group = ListviewObject,
	    MUIA_Listview_List, (IPTR)(external_list = ListObject,
	        InputListFrame,
	        MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
		MUIA_List_DestructHook, MUIV_List_DestructHook_String,
	        End),
	    End;
    }
    
    if (adjust_type == MUIV_Imageadjust_Type_All ||
	adjust_type == MUIV_Imageadjust_Type_Background)
    {
	bitmap_group = VGroup,
	    Child, (IPTR)(bitmap_image = ImagedisplayObject,
	        TextFrame,
	        InnerSpacing(0,0),
	        MUIA_Imagedisplay_FreeHoriz, TRUE,
	        MUIA_Imagedisplay_FreeVert, TRUE,
		MUIA_Dropable, FALSE,
	        End),
	    Child, (IPTR)(bitmap_popasl = PopaslObject,
	        MUIA_Popstring_String, (IPTR)(bitmap_string = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                    End),
	        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
	        End),
	    End;

	gradient_group = ColGroup(2),
	    Child, (IPTR)FreeLabel("Type:"),
	    Child, (IPTR)(gradient_type_cycle = MUI_MakeObject(MUIO_Cycle, (IPTR)"Type:", (IPTR)gradient_type_entries)),
	    Child, (IPTR)FreeLabel("Angle:"),
	    Child, (IPTR)VGroup,
	        Child, (IPTR)HGroup,
	            MUIA_Group_SameWidth, TRUE,
	            Child, (IPTR)(gradient_horiz_button = TextObject,
			  ButtonFrame,
			  MUIA_Background,               MUII_ButtonBack,
			  MUIA_InputMode, MUIV_InputMode_RelVerify,
			  MUIA_Text_PreParse, (IPTR)"\33c",
			  MUIA_Text_Contents, (IPTR)"Vertical",
			  End),
	            Child, (IPTR)(gradient_vert_button = TextObject,
			  ButtonFrame,
			  MUIA_Background,               MUII_ButtonBack,
			  MUIA_InputMode, MUIV_InputMode_RelVerify,
			  MUIA_Text_PreParse, (IPTR)"\33c",
			  MUIA_Text_Contents, (IPTR)"Horizontal",
			  End),
	            End,
	        Child, (IPTR)(gradient_angle_slider = SliderObject, MUIA_Group_Horiz, TRUE, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 179, End),
	        End,
	    Child, (IPTR)FreeLabel("Colors:"),
	    Child, (IPTR)HGroup,
		Child, (IPTR)(gradient_start_poppen = PoppenObject,
			      MUIA_Window_Title,    (IPTR) "Start pen",
			      MUIA_Pendisplay_Spec, (IPTR) "rbbbbbbbb,bbbbbbbb,bbbbbbbb", End),
	        Child, (IPTR)VCenter((gradient_swap_button = TextObject,
			      ButtonFrame,
			      MUIA_Background,               MUII_ButtonBack,
			      MUIA_InputMode, MUIV_InputMode_RelVerify,
			      MUIA_Text_Contents, (IPTR)"<->",
			      MUIA_Weight, 0,
			      End)),
		Child, (IPTR)(gradient_end_poppen = PoppenObject,
			      MUIA_Window_Title,    (IPTR) "End pen",
			      MUIA_Pendisplay_Spec, (IPTR)"r55555555,55555555,55555555", End),
		End,
	    Child, (IPTR)FreeLabel("Preview:"),
	    Child, (IPTR)(gradient_imagedisplay = ImagedisplayObject,
		TextFrame,
		InnerSpacing(0,0),
	        MUIA_Imagedisplay_FreeHoriz, TRUE,
	        MUIA_Imagedisplay_FreeVert, TRUE,
		MUIA_Dropable, FALSE,
	        End),
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
				       Child, (IPTR)gradient_group,
				       TAG_MORE, (IPTR)msg->ops_AttrList);
	    break;
	case MUIV_Imageadjust_Type_Background:
	    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				       MUIA_Register_Titles, (IPTR)labels_bg,
				       Child, (IPTR)HCenter((pattern_group = ColGroup(6), End)),
				       Child, (IPTR)color_group,
				       Child, (IPTR)bitmap_group,
				       Child, (IPTR)gradient_group,
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
    data->originator  = (APTR)GetTagData(MUIA_Imageadjust_Originator, 0, msg->ops_AttrList);

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

    if (gradient_imagedisplay)
    {
	data->gradient_imagedisplay = gradient_imagedisplay;
	data->gradient_start_poppen = gradient_start_poppen;
	data->gradient_end_poppen = gradient_end_poppen;
	data->gradient_angle_slider = gradient_angle_slider;
	data->gradient_type_cycle = gradient_type_cycle;
	data->gradient_vert_button = gradient_vert_button;
	data->gradient_horiz_button = gradient_horiz_button;
	data->gradient_swap_button = gradient_swap_button;

	DoMethod(gradient_vert_button, MUIM_Notify, MUIA_Pressed, FALSE,
		 (IPTR)gradient_angle_slider, 3, MUIM_Set, MUIA_Numeric_Value, 90);
	DoMethod(gradient_horiz_button, MUIM_Notify, MUIA_Pressed, FALSE,
		 (IPTR)gradient_angle_slider, 3, MUIM_Set, MUIA_Numeric_Value, 0);

	data->gradient_swap_hook.h_Entry = HookEntry;
	data->gradient_swap_hook.h_SubEntry = (HOOKFUNC)GradientSwap_Function;
	DoMethod(gradient_swap_button, MUIM_Notify, MUIA_Pressed, FALSE,
		 (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradient_swap_hook, (IPTR)data);
	
	data->gradient_hook.h_Entry = HookEntry;
	data->gradient_hook.h_SubEntry = (HOOKFUNC)Gradient_Function;
	DoMethod(gradient_start_poppen, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradient_hook, (IPTR)data);
	DoMethod(gradient_end_poppen, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradient_hook, (IPTR)data);
	DoMethod(gradient_angle_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradient_hook, (IPTR)data);
	DoMethod(gradient_type_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		     (IPTR)obj, 3, MUIM_CallHook, (IPTR)&data->gradient_hook, (IPTR)data);

	/* Set the gradient image to correct values */
	Gradient_Function(NULL,obj,&data);

    } /* if (gradient_imagedisplay) */

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
	set(data->external_list, MUIA_List_DisplayHook, (IPTR)&data->external_display_hook);

	data->external_construct_hook.h_Entry = HookEntry;
	data->external_construct_hook.h_SubEntry = (HOOKFUNC)Imageadjust_External_Construct;
	set(data->external_list, MUIA_List_ConstructHook, (IPTR)&data->external_construct_hook);

	data->external_destruct_hook.h_Entry = HookEntry;
	data->external_destruct_hook.h_SubEntry = (HOOKFUNC)Imageadjust_External_Destruct;
	set(data->external_list, MUIA_List_DestructHook, (IPTR)&data->external_destruct_hook);

	/* Because we have many childs, we disable the forwarding of the notify method */
	DoMethod(obj, MUIM_Group_DoMethodNoForward, MUIM_Notify,
		 MUIA_Group_ActivePage, 3, (IPTR)obj, 1,
		 MUIM_Imageadjust_ReadExternal);

	if (data->originator)
	    DoMethod(data->external_list, MUIM_Notify,
		     MUIA_Listview_DoubleClick, TRUE,
		     MUIV_Notify_Application, 5, MUIM_Application_PushMethod,
		     (IPTR)data->originator, 2,  MUIM_Popimage_CloseWindow, TRUE);
    }
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
	LONG pos[6];
    };

    const static struct pages pages_per_type[] = 
    {
	{ MUIV_Imageadjust_Type_Pen, { 2, -1, -1, -1, -1, -1} },
	{ MUIV_Imageadjust_Type_Background, { 0, 2, 4, 5, -1, -1} },
	{ MUIV_Imageadjust_Type_Image, { 0, 1, 2, 3, -1, -1} },
	{ MUIV_Imageadjust_Type_All, { 0, 1, 2, 3, 4, 5} },
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

		    get(obj, MUIA_Group_ActivePage, &act);
		    
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

			case 3: /* External */
			{
			    struct ExternalListEntry *entry;

			    DoMethod(data->external_list, MUIM_List_GetEntry,
				     MUIV_List_GetEntry_Active, (IPTR)&entry);
			    if (entry != NULL && entry->filename != NULL&& entry->reldir != NULL)
			    {
				LONG len;
				len = 2 + strlen(entry->reldir) + 1 + strlen(entry->filename) + 1;
				if ((data->imagespec = AllocVec(len, 0)))
				{
				    snprintf(data->imagespec, len, "3:%s/%s",
					     entry->reldir, entry->filename);
				    D(bug("Imageadjust_OM_GET: imspec=%s\n", data->imagespec));
				}
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

			case	5: /* Gradient */
				data->imagespec = StrDup(data->gradient_imagespec);
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

    DoMethod(data->external_list, MUIM_List_Clear);
    set(data->external_list, MUIA_List_Quiet, TRUE);
    AddDirectory(data->external_list, IMSPEC_EXTERNAL_PREFIX, -1);
    set(data->external_list, MUIA_List_Quiet, FALSE);
    return 0;
}

#if 0
IPTR Imageadjust__MUIM_Imageadjust_ExternalSelected(struct IClass *cl, Object *obj, Msg msg)
{
    struct Imageadjust_DATA *data = INST_DATA(cl, obj);

    if (data->originator)
	DoMethod(_app(obj), MUIM_Application_PushMethod, data->originator, 2,
		 MUIM_Popimage_CloseWindow, TRUE);
    return 0;
}
#endif

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

	    //case MUIM_Imageadjust_ExternalSelected: 
            //return Imageadjust__MUIM_Imageadjust_ExternalSelected(cl,obj,(APTR)msg);
        
        default:
            return DoSuperMethodA(cl, obj, msg);
    }   
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Imageadjust_desc =
{ 
    MUIC_Imageadjust, 
    MUIC_Register,
    sizeof(struct Imageadjust_DATA), 
    (void*)Imageadjust_Dispatcher 
};
#endif /* ZUNE_BUILTIN_IMAGEADJUST */
