/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <inttypes.h>
#include <stdio.h>
#include <asm/bcm2835.h>
#include <hardware/pl011uart.h>

#include "serialdebug.h"
#include "bootconsole.h"

#define ICR_FLAGS (ICR_RXIC|ICR_TXIC|ICR_RTIC|ICR_FEIC|ICR_PEIC|ICR_BEIC|ICR_OEIC)


inline void waitSerIN(volatile uint32_t *uart)
{
    while(1)
    {
       if ((uart[UART_FR] & FR_RXFE) == 0) break;
    }
}

inline void waitSerOUT(volatile uint32_t *uart)
{
    while(1)
    {
       if ((uart[UART_FR] & FR_TXFF) == 0) break;
    }
}

inline void putByte(uint8_t chr)
{
    volatile uint32_t *uart = (uint32_t *)UART0_BASE;

    waitSerOUT(uart);

    uart[UART_DR] = chr;
    if (chr == '\n')
        uart[UART_DR] = '\r';
}

void putBytes(const char *str)
{
    while(*str)
    {
        fb_Putc(*str);
        putByte(*str++);
    }
}

static char tmpbuf[512];

void kprintf(const char *format, ...)
{
	char *out = tmpbuf;
	va_list vp;

	va_start(vp, format);
	vsnprintf(tmpbuf, 511, format, vp);
	va_end(vp);

	putBytes(out);
}

void serInit(void)
{
    volatile uint32_t *uart = (uint32_t *)UART0_BASE;
    unsigned int ra;

    uart[UART_CR] = 0;

    ra = *(volatile uint32_t *)GPFSEL1;
    ra &= ~(7<<12);                     // TX on GPIO14
    ra |= 4<<12;                        // alt0
    ra &= ~(7<<15);                     // RX on GPIO15
    ra |= 4<<15;                        // alt0
    *(volatile uint32_t *)GPFSEL1 = ra;

    *(volatile uint32_t *)GPPUD = 0;

    for(ra = 0; ra < 150; ra++) ;

    *(volatile uint32_t *)GPPUDCLK0 = (1<<14)|(1<<15);

    for(ra = 0; ra < 150; ra++) ;

    *(volatile uint32_t *)GPPUDCLK0 = 0;

    uart[UART_ICR] = ICR_FLAGS;
                                        // default clock speed for uart0 = 3Mhz
    uart[UART_IBRD] = 0x1;              // divisor = 1.628 ( ~115172baud)
    uart[UART_FBRD] = 0x274;
    uart[UART_LCRH] = LCRH_WLEN8;       // 8N1 (Fifo disabled)
    uart[UART_CR] = CR_UARTEN|CR_TXE;   // enable the uart + tx
}
