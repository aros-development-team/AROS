/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
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
    ULONG   	    	baseaddr;
    BOOL                stopped;
    
#if 0
    struct MsgPort	*replyport_read;
    struct Interrupt 	*softint_read;
    HIDD		unixio_read;
    
    struct MsgPort	*replyport_write;
    struct Interrupt 	*softint_write;
    HIDD		unixio_write;
#endif    
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


#define CSD(x) ((struct class_static_data *)x)

/*
 * Define some names for the registers
 */
#define PAR_DATA       0       /* In/Out: Parallel Data */
#define PAR_SP         1       /* In: Status Port */
#define PAR_PCP        2       /* In/Out: Parallel Control Port */

#define PAR_SP_BUSY       0x80
#define PAR_SP_ACK        0x40
#define PAR_SP_PE         0x20
#define PAR_SP_SLCT       0x10
#define PAR_SP_ERROR      0x08
#define PAR_SP_IRQ_STATUS 0x04

#define PAR_PCP_DIRECTION   0x20   /* Mask for direction bit */
#define PAR_PCP_IRQ_EN      0x10   /* enables the parallel port irq */
#define PAR_PCP_SLCT_IN     0x08   /* control fr select in signal */
#define PAR_PCP_INIT        0x04   /* initialize printer signal */
#define PAR_PCP_AUTO_FD_XT  0x02   /* automatic feed xt */
#define PAR_PCP_STROBE      0x01   /* strobe signal */

#endif /* PARALLEL_HIDD_INTERN_H */
