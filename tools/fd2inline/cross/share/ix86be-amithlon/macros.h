#ifndef __INLINE_MACROS_H
#define __INLINE_MACROS_H

/*
   General macros for Amiga function calls from little- or big-endian
   ix86 code.

   LPX - functions that take X arguments.

   Modifiers (variations are possible):
   NR - no return (void),
   UB - base will be given explicitly by user (see cia.resource).

*/

#ifndef __INLINE_STUB_H
#include <inline/stubs.h>
#endif

#ifndef __INLINE_MACROS_H_REGS
#define __INLINE_MACROS_H_REGS

struct _Regs
{
    ULONG d0;
    ULONG d1;
    ULONG d2;
    ULONG d3;
    ULONG d4;
    ULONG d5;
    ULONG d6;
    ULONG d7;
    ULONG a0;
    ULONG a1;
    ULONG a2;
    ULONG a3;
    ULONG a4;
    ULONG a5;
    ULONG a6;
    ULONG a7;
};

#endif /* __INLINE_MACROS_H_REGS */

ULONG _CallLib68k(struct _Regs*,LONG) __attribute__((__regparm__(3)));
static __inline__ __SIZE_TYPE__ _Return1(void) { return 1; }

#define LP0(offs, rt, name, bt, bn)					\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP0NR(offs, name, bt, bn)					\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn)				\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP1UB(offs, rt, name, t1, v1, r1)				\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn)				\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn)		\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2)			\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn)		\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP2NRUB(offs, name, t1, v1, r1, t2, v2, r2)			\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn)	\
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn)	\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3)	\
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r10):"ri"((ULONG)(v10)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r10):"ri"((ULONG)(v10)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})

#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({									\
   {									\
      rt _##name##_re;                                          	\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r10):"ri"((ULONG)(v10)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r11):"ri"((ULONG)(v11)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _##name##_re = (rt) _CallLib68k(&_##name##_r[0],-offs);		\
      _##name##_re;                                             	\
   }									\
})

#define LP11NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({									\
   {									\
      struct _Regs _##name##_r[_Return1()];				\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r1):"ri"((ULONG)(v1)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r2):"ri"((ULONG)(v2)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r3):"ri"((ULONG)(v3)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r4):"ri"((ULONG)(v4)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r5):"ri"((ULONG)(v5)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r6):"ri"((ULONG)(v6)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r7):"ri"((ULONG)(v7)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r8):"ri"((ULONG)(v8)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r9):"ri"((ULONG)(v9)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r10):"ri"((ULONG)(v10)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].##r11):"ri"((ULONG)(v11)));\
      __asm__("movl %1,%0":"=m"(_##name##_r[0].a6)  :"ri"((ULONG)(bn)));\
      _CallLib68k(&_##name##_r[0],-offs);				\
   }									\
})


#endif /* __INLINE_MACROS_H */
