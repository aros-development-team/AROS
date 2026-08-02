/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private definitions for the DesignWare APB UART driver.
*/

#ifndef SERIAL_HIDD_INTERN_H
#define SERIAL_HIDD_INTERN_H

/* Include files */

// #include <sys/termios.h>

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
#include <exec/interrupts.h>
#include <hardware/uart.h>
#include <exec/nodes.h>

#define SER_MAX_UNITS	4

struct HIDDSerialData
{
    OOP_Class *SerialHIDDClass;

    OOP_Object		*SerialUnits[SER_MAX_UNITS];
    UBYTE		usedunits;
};

#define SER_UNIT_0_F	1
#define SER_UNIT_1_F	2
#define SER_UNIT_2_F	4
#define SER_UNIT_3_F	8

struct HIDDSerialUnitData
{
    ULONG               (*DataWriteCallBack)	(ULONG unitnum, APTR userdata);
    VOID                (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID 		        *DataWriteUserData;
    VOID		        *DataReceivedUserData;
    
    struct Interrupt    unitsdh;

    ULONG 		        unitnum;
    IPTR		        baseaddr;       /* mapped registers          */
    UBYTE		        regshift;       /* device tree "reg-shift"   */
    UBYTE		        regwidth;       /* device tree "reg-io-width"*/
    ULONG		        inclk;          /* reference clock, in Hz    */
    ULONG		        irq;            /* PLIC source, 0 if none    */
    APTR		        irqHandle;      /* KrnAddIRQHandler() handle */
    ULONG		        baudrate;
    UBYTE		        datalength;
    BOOL		        parity;
    UBYTE		        paritytype;
    UBYTE		        stopbits;
    BOOL		        breakcontrol;    
    BOOL		        stopped;
};

/* One UART as the device tree describes it, before a unit exists */
struct dwuart_port
{
    IPTR    base;
    IPTR    size;
    ULONG   clock;
    ULONG   irq;
    UBYTE   regshift;
    UBYTE   regwidth;
};

struct class_static_data
{
    struct dwuart_port          ports[SER_MAX_UNITS];
    ULONG                       nports;
    APTR                        kernelBase;
    OOP_Class		 *serialhiddclass;
    OOP_Class		 *serialunitclass;

    struct HIDDSerialUnitData   *units[SER_MAX_UNITS];
    OOP_AttrBase	            hiddAB;
    OOP_AttrBase                hiddSerialUnitAB;
};

#define __IHidd                 (csd->hiddAB)
#define __IHidd_SerialUnitAB    (csd->hiddSerialUnitAB)

#define SER_DEFAULT_BAUDRATE	57600



/* Library base */

struct IntHIDDSerialBase
{
    struct Library            hdg_LibNode;

    struct class_static_data  hdg_csd;
};


#define CSD(x) (&((struct IntHIDDSerialBase *)x)->hdg_csd)

/* pre declarations */

OOP_Class *init_serialhiddclass(struct class_static_data *csd);
void   free_serialhiddclass(struct class_static_data *csd);

OOP_Class *init_serialunitclass(struct class_static_data *csd);
void   free_serialunitclass(struct class_static_data *csd);

#include "dwuart_io.h"

#endif /* SERIAL_HIDD_INTERN_H */
