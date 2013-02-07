/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <inttypes.h>
#include <stdio.h>

#include <asm/bcm2835.h>
#include <hardware/videocore.h>
#include <hardware/pl011uart.h>

#include "serialdebug.h"
#include "bootconsole.h"
#include "vc_mb.h"
#include "vc_fb.h"

#define ICR_FLAGS (ICR_RXIC|ICR_TXIC|ICR_RTIC|ICR_FEIC|ICR_PEIC|ICR_BEIC|ICR_OEIC|ICR_RIMIC|ICR_CTSMIC|ICR_DSRMIC|ICR_DCDMIC)

#define DEF_BAUD 115200

#define PL011_DIVCLOCK(baud, clock)     ((clock * 4) / baud) 
#define PL011_BAUDINT(baud, clock)      ((PL011_DIVCLOCK(baud, clock) & 0xFFFFFFC0) >> 6) 
#define PL011_BAUDFRAC(baud, clock)     ((PL011_DIVCLOCK(baud, clock) & 0x0000003F) >> 0) 

unsigned int uartclock;
unsigned int uartdivint;
unsigned int uartdivfrac;
unsigned int uartbaud;

inline void waitSerOUT()
{
    while(1)
    {
       if ((*(volatile uint32_t *)(UART0_BASE + UART_FR) & FR_TXFF) == 0) break;
    }
}

inline void putByte(uint8_t chr)
{
    waitSerOUT();

    if (chr == '\n')
        *(volatile uint32_t *)(UART0_BASE + UART_DR) = '\r';
    *(volatile uint32_t *)(UART0_BASE + UART_DR) = chr;
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
    unsigned int        uartvar;

    volatile unsigned int *uart_msg = (unsigned int *) MESSAGE_BUFFER;

    uartbaud = DEF_BAUD;

    uart_msg[0] = 8 * 4;
    uart_msg[1] = VCTAG_REQ;
    uart_msg[2] = VCTAG_GETCLKRATE;
    uart_msg[3] = 8;
    uart_msg[4] = 4;
    uart_msg[5] = 0x000000002;                  // UART clock
    uart_msg[6] = 0;
    uart_msg[7] = 0;		                // terminate tag

    vcmb_write(VCMB_BASE, VCMB_FBCHAN, uart_msg);
    uart_msg = vcmb_read(VCMB_BASE, VCMB_FBCHAN);
    
    uartclock = uart_msg[6];
    
    *(volatile uint32_t *)(UART0_BASE + UART_CR) = 0;

    uartvar = *(volatile uint32_t *)GPFSEL1;
    uartvar &= ~(7<<12);                        // TX on GPIO14
    uartvar |= 4<<12;                           // alt0
    uartvar &= ~(7<<15);                        // RX on GPIO15
    uartvar |= 4<<15;                           // alt0
    *(volatile uint32_t *)GPFSEL1 = uartvar;

    *(volatile uint32_t *)GPPUD = 0;

    for (uartvar = 0; uartvar < 150; uartvar++);

    *(volatile uint32_t *)GPPUDCLK0 = (1 << 14)|(1 << 15);

    for (uartvar = 0; uartvar < 150; uartvar++);

    *(volatile uint32_t *)GPPUDCLK0 = 0;

    *(volatile uint32_t *)(UART0_BASE + UART_ICR) = ICR_FLAGS;
    uartdivint = PL011_BAUDINT(uartbaud, uartclock);
    *(volatile uint32_t *)(UART0_BASE + UART_IBRD) = uartdivint;
    uartdivfrac = PL011_BAUDFRAC(uartbaud, uartclock);
    *(volatile uint32_t *)(UART0_BASE + UART_FBRD) = uartdivfrac;
    *(volatile uint32_t *)(UART0_BASE + UART_LCRH) = LCRH_WLEN8|LCRH_FEN;                       // 8N1, Fifo enabled
    *(volatile uint32_t *)(UART0_BASE + UART_CR) = CR_UARTEN|CR_TXE|CR_RXE|CR_RTSEN|CR_CTSEN;   // enable the uart + tx
}
