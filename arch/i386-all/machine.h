#ifndef AROS_MACHINE_H
#define AROS_MACHINE_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/10/21 20:53:17  aros
    Changed BIG_ENDIAN to AROS_BIG_ENDIAN

    Revision 1.6  1996/10/19 17:12:24  aros
    This file is now linked to aros/machine.h
    Changed the #define to reflect this
    ALLOCVEC_TOTAL is now here because I need it in malloc(), too.

    Revision 1.5  1996/10/10 13:25:09  digulla
    Handle BPTR

    Revision 1.4  1996/08/15 13:21:56  digulla
    BIG_ENDIAN is already defined in some system header file.
    New #define: SIZEOFULONG is the number of bytes in an ULONG (ie. the
	result of sizeof(ULONG)

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
#ifndef AROS_BIG_ENDIAN
#define AROS_BIG_ENDIAN 0
#endif
#define SIZEOFULONG	4

/*
 * Replace BPTRs by simple APTRs for this machine.
 */
typedef APTR BPTR;
#define MKBADDR(a) ((APTR)(a))
#define BADDR(a) (a)

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

/* This is the extra memory needed by AllocVec() or any other function
   which wants to store an ULONG in the first part of an memory allocation
   and calculate the offset for the first byte to return. */
#define ALLOCVEC_TOTAL \
(DOUBLEALIGN>sizeof(ULONG)?DOUBLEALIGN:sizeof(ULONG))

#endif /* AROS_MACHINE_H */
