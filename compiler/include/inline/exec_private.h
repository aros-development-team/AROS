#ifndef _INLINE_EXEC_PRIVATE_H
#define _INLINE_EXEC_PRIVATE_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
#endif

#define RawIOInit() \
	LP0NR(0x1f8, LONG, RawIOInit, \
	, EXEC_BASE_NAME)

#define RawMayGetChar() \
	LP0NR(0x1fe, LONG, RawMayGetChar, \
	, EXEC_BASE_NAME)

#define RawPutChar(chr) \
	LP1NR(0x204, RawPutChar, UBYTE, chr, d0, \
	, EXEC_BASE_NAME)

#endif /* _INLINE_EXEC_PRIVATE_H */
