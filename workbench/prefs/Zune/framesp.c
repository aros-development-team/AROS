/*
    Copyright  2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
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

extern struct Library *MUIMasterBase;

struct MUI_FramesPData
{
    Object *frames_config_string[16];
};

static IPTR FramesP_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_FramesPData *data;
    struct MUI_FramesPData d;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        
        Child, (IPTR) HGroup,
            Child, (IPTR) VGroup,
                Child, (IPTR) VSpace(0),
                Child, ColGroup(4),
                    GroupFrameT(_(MSG_CUSTOMFRAMES)),
                    Child, (IPTR) Label("#1"),
                    Child, (IPTR) (d.frames_config_string[0] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#2"),
                    Child, (IPTR) (d.frames_config_string[1] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#3"),
                    Child, (IPTR) (d.frames_config_string[2] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#4"),
                    Child, (IPTR) (d.frames_config_string[3] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#5"),
                    Child, (IPTR) (d.frames_config_string[4] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#6"),
                    Child, (IPTR) (d.frames_config_string[5] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#7"),
                    Child, (IPTR) (d.frames_config_string[6] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#8"),
                    Child, (IPTR) (d.frames_config_string[7] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#9"),
                    Child, (IPTR) (d.frames_config_string[8] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#10"),
                    Child, (IPTR) (d.frames_config_string[9] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#11"),
                    Child, (IPTR) (d.frames_config_string[10] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#12"),
                    Child, (IPTR) (d.frames_config_string[11] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#13"),
                    Child, (IPTR) (d.frames_config_string[12] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#14"),
                    Child, (IPTR) (d.frames_config_string[13] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#15"),
                    Child, (IPTR) (d.frames_config_string[14] = MakePopfile(FALSE, "#?.config")),
                    Child, (IPTR) Label("#16"),
                    Child, (IPTR) (d.frames_config_string[15] = MakePopfile(FALSE, "#?.config")),
                End,
                Child, (IPTR) VSpace(0),

            End,

        End,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
    *data = d;

    return (IPTR)obj;
}


static IPTR FramesP_ConfigToGadgets(struct IClass *cl, Object *obj,
				     struct MUIP_Settingsgroup_ConfigToGadgets *msg)
{
    struct MUI_FramesPData *data = INST_DATA(cl, obj);

    ConfigToString (msg->configdata, MUICFG_CustomFrame_1, data->frames_config_string[0]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_2, data->frames_config_string[1]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_3, data->frames_config_string[2]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_4, data->frames_config_string[3]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_5, data->frames_config_string[4]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_6, data->frames_config_string[5]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_7, data->frames_config_string[6]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_8, data->frames_config_string[7]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_9, data->frames_config_string[8]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_10, data->frames_config_string[9]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_11, data->frames_config_string[10]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_12, data->frames_config_string[11]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_13, data->frames_config_string[12]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_14, data->frames_config_string[13]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_15, data->frames_config_string[14]);
    ConfigToString (msg->configdata, MUICFG_CustomFrame_16, data->frames_config_string[15]);

    return 1;
}


static IPTR FramesP_GadgetsToConfig(struct IClass *cl, Object *obj,
				     struct MUIP_Settingsgroup_GadgetsToConfig *msg)
{
    struct MUI_FramesPData *data = INST_DATA(cl, obj);

    StringToConfig (data->frames_config_string[0], msg->configdata, MUICFG_CustomFrame_1);
    StringToConfig (data->frames_config_string[1], msg->configdata, MUICFG_CustomFrame_2);
    StringToConfig (data->frames_config_string[2], msg->configdata, MUICFG_CustomFrame_3);
    StringToConfig (data->frames_config_string[3], msg->configdata, MUICFG_CustomFrame_4);
    StringToConfig (data->frames_config_string[4], msg->configdata, MUICFG_CustomFrame_5);
    StringToConfig (data->frames_config_string[5], msg->configdata, MUICFG_CustomFrame_6);
    StringToConfig (data->frames_config_string[6], msg->configdata, MUICFG_CustomFrame_7);
    StringToConfig (data->frames_config_string[7], msg->configdata, MUICFG_CustomFrame_8);
    StringToConfig (data->frames_config_string[8], msg->configdata, MUICFG_CustomFrame_9);
    StringToConfig (data->frames_config_string[9], msg->configdata, MUICFG_CustomFrame_10);
    StringToConfig (data->frames_config_string[10], msg->configdata, MUICFG_CustomFrame_11);
    StringToConfig (data->frames_config_string[11], msg->configdata, MUICFG_CustomFrame_12);
    StringToConfig (data->frames_config_string[12], msg->configdata, MUICFG_CustomFrame_13);
    StringToConfig (data->frames_config_string[13], msg->configdata, MUICFG_CustomFrame_14);
    StringToConfig (data->frames_config_string[14], msg->configdata, MUICFG_CustomFrame_15);
    StringToConfig (data->frames_config_string[15], msg->configdata, MUICFG_CustomFrame_16);

    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, FramesP_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return FramesP_New(cl, obj, (struct opSet *)msg);
	case MUIM_Settingsgroup_ConfigToGadgets: return FramesP_ConfigToGadgets(cl,obj,(APTR)msg);break;
	case MUIM_Settingsgroup_GadgetsToConfig: return FramesP_GadgetsToConfig(cl,obj,(APTR)msg);break;
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Frames_desc = { 
    "Frames",
    MUIC_Group, 
    sizeof(struct MUI_FramesPData),
    (void*)FramesP_Dispatcher 
};
