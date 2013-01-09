/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SERIALDEBUG_H_
#define SERIALDEBUG_H_

#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028
#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098

#define UART0_BASE   0x20201000
#define UART_DR     (0x00)
#define UART_RSRECR (0x04)
#define UART_FR     (0x18)
#define UART_ILPR   (0x20)
#define UART_IBRD   (0x24)
#define UART_FBRD   (0x28)
#define UART_LCRH   (0x2C)
#define UART_CR     (0x30)
#define UART_IFLS   (0x34)
#define UART_IMSC   (0x38)
#define UART_RIS    (0x3C)
#define UART_MIS    (0x40)
#define UART_ICR    (0x44)
#define UART_DMACR  (0x48)
#define UART_ITCR   (0x80)
#define UART_ITIP   (0x84)
#define UART_ITOP   (0x88)
#define UART_TDR    (0x8C)


#define ONEMS	(0xb0/4)
#define UBIR	(0xa4/4)
#define UBMR	(0xa8/4)
#define UCR2	(0x84/4)

void serInit();
//void waitBusy();
void waitSerIN();
void waitSerOUT();
void putByte(uint8_t chr);
void putBytes(const char *str);

#endif /* SERIALDEBUG_H_ */
