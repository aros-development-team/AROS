/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.
*/

#ifndef PARALLEL_HIDD_INTERN_H
#define PARALLEL_HIDD_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_PARALLEL_H
#   include <hidd/parallel.h>
#endif
#include <dos/dos.h>

#define PAR_MAX_UNITS	1

struct HIDDParallelData
{
    OOP_Class *ParallelHIDDClass;

    OOP_Object		*ParallelUnits[PAR_MAX_UNITS];
    UBYTE		usedunits;
};


struct class_static_data
{
    OOP_Class		 *parallelhiddclass;
    OOP_Class		 *parallelunitclass;
};

struct HIDDParallelUnitData
{
    ULONG (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    ULONG (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID		*DataWriteUserData;
    VOID		*DataReceivedUserData;

    BOOL                stopped;
    struct Interrupt    parint;
    struct Resource    *ciares;
};


/* Library base */

struct IntHIDDParallelBase
{
    struct Library            hdg_LibNode;
    struct Library           *hdg_UtilityBase;

    struct class_static_data  hdg_csd;
};


#define CSD(x) ((struct class_static_data *)x)

#endif /* PARALLEL_HIDD_INTERN_H */
