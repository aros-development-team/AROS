/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros for calling regular library functions with base-relative
	  global variables
    Lang: english
*/

#include <aros/relbase.h>

#ifndef __AROS_CPU_SPECIFIC_LH
/* Library functions which need the libbase */
#ifndef __AROS_LH_DEFBASE
#define __AROS_LH_DEFBASE(bt,bn) \
    bt __attribute__((__unused__)) bn = (bt)AROS_GET_LIBBASE;
#endif

#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHAQUAD(a1)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHAQUAD(a1),\
    __AROS_LHAQUAD(a2)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH1QUAD1(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHAQUAD(a2)) { \
    __AROS_LH_DEFBASE(bt, bn)

#define AROS_LH0(t,n,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    void) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10),\
    __AROS_LHA(a11)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10),\
    __AROS_LHA(a11),\
    __AROS_LHA(a12)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10),\
    __AROS_LHA(a11),\
    __AROS_LHA(a12),\
    __AROS_LHA(a13)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10),\
    __AROS_LHA(a11),\
    __AROS_LHA(a12),\
    __AROS_LHA(a13),\
    __AROS_LHA(a14)) { \
    __AROS_LH_DEFBASE(bt, bn)
#define AROS_LH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10),\
    __AROS_LHA(a11),\
    __AROS_LHA(a12),\
    __AROS_LHA(a13),\
    __AROS_LHA(a14),\
    __AROS_LHA(a15)) { \
    __AROS_LH_DEFBASE(bt, bn)
#endif /* !__AROS_CPU_SPECIFIC_LH */

#ifndef AROS_LC_CALL

#define AROS_LC_CALL(t,x,bn) \
({ \
    t _ret; \
    void *old = AROS_SET_LIBBASE(bn); \
    _ret = x; \
    (void)AROS_SET_LIBBASE(old); \
    _ret; \
})
#define AROS_LC_CALLNR(x,bn) \
({ \
    void * old = AROS_SET_LIBBASE(bn); \
    x; \
    (void)AROS_SET_LIBBASE(old); \
})

#endif //AROS_LC_CALL

/* Call a library function which requires the libbase */
#ifndef __AROS_CPU_SPECIFIC_LC
# define AROS_LCQUAD1(t,n,a1,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPAQUAD(a1)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCAQUAD(a1))),\
    bn)
#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPAQUAD(a1),\
    __AROS_LPAQUAD(a2)))__AROS_GETVECADDR(bn,o))(       \
    __AROS_LCAQUAD(a1),\
    __AROS_LCAQUAD(a2))),\
    bn)
#define AROS_LC1QUAD1(t,n,a1,a2,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPAQUAD(a2)))__AROS_GETVECADDR(bn,o))(       \
    __AROS_LCA(a1),\
    __AROS_LCAQUAD(a2))),\
    bn)

#define AROS_LC0(t,n,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(void))__AROS_GETVECADDR(bn,o))()),\
    bn)
#define AROS_LC1(t,n,a1,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1))),\
    bn)
#define AROS_LC2(t,n,a1,a2,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2))),\
    bn)
#define AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3))),\
    bn)
#define AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4))),\
    bn)
#define AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5))),\
    bn)
#define AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6))),\
    bn)
#define AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7))),\
    bn)
#define AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8))),\
    bn)
#define AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9))),\
    bn)
#define AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10))),\
    bn)
#define AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11))),\
    bn)
#define AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12))),\
    bn)
#define AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13))),\
    bn)
#define AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14))),\
    bn)
#define AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    AROS_LC_CALL(t, \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14),\
    __AROS_LPA(a15)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14),\
    __AROS_LCA(a15))),\
    bn)

#define AROS_LC0NR(t,n,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(void))__AROS_GETVECADDR(bn,o))()),\
    bn)
#define AROS_LC1NR(t,n,a1,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1))),\
    bn)
#define AROS_LC2NR(t,n,a1,a2,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2))),\
    bn)
#define AROS_LC3NR(t,n,a1,a2,a3,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3))),\
    bn)
#define AROS_LC4NR(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4))),\
    bn)
#define AROS_LC5NR(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5))),\
    bn)
#define AROS_LC6NR(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6))),\
    bn)
#define AROS_LC7NR(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7))),\
    bn)
#define AROS_LC8NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8))),\
    bn)
#define AROS_LC9NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9))),\
    bn)
#define AROS_LC10NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10))),\
    bn)
#define AROS_LC11NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11))),\
    bn)
#define AROS_LC12NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12))),\
    bn)
#define AROS_LC13NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13))),\
    bn)
#define AROS_LC14NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14))),\
    bn)
