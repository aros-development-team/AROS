#ifndef _MACHINE_H_
#define _MACHINE_H_
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 15:36:21  digulla
    machine.h is now local for each $(ARCH)
    Removed m68k-Code and i386-#ifdef's


    Desc:
    Lang:
*/

#include <exec/types.h>
#include <stddef.h>

#define SP_OFFSET 0
#define STACK_GROWS_DOWNWARDS 1
#define BIG_ENDIAN 0
struct JumpVec
{
    UBYTE jmp;
    UBYTE vec[4];
};
#define SET_JMP(v)   ((v)->jmp=0xe9)
#define SET_VEC(v,a) (*(ULONG*)(v)->vec=(ULONG)(a)-(ULONG)(v)-5)
#define GET_VEC(v)   ((APTR)(*(ULONG*)(v)->vec+(ULONG)(v)+5))
#define RDFCALL(hook,data,dptr) ((void(*)(UBYTE,APTR))(hook))(data,dptr);

#ifndef UDIVMOD10
#define UDIVMOD10(v,q,r) { ULONG a=v; q=a/10ul; r=a%10ul; }
#endif

#include <exec/libraries.h>
#undef LIBVECTSIZE
#define LIBVECTSIZE sizeof(struct JumpVec)

struct _longalign
{
    char dummy;
    LONG offset;
};

#define LONGALIGN	offsetof(struct _longalign,offset)

struct _wordalign
{
    char dummy;
    WORD offset;
};

#define WORDALIGN	offsetof(struct _wordalign,offset)

struct _ptralign
{
    char dummy;
    APTR offset;
};

#define PTRALIGN	offsetof(struct _ptralign,offset)

struct _doublealign
{
    char dummy;
    double offset;
};

#define DOUBLEALIGN	offsetof(struct _doublealign,offset)

#define LIBALIGN	(DOUBLEALIGN>sizeof(long)?DOUBLEALIGN:sizeof(long))

#define CALLENTRY					\
LONG entry(struct ExecBase *sysbase);                   \
__AROS_LH0(LONG,__entry,struct ExecBase *,sysbase,,)    \
{							\
    __AROS_FUNC_INIT					\
    return entry(sysbase);                              \
    __AROS_FUNC_EXIT					\
}

#endif /* _MACHINE_H_ */
