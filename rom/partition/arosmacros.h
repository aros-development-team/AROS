#ifndef AROSMACROS_H
#define AROSMACROS_H

#ifdef __AMIGAOS__
#   define A1 a1
#   define A2 a2
#   define D1 d1
/* lib macros */
#   define AROS_LH1(r, n, arg1, bt, bn, bo, bnb) \
        r ASM SAVEDS bnb##_##n(arg1, AROS_LHA(bt, bn, a6))
#   define AROS_LH2(r, n, arg1, arg2, bt, bn, bo, bnb) \
        r ASM SAVEDS bnb##_##n(arg1, arg2, AROS_LHA(bt, bn, a6))
#   define AROS_LHA(t,n,r) REGPARAM(r,t,n)
#   define AROS_LIBFUNC_INIT
#   define AROS_LIBFUNC_EXIT
/* endian macros */
#   define AROS_BE2LONG(x) x
#   define AROS_LONG2BE(x) x
#   define AROS_WORD2BE(x) x
#   define AROS_WORD2LE(w)     ((((w) >> 8) & 0x00FF) | (((w) & 0x00FF) << 8))
#   define AROS_LONG2LE(l)     \
   (                                  \
       ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
       ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
       ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
       ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
   )
#   define AROS_LE2WORD(w)     AROS_WORD2LE(w)
#   define AROS_LE2LONG(l)     AROS_LONG2LE(l)
#else
#   include <aros/macros.h>
#endif

#endif /* AROSMACROS_H */

