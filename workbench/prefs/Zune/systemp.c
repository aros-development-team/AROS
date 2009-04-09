/*
    Copyright  2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <zune/customclasses.h>

#ifdef __AROS__
#include <proto/alib.h>
//#define DEBUG 1
//#include <aros/debug.h>
#endif

#include "zunestuff.h"
#include <string.h>
#include <stdio.h>

extern struct Library *MUIMasterBase;

/* Utility class for the bubble help time slider */

struct BubbleSlider_DATA
{
    char buf[20];
};

IPTR BubbleSlider__MUIM_Numeric_Stringify(struct IClass *cl, Object * obj, struct MUIP_Numeric_Stringify *msg)
{
    struct BubbleSlider_DATA *data = INST_DATA(cl, obj);

    if(msg->value != 0)
	snprintf(data->buf, sizeof(data->buf) - 1, "%.1f s", 0.1 * msg->value);
    else
	snprintf(data->buf, sizeof(data->buf) - 1, "off");

    data->buf[sizeof(data->buf) - 1] = 0;
    
    return (IPTR)data->buf;
}

ZUNE_CUSTOMCLASS_1(
    BubbleSlider, NULL, MUIC_Slider, NULL,
    MUIM_Numeric_Stringify, struct MUIP_Numeric_Stringify*
)

/* Preferences System tab class */

struct MUI_SystemPData
{
    Object *screen_name_string;
    Object *call_psi_button;
    Object *pop_to_front_checkmark;
    Object *hotkey_string;
    Object *show_icon_checkmark;
    Object *show_menu_checkmark;
    Object *on_startup_checkmark;
    Object *arexx_checkmark;
    Object *first_bubble_slider;
    Object *next_bubble_slider;
};

static IPTR SystemP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_SystemPData *data;
    struct MUI_SystemPData d;

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	
        MUIA_Group_Columns, 2,
        MUIA_Group_SameSize, TRUE,

        Child, (IPTR) VGroup,
            GroupFrameT(_(MSG_PUBLIC_SCREEN)),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) ColGroup(2),
                Child, (IPTR) Label1(_(MSG_NAME)),
                Child, PopscreenObject, 
        	    MUIA_Popstring_String, (IPTR) (d.screen_name_string = StringObject, MUIA_Frame, MUIV_Frame_String, End),
        	    MUIA_Popstring_Button, PopButton(MUII_PopUp),
                    End,
                Child, HSpace(0),
                Child, (IPTR) (d.call_psi_button = SimpleButton(_(MSG_CALL_INSPECTOR))),
                Child, (IPTR) Label1(_(MSG_POP_TO_FRONT)),
                Child, HGroup,
                    Child, (IPTR) (d.pop_to_front_checkmark = MakeCheck(NULL)),
                    Child, RectangleObject, End,
                    End,
                End,
            Child, (IPTR) VSpace(0),
            End,

        Child, (IPTR) VGroup,
            GroupFrameT(_(MSG_ICONIFICATION)),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) ColGroup(2),
                Child, (IPTR) Label1(_(MSG_HOTKEY)),
                Child, (IPTR) (d.hotkey_string = MakeString()),
                Child, (IPTR) Label1(_(MSG_SHOW)),
                Child, HGroup,
                    Child, (IPTR) (d.show_icon_checkmark = MakeCheck(NULL)),
                    Child, (IPTR) Label1(_(MSG_ICON)),
                    Child, (IPTR) (d.show_menu_checkmark = MakeCheck(NULL)),
                    Child, (IPTR) Label1(_(MSG_MENU)),
                    End,
                Child, (IPTR) Label1(_(MSG_ON_STARTUP)),
                Child, HGroup,
                    Child, (IPTR) (d.on_startup_checkmark = MakeCheck(NULL)),
                    Child, RectangleObject, End,
                    End,
                End,
            Child, (IPTR) VSpace(0),
            End,

        Child, (IPTR) VGroup,
            GroupFrameT(_(MSG_INTERFACES)),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) HGroup,
                Child, HSpace(0),
                Child, (IPTR) Label1(_(MSG_AREXX)),
                Child, (IPTR) (d.arexx_checkmark = MakeCheck(NULL)),
                Child, HSpace(0),
                End,
            Child, (IPTR) VSpace(0),
            End,

        Child, (IPTR) VGroup,
            GroupFrameT(_(MSG_BUBBLE_HELP)),
            MUIA_Group_VertSpacing, 0,
            Child, (IPTR) VSpace(0),
            Child, (IPTR) ColGroup(2),
                Child, (IPTR) Label1(_(MSG_FIRST_BUBBLE)),
                Child, (IPTR) (d.first_bubble_slider = NewObject(BubbleSlider_CLASS->mcc_Class, NULL, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 50, TAG_END)),
                Child, (IPTR) Label1(_(MSG_NEXT_BUBBLE)),
                Child, (IPTR) (d.next_bubble_slider = NewObject(BubbleSlider_CLASS->mcc_Class, NULL, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 50, TAG_END)),
                End,
            Child, (IPTR) VSpace(0),
            End,

    	TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    DoMethod(
	d.first_bubble_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	d.next_bubble_slider, (IPTR) 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
    );
    
    return (IPTR)obj;
}


