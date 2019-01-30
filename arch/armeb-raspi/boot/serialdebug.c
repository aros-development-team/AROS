/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/macros.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/videocore.h>
#include <hardware/pl011uart.h>

#include "serialdebug.h"
#include "bootconsole.h"
#include "vc_mb.h"
#include "io.h"

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)
extern uint32_t __arm_periiobase;

#define PL011_ICR_FLAGS (PL011_ICR_RXIC|PL011_ICR_TXIC|PL011_ICR_RTIC|PL011_ICR_FEIC|PL011_ICR_PEIC|PL011_ICR_BEIC|PL011_ICR_OEIC|PL011_ICR_RIMIC|PL011_ICR_CTSMIC|PL011_ICR_DSRMIC|PL011_ICR_DCDMIC)

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
       if ((rd32le(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
    }
}

inline void putByte(uint8_t chr)
{
    waitSerOUT();

    if (chr == '\n')
    {
        wr32le(PL011_0_BASE + PL011_DR, '\r');
        waitSerOUT();
    }
    wr32le(PL011_0_BASE + PL011_DR, chr);
}

void serInit(void)
{
    unsigned int        uartvar;

    volatile unsigned int *uart_msg = (unsigned int *) BOOTMEMADDR(bm_mboxmsg);

    uartbaud = DEF_BAUD;

    uart_msg[0] = AROS_LONG2LE(8 * 4);
    uart_msg[1] = AROS_LONG2LE(VCTAG_REQ);
    uart_msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    uart_msg[3] = AROS_LONG2LE(8);
    uart_msg[4] = AROS_LONG2LE(4);
    uart_msg[5] = AROS_LONG2LE(0x000000002);                  // UART clock
    uart_msg[6] = 0;
    uart_msg[7] = 0;		                // terminate tag

    vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void*)uart_msg);
    uart_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

    uartclock = AROS_LE2LONG(uart_msg[6]);
    
    wr32le(PL011_0_BASE + PL011_CR, 0);

    uartvar = rd32le(GPFSEL1);
    uartvar &= ~(7<<12);                        // TX on GPIO14
    uartvar |= 4<<12;                           // alt0
    uartvar &= ~(7<<15);                        // RX on GPIO15
    uartvar |= 4<<15;                           // alt0
    wr32le(GPFSEL1, uartvar);

    /* Disable pull-ups and pull-downs on rs232 lines */
    wr32le(GPPUD, 0);

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("mov r0, r0\n");

    wr32le(GPPUDCLK0, (1 << 14)|(1 << 15));

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("mov r0, r0\n");

    wr32le(GPPUDCLK0, 0);

    wr32le(PL011_0_BASE + PL011_ICR, PL011_ICR_FLAGS);
    uartdivint = PL011_BAUDINT(uartbaud, uartclock);
    wr32le(PL011_0_BASE + PL011_IBRD, uartdivint);
    uartdivfrac = PL011_BAUDFRAC(uartbaud, uartclock);
    wr32le(PL011_0_BASE + PL011_FBRD, uartdivfrac);
    wr32le(PL011_0_BASE + PL011_LCRH, PL011_LCRH_WLEN8|PL011_LCRH_FEN);           // 8N1, Fifo enabled
    wr32le(PL011_0_BASE + PL011_CR, PL011_CR_UARTEN|PL011_CR_TXE|PL011_CR_RXE);   // enable the uart, tx and rx

    for (uartvar = 0; uartvar < 150; uartvar++) asm volatile ("mov r0, r0\n");
}
