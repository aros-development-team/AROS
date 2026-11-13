#ifndef __INLINE_MACROS_H
#define __INLINE_MACROS_H

/*
   General macros for Amiga function calls. Not all the possibilities have
   been created - only the ones which exist in OS 3.1. Third party libraries
   and future versions of AmigaOS will maybe need some new ones...

   LPX - functions that take X arguments.

   Modifiers (variations are possible):
   NR - no return (void),
   A4, A5 - "a4" or "a5" is used as one of the arguments,
   UB - base will be given explicitly by user (see cia.resource).
   FP - one of the parameters has type "pointer to function".

   "bt" arguments are not used - they are provided for backward compatibility
   only.
*/

#ifndef __INLINE_STUB_H
#include <inline/stubs.h>
#endif

#define LP0(offs, rt, name, bt, bn)				\
({								\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn)					\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP0NR(offs, name, bt, bn)				\
({								\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn)					\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only graphics.library/AttemptLockLayerRom() */
#define LP1A5(offs, rt, name, t1, v1, r1, bt, bn)		\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr a6@(-"#offs":W)\n\texg d7,a5" \
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Only graphics.library/LockLayerRom() and graphics.library/UnlockLayerRom() */
#define LP1NRA5(offs, name, t1, v1, r1, bt, bn)			\
({								\
   t1 _##name##_v1 = (v1);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr a6@(-"#offs":W)\n\texg d7,a5" \
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only exec.library/Supervisor() */
#define LP1A5FP(offs, rt, name, t1, v1, r1, bt, bn, fpt)	\
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      __asm volatile ("exg d7,a5\n\tjsr a6@(-"#offs":W)\n\texg d7,a5" \
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only cia.resource/AbleICR() and cia.resource/SetICR() */
#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2)		\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r"(_n1), "rf"(_n2)					\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Only dos.library/InternalUnLoadSeg() */
#define LP2FP(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2)		\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only cia.resource/AddICRVector() */
#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r"(_n1), "rf"(_n2), "rf"(_n3)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Only cia.resource/RemICRVector() */
#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3)	\
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r"(_n1), "rf"(_n2), "rf"(_n3)				\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only exec.library/SetFunction() */
#define LP3FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Only graphics.library/SetCollision() */
#define LP3NRFP(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3)	\
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only exec.library/RawDoFmt() */
#define LP4FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only exec.library/MakeLibrary() */
#define LP5FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, fpt) \
({								\
   typedef fpt;							\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only workbench.library/AddAppIconA() */
#define LP7A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      __asm volatile ("exg d7,a4\n\tjsr a6@(-"#offs":W)\n\texg d7,a4" \
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Would you believe that there really are beasts that need more than 7
   arguments? :-) */

/* For example intuition.library/AutoRequest() */
#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* For example intuition.library/ModifyProp() */
#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* For example layers.library/CreateUpfrontHookLayer() */
#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* For example intuition.library/NewModifyProp() */
#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Kriton Kyrimis <kyrimis@cti.gr> says CyberGraphics needs the following */
#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

/* Only graphics.library/BltMaskBitMapRastPort() */
#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   {								\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : /* no output */						\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
   }								\
})

/* Only graphics.library/BltBitMap() */
#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn) \
({								\
   t1 _##name##_v1 = (v1);					\
   t2 _##name##_v2 = (v2);					\
   t3 _##name##_v3 = (v3);					\
   t4 _##name##_v4 = (v4);					\
   t5 _##name##_v5 = (v5);					\
   t6 _##name##_v6 = (v6);					\
   t7 _##name##_v7 = (v7);					\
   t8 _##name##_v8 = (v8);					\
   t9 _##name##_v9 = (v9);					\
   t10 _##name##_v10 = (v10);					\
   t11 _##name##_v11 = (v11);					\
   {								\
      register rt _##name##_re __asm("d0");			\
      register struct Library *const _##name##_bn __asm("a6") = (struct Library*)(bn); \
      register t1 _n1 __asm(#r1) = _##name##_v1;		\
      register t2 _n2 __asm(#r2) = _##name##_v2;		\
      register t3 _n3 __asm(#r3) = _##name##_v3;		\
      register t4 _n4 __asm(#r4) = _##name##_v4;		\
      register t5 _n5 __asm(#r5) = _##name##_v5;		\
      register t6 _n6 __asm(#r6) = _##name##_v6;		\
      register t7 _n7 __asm(#r7) = _##name##_v7;		\
      register t8 _n8 __asm(#r8) = _##name##_v8;		\
      register t9 _n9 __asm(#r9) = _##name##_v9;		\
      register t10 _n10 __asm(#r10) = _##name##_v10;		\
      register t11 _n11 __asm(#r11) = _##name##_v11;		\
      __asm volatile ("jsr a6@(-"#offs":W)"			\
      : "=r" (_##name##_re)					\
      : "r" (_##name##_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4), "rf"(_n5), "rf"(_n6), "rf"(_n7), "rf"(_n8), "rf"(_n9), "rf"(_n10), "rf"(_n11) \
      : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");	\
      _##name##_re;						\
   }								\
})

#endif /* __INLINE_MACROS_H */
