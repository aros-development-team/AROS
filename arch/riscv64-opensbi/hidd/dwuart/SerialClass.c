/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Serial hidd class implementation.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <aros/config.h>
#include <aros/symbolsets.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#include <aros/debug.h>

/*** HIDDSerial::NewUnit() *********************************************************/

OOP_Object *DWSer__Hidd_Serial__NewUnit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_NewUnit *msg)
{
  struct class_static_data *csd = (&((struct IntHIDDSerialBase *)cl->UserData)->hdg_csd);
  struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
  OOP_Object *su = NULL;
  ULONG unitnum = -1;

  EnterFunc(bug("HIDDSerial::NewSerial()\n"));

  D(bug("[Serial:DW] %s: Request for unit number %d\n", __func__, msg->unitnum));

#if (AROS_SERIAL_DEBUG > 0)
    if (msg->unitnum == (AROS_SERIAL_DEBUG-1))
        ReturnPtr("HIDDSerial::NewSerial", OOP_Object *, su);
#endif

  switch (msg->unitnum)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      unitnum = msg->unitnum;
      if (0 != (data->usedunits & (1 << unitnum)))
        unitnum = -1;
    break;
    
    case -1: /* search for the next available unit */
    {
      unitnum = 0;
      while (unitnum < csd->nports)
      {
        if (0 == (data->usedunits & (1 << unitnum)))
          break;
        unitnum++;
      }
    }
    break;


  }

  /* Only the ports the device tree described can be handed out */
  if (unitnum < csd->nports)
  {
    struct TagItem tags[] =
    {
        {aHidd_SerialUnit_Unit, unitnum},
        {TAG_DONE                      }
    };

    su = OOP_NewObject(NULL, CLID_Hidd_SerialUnit, tags);
    if (su)
    {
      data->SerialUnits[unitnum] = su;
      data->usedunits |= (1 << unitnum);
    }
  }

  ReturnPtr("HIDDSerial::NewSerial", OOP_Object *, su);
}


/*** HIDDSerial::DisposeUnit() ****************************************************/

VOID DWSer__Hidd_Serial__DisposeUnit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_DisposeUnit *msg)
{
  struct class_static_data *csd = (&((struct IntHIDDSerialBase *)cl->UserData)->hdg_csd);
  struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
  OOP_Object * su = msg->unit;

  EnterFunc(bug("HIDDSerial::DisposeUnit()\n"));

  if(su)
  {
    ULONG unitnum = 0;
    while (unitnum < SER_MAX_UNITS)
    {
      if (data->SerialUnits[unitnum] == su)
      {
        D(bug("[Serial:DW] %s: Disposing SerialUnit!\n", __func__));
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

/******* SerialUnit::New() ***********************************/
OOP_Object *DWSer__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
  struct class_static_data *csd = (&((struct IntHIDDSerialBase *)cl->UserData)->hdg_csd);
  struct pRoot_New serhidNew;
  struct TagItem serhidTags[] =
  {
      { aHidd_Name,           (IPTR)"serial.hidd"                            },
      { aHidd_HardwareName,   (IPTR)"DesignWare APB UART Controller"         },
      { TAG_DONE,             0                                              }
  };
  /* Chain on the caller's tags, keeping ours rather than replacing one */
  if (msg->attrList)
  {
      serhidTags[2].ti_Tag  = TAG_MORE;
      serhidTags[2].ti_Data = (IPTR)msg->attrList;
  }
  serhidNew.mID      = msg->mID;
  serhidNew.attrList = serhidTags;
  return (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)&serhidNew);
}
