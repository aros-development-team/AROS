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
    char *winTitle;
};

static Object *PowerWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    Object *windowObjGrp = NULL;
    char title_str[32], *typestr = NULL, *wintitle;
    IPTR val;

    D(bug("[power.sysexp] %s: cl @ %p\n", __func__, cl));

    if ((!dev))
        return NULL;

    OOP_GetAttr(dev, aHidd_Power_Type, &val);
    switch (val)
    {
        case vHW_PowerType_Battery:
            typestr = "Battery";
            windowObjGrp = VGroup,
                Child, (IPTR)GraphObject,
                    MUIA_Graph_InfoText,        (IPTR)"",
                    MUIA_Graph_ValueCeiling,    1000,
                    MUIA_Graph_ValueStep,       100,
                    MUIA_Graph_PeriodCeiling,   100000,
                    MUIA_Graph_PeriodStep,      10000,
                    MUIA_Graph_PeriodInterval,  1000,
                    MUIA_UserData,              0,
                End,
                Child, (IPTR)(ColGroup(2),
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
                    Child, (IPTR)Label("Level"),
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)"",
                    End),
                End),
            End;
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

    return (Object *) DoSuperNewTags
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
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    PowerWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
