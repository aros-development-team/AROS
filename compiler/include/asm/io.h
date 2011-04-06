#ifndef ASM_IO_H
#define ASM_IO_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    I/O operations, generic header.
*/

#include <aros/macros.h>

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

#ifndef HAVE_MMIO_IO

#define mmio_inb(address) (*((volatile unsigned char  *)(address)))
#define mmio_inw(address) (*((volatile unsigned short *)(address)))
#define mmio_inl(address) (*((volatile unsigned int   *)(address)))

#define mmio_outb(value, address) *((volatile unsigned char  *)(address)) = (value)
#define mmio_outw(value, address) *((volatile unsigned short *)(address)) = (value)
#define mmio_outl(value, address) *((volatile unsigned int   *)(address)) = (value)

#endif

#ifndef HAVE_LE_IO

#define inw_le(address) AROS_LE2WORD(inw(address))
#define inl_le(address) AROS_LE2LONG(inl(address))

#define outw_le(value, address) outw(AROS_WORD2LE(value), address)
#define outl_le(value, address) outl(AROS_LONG2LE(value), address)

#endif

#ifndef HAVE_LE_MMIO_IO

#define mmio_inw_le(address) AROS_LE2WORD(mmio_inw(address))
#define mmio_inl_le(address) AROS_LE2LONG(mmio_inl(address))

#define mmio_outw_le(value, address) mmio_outw(AROS_WORD2LE(value), address)
#define mmio_outl_le(value, address) mmio_outl(AROS_LONG2LE(value), address)

#endif

#endif
