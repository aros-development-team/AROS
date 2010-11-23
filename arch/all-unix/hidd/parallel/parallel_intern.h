/*
    Copyright © 1995-2010 The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARALLEL_HIDD_INTERN_H
#define PARALLEL_HIDD_INTERN_H

/* Include files */

#include <exec/libraries.h>
#include <oop/oop.h>
#include <hidd/parallel.h>
#include <hidd/unixio.h>
#include <dos/dos.h>

#define PAR_MAX_UNITS	3

struct HIDDParallelData
{
    OOP_Class *ParallelHIDDClass;

    OOP_Object		*ParallelUnits[PAR_MAX_UNITS];
    UBYTE		usedunits;
};


#define PAR_UNIT_0_F	1
#define PAR_UNIT_1_F	2
#define PAR_UNIT_2_F	4


struct class_static_data
{
    struct Library       * utilitybase;
    struct Library       * oopbase;

    OOP_Class		 *parallelhiddclass;
    OOP_Class		 *parallelunitclass;
};

struct HIDDParallelUnitData
{
    VOID (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    VOID (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID		*DataWriteUserData;
    VOID		*DataReceivedUserData;
    
    ULONG 		unitnum;
    int			filedescriptor;

    BOOL                stopped;
    
    struct MsgPort	*replyport_read;
    struct Interrupt 	*softint_read;
    HIDD		unixio_read;
    
    struct MsgPort	*replyport_write;
    struct Interrupt 	*softint_write;
    HIDD		unixio_write;
    
    struct uioInterrupt  unixio_int;
    OOP_Object		*unixio;
};


/* Library base */

struct IntHIDDParallelBase
{
    struct Library            hdg_LibNode;

    struct class_static_data  hdg_csd;
};


#define CSD(cl) (&((struct IntHIDDParallelBase *)cl)->hdg_csd)

#endif /* PARALLEL_HIDD_INTERN_H */
