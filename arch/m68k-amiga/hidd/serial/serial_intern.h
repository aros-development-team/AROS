/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Paula serial hidd private definitions.
*/

#ifndef SERIAL_HIDD_INTERN_H
#define SERIAL_HIDD_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_SERIAL_H
#   include <hidd/serial.h>
#endif
#include <exec/interrupts.h>
#include <dos/dos.h>

/* Paula has exactly one UART. */
#define SER_MAX_UNITS   1

/* Custom chip registers, same names as arch/m68k-amiga/kernel/kernel_debug.c. */
#define SERDATR                 0x18
#define   SERDATR_OVRUN         (1 << 15)       /* Overrun */
#define   SERDATR_RBF           (1 << 14)       /* Rx Buffer Full */
#define   SERDATR_TBE           (1 << 13)       /* Tx Buffer Empty */
#define   SERDATR_TSRE          (1 << 12)       /* Tx Shift Empty */
#define   SERDATR_DB8_of(x)     ((x) & 0xff)    /* 8-bit data */
#define SERDAT                  0x30
#define   SERDAT_STP8           (1 << 8)        /* Stop bit for 8 data bits */
#define   SERDAT_DB8(x)         ((x) & 0xff)
#define SERPER                  0x32

/*
 * The bootstrap programs SERPER from the PAL colour clock unconditionally
 * (arch/m68k-amiga/boot/debug.c, arch/m68k-amiga/c/AROSBootstrap.c), so using
 * the same base here means asking for the rate it already set reproduces its
 * divisor exactly and leaves the debug stream untouched. On NTSC machines both
 * are equally off, which is a pre-existing quirk rather than one added here.
 */
#define SERPER_BASE_PAL         3546895
#define SERPER_BAUD(base, x)    ((((base) + (x)/2)/(x) - 1) & 0x7fff)
#define SERPER_DIVISOR_MAX      0x7fff

/* CIA-B port A carries the handshake lines, all active low. */
#define CIAB_PRA                ((volatile UBYTE *)0xbfd000)
#define   CIAB_PRA_HANDSHAKE    0xb8            /* DSR, CTS, CD, DTR */

static inline void custom_w(ULONG reg, UWORD val)
{
    volatile UWORD *r = (volatile UWORD *)(0xdff000 + reg);

    *r = val;
}

static inline UWORD custom_r(ULONG reg)
{
    volatile UWORD *r = (volatile UWORD *)(0xdff000 + reg);

    return *r;
}

struct HIDDSerialData
{
    OOP_Class   *SerialHIDDClass;

    OOP_Object  *SerialUnits[SER_MAX_UNITS];
    UBYTE       usedunits;
};

struct class_static_data
{
    OOP_Class    *serialhiddclass;
    OOP_Class    *serialunitclass;
    OOP_AttrBase hiddSerialUnitAB;
};

struct HIDDSerialUnitData
{
    ULONG (*DataWriteCallBack)(ULONG unitnum, APTR userdata);
    VOID  (*DataReceivedCallBack)(UBYTE *buffer, ULONG len, ULONG unitnum, APTR userdata);
    VOID  *DataWriteUserData;
    VOID  *DataReceivedUserData;

    ULONG            unitnum;
    ULONG            baudrate;
    BOOL             stopped;

    struct Interrupt rbfint;
    struct Interrupt tbeint;
    BOOL             intsadded;
};

/* Library base */

struct IntHIDDSerialBase
{
    struct Library            hdg_LibNode;

    struct class_static_data  hdg_csd;
};

#define CSD(x) (&((struct IntHIDDSerialBase *)x)->hdg_csd)

/*
 * hidd/serial.h spells the unit attribute base this way but leaves it to the
 * driver to define, unlike hidd/parallel.h which declares its own. Keeping it
 * in the class static data means both classes share the one base; a per-file
 * static would leave whichever file never obtains it expanding to zero.
 * Functions using the aHidd_SerialUnit_* macros need a local named "csd".
 */
#define __IHidd_SerialUnitAB    (csd->hiddSerialUnitAB)

#endif /* SERIAL_HIDD_INTERN_H */
