/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
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

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include "zunestuff.h"
#include <string.h>

/*  #define DEBUG 1 */
/*  #include <aros/debug.h> */

extern struct Library *MUIMasterBase;

#define NSHORTCUTS (MUICFG_Keyboard_Popup - MUICFG_Keyboard_Press + 1)

struct MUI_NavigationPData
{
    Object *drag_leftbutton_checkmark;
    Object *drag_leftbutton_string;
    Object *drag_middlebutton_checkmark;
    Object *drag_middlebutton_string;
    Object *drag_autostart_checkmark;
    Object *drag_autostart_slider;
    Object *dnd_popframe;
    Object *drag_look_cycle;
    Object *balance_look_cycle;
    Object *active_poppen;
    Object *keyboard_string[NSHORTCUTS];
};

static CONST_STRPTR dnd_labels[] =
{
    "always solid",
    "ghosted on box",
    "ghosted outside box",
    "always ghosted",
    NULL,
};

static CONST_STRPTR balancing_labels[] =
{
    "show frames",
    "show objects",
    NULL,
};

static CONST_STRPTR keyboard_label[NSHORTCUTS] =
{
    "Press:",
    "Toggle:",
    "Up:",
    "Down:",
    "Page up:",
    "Page down:",
    "Top:",
    "Bottom:",
    "Left:",
    "Right:",
    "Word left:",
    "Word right:",
    "Line start:",
    "Line end:",
    "Next gadget:",
    "Prev. gadget:",
    "Gadget off:",
    "Close window:",
    "Next window:",
    "Prev. window:",
    "Help:",
    "Popup:",
};

static Object *MakeScrollgroup (struct MUI_NavigationPData *data)
{
    int i;
    struct TagItem tags[NSHORTCUTS * 2 + 1];

    for (i = 0; i < NSHORTCUTS; i++)
    {
	tags[2 * i].ti_Tag = Child;
	tags[2 * i].ti_Data = (IPTR) Label(keyboard_label[i]);
	tags[2 * i + 1].ti_Tag = Child;
	tags[2 * i + 1].ti_Data = (IPTR) (data->keyboard_string[i] = MakeString());
    }
    tags[NSHORTCUTS * 2].ti_Tag = TAG_DONE;
    tags[NSHORTCUTS * 2].ti_Data = 0;

    return ScrollgroupObject,
	MUIA_Scrollgroup_FreeHoriz, FALSE,
	MUIA_Scrollgroup_Contents, (IPTR) ColGroupV(2),
	InputListFrame,
	TAG_MORE, (IPTR) tags,
	End,
	End;
}


