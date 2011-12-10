/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef LIBCALL_CC_H
#define LIBCALL_CC_H

/* Abuse the d7 register to return condition codes */
register UWORD AROS_LIBFUNC_cc asm ("d7");

/* This ensures that 'SetSR' and friends actually work */
#undef SetSR
#define SetSR(v,m) \
    do { AROS_LIBFUNC_cc = (v) | (AROS_LIBFUNC_cc & ~(m)); } while (0)

#define _S(x)   #x

#define AROS_CCWRAP(t,func, subfunc) \
    asm ( ".global " _S(func) "\n" _S(func) ":\n" \
          "move.l %d7,%sp@-\n" \
          "clr.w  %d7\n" \
          "jsr " _S(subfunc) "\n" \
          "move.w %d7,%a1\n" \
          "move.l %sp@+,%d7\n" \
          "move.w %a1,%sp@-\n" \
          "rtr\n" ); \

#undef AROS_LH2
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
    AROS_CCWRAP(t, AROS_SLIB_ENTRY(n,s,o), AROS_SLIB_ENTRY(n##_cc,s,o)) \
    __AROS_LH2(t, n##_cc, AROS_LHA(a1), AROS_LHA(a2), bt, bn, o, s)

#undef AROS_LH1
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
    AROS_CCWRAP(t, AROS_SLIB_ENTRY(n,s,o), AROS_SLIB_ENTRY(n##_cc,s,o)) \
    __AROS_LH1(t, n##_cc, AROS_LHA(a1), bt, bn, o, s)

#undef AROS_LH1QUAD1
#define AROS_LH1QUAD1(t,n,a1,a2,bt,bn,o,s) \
    AROS_CCWRAP(t, AROS_SLIB_ENTRY(n,s,o), AROS_SLIB_ENTRY(n##_cc,s,o)) \
    __AROS_LH1QUAD1(t, n##_cc, AROS_LHA(a1), AROS_LHAQUAD(a2), bt, bn, o, s)

#undef AROS_LHQUAD1
#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
    AROS_CCWRAP(t, AROS_SLIB_ENTRY(n,s,o), AROS_SLIB_ENTRY(n##_cc,s,o)) \
    __AROS_LHQUAD1(t, n##_cc, AROS_LHAQUAD(a1), bt, bn, o, s)

#undef AROS_LHQUAD2
#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
    AROS_CCWRAP(t, AROS_SLIB_ENTRY(n,s,o), AROS_SLIB_ENTRY(n##_cc,s,o)) \
    __AROS_LHQUAD2(t, n##_cc, AROS_LHAQUAD(a1), AROS_LHAQUAD(a2), bt, bn, o, s)

#endif /* LIBCALL_CC_H */
