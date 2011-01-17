/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


/*static AttrBase HiddGCAttrBase;*/

/*** HIDDSerial::NewUnit() *********************************************************/

static OOP_Object *hiddserial_newunit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_NewUnit *msg)
{
	OOP_Object *su = NULL;
	struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
	ULONG unitnum = -1;

	EnterFunc(bug("HIDDSerial::NewSerial()\n"));

	D(bug("Request for unit number %d\n",msg->unitnum));

	switch (msg->unitnum) {
		case 0:
		case 1:
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

	if (unitnum >= 0 && unitnum < SER_MAX_UNITS) {
		struct TagItem tags[] =
		{
#define csd CSD(cl->UserData)
				{aHidd_SerialUnit_Unit, unitnum},
#undef csd
				{TAG_DONE }
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

static VOID hiddserial_disposeunit(OOP_Class *cl, OOP_Object *obj, struct pHidd_Serial_DisposeUnit *msg)
{
	OOP_Object * su = msg->unit;
	struct HIDDSerialData * data = OOP_INST_DATA(cl, obj);
	EnterFunc(bug("HIDDSerial::DisposeUnit()\n"));

	if(su) {
		ULONG unitnum = 0;
		while (unitnum < SER_MAX_UNITS) {
			if (data->SerialUnits[unitnum] == su) {
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

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase		 (csd->sysbase)
#define OOPBase		 (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_SERIALHIDD_METHODS moHidd_Serial_NumMethods

OOP_Class *init_serialhiddclass (struct class_static_data *csd)
{
	OOP_Class *cl = NULL;
	BOOL	ok	= FALSE;
	
	struct OOP_MethodDescr serialhidd_descr[NUM_SERIALHIDD_METHODS + 1] = 
	{
		{(IPTR (*)())hiddserial_newunit,     moHidd_Serial_NewUnit},
		{(IPTR (*)())hiddserial_disposeunit, moHidd_Serial_DisposeUnit},
		{NULL, 0UL}
	};
	
	
	struct OOP_InterfaceDescr ifdescr[] =
	{
		{serialhidd_descr, IID_Hidd_Serial, NUM_SERIALHIDD_METHODS},
		{NULL, NULL, 0}
	};
	
	OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
		
	struct TagItem tags[] =
	{
		{ aMeta_SuperID,       (IPTR)CLID_Root},
		{ aMeta_InterfaceDescr,(IPTR)ifdescr},
		{ aMeta_ID,            (IPTR)CLID_Hidd_Serial},
		{ aMeta_InstSize,      (IPTR)sizeof (struct HIDDSerialData) },
		{TAG_DONE, 0UL}
	};
	

	EnterFunc(bug("	init_serialhiddclass(csd=%p)\n", csd));

	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	D(bug("Class=%p\n", cl));
	if(cl) {
		D(bug("SerialHiddClass ok\n"));
		cl->UserData = (APTR)csd;

		csd->serialunitclass = init_serialunitclass(csd);
		D(bug("serialunitclass: %p\n", csd->serialunitclass));

		if(csd->serialunitclass) {
			__IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);
			if (NULL != __IHidd_SerialUnitAB) {
				D(bug("SerialUnitClass ok\n"));

				ok = TRUE;
			}
		}
	}

	if(ok == FALSE) {
		free_serialhiddclass(csd);
		cl = NULL;
	} else {
		OOP_AddClass(cl);
	}

	ReturnPtr("init_serialhiddclass", OOP_Class *, cl);
}


void free_serialhiddclass(struct class_static_data *csd)
{
	EnterFunc(bug("free_serialhiddclass(csd=%p)\n", csd));

	if(csd) {
		OOP_RemoveClass(csd->serialhiddclass);

		free_serialunitclass(csd);

		if(csd->serialhiddclass) OOP_DisposeObject((OOP_Object *) csd->serialhiddclass);
		csd->serialhiddclass = NULL;
	}

	ReturnVoid("free_serialhiddclass");
}