static IPTR NavigationP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_NavigationPData *data;
    struct MUI_NavigationPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
	MUIA_Group_Horiz, TRUE,
	Child, (IPTR) VGroup,
	Child, (IPTR) VGroup,
	GroupFrameT("Drag & Drop"),
	Child, (IPTR) ColGroup(3),
	Child, (IPTR) Label("Left Button:"),
	Child, (IPTR) (d.drag_leftbutton_checkmark = MakeCheck(NULL)),
	Child, (IPTR) (d.drag_leftbutton_string = MakeString()),

	Child, (IPTR) Label("Middle Button:"),
	Child, (IPTR) (d.drag_middlebutton_checkmark = MakeCheck(NULL)),
	Child, (IPTR) (d.drag_middlebutton_string = MakeString()),

	Child, (IPTR) Label("Autostart:"),
	Child, (IPTR) (d.drag_autostart_checkmark = MakeCheck(NULL)),
	Child, (IPTR) (d.drag_autostart_slider = SliderObject,
		       MUIA_CycleChain, 1,
		       MUIA_Numeric_Format, (IPTR) "%ld pixel",
		       MUIA_Numeric_Min, 1,
		       MUIA_Numeric_Max, 20,
	End), // Slider
	End, // ColGroup(3)
	Child, ColGroup(2),
	Child, (IPTR) FreeLabel("Frame:"),
	Child, (IPTR) (d.dnd_popframe = MakePopframe()),
	Child, (IPTR) Label("Look:"),
	Child, (IPTR) (d.drag_look_cycle = MakeCycle(NULL, dnd_labels)),
	End, // ColGroup(2),
	End, // Drag & Drop
	Child, VGroup,
	GroupFrameT("Balancing Groups"),
	Child, (IPTR) HVSpace,
	Child, (IPTR) ColGroup(2),
	Child, (IPTR) Label("Look:"),
	Child, (IPTR) (d.balance_look_cycle = MakeCycle(NULL, balancing_labels)),
	Child, (IPTR) Label("Example:"),
	Child, (IPTR) HGroup,
	Child, (IPTR) TextObject,
	TextFrame,
	MUIA_Text_SetMin, FALSE,
	MUIA_Text_PreParse, "\33c",
	MUIA_Text_Contents, "Try with",
	End,
	Child, (IPTR) BalanceObject, End,
	Child, (IPTR) TextObject,
	TextFrame,
	MUIA_Text_SetMin, FALSE,
	MUIA_Text_PreParse, "\33c",
	MUIA_Text_Contents, "shift",
	End,
	Child, (IPTR) BalanceObject, End,
	Child, (IPTR) TextObject,
	TextFrame,
	MUIA_Text_SetMin, FALSE,
	MUIA_Text_PreParse, "\33c",
	MUIA_Text_Contents, "too!",
	End,	
	End, // HGroup
	End, // ColGroup
	Child, (IPTR) HVSpace,
	End, // Balancing Groups
	End, // VGroup Left
	Child, (IPTR) VGroup,
	GroupFrameT("Keyboard Control"),
	Child, (IPTR) HGroup,
	Child, (IPTR) VGroup,
	MUIA_Group_VertSpacing, 0,
	Child, (IPTR) VSpace(3),
	Child, (IPTR) Label("Color of active object:"),
	Child, (IPTR) VSpace(3),
	End, // VGroup label
	Child, (IPTR) (d.active_poppen = MakePoppen()),
	End, // HGroup
	Child, (IPTR) MakeScrollgroup(&d),
	End, // VGroup KB Ctrl
    	TAG_MORE, (IPTR) msg->ops_AttrList);
	
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    DoMethod
    (
        data->drag_leftbutton_checkmark, MUIM_Notify,
        MUIA_Selected, MUIV_EveryTime,
        (IPTR) data->drag_leftbutton_string, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue
    );

    DoMethod
    (
        data->drag_middlebutton_checkmark, MUIM_Notify,
        MUIA_Selected, MUIV_EveryTime,
        (IPTR) data->drag_middlebutton_string, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue
    );

    DoMethod
    (
        data->drag_autostart_checkmark, MUIM_Notify,
        MUIA_Selected, MUIV_EveryTime,
        (IPTR) data->drag_autostart_slider, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue
    );

    return (IPTR)obj;
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR NavigationP_ConfigToGadgets(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_NavigationPData *data = INST_DATA(cl, obj);
    int i;

    ConfigToCheckmark(msg->configdata, MUICFG_Drag_LeftButton, data->drag_leftbutton_checkmark);
    ConfigToString(msg->configdata, MUICFG_Drag_LMBModifier, data->drag_leftbutton_string);

    ConfigToCheckmark(msg->configdata, MUICFG_Drag_MiddleButton, data->drag_middlebutton_checkmark);
    ConfigToString(msg->configdata, MUICFG_Drag_MMBModifier, data->drag_middlebutton_string);

    ConfigToCheckmark(msg->configdata, MUICFG_Drag_Autostart, data->drag_autostart_checkmark);
    ConfigToSlider(msg->configdata, MUICFG_Drag_Autostart_Length, data->drag_autostart_slider);

    ConfigToFrame(msg->configdata, MUICFG_Frame_Drag,data->dnd_popframe);
    ConfigToCycle(msg->configdata, MUICFG_Dragndrop_Look, data->drag_look_cycle);

    ConfigToCycle(msg->configdata, MUICFG_Balance_Look, data->balance_look_cycle);

    ConfigToPen(msg->configdata, MUICFG_ActiveObject_Color, data->active_poppen);

    for (i = 0; i < NSHORTCUTS; i++)
	ConfigToString(msg->configdata, MUICFG_Keyboard_Press + i, data->keyboard_string[i]);

    return TRUE;    
}


/*
 * MUIM_Settingsgroup_ConfigToGadgets
 */
static IPTR NavigationP_GadgetsToConfig(struct IClass *cl, Object *obj,
				    struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_NavigationPData *data = INST_DATA(cl, obj);
    int i;

    CheckmarkToConfig(data->drag_leftbutton_checkmark, msg->configdata, MUICFG_Drag_LeftButton);
    StringToConfig(data->drag_leftbutton_string, msg->configdata, MUICFG_Drag_LMBModifier);

    CheckmarkToConfig(data->drag_middlebutton_checkmark, msg->configdata, MUICFG_Drag_MiddleButton);
    StringToConfig(data->drag_middlebutton_string, msg->configdata, MUICFG_Drag_MMBModifier);

    CheckmarkToConfig(data->drag_autostart_checkmark, msg->configdata, MUICFG_Drag_Autostart);
    SliderToConfig(data->drag_autostart_slider, msg->configdata, MUICFG_Drag_Autostart_Length);

    FrameToConfig(data->dnd_popframe, msg->configdata, MUICFG_Frame_Drag);
    CycleToConfig(data->drag_look_cycle, msg->configdata, MUICFG_Dragndrop_Look);

    CycleToConfig(data->balance_look_cycle, msg->configdata, MUICFG_Balance_Look);

    PenToConfig(data->active_poppen, msg->configdata, MUICFG_ActiveObject_Color);

    for (i = 0; i < NSHORTCUTS; i++)
	StringToConfig(data->keyboard_string[i], msg->configdata, MUICFG_Keyboard_Press + i);

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, NavigationP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return NavigationP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return NavigationP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return NavigationP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Navigation_desc = { 
    "Navigation",
    MUIC_Group,
    sizeof(struct MUI_NavigationPData),
    (void*)NavigationP_Dispatcher 
};
