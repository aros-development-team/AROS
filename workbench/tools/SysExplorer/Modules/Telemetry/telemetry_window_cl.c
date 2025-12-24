/*
    Copyright (C) 2025, The AROS Development Team.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "locale.h"

#include "telemetry_classes.h"
#include "telemetry_intern.h"

#include <zune/customclasses.h>

/*** Instance Data **********************************************************/
struct TelemetryWindow_DATA
{
    /* Nothing to add */
};

static CONST_STRPTR TelemetryUnitToString(ULONG units)
{
    switch (units)
    {
        case vHW_TelemetryUnit_Boolean:
            return "Boolean";
        case vHW_TelemetryUnit_Raw:
            return "Raw";
        case vHW_TelemetryUnit_Percent:
            return "%";
        case vHW_TelemetryUnit_RPM:
            return "RPM";
        case vHW_TelemetryUnit_Celsius:
            return "Celsius";
        case vHW_TelemetryUnit_Volts:
            return "Volts";
        case vHW_TelemetryUnit_Amps:
            return "Amps";
        case vHW_TelemetryUnit_Watts:
            return "Watts";
        case vHW_TelemetryUnit_Unknown:
        default:
            return "Unknown";
    }
}

static Object *TelemetryWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpTelemetryBase *SeTelemetryBase = (struct SysexpTelemetryBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    const char *unitStr;
    IPTR value = 0;
    IPTR min = 0;
    IPTR max = 0;
    IPTR units = 0;
    char valueStr[32];
    char minStr[32];
    char maxStr[32];

    if (!dev)
        return NULL;

    OOP_GetAttr(dev, aHidd_Telemetry_Value, &value);
    OOP_GetAttr(dev, aHidd_Telemetry_Min, &min);
    OOP_GetAttr(dev, aHidd_Telemetry_Max, &max);
    OOP_GetAttr(dev, aHidd_Telemetry_Units, &units);

    sprintf(valueStr, "%ld", (LONG)value);
    sprintf(minStr, "%ld", (LONG)min);
    sprintf(maxStr, "%ld", (LONG)max);
    unitStr = TelemetryUnitToString((ULONG)units);

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, (IPTR)"Telemetry Properties",
        MUIA_Window_ID, MAKE_ID('T', 'E', 'L', 'M'),
        WindowContents, (IPTR)(VGroup,
            Child, (IPTR)(BOOPSIOBJMACRO_START(SeTelemetryBase->sesb_DevicePageCLASS->mcc_Class),
                MUIA_PropertyWin_Object, (IPTR)dev,
            End),
            Child, (IPTR)(ColGroup(2),
                MUIA_Group_SameSize, TRUE,
                MUIA_FrameTitle, (IPTR)"Telemetry",
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label("Value"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)valueStr,
                End),
                Child, (IPTR)Label("Minimum"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)minStr,
                End),
                Child, (IPTR)Label("Maximum"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)maxStr,
                End),
                Child, (IPTR)Label("Units"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)unitStr,
                End),
            End),
            TAG_MORE, (IPTR)msg->ops_AttrList,
        End),
        TAG_DONE
    );

    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    TelemetryWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
