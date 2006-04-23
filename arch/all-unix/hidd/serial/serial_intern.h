/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SERIAL_HIDD_INTERN_H
#define SERIAL_HIDD_INTERN_H

/* Include files */

#include <sys/termios.h>

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

#define SER_MAX_UNITS	4

struct HIDDSerialData
{
    OOP_Class 		*SerialHIDDClass;

    OOP_Object		*SerialUnits[SER_MAX_UNITS];
    UBYTE		usedunits;
};


#define SER_UNIT_0_F	1
#define SER_UNIT_1_F	2
#define SER_UNIT_2_F	4
#define SER_UNIT_3_F	8


struct class_static_data
{
    OOP_Class		 *serialhiddclass;
    OOP_Class		 *serialunitclass;
    OOP_AttrBase          hiddSerialUnitAB;
};

#define __IHidd_SerialUnitAB   (CSD(cl)->hiddSerialUnitAB)

struct HIDDSerialUnitData
{
    VOID (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    VOID (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID 		*DataWriteUserData;
    VOID		*DataReceivedUserData;
    
    ULONG 		unitnum;
    int			filedescriptor;
    ULONG		baudrate;
    UBYTE		datalength;
    BOOL		parity;
    UBYTE		paritytype;
    UBYTE		stopbits;
    BOOL		breakcontrol;

    BOOL		stopped;
    
    struct MsgPort	*replyport_read;
    struct Interrupt 	*softint_read;
    HIDD		unixio_read;
    
    struct MsgPort	*replyport_write;
    struct Interrupt 	*softint_write;
    HIDD		unixio_write;
    
    struct termios	orig_termios;
};


#define SER_DEFAULT_BAUDRATE	57600


/* Library base */

struct IntHIDDSerialBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;

    struct class_static_data  hdg_csd;
};


#define CSD(cl) (&((struct IntHIDDSerialBase *)cl->UserData)->hdg_csd)

#endif /* SERIAL_HIDD_INTERN_H */
