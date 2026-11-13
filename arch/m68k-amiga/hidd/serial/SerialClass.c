/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Paula serial hidd class implementation.
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>

#include "serial_intern.h"

#include <aros/debug.h>

/*** HIDDSerial::NewUnit() *************************************************/

OOP_Object *AmigaSer__Hidd_Serial__NewUnit(OOP_Class *cl, OOP_Object *obj,
                                           struct pHidd_Serial_NewUnit *msg)
{
    struct class_static_data *csd = CSD(cl->UserData);
    struct HIDDSerialData *data = OOP_INST_DATA(cl, obj);
    OOP_Object *su = NULL;
    ULONG unitnum = -1;

    EnterFunc(bug("HIDDSerial::NewUnit()\n"));

    D(bug("Request for unit number %d\n", msg->unitnum));

    if (msg->unitnum >= 0 && msg->unitnum < SER_MAX_UNITS)
    {
        unitnum = msg->unitnum;

        if (0 != (data->usedunits & (1 << unitnum)))
            unitnum = -1;
    }
    else if (msg->unitnum == -1)
    {
        /* search for the next available unit */
        unitnum = 0;

        while (unitnum < SER_MAX_UNITS)
        {
            if (0 == (data->usedunits & (1 << unitnum)))
                break;

            unitnum++;
        }
    }

    if (unitnum >= 0 && unitnum < SER_MAX_UNITS)
    {
        struct TagItem tags[] =
        {
            {aHidd_SerialUnit_Unit, unitnum},
            {TAG_DONE                      }
        };

        su = OOP_NewObject(NULL, CLID_Hidd_SerialUnit, tags);
        data->SerialUnits[unitnum] = su;

        /* Mark it as used */
        data->usedunits |= (1 << unitnum);
    }

    ReturnPtr("HIDDSerial::NewUnit", OOP_Object *, su);
}

/*** HIDDSerial::DisposeUnit() *********************************************/

VOID AmigaSer__Hidd_Serial__DisposeUnit(OOP_Class *cl, OOP_Object *obj,
                                        struct pHidd_Serial_DisposeUnit *msg)
{
    struct HIDDSerialData *data = OOP_INST_DATA(cl, obj);
    OOP_Object *su = msg->unit;

    EnterFunc(bug("HIDDSerial::DisposeUnit()\n"));

    if (su)
    {
        ULONG unitnum = 0;

        while (unitnum < SER_MAX_UNITS)
        {
            if (data->SerialUnits[unitnum] == su)
            {
                D(bug("Disposing SerialUnit!\n"));
                OOP_DisposeObject(su);
                data->SerialUnits[unitnum] = NULL;
                data->usedunits &= ~(1 << unitnum);
                break;
            }

            unitnum++;
        }
    }

    ReturnVoid("HIDDSerial::DisposeUnit");
}