#define AROS_LC15NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    AROS_LC_CALLNR( \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14),\
    __AROS_LPA(a15)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14),\
    __AROS_LCA(a15))),\
    bn)


/* Special calls: Call a library function without the name just by the ADDRESS */
#ifndef AROS_CALL0
#define AROS_CALL0(returntype,address,basetype,basename) \
    AROS_LC_CALL(returntype,\
    (((__AROS_LC_PREFIX returntype(*)(void))\
    (void *)(address))()),\
    basename)
#endif

#ifndef AROS_CALL0NR
#define AROS_CALL0NR(returntype,address,basetype,basename) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX returntype(*)(void))\
    (void *)(address))()),\
    basename)
#endif

#ifndef AROS_CALL1
#define AROS_CALL1(t,a,a1,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1)))\
    (void *)(a))(\
    __AROS_LCA(a1))),\
    bn)
#endif

#ifndef AROS_CALL1NR
#define AROS_CALL1NR(t,a,a1,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1)))\
    (void *)(a))(\
    __AROS_LCA(a1))),\
    bn)
#endif

#ifndef AROS_CALL2
#define AROS_CALL2(t,a,a1,a2,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2))),\
    bn)
#endif

#ifndef AROS_CALL2NR
#define AROS_CALL2NR(t,a,a1,a2,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2))),\
    bn)
#endif

#ifndef AROS_CALL3
#define AROS_CALL3(t,a,a1,a2,a3,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3))),\
    bn)
#endif

#ifndef AROS_CALL3NR
#define AROS_CALL3NR(t,a,a1,a2,a3,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3))),\
    bn)
#endif

#ifndef AROS_CALL4
#define AROS_CALL4(t,a,a1,a2,a3,a4,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4))),\
    bn)
#endif

#ifndef AROS_CALL4NR
#define AROS_CALL4NR(t,a,a1,a2,a3,a4,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4))),\
    bn)
#endif

#ifndef AROS_CALL5
#define AROS_CALL5(t,a,a1,a2,a3,a4,a5,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5))),\
    bn)
#endif

#ifndef AROS_CALL5NR
#define AROS_CALL5NR(t,a,a1,a2,a3,a4,a5,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5))),\
    bn)
#endif

#ifndef AROS_CALL6
#define AROS_CALL6(t,a,a1,a2,a3,a4,a5,a6,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6))),\
    bn)
#endif

#ifndef AROS_CALL6NR
#define AROS_CALL6NR(t,a,a1,a2,a3,a4,a5,a6,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6))),\
    bn)
#endif

#ifndef AROS_CALL7
#define AROS_CALL7(t,a,a1,a2,a3,a4,a5,a6,a7,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7))),\
    bn)
#endif

#ifndef AROS_CALL7NR
#define AROS_CALL7NR(t,a,a1,a2,a3,a4,a5,a6,a7,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7))),\
    bn)
#endif

#ifndef AROS_CALL8
#define AROS_CALL8(t,a,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8))),\
    bn)
#endif

#ifndef AROS_CALL8NR
#define AROS_CALL8NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8))),\
    bn)
#endif

#ifndef AROS_CALL9
#define AROS_CALL9(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9))),\
    bn)
#endif

#ifndef AROS_CALL9NR
#define AROS_CALL9NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9))),\
    bn)
#endif

#ifndef AROS_CALL10
#define AROS_CALL10(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn) \
    AROS_LC_CALL(t,\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9),\
    __AROS_LDA(a10)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10))),\
    bn)
#endif

#ifndef AROS_CALL10NR
#define AROS_CALL10NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn) \
    AROS_LC_CALLNR(\
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9),\
    __AROS_LDA(a10)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10))),\
    bn)
#endif

/* Special calls: Call a library function without the name just by the OFFSET */

#ifndef AROS_LVO_CALL0
#define AROS_LVO_CALL0(returntype,basetype,basename,offset,system) \
    AROS_CALL0(returntype,__AROS_GETVECADDR(basename,offset),basetype,basename)
#endif

#ifndef AROS_LVO_CALL0NR
#define AROS_LVO_CALL0NR(returntype,basetype,basename,offset,system) \
    AROS_CALL0NR(returntype,__AROS_GETVECADDR(basename,offset),basetype,basename)
#endif

#ifndef AROS_LVO_CALL1
#define AROS_LVO_CALL1(t,a1,bt,bn,o,s) \
    AROS_CALL1(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),bt,bn)
#endif

#ifndef AROS_LVO_CALL1NR
#define AROS_LVO_CALL1NR(t,a1,bt,bn,o,s) \
    AROS_CALL1NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),bt,bn)
