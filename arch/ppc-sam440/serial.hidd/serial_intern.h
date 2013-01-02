/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
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
#include <hardware/uart.h>

#define SER_MAX_UNITS   4

struct HIDDSerialData
{
    OOP_Class           *sd_HIDDSerialClass;

    OOP_Object          *sd_Unit[SER_MAX_UNITS];
    UBYTE               sd_UsedMask;
};

#define SER_UNIT_0_F    1
#define SER_UNIT_1_F    2
#define SER_UNIT_2_F    4
#define SER_UNIT_3_F    8

struct HIDDSerialUnitData
{
    ULONG (*DataWriteCallBack)  (ULONG unitnum, APTR userdata);
    VOID (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID                *DataWriteUserData;
    VOID                *DataReceivedUserData;
    
    ULONG               unitnum;
    ULONG               baseaddr;
    ULONG               baudrate;
    UBYTE               datalength;
    BOOL                parity;
    UBYTE               paritytype;
    UBYTE               stopbits;
    BOOL                breakcontrol;    
    BOOL                stopped;
};

struct class_static_data
{
    OOP_Class            *cs_SerialHIDDClass;
    OOP_Class            *cs_SerialUnitClass;

    OOP_Object           *cs_IRQHidd;
    UBYTE                 cs_IRQ[4];

    struct HIDDSerialUnitData   *cs_Unit[SER_MAX_UNITS];
    OOP_AttrBase                cs_hiddSerialUnitAB;
};

#define __IHidd_SerialUnitAB   (csd->cs_hiddSerialUnitAB)

#define SER_DEFAULT_BAUDRATE    115200



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

#endif /* SERIAL_HIDD_INTERN_H */
