/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Legacy Intel 8259A PIC driver.
*/

#include <asm/io.h>
#include <inttypes.h>

#include "xtpic.h"

void XTPIC_Init(uint16_t *irqmask)
{
    /*
     * After initialization everything is disabled except cascade interrupt (IRQ 2).
     * Be careful and don't miss this! Without IRQ2 enabled slave 8259 (and its IRQs)
     * will not work!
     */
    *irqmask = 0xFFFB;

    /* Setup the 8259. Send four ICWs (see 8529 datasheet) */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0x20)); /* Initialization sequence for 8259A-1 (edge-triggered, cascaded, ICW4 needed) */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0xa0)); /* Initialization sequence for 8259A-2, the same as above */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x20),"i"(0x21)); /* IRQs for master at 0x20 - 0x27 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x28),"i"(0xa1)); /* IRQs for slave at 0x28 - 0x2f */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x04),"i"(0x21)); /* 8259A-1 is master, slave at IRQ2 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(0xa1)); /* 8259A-2 is slave, ID = 2 */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0x21)); /* 8086 mode, non-buffered, nonspecial fully nested mode for both */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0xa1));
    
    /* Now initialize interrupt masks */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xfb),"i"(0x21)); /* Enable cascade int */
    asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0xa1)); /* Mask all interrupts */
}

/* Small delay routine used by XTPIC_Init() initializer */
asm("\ndelay:\t.short   0x00eb\n\tret");

void XTPIC_DisableIRQ(uint8_t irqnum, uint16_t *irqmask)
{
    irqnum &= 15;

    if (irqnum == 2)
    {
    	/* IRQ2 must never be disabled. Doing so breaks communication between two 8259's */
    	return;
    }

    *irqmask |= 1 << irqnum;

    if (irqnum >= 8)
        outb((*irqmask >> 8) & 0xff, 0xA1);
    else
        outb(*irqmask & 0xff, 0x21);
}

void XTPIC_EnableIRQ(uint8_t irqnum, uint16_t *irqmask)
{
    irqnum &= 15;
    *irqmask &= ~(1 << irqnum);    

    if (irqnum >= 8)
        outb((*irqmask >> 8) & 0xff, 0xA1);
    else
        outb(*irqmask & 0xff, 0x21);
}

/*
 * Careful! The 8259A is a fragile beast, it pretty much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI to the two 8259s is important!
 */
void XTPIC_AckIntr(uint8_t intnum, uint16_t *irqmask)
{
    intnum &= 15;
    *irqmask |= 1 << intnum;

    if (intnum >= 8)
    {
        outb((*irqmask >> 8) & 0xff, 0xA1);
        outb(0x62, 0x20);
        outb(0x20, 0xa0);
    }
    else
    {
        outb(*irqmask & 0xff, 0x21);
        outb(0x20, 0x20);
    }
}
