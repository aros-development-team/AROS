/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Defines and macros to create and fill an inittable
    Lang: english
*/
#ifndef _INITSTRUCT_H_
#define _INITSTRUCT_H_
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

typedef BYTE type_B;
typedef WORD type_W;
typedef LONG type_L;

#define S_DEF(i,l) union                                                        \
		   {								\
		     struct _##i { l } _l;					\
		     char _s[(sizeof(struct _##i)+AROS_LONGALIGN-1)&~(AROS_LONGALIGN-1)]; \
		   } _##i


#define CODE_B 0x20
#define CODE_W 0x10
#define CODE_L 0x00

#define S_CPY(i,n,t)      S_DEF(i,UBYTE _cmd; type_##t _data[(n)];)
#define S_REP(i,n,t)      S_DEF(i,UBYTE _cmd; type_##t _data;)
#define S_CPYO(i,n,t)     S_DEF(i,UBYTE _cmd; UBYTE _ofst; type_##t _data[(n)];)
#define S_CPYO24(i,n,t)   S_DEF(i,ULONG _cmd; type_##t _data[(n)];)
#define S_END(i)          UBYTE _##i
#define I_CPY(n,t)        CODE_##t|((n)-1)
#define I_REP(n,t)        0x40|CODE_##t|((n)-1)
#define I_CPYO(n,t,o)     0x80|CODE_##t|((n)-1), (o)
#if AROS_BIG_ENDIAN
#define I_CPYO24(n,t,o)   (0xc0|CODE_##t|((n)-1))<<24|(o)
#else
#define I_CPYO24(n,t,o)   0xc0|CODE_##t|((n)-1)|(o)<<8
#endif
#define I_END()           0

#endif
