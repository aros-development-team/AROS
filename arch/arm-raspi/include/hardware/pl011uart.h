/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PL011UART_H
#define PL011UART_H

#define UART0_BASE              (BCM_PHYSBASE + 0x201000)
#define UART1_BASE              (BCM_PHYSBASE + 0x215000)

#define UART_DR                 (0x00)
#define UART_RSRECR             (0x04)
#define UART_FR                 (0x18)
#define UART_ILPR               (0x20)
#define UART_IBRD               (0x24)
#define UART_FBRD               (0x28)
#define UART_LCRH               (0x2C)
#define UART_CR                 (0x30)
#define UART_IFLS               (0x34)
#define UART_IMSC               (0x38)
#define UART_RIS                (0x3C)
#define UART_MIS                (0x40)
#define UART_ICR                (0x44)
#define UART_DMACR              (0x48)
#define UART_ITCR               (0x80)
#define UART_ITIP               (0x84)
#define UART_ITOP               (0x88)
#define UART_TDR                (0x8C)

#define FR_CTS                  (1 << 0)
#define FR_DSR                  (1 << 1)
#define FR_DCD                  (1 << 2)
#define FR_BUSY                 (1 << 3)
#define FR_RXFE                 (1 << 4)
#define FR_TXFF                 (1 << 5)
#define FR_RXFF                 (1 << 6)
#define FR_TXFE                 (1 << 7)

#define LCRH_BRK                (1 << 0)
#define LCRH_PEN                (1 << 1)
#define LCRH_EPS                (1 << 2)
#define LCRH_STP2               (1 << 3)
#define LCRH_FEN                (1 << 4)
#define LCRH_WLEN5              (0 << 5)
#define LCRH_WLEN6              (1 << 5)
#define LCRH_WLEN7              (2 << 5)
#define LCRH_WLEN8              (3 << 5)
#define LCRH_SPS                (1 << 7)

#define CR_UARTEN               (1 << 0)
#define CR_SIREN                (1 << 1)
#define CR_SIRLP                (1 << 2)
#define CR_LBE                  (1 << 7)
#define CR_TXE                  (1 << 8)
#define CR_RXE                  (1 << 9)

#define ICR_RXIC                (1 << 4)
#define ICR_TXIC                (1 << 5)
#define ICR_RTIC                (1 << 6)
#define ICR_FEIC                (1 << 7)
#define ICR_PEIC                (1 << 8)
#define ICR_BEIC                (1 << 9)
#define ICR_OEIC                (1 << 10)

#endif /* PL011UART_H */
