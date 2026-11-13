#ifndef __INLINE_MACROS_H
#define __INLINE_MACROS_H

#include <emul/emulregs.h> /* Emm */
#include <emul/emulinterface.h>


#define __CACHE_START(start) ((void *) ((unsigned long int) (start) & ~31))
#define __CACHE_LENGTH(start,length) ((((length) + (unsigned long int) (start) + 31) & ~31) - ((unsigned long int) (start) & ~31))



#ifndef __INLINE_STUB_H
#include <inline/stubs.h>
#endif


#ifdef __MORPHOS_DIRECTCALL

#define	REG_d0	REG_D0
#define	REG_d1	REG_D1
#define	REG_d2	REG_D2
#define	REG_d3	REG_D3
#define	REG_d4	REG_D4
#define	REG_d5	REG_D5
#define	REG_d6	REG_D6
#define	REG_d7	REG_D7
#define	REG_a0	REG_A0
#define	REG_a1	REG_A1
#define	REG_a2	REG_A2
#define	REG_a3	REG_A3
#define	REG_a4	REG_A4
#define	REG_a5	REG_A5
#define	REG_a6	REG_A6
#define	REG_a7	REG_A7


#define LP0(offs, rt, name, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
      rt _##name##_re;                                          \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})

#define LP0NR(offs, name, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      REG_##r1           = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})

#define LP1FP(offs, rt, name, t1, v1, r1, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#if 0


#define LP1A5(offs, rt, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})


#define LP1NRA5(offs, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      REG_##r1           = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP1A5FP(offs, rt, name, t1, v1, r1, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 ) \
({                                                              \
   typedef fpt;                                                 \
      rt _##name##_re;                                          \
      REG_A5             = (ULONG) (v1);                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#endif


#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                                         \
      _##name##_re;                                             \
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})


#define LP2FP(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )       \
({                                                              \
   typedef fpt;                                                 \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})


#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP3FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})


#define LP3NRFP(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )     \
({                                                              \
   typedef fpt;                                                 \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP4FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )       \
({                                                              \
   typedef fpt;                                                 \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})


#define LP5FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#if 0

#define LP6A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#endif

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_A6             = (ULONG) (bn);                 \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                                     \
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_A6             = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs);                 \
      _##name##_re;                                             \
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_A6             = (ULONG) (bn);                         \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                     \
})

#if 0


#define LP7A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_A6             = (ULONG) (bn);                         \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})

#endif




#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_A6             = (ULONG) (bn);                         \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})


#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_A6             = (ULONG) (bn);                         \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                     \
})


#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      ULONG _##name##_v9=(ULONG)(v9);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_##r9           = _##name##_v9;                 \
      REG_A6             = (ULONG) (bn);                         \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})


#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      ULONG _##name##_v9=(ULONG)(v9);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_##r9           = _##name##_v9;                 \
      REG_A6             = (ULONG) (bn);                         \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                     \
})



#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      ULONG _##name##_v9=(ULONG)(v9);                  \
      ULONG _##name##_v10=(ULONG)(v10);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_##r9           = _##name##_v9;                 \
      REG_##r10          = _##name##_v10;                \
      REG_A6             = (ULONG) (bn);                         \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})


#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      ULONG _##name##_v9=(ULONG)(v9);                  \
      ULONG _##name##_v10=(ULONG)(v10);                  \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_##r9           = _##name##_v9;                 \
      REG_##r10          = _##name##_v10;                \
      REG_A6             = (ULONG) (bn);                         \
      (*MyEmulHandle->EmulCallDirectOS)(-offs);                     \
})


#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                      \
      ULONG _##name##_v2=(ULONG)(v2);                  \
      ULONG _##name##_v3=(ULONG)(v3);                  \
      ULONG _##name##_v4=(ULONG)(v4);                  \
      ULONG _##name##_v5=(ULONG)(v5);                  \
      ULONG _##name##_v6=(ULONG)(v6);                  \
      ULONG _##name##_v7=(ULONG)(v7);                  \
      ULONG _##name##_v8=(ULONG)(v8);                  \
      ULONG _##name##_v9=(ULONG)(v9);                  \
      ULONG _##name##_v10=(ULONG)(v10);                  \
      ULONG _##name##_v11=(ULONG)(v11);                  \
      rt _##name##_re;                                          \
      REG_##r1           = (ULONG) (v1);                 \
      REG_##r2           = _##name##_v2;                 \
      REG_##r3           = _##name##_v3;                 \
      REG_##r4           = _##name##_v4;                 \
      REG_##r5           = _##name##_v5;                 \
      REG_##r6           = _##name##_v6;                 \
      REG_##r7           = _##name##_v7;                 \
      REG_##r8           = _##name##_v8;                 \
      REG_##r9           = _##name##_v9;                 \
      REG_##r10          = _##name##_v10;                \
      REG_##r11          = _##name##_v11;                \
      REG_A6                     = (ULONG) (bn);                 \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallDirectOS)(-offs); \
      _##name##_re;                                             \
})

