/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel hidd class implementation.
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <hidd/parallel.h>


#include "parallel_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>


/*static AttrBase HiddGCAttrBase;*/

static OOP_AttrBase HiddParallelUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ParallelUnit, &HiddParallelUnitAB },
    { NULL,	NULL }
};

/*** HIDDParallel::NewUnit() *********************************************************/


static OOP_Object *hiddparallel_newunit(OOP_Class *cl, OOP_Object *obj,
					struct pHidd_Parallel_NewUnit *msg)
{
	OOP_Object *su = NULL;
	struct HIDDParallelData * data = OOP_INST_DATA(cl, obj);
	ULONG unitnum = -1;
	
	EnterFunc(bug("HIDDParallel::NewParallel()\n"));
    
	D(bug("Request for unit number %d\n",msg->unitnum));
    
	switch (msg->unitnum) {
		case 0:
		case 1:
		case 2:
			unitnum = msg->unitnum;

			if (0 != (data->usedunits & (1 << unitnum))) {
				unitnum = -1;
			}
	
		break;
	
		case -1: /* search for the next available unit */
			unitnum = 0;

			while (unitnum < PAR_MAX_UNITS) {
				if (0 == (data->usedunits & (1 << unitnum))) {
					break;
				}
	    
				unitnum++;
			}
		break;
	
	
	}
    
	if (unitnum >= 0 && unitnum < PAR_MAX_UNITS) {
		struct TagItem tags[] =
		{
			{aHidd_ParallelUnit_Unit, unitnum},
			{TAG_DONE		       }
		};

		su = OOP_NewObject(NULL, CLID_Hidd_ParallelUnit, tags);
		data->ParallelUnits[unitnum] = su;
	
		/*
		** Mark it as used
		*/
		data->usedunits |= (1 << unitnum); 
	}
    
	ReturnPtr("HIDDParallel::NewParallel", OOP_Object *, su);
}


/*** HIDDParallel::DisposeUnit() ****************************************************/

static VOID hiddparallel_disposeunit(OOP_Class *cl, OOP_Object *obj,
				     struct pHidd_Parallel_DisposeUnit *msg)
{
	OOP_Object * pu = msg->unit;
	struct HIDDParallelData * data = OOP_INST_DATA(cl, obj);

	EnterFunc(bug("HIDDParallel::DisposeUnit()\n"));
    
	if(pu) {
		ULONG unitnum = 0;

		while (unitnum < PAR_MAX_UNITS) {
			if (data->ParallelUnits[unitnum] == pu) {
				D(bug("Disposing ParallelUnit!\n"));
				OOP_DisposeObject(pu);
				data->ParallelUnits[unitnum] = NULL;
				data->usedunits &= ~(1 << unitnum);
				break;
			}

			unitnum++;
		}
	}

	ReturnVoid("HIDDParallel::DisposeUnit");
}



/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_PARALLELHIDD_METHODS 2

OOP_Class *init_parallelhiddclass (struct class_static_data *csd)
{
	OOP_Class *cl = NULL;
	BOOL  ok  = FALSE;
    
	struct OOP_MethodDescr parallelhidd_descr[NUM_PARALLELHIDD_METHODS + 1] = 
	{
        {(IPTR (*)())hiddparallel_newunit,     moHidd_Parallel_NewUnit},
		{(IPTR (*)())hiddparallel_disposeunit, moHidd_Parallel_DisposeUnit},
		{NULL, 0UL}
	};
    
    
	struct OOP_InterfaceDescr ifdescr[] =
	{
		{parallelhidd_descr, IID_Hidd_Parallel, NUM_PARALLELHIDD_METHODS},
		{NULL, NULL, 0}
	};
	
	OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
	    
	struct TagItem tags[] =
	{
		{ aMeta_SuperID,                (IPTR)CLID_Root},

		{ aMeta_InterfaceDescr,         (IPTR)ifdescr},
		{ aMeta_ID,                     (IPTR)CLID_Hidd_Parallel},
		{ aMeta_InstSize,               (IPTR)sizeof (struct HIDDParallelData) },
		{TAG_DONE, 0UL}
	};
    

	EnterFunc(bug("    init_parallelhiddclass(csd=%p)\n", csd));

	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	D(bug("Class=%p\n", cl));

	if(cl) {
		D(bug("ParallelHiddClass ok\n"));
		cl->UserData = (APTR)csd;
	
		csd->parallelunitclass = init_parallelunitclass(csd);
		D(bug("parallelunitclass: %p\n", csd->parallelunitclass));

		if(csd->parallelunitclass) {
 			if (OOP_ObtainAttrBases(attrbases)) {
				D(bug("ParallelUnitClass ok\n"));
		
				ok = TRUE;
			}
		}
	}
    
	if(ok == FALSE) {
		free_parallelhiddclass(csd);
		cl = NULL;
	} else {
		OOP_AddClass(cl);
	}
    
	ReturnPtr("init_parallelhiddclass", OOP_Class *, cl);
}


void free_parallelhiddclass(struct class_static_data *csd)
{
	EnterFunc(bug("free_parallelhiddclass(csd=%p)\n", csd));

	if(csd) {
		OOP_RemoveClass(csd->parallelhiddclass);
	
		free_parallelunitclass(csd);

		if(csd->parallelhiddclass) {
			OOP_DisposeObject((OOP_Object *) csd->parallelhiddclass);
		}
	
		csd->parallelhiddclass = NULL;
	}
	
	ReturnVoid("free_parallelhiddclass");
}
