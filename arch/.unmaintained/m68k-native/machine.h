#ifndef _MACHINE_H_
#define _MACHINE_H_
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/10/21 20:55:01  aros
    Changed BIG_ENDIAN to AROS_BIG_ENDIAN
    Added ALLOCVEC_TOTAL

    Revision 1.3  1996/10/20 02:51:49  aros
    More parallelisation with i386-emul

    Revision 1.2  1996/08/01 17:41:27  digulla
    Added standard header for all files

    Desc:
    Lang:
*/

#include <exec/types.h>
#include <stddef.h>

/*
    How much do I have to add to sp to get the address of the first
    byte on the stack?
*/
#define SP_OFFSET 0

/*
 * Define this to either 1 or 0. The advantage of this single define over
 * one define for each choice is that the compiler groks about a missing
 * define.
 */
#define STACK_GROWS_DOWNWARDS 1

/* Define this to 0 for big endian and 1 for little endian (2 for others?) */
#ifndef AROS_BIG_ENDIAN
#define AROS_BIG_ENDIAN 1
#endif

/* Number of bytes in a ULONG (i.e. result of sizeof(ULONG)) */
#define SIZEOFULONG 4

/*
 * Replace BPTRs by simple APTRs for this machine.
 */
typedef APTR BPTR;
#define MKBADDR(a) ((APTR)(a))
#define BADDR(a) (a)

/* A jumptable entry */
struct JumpVec
{
    UWORD jmp;
    APTR  vec;
};

/* Prepare the jump instruction */
#define SET_JMP(v)   ((v)->jmp=0x4ef9)
/* Set the vector */
#define SET_VEC(v,a) ((v)->vec=(a))
/* Get the vector */
#define GET_VEC(v)   ((v)->vec)

#ifdef __GNUC__

/* Try to avoid linking with special 32 bit code */
#define UDIVMOD10(v,q,r)        \
{ asm("movel %2,%0   ;"         \
      "clrw  %0      ;"         \
      "swap  %0      ;"         \
      "divu  #10:W,%0;"         \
      "movel %0,%1   ;"         \
      "movew %2,%1   ;"         \
      "swap  %0      ;"         \
      "divu  #10:W,%1;"         \
      "movew %1,%0   ;"         \
      "clrw  %1      ;"         \
      "swap  %1      ;"         \
      :"=&d"(q),"=&d"(r):"r"(v) \
    );				\
}

#define RDFCALL(hook,data,dptr)                     \
{						    \
    register unsigned char __d0 __asm("d0") = data; \
    register void *	   __a3 __asm("a3") = dptr; \
    asm volatile("jsr %1@"                          \
	:"=r"(__a3)                                 \
	:"a"(hook),"0"(__a3),"r"(__d0)              \
	:A0,A1,D0,D1,"memory");                     \
    dptr = __a3;				    \
}

#endif

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
LONG entry(struct ExecBase *sysbase);			\
__AROS_LH0(LONG,__entry,struct ExecBase *,sysbase,,)	\
{							\
    __AROS_FUNC_INIT					\
    return entry(sysbase);				\
    __AROS_FUNC_EXIT					\
}

/* This is the extra memory needed by AllocVec() or any other function
   which wants to store an ULONG in the first part of an memory allocation
   and calculate the offset for the first byte to return. */
#define ALLOCVEC_TOTAL \
(DOUBLEALIGN>sizeof(ULONG)?DOUBLEALIGN:sizeof(ULONG))

#endif /* _MACHINE_H_ */
