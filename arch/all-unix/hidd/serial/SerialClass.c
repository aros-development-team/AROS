/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Serial hidd class implementation.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>


#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>


/*static AttrBase HiddGCAttrBase;*/

/*** HIDDSerial::NewUnit() *********************************************************/

static Object *hiddserial_newunit(Class *cl, Object *obj, struct pHidd_Serial_NewUnit *msg)
{
  Object *su = NULL;
  struct HIDDSerialData * data = INST_DATA(cl, obj);
  ULONG unitnum = -1;

  EnterFunc(bug("HIDDSerial::NewSerial()\n"));

  D(bug("Request for unit number %d\n",msg->unitnum));

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
    su = NewObject(NULL, CLID_Hidd_SerialUnit, (APTR)&msg->unitnum);
    data->SerialUnits[unitnum] = su;
    /*
    ** Mark it as used
    */
    data->usedunits |= (1 << unitnum); 
  }

  ReturnPtr("HIDDSerial::NewSerial", Object *, su);
}


/*** HIDDSerial::DisposeUnit() ****************************************************/

static VOID hiddserial_disposeunit(Class *cl, Object *obj, struct pHidd_Serial_DisposeUnit *msg)
{
  Object * su = msg->unit;
  struct HIDDSerialData * data = INST_DATA(cl, obj);
  EnterFunc(bug("HIDDSerial::DisposeUnit()\n"));

  if(su)
  {
    ULONG unitnum = 0;
    while (unitnum < SER_MAX_UNITS)
    {
      if (data->SerialUnits[unitnum] == su)
      {
        D(bug("Disposing SerialUnit!\n"));
        DisposeObject(su);
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

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_SERIALHIDD_METHODS moHidd_Serial_NumMethods

Class *init_serialhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct MethodDescr serialhidd_descr[NUM_SERIALHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddserial_newunit,         moHidd_Serial_NewUnit},
        {(IPTR (*)())hiddserial_disposeunit,     moHidd_Serial_DisposeUnit},
        {NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
        {serialhidd_descr, IID_Hidd_Serial, NUM_SERIALHIDD_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_Serial},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDSerialData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("    init_serialhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("SerialHiddClass ok\n"));
        cl->UserData = (APTR)csd;

        csd->serialunitclass = init_serialunitclass(csd);
        D(bug("serialunitclass: %p\n", csd->serialunitclass));

        if(csd->serialunitclass)
        {
            D(bug("SerialUnitClass ok\n"));

            ok = TRUE;
        }
    }

    if(ok == FALSE)
    {
        free_serialhiddclass(csd);
        cl = NULL;
    }
    else
    {
        AddClass(cl);
    }

    ReturnPtr("init_serialhiddclass", Class *, cl);
}


void free_serialhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_serialhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->serialhiddclass);
	
	free_serialunitclass(csd);

        if(csd->serialhiddclass) DisposeObject((Object *) csd->serialhiddclass);
        csd->serialhiddclass = NULL;
    }

    ReturnVoid("free_serialhiddclass");
}


