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

/* These settings can be kept accross warm reboots in kickstart */
static unsigned short base     = 0x03F8;
static unsigned int   baudRate = 115200;

/* Standard base addresses for four PC AT serial ports */
static unsigned short standard_ports[] = {
    0x03F8,
    0x02F8,
    0x03E8,
    0x02E8
};

void serial_Init(char *opts)
{
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
	    	base = standard_ports[port];
	    else
	    	base = port;
	}

	/* Set baud rate */
	if (opts[0] == '@')
    	{
    	    unsigned int baud = strtoul(++opts, NULL, 10);

	    if (baud <= SER_MAXBAUD)
	    	baudRate = baud;
	}
    }

    uDivisor = SER_MAXBAUD / baudRate;
    tmp = inb_p(base + UART_LCR);
    outb_p(tmp | UART_LCR_DLAB, base + UART_LCR);
    outb_p(uDivisor & 0xFF, base + UART_DLL);
    outb_p(uDivisor >> 8, base + UART_DLM);
    outb_p(tmp, base + UART_LCR);

    outb_p(UART_LCR_WLEN8, base + UART_LCR);
    inb_p(base + UART_RX);
}

static void serial_RawPutc(unsigned char data) 
{
    /* Wait until the transmitter is empty */
    while (!(inb_p(base + UART_LSR) & UART_LSR_TEMT));

    /* Send out the byte */
    outb_p(data, base + UART_TX);
}

void serial_Putc(unsigned char data)
{
    /* Prepend CR to LF */
    if (data == '\n')
    	serial_RawPutc('\r');

    serial_RawPutc(data);
}
