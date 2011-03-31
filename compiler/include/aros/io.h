#ifndef AROS_IO_H
#define AROS_IO_H

/*
    Copyright © 2006-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: I/O macros
    Lang: english
*/

#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

/* First include the actual CPU-dependent definitions */
#if defined __i386__
#  include <aros/i386/io.h>
#elif defined __x86_64__
/* I/O operations are the same on i386 and x86-64 */
#  include <aros/i386/io.h>
#elif defined __powerpc__
#  include <aros/ppc/io.h>
#else

#define inb(address) (*((volatile UBYTE *)(address)))
#define inw(address) (*((volatile UWORD *)(address)))
#define inl(address) (*((volatile ULONG *)(address)))

#define outb(value, address) *((volatile UBYTE *)(address)) = (value)
#define outw(value, address) *((volatile UWORD *)(address)) = (value)
#define outl(value, address) *((volatile ULONG *)(address)) = (value)

#endif

#ifndef port_t
#define port_t APTR
#endif

#define BYTEIN(address) inb((port_t)address)
#define WORDIN(address) inw((port_t)address)
#define LONGIN(address) inl((port_t)address)

#define BYTEOUT(address, value) outb(value, (port_t)address)
#define WORDOUT(address, value) outw(value, (port_t)address)
#define LONGOUT(address, value) outl(value, (port_t)address)

#endif /* AROS_IO_H */
