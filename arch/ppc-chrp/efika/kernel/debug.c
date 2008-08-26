/*
 * debug.c
 *
 *  Created on: Aug 25, 2008
 *      Author: misc
 */

#define PCIC0_IO 0

#include <asm/io.h>
#include <aros/libcall.h>
#include <stdarg.h>
#include "kernel_intern.h"


void __putc(uint8_t chr)
{
	if (chr == '\n')
		__putc('\r');

	while (!(inw(0xf0002004) & 0x400));
	outb(chr, 0xf0002080);
	while (!(inw(0xf0002004) & 0x400));
}


AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int result;

    result = __vcformat(NULL, __putc, format, args);

    return result;

    AROS_LIBFUNC_EXIT
}
