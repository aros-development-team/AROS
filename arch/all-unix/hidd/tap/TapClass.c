/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: TapClass.c 23835 2005-12-20 14:43:30Z stegerg $

    Desc: Tap hidd class implementation.
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/tap.h>

#include <aros/symbolsets.h>


#include "tap_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>


/*static AttrBase HiddGCAttrBase;*/

static OOP_AttrBase HiddTapUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_TapUnit, &HiddTapUnitAB },
    { NULL,	NULL }
};

/*** HIDDTap::NewUnit() *********************************************************/


OOP_Object *UXTap__Hidd_Tap__NewUnit(OOP_Class *cl, OOP_Object *obj,
					  struct pHidd_Tap_NewUnit *msg)
{
    OOP_Object *su = NULL;
    struct HIDDTapData * data = OOP_INST_DATA(cl, obj);
    ULONG unitnum = -1;
    
    EnterFunc(bug("HIDDTap::NewUnit()\n"));
    
    D(bug("Request for unit number %d\n",msg->unitnum));
    
    switch (msg->unitnum)
    {
    case 0:
    case 1:
    case 2:
	unitnum = msg->unitnum;

	if (0 != (data->usedunits & (1 << unitnum)))
	{
	    unitnum = -1;
	}
	
	break;
	
    case -1: /* search for the next available unit */
	unitnum = 0;

	while (unitnum < PAR_MAX_UNITS)
	{
	    if (0 == (data->usedunits & (1 << unitnum)))
	    {
		break;
	    }
	    
	    unitnum++;
	}
	break;
	
	
    }
    
    if (unitnum >= 0 && unitnum < PAR_MAX_UNITS)
    {
	struct TagItem tags[] =
	{
	    {aHidd_TapUnit_Unit, unitnum},
	    {TAG_DONE		       }
	};

	su = OOP_NewObject(NULL, CLID_Hidd_TapUnit, tags);
	data->TapUnits[unitnum] = su;
	
	/*
	** Mark it as used
	*/
	data->usedunits |= (1 << unitnum); 
    }
    
    ReturnPtr("HIDDTap::NewUnit", OOP_Object *, su);
}


/*** HIDDTap::DisposeUnit() ****************************************************/

VOID UXTap__Hidd_Tap__DisposeUnit(OOP_Class *cl, OOP_Object *obj,
				       struct pHidd_Tap_DisposeUnit *msg)
{
    OOP_Object * pu = msg->unit;
    struct HIDDTapData * data = OOP_INST_DATA(cl, obj);

    EnterFunc(bug("HIDDTap::DisposeUnit()\n"));
    
    if(pu)
    {
	ULONG unitnum = 0;

	while (unitnum < PAR_MAX_UNITS)
	{
	    if (data->TapUnits[unitnum] == pu)
	    {
		D(bug("Disposing TapUnit!\n"));
		OOP_DisposeObject(pu);
		data->TapUnits[unitnum] = NULL;
		data->usedunits &= ~(1 << unitnum);
		break;
	    }

	    unitnum++;
	}
    }

    ReturnVoid("HIDDTap::DisposeUnit");
}



/*************************** Classes *****************************/

static int UXTap_InitAttrBases(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

static int UXTap_ExpungeAttrBases(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(UXTap_InitAttrBases, 0)
ADD2EXPUNGELIB(UXTap_InitAttrBases, 0)
