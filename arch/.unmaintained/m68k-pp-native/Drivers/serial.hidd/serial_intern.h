/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SERIAL_HIDD_INTERN_H
#define SERIAL_HIDD_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_SERIAL_H
#   include <hidd/serial.h>
#endif
#include <dos/dos.h>

#define SER_MAX_UNITS	2

struct HIDDSerialData
{
    OOP_Class *SerialHIDDClass;

    OOP_Object		*SerialUnits[SER_MAX_UNITS];
    UBYTE		usedunits;
};

#define SER_UNIT_0_F	1
#define SER_UNIT_1_F	2

struct HIDDSerialUnitData
{
    VOID (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    VOID (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID 		*DataWriteUserData;
    VOID		*DataReceivedUserData;
    
    ULONG 		unitnum;
    ULONG		baseaddr;
    ULONG		baudrate;
    UBYTE		datalength;
    BOOL		parity;
    UBYTE		paritytype;
    UBYTE		stopbits;
    BOOL		breakcontrol;    
    BOOL		stopped;
};

struct class_static_data
{
    struct ExecBase      * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;

    OOP_Class		 *serialhiddclass;
    OOP_Class		 *serialunitclass;

    OOP_Object               *irqhidd;

    struct HIDDSerialUnitData   *units[SER_MAX_UNITS];
    OOP_AttrBase                hiddSerialUnitAB;
};

#define __IHidd_SerialUnitAB   (csd->hiddSerialUnitAB)

#define SER_DEFAULT_BAUDRATE	57600



/* Library base */

struct IntHIDDSerialBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;
    struct Library           *hdg_UtilityBase;

    struct class_static_data *hdg_csd;
};


#define CSD(x) ((struct class_static_data *)x)

#undef SysBase
#define SysBase (CSD(cl->UserData)->sysbase)

#undef UtilityBase
#define UtilityBase (CSD(cl->UserData)->utilitybase)

#undef OOPBase
#define OOPBase (CSD(cl->UserData)->oopbase)


/* pre declarations */

OOP_Class *init_serialhiddclass(struct class_static_data *csd);
void   free_serialhiddclass(struct class_static_data *csd);

OOP_Class *init_serialunitclass(struct class_static_data *csd);
void   free_serialunitclass(struct class_static_data *csd);


/*
 * Bits for USTCNT1/2 register
 */
#define UEN		(1 << 15)
#define RXEN		(1 << 14)
#define TXEN		(1 << 13)
#define CLKM		(1 << 12)
#define PEN		(1 << 11)
#define ODD		(1 << 10)
#define STOP		(1 <<  9)
#define EITHER8OR7	(1 <<  8)
#define ODEN		(1 <<  7)
#define CTSD		(1 <<  6)
#define RXFE		(1 <<  5)
#define RXHE		(1 <<  4)
#define RXRE		(1 <<  3)
#define TXEE		(1 <<  2)
#define TXHE		(1 <<  1)
#define TXAE		(1 <<  0)

/*
 * Bits for UBAUD1/2 register
 */
#define UCLKDIR		(1 << 13)
#define BAUD_SRC	(1 << 11)

/*
 * Bits for URX1/2 register
 */
#define FIFO_FULL	(1 << 15)
#define FIFO_HALF	(1 << 14)
#define DATA_READY	(1 << 13)
#define OLD_DATA	(1 << 12)
#define OVRUN		(1 << 11)
#define FRAME_ERROR	(1 << 10)
#define BREAK		(1 <<  9)
#define PARITY_ERROR	(1 <<  8)

/*
 * Bits for URX1/2 register
 */
#define FIFO_EMPTY	(1 << 15)
#define FIFO_HALF	(1 << 14)
#define TX_AVAIL	(1 << 13)
#define SEND_BREAK	(1 << 12)
#define NOCTS1		(1 << 11)
#define BUSY		(1 << 10)
#define CTS1_STAT	(1 <<  9)
#define CTS1_DELTA	(1 <<  8)

/*
 * Bits for the UMISC1/2 register
 */
#define BAUD_TEST	(1 << 15)
#define CLKSRC		(1 << 14)
#define FORCE_PERR	(1 << 13)
#define LOOP		(1 << 12)
#define BAUD_RESET	(1 << 11)
#define IRTEST		(1 << 10)
#define RTS1_CONT	(1 <<  7)
#define RTS1		(1 <<  6)
#define IRDAEN		(1 <<  5)
#define IRDA_LOOP	(1 <<  4)
#define RXPOL		(1 <<  3)
#define TXPOL		(1 <<  2)


/*
 * Offsets
 */
#define USTCNT		0
#define UBAUD		2
#define URX		4
#define UTX		6
#define UMISC		8
#define NIPR		10
 
#endif /* SERIAL_HIDD_INTERN_H */
