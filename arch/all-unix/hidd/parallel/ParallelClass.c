/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Parallel hidd class implementation.
    Lang: english
*/

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

/*** HIDDParallel::NewUnit() *********************************************************/

static Object *hiddparallel_newunit(Class *cl, Object *obj, struct pHidd_Parallel_NewUnit *msg)
{
  Object *su = NULL;
  struct HIDDParallelData * data = INST_DATA(cl, obj);
  ULONG unitnum = -1;

  EnterFunc(bug("HIDDParallel::NewParallel()\n"));

  D(bug("Request for unit number %d\n",msg->unitnum));

  switch (msg->unitnum)
  {
    case 0:
    case 1:
    case 2:
      unitnum = msg->unitnum;
      if (0 != (data->usedunits & (1 << unitnum)))
        unitnum = -1; 
    break;
    
    case -1: /* search for the next available unit */
    {
      unitnum = 0;
      while (unitnum < PAR_MAX_UNITS)
      {
        if (0 == (data->usedunits & (1 << unitnum)))
          break;
        unitnum++;
      }
    }
    break;
    
    
  }

  if (unitnum >= 0 && unitnum < PAR_MAX_UNITS)
  {
    su = NewObject(NULL, CLID_Hidd_ParallelUnit, (APTR)&msg->unitnum);
    data->ParallelUnits[unitnum] = su;
    /*
    ** Mark it as used
    */
    data->usedunits |= (1 << unitnum); 
  }

  ReturnPtr("HIDDParallel::NewParallel", Object *, su);
}


/*** HIDDParallel::DisposeUnit() ****************************************************/

static VOID hiddparallel_disposeunit(Class *cl, Object *obj, struct pHidd_Parallel_DisposeUnit *msg)
{
  Object * pu = msg->unit;
  struct HIDDParallelData * data = INST_DATA(cl, obj);
  EnterFunc(bug("HIDDParallel::DisposeUnit()\n"));

  if(pu)
  {
    ULONG unitnum = 0;
    while (unitnum < PAR_MAX_UNITS)
    {
      if (data->ParallelUnits[unitnum] == pu)
      {
        D(bug("Disposing ParallelUnit!\n"));
        DisposeObject(pu);
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

Class *init_parallelhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct MethodDescr parallelhidd_descr[NUM_PARALLELHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddparallel_newunit,         moHidd_Parallel_NewUnit},
        {(IPTR (*)())hiddparallel_disposeunit,     moHidd_Parallel_DisposeUnit},
        {NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
        {parallelhidd_descr, IID_Hidd_Parallel, NUM_PARALLELHIDD_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_Parallel},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDParallelData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("    init_parallelhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("ParallelHiddClass ok\n"));
        cl->UserData = (APTR)csd;

        csd->parallelunitclass = init_parallelunitclass(csd);
        D(bug("parallelunitclass: %p\n", csd->parallelunitclass));

        if(csd->parallelunitclass)
        {
            D(bug("ParallelUnitClass ok\n"));

            ok = TRUE;
        }
    }

    if(ok == FALSE)
    {
        free_parallelhiddclass(csd);
        cl = NULL;
    }
    else
    {
        AddClass(cl);
    }

    ReturnPtr("init_parallelhiddclass", Class *, cl);
}


void free_parallelhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_parallelhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->parallelhiddclass);
	
	free_parallelunitclass(csd);

        if(csd->parallelhiddclass) DisposeObject((Object *) csd->parallelhiddclass);
        csd->parallelhiddclass = NULL;
    }

    ReturnVoid("free_parallelhiddclass");
}


