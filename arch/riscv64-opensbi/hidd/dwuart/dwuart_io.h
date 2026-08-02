/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Register access for a memory mapped 16550.

    The DesignWare APB UART is a 16550 with the registers spread out:
    the device tree gives "reg-shift" (how far apart they are) and
    "reg-io-width" (how wide each access must be). A typical part uses
    a shift of 2 and 32-bit accesses, so what is one byte on a PC is
    one long every four bytes here.
*/

#ifndef DWUART_IO_H
#define DWUART_IO_H

#include "serial_intern.h"

/*
 * Two registers the 16550 does not have. The UART reports "busy
 * detect" through the interrupt identification register when the line
 * control register is written while a character is still going out;
 * the write is discarded and the condition only clears once the status
 * register has been read. Left unread, the interrupt stays asserted.
 */
#define DWUART_USR      0x1f
#define DWUART_IIR_BUSY 0x07

static inline IPTR dwuart_reg(struct HIDDSerialUnitData *data, int offset)
{
    return data->baseaddr + ((IPTR)offset << data->regshift);
}

static inline void serial_out(struct HIDDSerialUnitData *data, int offset,
                              int value)
{
    IPTR addr = dwuart_reg(data, offset);

    if (data->regwidth == 4)
        *(volatile ULONG *)addr = (ULONG)(UBYTE)value;
    else if (data->regwidth == 2)
        *(volatile UWORD *)addr = (UWORD)(UBYTE)value;
    else
        *(volatile UBYTE *)addr = (UBYTE)value;
}

static inline unsigned int serial_in(struct HIDDSerialUnitData *data,
                                     int offset)
{
    IPTR addr = dwuart_reg(data, offset);

    if (data->regwidth == 4)
        return (unsigned int)(*(volatile ULONG *)addr) & 0xff;
    if (data->regwidth == 2)
        return (unsigned int)(*(volatile UWORD *)addr) & 0xff;
    return (unsigned int)(*(volatile UBYTE *)addr);
}

/*
 * The PC driver distinguishes "slow" accesses, which existed to give
 * ISA cards time to settle. Memory mapped registers need no such
 * thing, so both forms are the same here.
 */
static inline void serial_outp(struct HIDDSerialUnitData *data, int offset,
                               int value)
{
    serial_out(data, offset, value);
}

static inline unsigned int serial_inp(struct HIDDSerialUnitData *data,
                                      int offset)
{
    return serial_in(data, offset);
}

#endif /* DWUART_IO_H */
