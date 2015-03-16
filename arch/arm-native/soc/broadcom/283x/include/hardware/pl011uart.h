/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PL011UART_H
#define PL011UART_H

// TODO: implement/use primecell bus subsystem to attach to ip's
#define PL011_0_BASE              (ARM_PERIIOBASE + 0x201000)
#if (0)
// Disabled on RasPi..
#define PL011_1_BASE              (ARM_PERIIOBASE + 0x215000)
#endif
#define PRIMECELLID_PL011       0x011

#define PL011_DR                 (0x00)
#define PL011_RSRECR             (0x04)
#define PL011_FR                 (0x18)
#define PL011_ILPR               (0x20)
#define PL011_IBRD               (0x24)
#define PL011_FBRD               (0x28)
#define PL011_LCRH               (0x2C)
#define PL011_CR                 (0x30)
#define PL011_IFLS               (0x34)
#define PL011_IMSC               (0x38)
#define PL011_RIS                (0x3C)
#define PL011_MIS                (0x40)
#define PL011_ICR                (0x44)
#define PL011_DMACR              (0x48)
#define PL011_ITCR               (0x80)
#define PL011_ITIP               (0x84)
#define PL011_ITOP               (0x88)
#define PL011_TDR                (0x8C)

#define PL011_FR_CTS             (1 << 0)
#define PL011_FR_DSR             (1 << 1)
#define PL011_FR_DCD             (1 << 2)
#define PL011_FR_BUSY            (1 << 3)
#define PL011_FR_RXFE            (1 << 4)
#define PL011_FR_TXFF            (1 << 5)
#define PL011_FR_RXFF            (1 << 6)
#define PL011_FR_TXFE            (1 << 7)

#define PL011_LCRH_BRK           (1 << 0)
#define PL011_LCRH_PEN           (1 << 1)
#define PL011_LCRH_EPS           (1 << 2)
#define PL011_LCRH_STP2          (1 << 3)
#define PL011_LCRH_FEN           (1 << 4)
#define PL011_LCRH_WLEN5         (0 << 5)
#define PL011_LCRH_WLEN6         (1 << 5)
#define PL011_LCRH_WLEN7         (2 << 5)
#define PL011_LCRH_WLEN8         (3 << 5)
#define PL011_LCRH_SPS           (1 << 7)

#define PL011_CR_UARTEN          (1 << 0)
#define PL011_CR_SIREN           (1 << 1)
#define PL011_CR_SIRLP           (1 << 2)
#define PL011_CR_LBE             (1 << 7)
#define PL011_CR_TXE             (1 << 8)
#define PL011_CR_RXE             (1 << 9)
#define PL011_CR_RTSEN           (1 << 14)
#define PL011_CR_CTSEN           (1 << 15)
 
#define PL011_ICR_RIMIC          (1 << 0)
#define PL011_ICR_CTSMIC         (1 << 1)
#define PL011_ICR_DSRMIC         (1 << 2)
#define PL011_ICR_DCDMIC         (1 << 3)
#define PL011_ICR_RXIC           (1 << 4)
#define PL011_ICR_TXIC           (1 << 5)
#define PL011_ICR_RTIC           (1 << 6)
#define PL011_ICR_FEIC           (1 << 7)
#define PL011_ICR_PEIC           (1 << 8)
#define PL011_ICR_BEIC           (1 << 9)
#define PL011_ICR_OEIC           (1 << 10)

#endif /* PL011UART_H */
