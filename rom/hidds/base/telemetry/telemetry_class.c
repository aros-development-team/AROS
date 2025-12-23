/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <utility/tagitem.h>
#include "telemetry_intern.h"

/*** Telemetry::New() ***********************************************************/

static struct TagItem *Telemetry_NextTagItem(struct TagItem **taglist)
{
    struct TagItem *tag = *taglist;

    while (tag)
    {
        switch (tag->ti_Tag)
        {
        case TAG_DONE:
            *taglist = NULL;
            return NULL;
        case TAG_IGNORE:
            tag++;
            break;
        case TAG_MORE:
            tag = (struct TagItem *)tag->ti_Data;
            break;
        case TAG_SKIP:
            tag += tag->ti_Data + 1;
            break;
        default:
            *taglist = tag + 1;
            return tag;
        }
    }

    return NULL;
}

# define base CSD(cl)

OOP_Object *Telemetry__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct HIDDTelemetryData *data;
    struct TagItem *tstate;
    struct TagItem *tag;

    D(bug("[Telemetry] Root__New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        data = OOP_INST_DATA(cl, o);
        data->telemetry_Value = 0;
        data->telemetry_Min = 0;
        data->telemetry_Max = 0;
        data->telemetry_Units = vHW_TelemetryUnit_Unknown;

        tstate = msg->attrList;
        while ((tag = Telemetry_NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_Telemetry_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_Telemetry_Value:
                data->telemetry_Value = (LONG)tag->ti_Data;
                break;
            case aoHidd_Telemetry_Min:
                data->telemetry_Min = (LONG)tag->ti_Data;
                break;
            case aoHidd_Telemetry_Max:
                data->telemetry_Max = (LONG)tag->ti_Data;
                break;
            case aoHidd_Telemetry_Units:
                data->telemetry_Units = (ULONG)tag->ti_Data;
                break;
            }
        }
    }

    D(bug ("[Telemetry] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

/*** Telemetry::Dispose() **********************************************************/
VOID Telemetry__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Telemetry] Root__Dispose(0x%p)\n", o));
    OOP_DoSuperMethod(cl, o, msg);
}

/*** Telemetry::Get() **************************************************************/

VOID Telemetry__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDTelemetryData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_Value:
        *msg->storage = (IPTR)data->telemetry_Value;
        return;
    case aoHidd_Telemetry_Min:
        *msg->storage = (IPTR)data->telemetry_Min;
        return;
    case aoHidd_Telemetry_Max:
        *msg->storage = (IPTR)data->telemetry_Max;
        return;
    case aoHidd_Telemetry_Units:
        *msg->storage = (IPTR)data->telemetry_Units;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*** Telemetry::Set() **************************************************************/

VOID Telemetry__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct HIDDTelemetryData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;

    while ((tag = Telemetry_NextTagItem(&tstate)))
    {
        ULONG idx;

        Hidd_Telemetry_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_Telemetry_Value:
            data->telemetry_Value = (LONG)tag->ti_Data;
            break;
        case aoHidd_Telemetry_Min:
            data->telemetry_Min = (LONG)tag->ti_Data;
            break;
        case aoHidd_Telemetry_Max:
            data->telemetry_Max = (LONG)tag->ti_Data;
            break;
        case aoHidd_Telemetry_Units:
            data->telemetry_Units = (ULONG)tag->ti_Data;
            break;
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
