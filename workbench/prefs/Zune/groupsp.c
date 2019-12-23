/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <mui/Rawimage_mcc.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include "zunestuff.h"
#include <string.h>

/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */

extern struct Library *MUIMasterBase;

struct MUI_GroupsPData
{
    Object *background_register_popimage;
    Object *background_framed_popimage;
    Object *background_page_popimage;
    Object *spacing_vert_slider;
    Object *spacing_horiz_slider;
    Object *truncate_titles_checkmark;
    Object *font_title_string;
    Object *normal_popframe;
    Object *virtual_popframe;
    Object *title_position_cycle;
    Object *title_color_cycle;
};

static CONST_STRPTR positions_labels[3];
static CONST_STRPTR color_labels[5];

static IPTR GroupsP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GroupsPData *data;
    struct MUI_GroupsPData d;
    
    positions_labels[0] = _(MSG_ABOVE);
    positions_labels[1] = _(MSG_CENTERED);

    color_labels[0] = _(MSG_STANDARD);
    color_labels[1] = _(MSG_HILITE);
    color_labels[2] = _(MSG_3D);
    color_labels[3] = _(MSG_OUTLINE);

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	    
        MUIA_Group_SameWidth, TRUE,
	MUIA_Group_Horiz, TRUE,
        Child, (IPTR) VGroup,
    	    Child, (IPTR) VGroup,
    		GroupFrameT(_(MSG_TITLE)),
    		Child, (IPTR) VSpace(0),
    		Child, (IPTR) ColGroup(2),
    		    MUIA_Group_VertSpacing, 2,
    		    Child, (IPTR) Label(_(MSG_POSITION)),
    		    Child, (IPTR) (d.title_position_cycle =
    			       MakeCycle(_(MSG_POSITION), positions_labels)),
    		    Child, (IPTR) Label(_(MSG_COLOR)),
    		    Child, (IPTR) (d.title_color_cycle =
    			       MakeCycle(_(MSG_COLOR), color_labels)),
    		    Child, (IPTR) Label(_(MSG_FONT)),
    		    Child, (IPTR) (d.font_title_string = MakePopfont(FALSE)),
    		End, /* Title */
    		Child, (IPTR) VSpace(0),
    	    End,
    	    Child, (IPTR) VGroup,
    		GroupFrameT(_(MSG_SPACING)),
    		Child, (IPTR) VSpace(0),
    		Child, (IPTR) ColGroup(2),
    		    MUIA_Group_VertSpacing, 2,
    		    Child, (IPTR) Label(_(MSG_HORIZONTAL)),
    		    Child, (IPTR) (d.spacing_horiz_slider = (Object*)MakeSpacingSlider()),
    		    Child, (IPTR) Label(_(MSG_VERTICAL)),
    		    Child, (IPTR) (d.spacing_vert_slider = (Object*)MakeSpacingSlider()),
    		End, /* Spacing */
    		Child, (IPTR) VSpace(0),
    	    End,
    	    Child, (IPTR) VGroup,
    		GroupFrameT(_(MSG_REGISTER)),
    		Child, (IPTR) VSpace(0),
    		Child, (IPTR) VGroup,
    		    Child, (IPTR) VSpace(0),
    		    Child, (IPTR) Label1(_(MSG_DEFAULT_SIZE_TRUNCATE_TITLES)),
    		    Child, (IPTR) (d.truncate_titles_checkmark = MakeCheck(NULL)),
    		End, /* Register */
    		Child, (IPTR) VSpace(0),
    	    End,
	End,
	Child, (IPTR) VGroup,
    	    Child, (IPTR) HGroup,
    		GroupFrameT(_(MSG_FRAME)),
    		MUIA_Group_SameWidth, TRUE,
    		Child, (IPTR) VGroup,
    		   MUIA_Group_VertSpacing, 1,
    		   Child, (IPTR) (d.normal_popframe = MakePopframe()),
    		   Child, (IPTR) CLabel(_(MSG_NORMAL)),
    		End,
    		Child, (IPTR) VGroup,
    		    MUIA_Group_VertSpacing, 1,
    		    Child, (IPTR) (d.virtual_popframe = MakePopframe()),
    		    Child, (IPTR) CLabel(_(MSG_VIRTUAL)),
    		End,
    	    End, /* Frame */
    	    Child, (IPTR) HGroup,
    		GroupFrameT(_(MSG_BACKGROUND)),
    		MUIA_Group_SameWidth, TRUE,
    		Child, (IPTR) VGroup,
    		    MUIA_Group_VertSpacing, 1,
    		    Child, (IPTR) (d.background_framed_popimage = MakeBackgroundPopimage()),
    		    Child, (IPTR) CLabel(_(MSG_FRAMED)),
    		End,
    		Child, (IPTR) VGroup,
    		    MUIA_Group_VertSpacing, 1,
    		    Child, (IPTR) (d.background_page_popimage = MakeBackgroundPopimage()),
    		    Child, (IPTR) CLabel(_(MSG_PAGE)),
    		End,
    		Child, (IPTR) VGroup,
    		    MUIA_Group_VertSpacing, 1,
    		    Child, (IPTR) (d.background_register_popimage = MakeBackgroundPopimage()),
    		    Child, (IPTR) CLabel(_(MSG_REGISTER)),
    		End,
    	    End, /* Background */
    	End,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


