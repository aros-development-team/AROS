/*
    Copyright (C) 2022-2025, The AROS Development Team.
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
#include <hidd/telemetry.h>
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** Instance Data **********************************************************/
struct PowerWindow_DATA
{
    Class           *cl;
    OOP_Object *dev;
    char            *winTitle;
    Object          *type;
    Object          *state;
    Object          *flags;
    Object          *capacity;
    Object          *rate;
    Object          *units;
    char            capacityText[32];
    char            rateText[32];
    struct Hook     graphReadHook;
};

struct PowerTelemetryEntry
{
    CONST_STRPTR id;
    LONG value;
    LONG min;
    LONG max;
    ULONG units;
    BOOL readOnly;
};

CONST_STRPTR    typeUnknown = "Unknown";
CONST_STRPTR    typeBattery = "Battery";
CONST_STRPTR    typeAC = "AC Adapter";

CONST_STRPTR    stateMissing = "Not Present";
CONST_STRPTR    stateCharging = "Charging";
CONST_STRPTR    stateDisacharging = "Discharging";

CONST_STRPTR    levelLow = "Low";
CONST_STRPTR    levelHigh = "High";
CONST_STRPTR    levelCrit = "Critical";
CONST_STRPTR    levelUnknown = " --- --- ";

CONST_STRPTR    unitsMilliAmp = "mA";
CONST_STRPTR    unitsMilliWatt = "mW";

static CONST_STRPTR PowerWindow_GetTypeString(IPTR value)
{
    switch (value)
    {
        case vHW_PowerType_Battery:
            return typeBattery;
        case vHW_PowerType_AC:
            return typeAC;
        case vHW_PowerType_Unknown:
        default:
            return typeUnknown;
    }
}

static CONST_STRPTR PowerWindow_GetStateString(IPTR value)
{
    switch (value)
    {
        case vHW_PowerState_Discharging:
            return stateDisacharging;
        case vHW_PowerState_Charging:
            return stateCharging;
        case vHW_PowerState_NotPresent:
        default:
            return stateMissing;
    }
}

static CONST_STRPTR PowerWindow_GetFlagsString(IPTR value)
{
    switch (value)
    {
        case vHW_PowerFlag_Low:
            return levelLow;
        case vHW_PowerFlag_High:
            return levelHigh;
        case vHW_PowerFlag_Critical:
            return levelCrit;
        case vHW_PowerFlag_Unknown:
        default:
            return levelUnknown;
    }
}

static CONST_STRPTR PowerWindow_GetUnitsString(IPTR value)
{
    switch (value)
    {
        case vHW_PowerUnit_mA:
            return unitsMilliAmp;
        case vHW_PowerUnit_mW:
            return unitsMilliWatt;
        default:
            return typeUnknown;
    }
}

static BOOL PowerWindow_IsPowerRawEntry(CONST_STRPTR id)
{
    if (!id)
        return FALSE;

    return (strcmp(id, "Capacity") == 0) ||
        (strcmp(id, "Full Capacity") == 0) ||
        (strcmp(id, "Rate") == 0);
}

static char *PowerWindow_AllocString(const char *fmt, ...)
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

static CONST_STRPTR PowerWindow_TelemetryUnitToString(ULONG units)
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

static char *PowerWindow_FormatTelemetryValue(LONG value, ULONG units)
{
    CONST_STRPTR unitStr = PowerWindow_TelemetryUnitToString(units);

    if (unitStr)
    {
        if (units <= vHW_TelemetryUnit_Raw)
            return PowerWindow_AllocString("%ld (%s)", value, unitStr);
        return PowerWindow_AllocString("%ld%s", value, unitStr);
    }

    return PowerWindow_AllocString("%ld", value);
}

static char *PowerWindow_FormatTelemetryValueWithPowerUnits(LONG value, ULONG units, IPTR powerUnits,
    BOOL usePowerUnits)
{
    if (usePowerUnits && units == vHW_TelemetryUnit_Raw)
    {
        CONST_STRPTR unitStr = PowerWindow_GetUnitsString(powerUnits);

        if (unitStr && unitStr != typeUnknown)
            return PowerWindow_AllocString("%ld%s", value, unitStr);
    }

    return PowerWindow_FormatTelemetryValue(value, units);
}

static BOOL PowerWindow_GetTelemetryEntry(OOP_Object *dev, struct SysexpPowerBase *PowerBase, ULONG index,
    struct PowerTelemetryEntry *entry)
{
    struct TagItem entryTags[] =
    {
        { tHidd_Telemetry_EntryID, (IPTR)&entry->id },
        { tHidd_Telemetry_EntryUnits, (IPTR)&entry->units },
        { tHidd_Telemetry_EntryMin, (IPTR)&entry->min },
        { tHidd_Telemetry_EntryMax, (IPTR)&entry->max },
        { tHidd_Telemetry_EntryValue, (IPTR)&entry->value },
        { tHidd_Telemetry_EntryReadOnly, (IPTR)&entry->readOnly },
        { TAG_DONE, 0 }
    };

