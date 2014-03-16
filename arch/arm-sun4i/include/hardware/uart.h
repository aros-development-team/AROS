/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i uart definitions (NS16550)
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

struct UART {
	uint32_t RBRTHRDLB;
	uint32_t IERDMB;
	uint32_t IIRFCRAFR;
	uint32_t LCR;
	uint32_t MCR;
	uint32_t LSR;
	uint32_t MSR;
	uint32_t SCR;
	uint32_t UART_RESERVED_1[2];
	uint32_t DSR;
	uint32_t DCR;
}__attribute__((__packed__));

#define RBR RBRTHRDLB
#define THR RBRTHRDLB
#define DLL RBRTHRDLB
#define IER IERDMB
#define DLM IERDMB
#define IIR IIRFCRAFR
#define FCR IIRFCRAFR
#define AFR IIRFCRAFR

#define LSR_DR      0x01
#define LSR_OE      0x02
#define LSR_PE      0x04
#define LSR_FE      0x08
#define LSR_BI      0x10
#define LSR_THRE    0x20
#define LSR_TEMT    0x40
#define LSR_ERR     0x80

#endif /* HARDWARE_SUN4I_UART_H */