static IPTR GroupsP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    STRPTR spec;

/* Fonts */
    setstring(data->font_title_string, (IPTR)FindFont(MUICFG_Font_Title));

/* Backgrounds */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Framed);
    set(data->background_framed_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Register);
    set(data->background_register_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Background_Page);
    set(data->background_page_popimage,MUIA_Imagedisplay_Spec, (IPTR)spec);

/* Frames */
    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Group);
    set(data->normal_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

    spec = (STRPTR)DoMethod(msg->configdata, MUIM_Configdata_GetString,
			    MUICFG_Frame_Virtual);
    set(data->virtual_popframe, MUIA_Framedisplay_Spec, (IPTR)spec);

/* Spacing */
    setslider(data->spacing_horiz_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Group_HSpacing));
    setslider(data->spacing_vert_slider,
	      DoMethod(msg->configdata, MUIM_Configdata_GetULong,
		       MUICFG_Group_VSpacing));

/* Checkmark */
    setcheckmark(data->truncate_titles_checkmark,
		 DoMethod(msg->configdata, MUIM_Configdata_GetULong,
			  MUICFG_Register_TruncateTitles));

/* Title (Cycles) */
    set(data->title_position_cycle, MUIA_Cycle_Active,
	DoMethod(msg->configdata, MUIM_Configdata_GetULong, MUICFG_GroupTitle_Position));
    set(data->title_color_cycle, MUIA_Cycle_Active,
	DoMethod(msg->configdata, MUIM_Configdata_GetULong, MUICFG_GroupTitle_Color));

    return 1;    
}


static IPTR GroupsP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_GroupsPData *data = INST_DATA(cl, obj);
    STRPTR str;

/* Fonts */
    str = getstring(data->font_title_string);
    DoMethod(msg->configdata, MUIM_Configdata_SetFont, MUICFG_Font_Title, (IPTR)str);

/* Backgrounds */
    str = (STRPTR)XGET(data->background_framed_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Framed,
	     (IPTR)str);

    str = (STRPTR)XGET(data->background_register_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Register,
	     (IPTR)str);

    str = (STRPTR)XGET(data->background_page_popimage,MUIA_Imagedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetImspec, MUICFG_Background_Page,
	     (IPTR)str);

/* Frames */
    str = (STRPTR)XGET(data->normal_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Group,
	     (IPTR)str);
    str = (STRPTR)XGET(data->virtual_popframe, MUIA_Framedisplay_Spec);
    DoMethod(msg->configdata, MUIM_Configdata_SetFramespec, MUICFG_Frame_Virtual,
	     (IPTR)str);

/* Spacing */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Group_HSpacing,
	     XGET(data->spacing_horiz_slider, MUIA_Numeric_Value));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Group_VSpacing,
	     XGET(data->spacing_vert_slider, MUIA_Numeric_Value));

/* Checkmark */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_Register_TruncateTitles,
	     XGET(data->truncate_titles_checkmark, MUIA_Selected));

/* Title (Cycles) */
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_GroupTitle_Position,
	     XGET(data->title_position_cycle, MUIA_Cycle_Active));
    DoMethod(msg->configdata, MUIM_Configdata_SetULong, MUICFG_GroupTitle_Color,
	     XGET(data->title_color_cycle, MUIA_Cycle_Active));

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, GroupsP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return GroupsP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return GroupsP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return GroupsP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Groups_desc = { 
    "Groups",
    MUIC_Group, 
    sizeof(struct MUI_GroupsPData),
    (void*)GroupsP_Dispatcher 
};


static const UBYTE icon32[] =
{
    0x00, 0x00, 0x00, 0x18,  // width
    0x00, 0x00, 0x00, 0x13,  // height
    'B', 'Z', '2', '\0',
    0x00, 0x00, 0x00, 0x5d,  // number of bytes

    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0x6d, 0x7f, 
    0xa4, 0x99, 0x00, 0x02, 0x9e, 0x66, 0x02, 0xa0, 0x00, 0x20, 0x00, 0x02, 
    0x00, 0x02, 0x01, 0x42, 0x40, 0x00, 0x00, 0xa0, 0x00, 0x91, 0x10, 0x64, 
    0xc4, 0x05, 0x25, 0x44, 0x3c, 0xa7, 0x55, 0x65, 0x81, 0x65, 0x8b, 0x64, 
    0x5f, 0x54, 0x93, 0x94, 0x93, 0xb4, 0x93, 0xd1, 0x1c, 0xd5, 0x5f, 0xb0, 
    0x39, 0xca, 0x7c, 0xf7, 0x6f, 0x35, 0x53, 0x80, 0xbb, 0x8d, 0x33, 0x2c, 
    0x53, 0x15, 0x62, 0xc8, 0x9e, 0x15, 0x69, 0x78, 0x5b, 0x6b, 0xc5, 0xdc, 
    0x91, 0x4e, 0x14, 0x24, 0x1b, 0x5f, 0xe9, 0x26, 0x40, 
};


Object *groupsclass_get_icon(void)
{
    return RawimageObject,
        MUIA_Rawimage_Data, icon32,
    End;
}
