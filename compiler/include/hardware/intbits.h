#ifndef HARDWARE_INTBITS_H
#define HARDWARE_INTBITS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt bits
    Lang: english
*/

#define INTB_TBE          0
#define INTF_TBE     (1L<<0)
#define INTB_DSKBLK       1
#define INTF_DSKBLK  (1L<<1)
#define INTB_SOFTINT      2
#define INTF_SOFTINT (1L<<2)
#define INTB_PORTS        3
#define INTF_PORTS   (1L<<3)
#define INTB_COPER        4
#define INTF_COPER   (1L<<4)
#define INTB_VERTB        5
#define INTF_VERTB   (1L<<5)
#define INTB_BLIT         6
#define INTF_BLIT    (1L<<6)
#define INTB_AUD0         7
#define INTF_AUD0    (1L<<7)
#define INTB_AUD1         8
#define INTF_AUD1    (1L<<8)
#define INTB_AUD2         9
#define INTF_AUD2    (1L<<9)
#define INTB_AUD3         10
#define INTF_AUD3    (1L<<10)
#define INTB_RBF          11
#define INTF_RBF     (1L<<11)
#define INTB_DSKSYNC      12
#define INTF_DSKSYNC (1L<<12)
#define INTB_EXTER        13
#define INTF_EXTER   (1L<<13)
#define INTB_INTEN        14
#define INTF_INTEN   (1L<<14)
#define INTB_SETCLR       15
#define INTF_SETCLR  (1L<<15)

/* AROS extensions */

/* Virtual 100 Hz vblank timer */
#define INTB_VERTB100 	INTB_COPER
#define INTF_VERTB100	INTF_COPER

#endif /* HARDWARE_INTBITS_H */
