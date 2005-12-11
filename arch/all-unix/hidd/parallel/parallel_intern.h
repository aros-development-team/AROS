/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARALLEL_HIDD_INTERN_H
#define PARALLEL_HIDD_INTERN_H

/* Include files */

#include <sys/termios.h>

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
    struct ExecBase      * sysbase;
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
    
};


/* Library base */

struct IntHIDDParallelBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;
    struct Library           *hdg_UtilityBase;

    struct class_static_data  hdg_csd;
};


#define CSD(cl) (&((struct IntHIDDParallelBase *)cl)->hdg_csd)

#undef UtilityBase
#define UtilityBase (CSD(cl)->utilitybase)

/* pre declarations */

OOP_Class *init_parallelhiddclass(struct class_static_data *csd);
void   free_parallelhiddclass(struct class_static_data *csd);

OOP_Class *init_parallelunitclass(struct class_static_data *csd);
void   free_parallelunitclass(struct class_static_data *csd);


#endif /* PARALLEL_HIDD_INTERN_H */
