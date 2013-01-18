/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <inttypes.h>
#include <stdio.h>
#include <asm/bcm2835.h>

#include "serialdebug.h"
#include "bootconsole.h"

inline void waitSerIN(volatile uint32_t *uart)
{
    while(1)
    {
       if ((uart[UART_FR] & 0x10) == 0) break;
    }
}

inline void waitSerOUT(volatile uint32_t *uart)
{
    while(1)
    {
       if ((uart[UART_FR] & 0x20) == 0) break;
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
    ra &= ~(7<<12); // TX on GPIO14
    ra |= 4<<12;    // alt0
    ra &= ~(7<<15); // RX on GPIO15
    ra |= 4<<15;    // alt0
    *(volatile uint32_t *)GPFSEL1 = ra;

    *(volatile uint32_t *)GPPUD = 0;

    for(ra = 0; ra < 150; ra++) ;

    *(volatile uint32_t *)GPPUDCLK0 = (1<<14)|(1<<15);

    for(ra = 0; ra < 150; ra++) ;

    *(volatile uint32_t *)GPPUDCLK0 = 0;

    uart[UART_ICR] = 0x7FF;
    uart[UART_IBRD] = 1;
    uart[UART_FBRD] = 40;
    uart[UART_LCRH] = 0x70;
    uart[UART_CR] = 0x301;
}