static IPTR SystemP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_SystemPData *data = INST_DATA(cl, obj);

    ConfigToString(msg->configdata, MUICFG_PublicScreen, data->screen_name_string);
    ConfigToCheckmark(msg->configdata, MUICFG_PublicScreen_PopToFront, data->pop_to_front_checkmark);
    ConfigToString(msg->configdata, MUICFG_Iconification_Hotkey, data->hotkey_string);
    ConfigToCheckmark(msg->configdata, MUICFG_Iconification_ShowIcon, data->show_icon_checkmark);
    ConfigToCheckmark(msg->configdata, MUICFG_Iconification_ShowMenu, data->show_menu_checkmark);
    ConfigToCheckmark(msg->configdata, MUICFG_Iconification_OnStartup, data->on_startup_checkmark);
    ConfigToCheckmark(msg->configdata, MUICFG_Interfaces_EnableARexx, data->arexx_checkmark);
    ConfigToSlider(msg->configdata, MUICFG_BubbleHelp_FirstDelay, data->first_bubble_slider);
    ConfigToSlider(msg->configdata, MUICFG_BubbleHelp_NextDelay, data->next_bubble_slider);

    return 1;    
}


static IPTR SystemP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_SystemPData *data = INST_DATA(cl, obj);
    
    StringToConfig(data->screen_name_string, msg->configdata, MUICFG_PublicScreen);
    CheckmarkToConfig(data->pop_to_front_checkmark, msg->configdata, MUICFG_PublicScreen_PopToFront);
    StringToConfig(data->hotkey_string, msg->configdata, MUICFG_Iconification_Hotkey);
    CheckmarkToConfig(data->show_icon_checkmark, msg->configdata, MUICFG_Iconification_ShowIcon);
    CheckmarkToConfig(data->show_menu_checkmark, msg->configdata, MUICFG_Iconification_ShowMenu);
    CheckmarkToConfig(data->on_startup_checkmark, msg->configdata, MUICFG_Iconification_OnStartup);
    CheckmarkToConfig(data->arexx_checkmark, msg->configdata, MUICFG_Interfaces_EnableARexx);
    SliderToConfig(data->first_bubble_slider, msg->configdata, MUICFG_BubbleHelp_FirstDelay);
    SliderToConfig(data->next_bubble_slider, msg->configdata, MUICFG_BubbleHelp_NextDelay);
    
    return TRUE;
}

BOOPSI_DISPATCHER(IPTR, SystemP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return SystemP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return SystemP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return SystemP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_System_desc = { 
    "System",
    MUIC_Group, 
    sizeof(struct MUI_SystemPData),
    (void*)SystemP_Dispatcher 
};
