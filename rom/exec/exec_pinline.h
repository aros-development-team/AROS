#ifndef _EXEC_PINLINE_H
#define _EXEC_PINLINE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private m68k inlines for exec.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
#endif

#define RawIOInit() \
	LP0NR(0x1f8, RawIOInit, \
	, EXEC_BASE_NAME)

#define RawMayGetChar() \
	LP0(0x1fe, LONG, RawMayGetChar, \
	, EXEC_BASE_NAME)

#ifndef RawPutChar
#define RawPutChar(chr) \
	LP1NR(0x204, RawPutChar, UBYTE, chr, d0, \
	, EXEC_BASE_NAME)
#endif

#endif /* _EXEC_PINLINE_H */
