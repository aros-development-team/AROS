#ifndef EXEC_INITIALIZERS_H
#define EXEC_INITIALIZERS_H

#include <aros/system.h>

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros to build init structs (eg. for InitStruct())
    Lang: english
*/

#define OFFSET(structName, structEntry) \
				(&(((struct structName *) 0)->structEntry))

#if AROS_BIG_ENDIAN
#define INITBYTE(offset,value)  0xe000,(UWORD) (offset),(UWORD) ((value)<<8)
#define INITWORD(offset,value)  0xd000,(UWORD) (offset),(UWORD) (value)
#define INITLONG(offset,value)  0xc000,(UWORD) (offset), \
				(UWORD) ((value)>>16), \
				(UWORD) ((value) & 0xffff)
#define INITSTRUCT(size,offset,value,count) \
				(UWORD) (0xc000|(size<<12)|(count<<8)| \
				((UWORD) ((offset)>>16)), \
				((UWORD) (offset)) & 0xffff)
#else
#define INITBYTE(offset,value)  (0x00e0 | ((((ULONG)offset) & 0xff) << 8)),(UWORD) (((ULONG)offset) >> 8),(UWORD) ((value) & 0xff)
#define INITWORD(offset,value)  (0x00d0 | ((((ULONG)offset) & 0xff) << 8)),(UWORD) (((ULONG)offset) >> 8),(UWORD) (value)
#define INITLONG(offset,value)  (0x00c0 | ((((ULONG)offset) & 0xff) << 8)),(UWORD) (((ULONG)offset) >> 8), \
				(UWORD) ((value) & 0xffff), \
				(UWORD) ((value) >> 16)
#define INITSTRUCT(size,offset,value,count) \
				(UWORD) (0x00c0|((size)<<4)|((count)<<0)| \
				((UWORD) ((((ULONG)offset) & 0xff) << 8))), \
				((UWORD) (((ULONG)offset) >> 8))
#endif

#endif /* EXEC_INITIALIZERS_H */
