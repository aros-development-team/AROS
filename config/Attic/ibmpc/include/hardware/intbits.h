#ifndef HARDWARE_INTBITS_H
#define HARDWARE_INTBITS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Interrupt bits
    Lang: english
*/

/*
    There is partial compatibility with Amiga, because on PC we have
    diffrent hardware.
*/

#define INTB_VERTB        0	/* Mapped to IRQ0 - timer */
#define INTF_VERTB   (1L<<0)	/* BTW - how about doing REAL VERTB */
#define INTB_SETCLR		/* Dummy definition */
#define INTF_SETCLR		/* Dummy */

#define INTB_IRQ0         0
#define INTF_IRQ0    (1L<<0)
#define INTB_IRQ1         1
#define INTF_IRQ1    (1L<<1)
#define INTB_IRQ2         2
#define INTF_IRQ2    (1L<<2)
#define INTB_IRQ3         3
#define INTF_IRQ3    (1L<<3)
#define INTB_IRQ4         4
#define INTF_IRQ4    (1L<<4)
#define INTB_IRQ5         5
#define INTF_IRQ5    (1L<<5)
#define INTB_IRQ6         6
#define INTF_IRQ6    (1L<<6)
#define INTB_IRQ7         7
#define INTF_IRQ7    (1L<<7)
#define INTB_IRQ8         8
#define INTF_IRQ8    (1L<<8)
#define INTB_IRQ9         9
#define INTF_IRQ9    (1L<<9)
#define INTB_IRQA         10
#define INTF_IRQA    (1L<<10)
#define INTB_IRQB         11
#define INTF_IRQB    (1L<<11)
#define INTB_IRQC         12
#define INTF_IRQC    (1L<<12)
#define INTB_IRQD         13
#define INTF_IRQD    (1L<<13)
#define INTB_IRQE         14
#define INTF_IRQE    (1L<<14)
#define INTB_IRQF         15
#define INTF_IRQF    (1L<<15)

#endif /* HARDWARE_INTBITS_H */
