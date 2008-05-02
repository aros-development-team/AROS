/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd class implementation.
    Lang: english
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

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


/*** HIDDSerial::NewUnit() *********************************************************/

OOP_Object *PCSer__Hidd_Serial__NewUnit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_NewUnit *msg)
{
  OOP_Object *su = NULL;
  struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
  ULONG unitnum = -1;

  EnterFunc(bug("HIDDSerial::NewSerial()\n"));

  D(bug("Request for unit number %d\n",msg->unitnum));

#if (AROS_SERIAL_DEBUG > 0)
    if (msg->unitnum == (AROS_SERIAL_DEBUG-1))
	ReturnPtr("HIDDSerial::NewSerial", Object *, su);
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
      while (unitnum < SER_MAX_UNITS)
      {
        if (0 == (data->usedunits & (1 << unitnum)))
          break;
        unitnum++;
      }
    }
    break;
    
    
  }

  if (unitnum >= 0 && unitnum < SER_MAX_UNITS)
  {
    struct TagItem tags[] =
    {
#define csd CSD(cl->UserData)
        {aHidd_SerialUnit_Unit, unitnum},
#undef csd
	{TAG_DONE		       }
    };

    su = OOP_NewObject(NULL, CLID_Hidd_SerialUnit, tags);
    data->SerialUnits[unitnum] = su;
    /*
    ** Mark it as used
    */
    data->usedunits |= (1 << unitnum); 
  }

  ReturnPtr("HIDDSerial::NewSerial", Object *, su);
}


/*** HIDDSerial::DisposeUnit() ****************************************************/

VOID PCSer__Hidd_Serial__DisposeUnit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_DisposeUnit *msg)
{
  OOP_Object * su = msg->unit;
  struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
  EnterFunc(bug("HIDDSerial::DisposeUnit()\n"));

  if(su)
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



/*************************** Classes *****************************/

#define csd (&(LIBBASE->hdg_csd))

static int PCSer_InitAttrs(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCSer_InitAttrs\n"));

    __IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);

    ReturnInt("PCSer_InitAttrs", ULONG, __IHidd_SerialUnitAB != NULL);
}


static int PCSer_ExpungeAttrs(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCSer_ExpungeAttrs\n"));

    OOP_ReleaseAttrBase(__IHidd_SerialUnitAB);
	
    ReturnInt("PCSer_ExpungeAttrs", int, TRUE);
}

ADD2INITLIB(PCSer_InitAttrs, 0)
ADD2EXPUNGELIB(PCSer_ExpungeAttrs, 0)
