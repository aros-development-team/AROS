#ifndef HIDD_PARALLEL_H
#define HIDD_PARALLEL_H

/*
    Copyright (C) 1998 AROS - The Amiga Research OS
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

enum
{
    moHidd_Parallel_NewUnit = 0,       
    moHidd_Parallel_DisposeUnit,
    moHidd_Parallel_NumMethods		// always keep this the last one!
};




struct pHidd_Parallel_NewUnit
{
    MethodID	mID;
    ULONG	unitnum;
};

struct pHidd_Parallel_DisposeUnit
{
    MethodID    mID;
    Object      *unit;
};


/**** Parallel Unit definitions ******************************************************/


enum
{
    /* Methods for a parallel unit */

    moHidd_ParallelUnit_Init,
    moHidd_ParallelUnit_Write,
    moHidd_ParallelUnit_NumMethods	// always keep this the last one!
};


/* messages for a parallel unit */

struct pHidd_ParallelUnit_Init
{
    MethodID	mID;
    VOID	*DataReceived;
    VOID	*WriteData;
};

struct pHidd_ParallelUnit_Write
{
    MethodID	mID;
    ULONG	Length;
    UBYTE	*Outbuffer;
};

/* Predeclarations of stubs in libhiddparallelstubs.h */

Object * HIDD_Parallel_NewUnit		(Object *obj, ULONG unitnum);
VOID     HIDD_Parallel_DisposeUnit	(Object *obj, Object *unit);

BOOL     HIDD_ParallelUnit_Init		(Object *obj, VOID * DataReceived, VOID * WriteData);
ULONG    HIDD_ParallelUnit_Write	(Object *obj, UBYTE * data, ULONG length);


#endif /* HIDD_PARALLEL_H */