    entry->id = NULL;
    entry->units = vHW_TelemetryUnit_Unknown;
    entry->min = 0;
    entry->max = 0;
    entry->value = 0;
    entry->readOnly = TRUE;

    return HIDD_Telemetry_GetEntryAttribs(dev, index, entryTags);
}

static BOOL PowerWindow_FindTelemetryEntry(struct PowerWindow_DATA *data, CONST_STRPTR id,
    struct PowerTelemetryEntry *entry)
{
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)data->cl->cl_UserData;
    OOP_Object *dev = data->dev;
    IPTR count = 0;
    ULONG i;

    OOP_GetAttr(dev, aHidd_Telemetry_EntryCount, &count);
    for (i = 0; i < (ULONG)count; i++)
    {
        if (!PowerWindow_GetTelemetryEntry(dev, PowerBase, i, entry))
            continue;
        if (entry->id && strcmp(entry->id, id) == 0)
            return TRUE;
    }

    return FALSE;
}

static Object *PowerWindow_CreateTelemetryGroup(OOP_Object *dev, struct SysexpPowerBase *PowerBase)
{
    Object *telemetryGroup;
    IPTR count = 0;
    IPTR powerUnits = 0;
    ULONG i;

    OOP_GetAttr(dev, aHidd_Telemetry_EntryCount, &count);
    if (count == 0)
        return NULL;

    OOP_GetAttr(dev, aHidd_Power_Units, &powerUnits);

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
        for (i = 0; i < (ULONG)count; i++)
        {
            struct PowerTelemetryEntry entry;
            char *valueStr;
            char *minmaxStr;

            if (!PowerWindow_GetTelemetryEntry(dev, PowerBase, i, &entry))
                continue;

            if (entry.id)
            {
                DoMethod(telemetryGroup, OM_ADDMEMBER, Label(entry.id));
            }
            else
            {
                char *label = PowerWindow_AllocString("Entry %lu", (unsigned long)(i + 1));
                DoMethod(telemetryGroup, OM_ADDMEMBER, Label(label ? label : (char *)"Entry"));
            }

            valueStr = PowerWindow_FormatTelemetryValueWithPowerUnits(
                entry.value,
                entry.units,
                powerUnits,
                PowerWindow_IsPowerRawEntry(entry.id));
            if (!valueStr)
                valueStr = (char *)"";
            DoMethod(telemetryGroup, OM_ADDMEMBER,
                TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)valueStr,
                End);

            minmaxStr = PowerWindow_AllocString("%ld / %ld", entry.min, entry.max);
            if (!minmaxStr)
                minmaxStr = (char *)"";
            DoMethod(telemetryGroup, OM_ADDMEMBER, Label("Min/Max"));
            DoMethod(telemetryGroup, OM_ADDMEMBER,
                TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)minmaxStr,
                End);
        }

        DoMethod(telemetryGroup, MUIM_Group_ExitChange);
    }

    return telemetryGroup;
}

static void PowerWindow_UpdateDetails(struct PowerWindow_DATA *data)
{
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)data->cl->cl_UserData;
    IPTR val = 0;
    IPTR type = vHW_PowerType_Unknown;
    IPTR units = 0;
    CONST_STRPTR unitsStr = NULL;

    OOP_GetAttr(data->dev, aHidd_Power_Type, &type);
    if (data->type)
        SET(data->type, MUIA_Text_Contents, PowerWindow_GetTypeString(type));

    OOP_GetAttr(data->dev, aHidd_Power_State, &val);
    if (data->state)
        SET(data->state, MUIA_Text_Contents, PowerWindow_GetStateString(val));

    OOP_GetAttr(data->dev, aHidd_Power_Flags, &val);
    if (data->flags)
        SET(data->flags, MUIA_Text_Contents, PowerWindow_GetFlagsString(val));

    OOP_GetAttr(data->dev, aHidd_Power_Units, &units);
    if (data->units)
        SET(data->units, MUIA_Text_Contents, PowerWindow_GetUnitsString(units));
    unitsStr = PowerWindow_GetUnitsString(units);

    {
        struct PowerTelemetryEntry entry;

        if (PowerWindow_FindTelemetryEntry(data, "Capacity", &entry))
        {
            if (unitsStr && unitsStr != typeUnknown)
                snprintf(data->capacityText, sizeof(data->capacityText), "%ld%s", (long)entry.value, unitsStr);
            else
                snprintf(data->capacityText, sizeof(data->capacityText), "%ld", (long)entry.value);
        }
        else
            snprintf(data->capacityText, sizeof(data->capacityText), "n/a");

        if (PowerWindow_FindTelemetryEntry(data, "Rate", &entry))
        {
            if (unitsStr && unitsStr != typeUnknown)
                snprintf(data->rateText, sizeof(data->rateText), "%ld%s", (long)entry.value, unitsStr);
            else
                snprintf(data->rateText, sizeof(data->rateText), "%ld", (long)entry.value);
        }
        else
            snprintf(data->rateText, sizeof(data->rateText), "n/a");
    }

    if (data->capacity)
        SET(data->capacity, MUIA_Text_Contents, data->capacityText);
    if (data->rate)
        SET(data->rate, MUIA_Text_Contents, data->rateText);
}

