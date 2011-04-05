#ifndef ASM_IO_H
#define ASM_IO_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    I/O operations, generic header.
*/

#ifdef __ppc__
#ifndef __powerpc__
#define __powerpc__
#endif
#endif

/* Include the actual CPU-dependent definitions */
#if defined __i386__
#  include <asm/i386/io.h>
#elif defined __x86_64__
/* I/O operations are the same on i386 and x86-64 */
#  include <asm/i386/io.h>
#elif defined __powerpc__
#  include <asm/ppc/io.h>
#else

/* Default version for memory-mapped I/O with strong ordering */

#define inb(address) (*((volatile unsigned char  *)(address)))
#define inw(address) (*((volatile unsigned short *)(address)))
#define inl(address) (*((volatile unsigned int   *)(address)))

#define outb(value, address) *((volatile unsigned char  *)(address)) = (value)
#define outw(value, address) *((volatile unsigned short *)(address)) = (value)
#define outl(value, address) *((volatile unsigned int   *)(address)) = (value)

#endif

#ifndef port_t
#define port_t void *
#endif

#ifndef WEAK_IO_ORDER

#define mmio_inb(address) (*((volatile unsigned char  *)(address)))
#define mmio_inw(address) (*((volatile unsigned short *)(address)))
#define mmio_inl(address) (*((volatile unsigned int   *)(address)))

#define mmio_outb(value, address) *((volatile unsigned char  *)(address)) = (value)
#define mmio_outw(value, address) *((volatile unsigned short *)(address)) = (value)
#define mmio_outl(value, address) *((volatile unsigned int   *)(address)) = (value)

#endif

#endif