#endif

#ifndef AROS_LVO_CALL2
#define AROS_LVO_CALL2(t,a1,a2,bt,bn,o,s) \
    AROS_CALL2(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),bt,bn)
#endif

#ifndef AROS_LVO_CALL2NR
#define AROS_LVO_CALL2NR(t,a1,a2,bt,bn,o,s) \
    AROS_CALL2NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),bt,bn)
#endif

#ifndef AROS_LVO_CALL3
#define AROS_LVO_CALL3(t,a1,a2,a3,bt,bn,o,s) \
    AROS_CALL3(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),bt,bn)
#endif

#ifndef AROS_LVO_CALL3NR
#define AROS_LVO_CALL3NR(t,a1,a2,a3,bt,bn,o,s) \
    AROS_CALL3NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),bt,bn)
#endif

#ifndef AROS_LVO_CALL4
#define AROS_LVO_CALL4(t,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_CALL4(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),bt,bn)
#endif

#ifndef AROS_LVO_CALL4NR
#define AROS_LVO_CALL4NR(t,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_CALL4NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),bt,bn)
#endif

#ifndef AROS_LVO_CALL5
#define AROS_LVO_CALL5(t,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_CALL5(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),bt,bn)
#endif

#ifndef AROS_LVO_CALL5NR
#define AROS_LVO_CALL5NR(t,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_CALL5NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),bt,bn)
#endif

#ifndef AROS_LVO_CALL6
#define AROS_LVO_CALL6(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_CALL6(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),bt,bn)
#endif

#ifndef AROS_LVO_CALL6NR
#define AROS_LVO_CALL6NR(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_CALL6NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),bt,bn)
#endif

#ifndef AROS_LVO_CALL7
#define AROS_LVO_CALL7(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_CALL7(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),bt,bn)
#endif

#ifndef AROS_LVO_CALL7NR
#define AROS_LVO_CALL7NR(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_CALL7NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),bt,bn)
#endif

#ifndef AROS_LVO_CALL8
#define AROS_LVO_CALL8(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_CALL8(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),bt,bn)
#endif

#ifndef AROS_LVO_CALL8NR
#define AROS_LVO_CALL8NR(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_CALL8NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),bt,bn)
#endif

#ifndef AROS_LVO_CALL9
#define AROS_LVO_CALL9(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL9(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),bt,bn)
#endif

#ifndef AROS_LVO_CALL9NR
#define AROS_LVO_CALL9NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL9NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),bt,bn)
#endif

#ifndef AROS_LVO_CALL10
#define AROS_LVO_CALL10(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    AROS_CALL10(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),AROS_LCA(a10),bt,bn)
#endif

#ifndef AROS_LVO_CALL10NR
#define AROS_LVO_CALL10NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL10NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),AROS_LCA(a10),bt,bn)
#endif
#endif /* !__AROS_CPU_SPECIFIC_LC */

#ifndef __AROS_CPU_SPECIFIC_LD
/* Declarations for library functions which need the libbase */
#   define AROS_LDQUAD1(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1))
#   define AROS_LDQUAD2(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1), \
	__AROS_LDAQUAD(a2))

#   define AROS_LD0(t,n,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) (void)
#   define AROS_LD1(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1))
#   define AROS_LD2(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2))
#   define AROS_LD3(t,n,a1,a2,a3,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3))
#   define AROS_LD4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4))
#   define AROS_LD5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5))
#   define AROS_LD6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6))
#   define AROS_LD7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7))
#   define AROS_LD8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8))
#   define AROS_LD9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9))
#   define AROS_LD10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10))
#   define AROS_LD11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11))
#   define AROS_LD12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12))
#   define AROS_LD13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13))
#   define AROS_LD14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14))
#   define AROS_LD15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14), \
	__AROS_LDA(a15))

/* Declarations for library functions which don't need the libbase */
#   define AROS_LD0I(t,n,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) (void)
#   define AROS_LD1I(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1))
#   define AROS_LD2I(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2))
#   define AROS_LD3I(t,n,a1,a2,a3,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3))
#   define AROS_LD4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4))
#   define AROS_LD5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5))
#   define AROS_LD6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6))
#   define AROS_LD7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7))
#   define AROS_LD8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8))
#   define AROS_LD9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9))
#   define AROS_LD10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10))
#   define AROS_LD11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11))
#   define AROS_LD12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12))
#   define AROS_LD13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13))
#   define AROS_LD14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14))
#   define AROS_LD15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14), \
	__AROS_LDA(a15))
#endif /* !__AROS_CPU_SPECIFIC_LD */
