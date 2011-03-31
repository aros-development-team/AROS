#ifndef ASM_IO_H
#define ASM_IO_H
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    CPU independent version of the <asm/io.h> header. This is the one
    that normal programs can include.
*/

/* Fix up __powerpc__ definition if missing */
#ifdef __ppc__
#ifndef __powerpc__
#define __powerpc__
#endif
#endif

/*
 * In fact this is better to be aros/io.h. However:
 * 1. We already have aros/io.h, which is broken and obsolete
 * 2. Many code rerefs to asm/io.h and expects to find these definitions in it.
 * So temporarily let's have it this way.
 */
#if defined __i386__
#  include <aros/i386/io.h>
#elif defined __x86_64__
/* I/O operations are the same on i386 and x86-64 */
#  include <aros/i386/io.h>
#elif defined __powerpc__
#  include <aros/ppc/io.h>
#else
#  error unsupported CPU type
#endif

#endif
