#ifndef EXEC_INITIALIZERS_H
#define EXEC_INITIALIZERS_H

#include <aros/machine.h>

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Macros to build init structs (eg. for InitStruct())
    Lang: english
*/

#define OFFSET(structName, structEntry) \
				(&(((struct structName *) 0)->structEntry))

#if AROS_BIG_ENDIAN
#warning Big ENDIAN machine!
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
#warning LITTLE ENDIAN!
#define INITBYTE(offset,value)  0x00e0,(UWORD) (offset),(UWORD) ((value)<<8)
#define INITWORD(offset,value)  0x00d0,(UWORD) (offset),(UWORD) (value)
#define INITLONG(offset,value)  0x00c0,(UWORD) (offset), \
				(UWORD) ((value)>>16), \
				(UWORD) ((value) & 0xffff)
#define INITSTRUCT(size,offset,value,count) \
				(UWORD) (0x00c0|(size<<4)|(count<<0)| \
				((UWORD) ((offset)>>16)), \
				((UWORD) (offset)) & 0xffff)
#endif

#endif /* EXEC_INITIALIZERS_H */