#else

#define LP0(offs, rt, name, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})

#define LP0NR(offs, name, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})

#define LP1(offs, rt, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP1NR(offs, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})

#define LP1FP(offs, rt, name, t1, v1, r1, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#if 0


#define LP1A5(offs, rt, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})


#define LP1NRA5(offs, name, t1, v1, r1, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP1A5FP(offs, rt, name, t1, v1, r1, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 ) \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_a5             = (ULONG) (v1);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#endif


#define LP2(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                                         \
      _##name##_re;                                             \
})

#define LP2NR(offs, name, t1, v1, r1, t2, v2, r2, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP2UB(offs, rt, name, t1, v1, r1, t2, v2, r2, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})


#define LP2FP(offs, rt, name, t1, v1, r1, t2, v2, r2, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )       \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP3(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP3NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP3UB(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})


#define LP3NRUB(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP3FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})


#define LP3NRFP(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )     \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})

#define LP4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP4NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP4FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )       \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP5(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP5NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})


#define LP5FP(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, bt, bn, fpt, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   typedef fpt;                                                 \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP6(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#if 0

#define LP6A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#endif

#define LP6NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                                     \
})

#define LP7(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos);                 \
      _##name##_re;                                             \
})

#define LP7NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                     \
})

#if 0


#define LP7A4(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})

#endif




#define LP8(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )      \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})


#define LP8NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )        \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                     \
})


#define LP9(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_##r9           = (ULONG) (v9);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})


#define LP9NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_##r9           = (ULONG) (v9);                 \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                     \
})



#define LP10(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )  \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_##r9           = (ULONG) (v9);                 \
      MyCaos.reg_##r10          = (ULONG) (v10);                \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})


#define LP10NR(offs, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )    \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_##r9           = (ULONG) (v9);                 \
      MyCaos.reg_##r10          = (ULONG) (v10);                \
      MyCaos.reg_a6             = (ULONG) (bn);                         \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      (*MyEmulHandle->EmulCallOS)(&MyCaos);                     \
})


#define LP11(offs, rt, name, t1, v1, r1, t2, v2, r2, t3, v3, r3, t4, v4, r4, t5, v5, r5, t6, v6, r6, t7, v7, r7, t8, v8, r8, t9, v9, r9, t10, v10, r10, t11, v11, r11, bt, bn, cm1, cs1, cl1, cm2, cs2, cl2 )   \
({                                                              \
   struct EmulCaos MyCaos;                                              \
      rt _##name##_re;                                          \
      MyCaos.reg_##r1           = (ULONG) (v1);                 \
      MyCaos.reg_##r2           = (ULONG) (v2);                 \
      MyCaos.reg_##r3           = (ULONG) (v3);                 \
      MyCaos.reg_##r4           = (ULONG) (v4);                 \
      MyCaos.reg_##r5           = (ULONG) (v5);                 \
      MyCaos.reg_##r6           = (ULONG) (v6);                 \
      MyCaos.reg_##r7           = (ULONG) (v7);                 \
      MyCaos.reg_##r8           = (ULONG) (v8);                 \
      MyCaos.reg_##r9           = (ULONG) (v9);                 \
      MyCaos.reg_##r10          = (ULONG) (v10);                \
      MyCaos.reg_##r11          = (ULONG) (v11);                \
      MyCaos.reg_a6                     = (ULONG) (bn);                 \
      MyCaos.caos_Un.Offset     =       -(offs);                \
      _##name##_re = (rt) (*MyEmulHandle->EmulCallOS)(&MyCaos); \
      _##name##_re;                                             \
})

#endif
#endif 
