/*
    Copyright (C) 2025, The AROS Development Team.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            return NULL;
    }
}
static char *Telemetry_AllocString(const char *fmt, ...)
{
    va_list args;
    char buffer[128];
    char *out;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    out = AllocVec(strlen(buffer) + 1, MEMF_PUBLIC);
    if (!out)
        return NULL;

    strcpy(out, buffer);
    return out;
}

static char *Telemetry_FormatValue(LONG value, ULONG units)
{
    const char *unitStr = TelemetryUnitToString(units);

    if (unitStr)
    {
        if (units <= vHW_TelemetryUnit_Raw)
            return Telemetry_AllocString("%ld (%s)", value, unitStr);
        return Telemetry_AllocString("%ld%s", value, unitStr);
    }

    return Telemetry_AllocString("%ld", value);
}

static char *Telemetry_FormatLabel(const char *base, ULONG index, ULONG total)
{
    if (total > 1)
        return Telemetry_AllocString("%s #%lu", base, (unsigned long)(index + 1));
    return Telemetry_AllocString("%s", base);
}

static Object *Telemetry_CreateTelemetryGroup(OOP_Object *dev, struct SysexpTelemetryBase *SeTelemetryBase, ULONG valueCount)
{
    Object *telemetryGroup;
    ULONG i;

    telemetryGroup = MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Columns, 2,
        MUIA_Group_SameSize, TRUE,
        MUIA_FrameTitle, (IPTR)"Telemetry",
        GroupFrame,
        MUIA_Background, MUII_GroupBack,
        TAG_DONE);

    if (!telemetryGroup)
        return NULL;

    if (DoMethod(telemetryGroup, MUIM_Group_InitChange))
    {
        for (i = 0; i < valueCount; i++)
        {
            IPTR value = 0;
            IPTR min = 0;
            IPTR max = 0;
            IPTR units = 0;
            BOOL readOnly = TRUE;
            CONST_STRPTR valueId;
            char *valueLabel;
            char *valueStr;
            struct TagItem entryTags[] =
            {
                { tHidd_Telemetry_EntryID, (IPTR)&valueId },
                { tHidd_Telemetry_EntryUnits, (IPTR)&units },
                { tHidd_Telemetry_EntryMin, (IPTR)&min },
                { tHidd_Telemetry_EntryMax, (IPTR)&max },
                { tHidd_Telemetry_EntryValue, (IPTR)&value },
                { tHidd_Telemetry_EntryReadOnly, (IPTR)&readOnly },
                { TAG_DONE, 0 }
            };

            valueId = NULL;
            units = vHW_TelemetryUnit_Unknown;
            value = 0;
            min = 0;
            max = 0;
            readOnly = TRUE;

            if (!HIDD_Telemetry_GetEntryAttribs(dev, i, entryTags))
                continue;

            if (valueId)
            {
                char *idLabel = Telemetry_FormatLabel("ID", i, valueCount);
                if (!idLabel)
                    idLabel = (char *)"ID";
                DoMethod(telemetryGroup, OM_ADDMEMBER, Label(idLabel));
                DoMethod(telemetryGroup, OM_ADDMEMBER,
                    TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)valueId,
                    End);
            }

            valueLabel = Telemetry_FormatLabel("Value", i, valueCount);
            valueStr = Telemetry_FormatValue((LONG)value, (ULONG)units);
            if (!valueLabel)
                valueLabel = (char *)"Value";
            if (!valueStr)
                valueStr = (char *)"";
            DoMethod(telemetryGroup, OM_ADDMEMBER, Label(valueLabel));
            DoMethod(telemetryGroup, OM_ADDMEMBER,
                TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)valueStr,
                End);

            {
                char *minmaxLabel = Telemetry_FormatLabel("Min/Max", i, valueCount);
                char *minmaxStr = Telemetry_AllocString("%ld / %ld", (LONG)min, (LONG)max);
                if (!minmaxLabel)
                    minmaxLabel = (char *)"Min/Max";
                if (!minmaxStr)
                    minmaxStr = (char *)"";
                DoMethod(telemetryGroup, OM_ADDMEMBER, Label(minmaxLabel));
                DoMethod(telemetryGroup, OM_ADDMEMBER,
                    TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)minmaxStr,
                    End);
            }

            {
                char *accessLabel = Telemetry_FormatLabel("Access", i, valueCount);
                CONST_STRPTR accessStr = readOnly ? "Read-only" : "Read/Write";
                if (!accessLabel)
                    accessLabel = (char *)"Access";
                DoMethod(telemetryGroup, OM_ADDMEMBER, Label(accessLabel));
                DoMethod(telemetryGroup, OM_ADDMEMBER,
                    TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)accessStr,
                    End);
            }
        }

        DoMethod(telemetryGroup, MUIM_Group_ExitChange);
    }

    return telemetryGroup;
}

static Object *TelemetryWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpTelemetryBase *SeTelemetryBase = (struct SysexpTelemetryBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    IPTR valueCount = 0;
    Object *telemetryGroup;

    if (!dev)
        return NULL;

    OOP_GetAttr(dev, aHidd_Telemetry_EntryCount, &valueCount);
    telemetryGroup = Telemetry_CreateTelemetryGroup(dev, SeTelemetryBase, (ULONG)valueCount);
    if (!telemetryGroup)
        return NULL;

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, (IPTR)"Device Properties",
        MUIA_Window_ID, MAKE_ID('T', 'E', 'L', 'M'),
        WindowContents, (IPTR)(VGroup,
            Child, (IPTR)(BOOPSIOBJMACRO_START(SeTelemetryBase->sesb_DevicePageCLASS->mcc_Class),
                MUIA_PropertyWin_Object, (IPTR)dev,
            End),
            Child, (IPTR)telemetryGroup,
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
