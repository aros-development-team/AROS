/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 16450 serial UART serial console.
*/

#include <bootconsole.h>

#include "console.h"

static unsigned short base = 0x03F8;

/* Standard base addresses for four PC AT serial ports */
static unsigned short standard_ports[] = {
    0x03F8,
    0x02F8,
    0x03E8,
    0x02E8
};

void serial_Init(char *opts)
{
    /* Command line option format: debug=serial[:N][@baud], where N - port number (1 - 4) */
    if (opts[0] == ':')
    {
    	unsigned char portnum = opts[1] - '1';

    	if (portnum < 4)
	    base = standard_ports[portnum];

	opts += 2;
    }
    /* TODO: set baud rate */
}

static unsigned char __inb(addr)
{
    unsigned char tmp;    
    asm volatile ("inb %w1,%b0":"=a"(tmp):"Nd"(addr):"memory");
    return tmp;
}

void serial_Putc(unsigned char data) 
{
    while (!(__inb(base + 0x05) & 0x40));
    asm volatile ("outb %b0,%w1"::"a"(data),"Nd"(base));
}
