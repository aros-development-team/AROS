/*
    Copyright (C) 2022, The AROS Development Team.
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/sysexp.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <zune/graph.h>
#include <mui/NFloattext_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include "locale.h"

#include "power_classes.h"
#include "power_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (0) // TODO : Move into libbase
extern OOP_AttrBase HiddPowerAB;
#endif

/*** Instance Data **********************************************************/
struct PowerWindow_DATA
{
    Class           *cl;
    OOP_Object *dev;
    char            *winTitle;
    Object          *status;
    Object          *level;
    struct Hook     graphReadHook;
};

CONST_STRPTR    stateMissing = "Not Present";
CONST_STRPTR    stateCharging = "Charging";
CONST_STRPTR    stateDisacharging = "Discharging";

CONST_STRPTR    levelLow = "Low";
CONST_STRPTR    levelHigh = "High";
CONST_STRPTR    levelCrit = "Critical";
CONST_STRPTR    levelUnknown = " --- --- ";

AROS_UFH3(IPTR, GraphUpdateFunc,
        AROS_UFHA(struct Hook *, procHook, A0),
        AROS_UFHA(IPTR *, storage, A2),
        AROS_UFHA(struct PowerWindow_DATA *, data, A1))
{
    AROS_USERFUNC_INIT
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)data->cl->cl_UserData;
    CONST_STRPTR pwrState, pwrLevel;
    IPTR val = 0;

    D(bug("[power:sysexp] %s()\n", __func__);)

    if (data->status)
    {
        OOP_GetAttr(data->dev, aHidd_Power_State, &val);
        switch (val)
        {
            case vHW_PowerState_Discharging:
                pwrState = stateDisacharging;
                break;
            case vHW_PowerState_Charging:
                pwrState = stateCharging;
                break;
            case vHW_PowerState_NotPresent:
            default:
                pwrState = stateMissing;
                break;
        }
        SET(data->status, MUIA_Text_Contents, pwrState);
    }
    if (data->level)
    {
        OOP_GetAttr(data->dev, aHidd_Power_Flags, &val);
        switch (val)
        {
            case vHW_PowerFlag_Low:
                pwrLevel = levelLow;
                break;
            case vHW_PowerFlag_High:
                pwrLevel = levelHigh;
                break;
            case vHW_PowerFlag_Critical:
                pwrLevel = levelCrit;
                break;
            case vHW_PowerFlag_Unknown:
            default:
                pwrLevel = levelUnknown;
                break;
        }
        SET(data->level, MUIA_Text_Contents, pwrLevel);
    }

    OOP_GetAttr(data->dev, aHidd_Power_Capacity, &val);
    *storage = val;

    D(bug("[power:sysexp] %s: 0x%p = %d\n", __func__, storage, *storage);)

    return TRUE;

    AROS_USERFUNC_EXIT
}

static Object *PowerWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    Object *winObj, *windowObjGrp = NULL, *battGraphObj = NULL, *statusObj = NULL, *levelObj = NULL;
    char title_str[32], *typestr = NULL, *wintitle;
    IPTR val;

    D(bug("[power.sysexp] %s: cl @ %p\n", __func__, cl));

    if ((!dev))
        return NULL;

    OOP_GetAttr(dev, aHidd_Power_Type, &val);
    switch (val)
    {
        case vHW_PowerType_Battery:
            {
                typestr = "Battery";
                windowObjGrp = VGroup,
                    Child, (IPTR)(battGraphObj = GraphObject,
                        MUIA_Graph_InfoText,        (IPTR)"",
                        MUIA_Graph_ValueCeiling,    1000,
                        MUIA_Graph_ValueStep,       100,
                        MUIA_Graph_PeriodCeiling,   100000,
                        MUIA_Graph_PeriodStep,      10000,
                        MUIA_Graph_PeriodInterval,  1000,
                    End),
                    Child, (IPTR)(ColGroup(2),
                        MUIA_Group_SameSize, TRUE,
                        MUIA_FrameTitle, (IPTR)typestr,
                        GroupFrame,
                        MUIA_Background, MUII_GroupBack,
                        Child, (IPTR)Label("Status"),
                        Child, (IPTR)(statusObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("Level"),
                        Child, (IPTR)(levelObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                    End),
                End;
            }
            break;
        case vHW_PowerType_AC:
            typestr = "AC Adapter";
            windowObjGrp = ColGroup(2),
                MUIA_Group_SameSize, TRUE,
                MUIA_FrameTitle, (IPTR)typestr,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label("Status"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
            End;
            break;
       case vHW_PowerType_Unknown:
       default:
            windowObjGrp = ColGroup(2),
                Child, (IPTR)HVSpace,
                Child, (IPTR)HVSpace,
            End;
           break;
    }
    if (typestr)
    {
        sprintf(title_str, "%s Properties", typestr);
    }
    else
    {
        sprintf(title_str, "Unknown Power Device");
    }
    wintitle = AllocVec(strlen(title_str) + 1, MEMF_ANY);
    CopyMem(title_str, wintitle, strlen(title_str) + 1);

    winObj = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, (IPTR)wintitle,
        MUIA_Window_ID, MAKE_ID('S', 'P', 'D', 'P'),
        WindowContents, (IPTR)(VGroup,
            Child, (IPTR)(BOOPSIOBJMACRO_START(PowerBase->sesb_DevicePageCLASS->mcc_Class),
                MUIA_PropertyWin_Object, (IPTR)dev,
            End),
            Child, (IPTR)windowObjGrp,
            TAG_MORE, (IPTR)msg->ops_AttrList,
        End),
        TAG_DONE
    );
    if (winObj)
    {
        struct PowerWindow_DATA *data = INST_DATA(cl, winObj);
        data->cl = cl;
        data->dev = dev;
        data->winTitle = wintitle;
        data->status = statusObj;
        data->level = levelObj;
        if (battGraphObj)
        {
            APTR graphDataSource = (APTR)DoMethod(battGraphObj, MUIM_Graph_GetSourceHandle, 0);
            data->graphReadHook.h_Entry = (APTR)GraphUpdateFunc;
            data->graphReadHook.h_Data = (APTR)data;
            DoMethod(battGraphObj, MUIM_Graph_SetSourceAttrib, graphDataSource, MUIV_Graph_Source_ReadHook, &data->graphReadHook);
        }
    }
    return winObj;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    PowerWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
