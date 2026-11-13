#ifndef HIDD_PARALLEL_H
#define HIDD_PARALLEL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the Parallel HIDD system.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <utility/utility.h>


#define CLID_Hidd_Parallel	"hidd.parallel.parallel"
#define CLID_Hidd_ParallelUnit	"hidd.parallel.parallelunit"

#define IID_Hidd_Parallel	"hidd.parallel.parallel"
#define IID_Hidd_ParallelUnit	"hidd.parallel.parallelunit"



/**** Graphics definitions ****************************************************/

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddParallelUnitAB;
#endif

enum {
    aoHidd_ParallelUnit_Unit,
    
    num_Hidd_ParallelUnit_Attrs
    
};

#define aHidd_ParallelUnit_Unit		(HiddParallelUnitAB + aoHidd_ParallelUnit_Unit)

#define IS_HIDDPARALLELUNIT_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddParallelUnitAB, num_Hidd_ParallelUnit_Attrs)

enum
{
    moHidd_Parallel_NewUnit = 0,       
    moHidd_Parallel_DisposeUnit,
    moHidd_Parallel_NumMethods		// always keep this the last one!
};




struct pHidd_Parallel_NewUnit
{
    OOP_MethodID	mID;
    ULONG		unitnum;
};

struct pHidd_Parallel_DisposeUnit
{
    OOP_MethodID    	mID;
    OOP_Object         	*unit;
};


/**** Parallel Unit definitions ******************************************************/


enum
{
    /* Methods for a parallel unit */

    moHidd_ParallelUnit_Init,
    moHidd_ParallelUnit_Write,
    moHidd_ParallelUnit_Start,
    moHidd_ParallelUnit_Stop,
    moHidd_ParallelUnit_GetStatus,
    moHidd_ParallelUnit_NumMethods	// always keep this the last one!
};


/* messages for a parallel unit */

struct pHidd_ParallelUnit_Init
{
    OOP_MethodID	mID;
    VOID		*DataReceived;
    VOID		*DataReceivedUserData;
    VOID		*WriteData;
    VOID		*WriteDataUserData;
};

struct pHidd_ParallelUnit_Write
{
    OOP_MethodID	mID;
    ULONG		Length;
    UBYTE		*Outbuffer;
};

struct pHidd_ParallelUnit_Start
{
    OOP_MethodID	mID;
};

struct pHidd_ParallelUnit_Stop
{
    OOP_MethodID	mID;
};

struct pHidd_ParallelUnit_GetStatus
{
    OOP_MethodID	mID;
};

/* Predeclarations of stubs in libhiddparallelstubs.h */

OOP_Object * HIDD_Parallel_NewUnit	(OOP_Object *obj, ULONG unitnum);
VOID     HIDD_Parallel_DisposeUnit	(OOP_Object *obj, OOP_Object *unit);

BOOL     HIDD_ParallelUnit_Init		(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData);
ULONG    HIDD_ParallelUnit_Write	(OOP_Object *obj, UBYTE * data, ULONG length);
VOID     HIDD_ParallelUnit_Start        (OOP_Object *obj);
VOID     HIDD_ParallelUnit_Stop         (OOP_Object *obj);
UWORD    HIDD_ParallelUnit_GetStatus    (OOP_Object *obj);

#endif /* HIDD_PARALLEL_H */
