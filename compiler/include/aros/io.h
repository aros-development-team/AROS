#ifndef AROS_IO_H
#define AROS_IO_H

/*
    Copyright © 2006-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Oldstyle I/O macros
    Lang: english
*/

#include <asm/io.h>

#define BYTEIN(address) inb((port_t)address)
#define WORDIN(address) inw((port_t)address)
#define LONGIN(address) inl((port_t)address)

#define BYTEOUT(address, value) outb(value, (port_t)address)
#define WORDOUT(address, value) outw(value, (port_t)address)
#define LONGOUT(address, value) outl(value, (port_t)address)

#endif /* AROS_IO_H */