AROS_UFH3(IPTR, GraphUpdateFunc,
        AROS_UFHA(struct Hook *, procHook, A0),
        AROS_UFHA(IPTR *, storage, A2),
        AROS_UFHA(struct PowerWindow_DATA *, data, A1))
{
    AROS_USERFUNC_INIT
    
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)data->cl->cl_UserData;
    IPTR val = 0;

    D(bug("[power:sysexp] %s()\n", __func__);)

    PowerWindow_UpdateDetails(data);

    {
        struct PowerTelemetryEntry entry;

        if (PowerWindow_FindTelemetryEntry(data, "Charge", &entry))
            *storage = entry.value;
        else
            *storage = 0;
    }

    D(bug("[power:sysexp] %s: 0x%p = %d\n", __func__, storage, *storage);)

    return TRUE;

    AROS_USERFUNC_EXIT
}

static Object *PowerWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpPowerBase *PowerBase = (struct SysexpPowerBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    Object *winObj, *windowObjGrp = NULL, *battGraphObj = NULL;
    Object *typeObj = NULL, *stateObj = NULL, *flagsObj = NULL;
    Object *capacityObj = NULL, *rateObj = NULL, *unitsObj = NULL;
    Object *telemetryGroup = NULL;
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
                typestr = (char *)typeBattery;
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
                        MUIA_FrameTitle, (IPTR)"Power Details",
                        GroupFrame,
                        MUIA_Background, MUII_GroupBack,
                        Child, (IPTR)Label("Type"),
                        Child, (IPTR)(typeObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("State"),
                        Child, (IPTR)(stateObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("Flags"),
                        Child, (IPTR)(flagsObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("Capacity"),
                        Child, (IPTR)(capacityObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("Rate"),
                        Child, (IPTR)(rateObj = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)Label("Units"),
                        Child, (IPTR)(unitsObj = TextObject,
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
            typestr = (char *)typeAC;
            windowObjGrp = ColGroup(2),
                MUIA_Group_SameSize, TRUE,
                MUIA_FrameTitle, (IPTR)typestr,
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label("Type"),
                Child, (IPTR)(typeObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
                Child, (IPTR)Label("State"),
                Child, (IPTR)(stateObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
                Child, (IPTR)Label("Flags"),
                Child, (IPTR)(flagsObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
                Child, (IPTR)Label("Capacity"),
                Child, (IPTR)(capacityObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
                Child, (IPTR)Label("Rate"),
                Child, (IPTR)(rateObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)"",
                End),
                Child, (IPTR)Label("Units"),
                Child, (IPTR)(unitsObj = TextObject,
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

    telemetryGroup = PowerWindow_CreateTelemetryGroup(dev, PowerBase);
    if (telemetryGroup && windowObjGrp)
    {
        windowObjGrp = (Object *)VGroup,
            Child, (IPTR)windowObjGrp,
            Child, (IPTR)telemetryGroup,
        End;
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
        data->type = typeObj;
        data->state = stateObj;
        data->flags = flagsObj;
        data->capacity = capacityObj;
        data->rate = rateObj;
        data->units = unitsObj;
        if (battGraphObj)
        {
            APTR graphDataSource = (APTR)DoMethod(battGraphObj, MUIM_Graph_GetSourceHandle, 0);
            data->graphReadHook.h_Entry = (APTR)GraphUpdateFunc;
            data->graphReadHook.h_Data = (APTR)data;
            DoMethod(battGraphObj, MUIM_Graph_SetSourceAttrib, graphDataSource, MUIV_Graph_Source_ReadHook, &data->graphReadHook);
        }
        PowerWindow_UpdateDetails(data);
    }
    return winObj;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    PowerWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
