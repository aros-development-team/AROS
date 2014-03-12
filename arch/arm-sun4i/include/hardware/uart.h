/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i uart definitions
    Lang: english
*/

#ifndef HARDWARE_SUN4I_UART_H
#define HARDWARE_SUN4I_UART_H

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_UART0_BASE		0x01c28000
#define SUN4I_UART1_BASE		0x01c28400
#define SUN4I_UART2_BASE		0x01c28800
#define SUN4I_UART3_BASE		0x01c28c00
#define SUN4I_UART4_BASE		0x01c29000
#define SUN4I_UART5_BASE		0x01c29400
#define SUN4I_UART6_BASE		0x01c29800
#define SUN4I_UART7_BASE		0x01c29c00
#define SUN4I_PS2_0_BASE		0x01c2a000
#define SUN4I_PS2_1_BASE		0x01c2a400

struct NS16550 {
	uint32_t rbrthrdlb;
	uint32_t ierdmb;
	uint32_t iirfcrafr;
	uint32_t lcr;
	uint32_t mcr;
	uint32_t lsr;
	uint32_t msr;
	uint32_t scr;
	uint32_t reserved[2];
	uint32_t dsr;
	uint32_t dcr;
}__attribute__((__packed__));

#define rbr rbrthrdlb
#define thr rbrthrdlb
#define dll rbrthrdlb
#define ier ierdmb
#define dlm ierdmb
#define iir iirfcrafr
#define fcr iirfcrafr
#define afr iirfcrafr

#define LSR_DR      0x01
#define LSR_OE      0x02
#define LSR_PE      0x04
#define LSR_FE      0x08
#define LSR_BI      0x10
#define LSR_THRE    0x20
#define LSR_TEMT    0x40
#define LSR_ERR     0x80

#endif /* HARDWARE_SUN4I_UART_H */
