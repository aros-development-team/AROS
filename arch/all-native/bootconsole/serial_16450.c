/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 16450 serial UART serial console.
*/

#include <asm/io.h>
#include <hardware/uart.h>

#include <bootconsole.h>
#include <stdlib.h>

#include "console.h"

#define SER_MAXBAUD 115200

/* These settings can be kept across warm reboots in kickstart */
unsigned short Serial_Base = 0x03F8;
static unsigned int baudRate = 115200;

#ifdef __ppc__
#define outb_p outb
#define inb_p inb

void *IO_Base;
#endif

/* Standard base addresses for four PC AT serial ports */
static unsigned short standard_ports[] =
{
    0x03F8,
    0x02F8,
    0x03E8,
    0x02E8
};

void serial_Init(char *opts)
{
    port_t base;
    unsigned short uDivisor;
    unsigned char tmp;

    if (opts)
    {
    	/* Command line option format: debug=serial[:N][@baud] */
    	if (opts[0] == ':')
    	{
            unsigned short port = strtoul(++opts, &opts, 0);

	    /* N can be either port number (0 - 4) or direct base address specification */
    	    if (port < 4)
	    	Serial_Base = standard_ports[port];
	    else
	    	Serial_Base = port;
	}

	/* Set baud rate */
	if (opts[0] == '@')
    	{
    	    unsigned int baud = strtoul(++opts, NULL, 10);

	    if (baud <= SER_MAXBAUD)
	    	baudRate = baud;
	}
    }

#ifdef __ppc__
    base = IO_Base + Serial_Base;
#else
    base = Serial_Base;
#endif

    uDivisor = SER_MAXBAUD / baudRate;
    tmp = inb_p(base + UART_LCR);
    outb_p(tmp | UART_LCR_DLAB, base + UART_LCR);
    outb_p(uDivisor & 0xFF, base + UART_DLL);
    outb_p(uDivisor >> 8, base + UART_DLM);
    outb_p(tmp, base + UART_LCR);

    outb_p(UART_LCR_WLEN8, base + UART_LCR);
    inb_p(base + UART_RX);
}

static void serial_RawPutc(char data) 
{
    port_t base;

#ifdef __ppc__
    base = IO_Base + Serial_Base;
#else
    base = Serial_Base;
#endif

    /* Wait until the transmitter is empty */
    while (!(inb_p(base + UART_LSR) & UART_LSR_TEMT));

    /* Send out the byte */
    outb_p(data, base + UART_TX);
}

void serial_Putc(char chr)
{
    switch (chr)
    {
    /* Ignore null bytes, they are output by formatting routines as terminators */
    case 0:
    	return;

    /* Prepend CR to LF */
    case '\n':
    	serial_RawPutc('\r');
    }

    serial_RawPutc(chr);
}
