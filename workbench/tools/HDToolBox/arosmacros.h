#ifndef AROSMACROS_H
#define AROSMACROS_H

#ifdef __AMIGAOS__
#	define A0 a0
#	define A1 a1
#	define A2 a2
#	define D1 d1
/* lib macros */
#	define AROS_LH1(r, n, arg1, bt, bn, bo, bnb) \
		r ASM SAVEDS n(arg1, AROS_LHA(bt, bn, a6))
#	define AROS_LH2(r, n, arg1, arg2, bt, bn, bo, bnb) \
		r ASM SAVEDS n(arg1, arg2, AROS_LHA(bt, bn, a6))
#	define AROS_LHA(t,n,r) REGPARAM(r,t,n)
#	define AROS_LIBFUNC_INIT
#	define AROS_LIBFUNC_EXIT
/* user function macros */
#	define AROS_UFH3S(r, n, arg1, arg2, arg3) \
		r ASM SAVEDS n(arg1, arg2, arg3)
#	define AROS_UFHA(t, n, r) REGPARAM(r,t,n)
#	define AROS_USERFUNC_INIT
#	define AROS_USERFUNC_EXIT
#	define AROS_ASMSYMNAME(n) n
/* endian macros */
#	define AROS_BE2WORD(x) x
#	define AROS_BE2LONG(x) x
#	define AROS_LONG2BE(x) x
#	define AROS_WORD2BE(x) x
#	define AROS_WORD2LE(w)     ((((w) >> 8) & 0x00FF) | (((w) & 0x00FF) << 8))
#	define AROS_LONG2LE(l)     \
   (                                  \
       ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
       ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
       ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
       ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
   )
#	define AROS_LE2WORD(w)     AROS_WORD2LE(w)
#	define AROS_LE2LONG(l)     AROS_LONG2LE(l)
/* BSTR handling */
#	define AROS_BSTR_ADDR(s)        (((STRPTR)BADDR(s))+1)
#	define AROS_BSTR_strlen(s)      (AROS_BSTR_ADDR(s)[-1])
#	define AROS_BSTR_setstrlen(s,l) (AROS_BSTR_ADDR(s)[-1] = l)
#	define AROS_BSTR_getchar(s,l)   (AROS_BSTR_ADDR(s)[l])
#	define AROS_BSTR_putchar(s,l,c) (AROS_BSTR_ADDR(s)[l] = c)

#	define dn_OldName dn_Name
#else
#	include <aros/macros.h>
#	include <aros/asmcall.h>
#	define AROS_WORD2LE(x) x
#	define AROS_LONG2LE(x) x
#	define AROS_LE2WORD(x) x
#	define AROS_LE2LONG(x) x
#endif

#endif /* AROSMACROS_H */

