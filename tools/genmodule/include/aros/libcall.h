#ifndef AROS_LIBCALL_H
#define AROS_LIBCALL_H

#ifndef __typedef_VOID_FUNC
#define __typedef_VOID_FUNC
typedef void (*VOID_FUNC)();
#endif
#ifndef __typedef_LONG_FUNC
#define __typedef_LONG_FUNC
typedef long (*LONG_FUNC)();
#endif
#ifndef __typedef_ULONG_FUNC
#define __typedef_ULONG_FUNC
typedef unsigned long (*ULONG_FUNC)();
#endif

#define AROS_LHA(type,name,reg) type,name,reg
#define __AROS_LHA_GENMOD(type,name,reg) type name ## _ ## reg
#define AROS_LHAQUAD(type,name,reg1,reg2) type,name,reg1,reg2
#define __AROS_LHAQUAD_GENMOD(type,name,reg1,reg2) type name ## _ ## reg1 ## _ ## reg2

#define AROS_LH0(t,n,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o (void)
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1))
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2))
#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3))
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4))
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5))
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6))
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7))
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8))
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9))
#define AROS_LH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10))
#define AROS_LH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11))
#define AROS_LH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12))
#define AROS_LH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13))
#define AROS_LH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14))
#define AROS_LH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14),\
    __AROS_LHA_GENMOD(a15))


#define AROS_NTLH0(t,n,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o (void)
#define AROS_NTLH1(t,n,a1,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1))
#define AROS_NTLH2(t,n,a1,a2,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2))
#define AROS_NTLH3(t,n,a1,a2,a3,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3))
#define AROS_NTLH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4))
#define AROS_NTLH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5))
#define AROS_NTLH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6))
#define AROS_NTLH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7))
#define AROS_NTLH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8))
#define AROS_NTLH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9))
#define AROS_NTLH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10))
#define AROS_NTLH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11))
#define AROS_NTLH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12))
#define AROS_NTLH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13))
#define AROS_NTLH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14))
#define AROS_NTLH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    t AROS_NTLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14),\
    __AROS_LHA_GENMOD(a15))


#define AROS_PLH0(t,n,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o (void)
#define AROS_PLH1(t,n,a1,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1))
#define AROS_PLH2(t,n,a1,a2,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2))
#define AROS_PLH3(t,n,a1,a2,a3,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3))
#define AROS_PLH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4))
#define AROS_PLH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5))
#define AROS_PLH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6))
#define AROS_PLH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7))
#define AROS_PLH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8))
#define AROS_PLH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9))
#define AROS_PLH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10))
#define AROS_PLH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11))
#define AROS_PLH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12))
#define AROS_PLH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13))
#define AROS_PLH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14))
#define AROS_PLH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    t AROS_PLH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14),\
    __AROS_LHA_GENMOD(a15))


#define AROS_LH0I(t,n,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o (void)
#define AROS_LH1I(t,n,a1,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1))
#define AROS_LH2I(t,n,a1,a2,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2))
#define AROS_LH3I(t,n,a1,a2,a3,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3))
#define AROS_LH4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4))
#define AROS_LH5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5))
#define AROS_LH6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6))
#define AROS_LH7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7))
#define AROS_LH8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8))
#define AROS_LH9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9))
#define AROS_LH10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10))
#define AROS_LH11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11))
#define AROS_LH12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12))
#define AROS_LH13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13))
#define AROS_LH14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14))
#define AROS_LH15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    t AROS_LH_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8),\
    __AROS_LHA_GENMOD(a9),\
    __AROS_LHA_GENMOD(a10),\
    __AROS_LHA_GENMOD(a11),\
    __AROS_LHA_GENMOD(a12),\
    __AROS_LHA_GENMOD(a13),\
    __AROS_LHA_GENMOD(a14),\
    __AROS_LHA_GENMOD(a15))

#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
    t AROS_LHQ_ ## s ## _ ## n ## _ ## o(\
    __AROS_LHAQUAD_GENMOD(a1))
#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s)(\
    __AROS_LHAQUAD(a1),\
    __AROS_LHAQUAD(a2),\
    __AROS_LH_BASE(bt,bn))


#define AROS_LP0(t,n,bt,bn,o,s)
#define AROS_LP1(t,n,a1,bt,bn,o,s)
#define AROS_LP2(t,n,a1,a2,bt,bn,o,s)
#define AROS_LP3(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LP4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LP5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LP6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LP7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#define AROS_LD0(t,n,bt,bn,o,s)
#define AROS_LD1(t,n,a1,bt,bn,o,s)
#define AROS_LD2(t,n,a1,a2,bt,bn,o,s)
#define AROS_LD3(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LD4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LD5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LD6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LD7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LD8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LD9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LD10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LD11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LD12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LD13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LD14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LD15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#define AROS_LD0I(t,n,bt,bn,o,s)
#define AROS_LD1I(t,n,a1,bt,bn,o,s)
#define AROS_LD2I(t,n,a1,a2,bt,bn,o,s)
#define AROS_LD3I(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LD4I(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LD5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LD6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LD7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LD8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LD9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LD10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LD11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LD12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LD13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LD14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LD15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#define AROS_LC0(t,n,bt,bn,o,s) ((void*)NULL)
#define AROS_LC1(t,n,a1,bt,bn,o,s) ((void*)NULL)
#define AROS_LC2(t,n,a1,a2,bt,bn,o,s) ((void*)NULL)
#define AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s) ((void*)NULL)
#define AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s) ((void*)NULL)
#define AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) ((void*)NULL)
#define AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) ((void*)NULL)
#define AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) ((void*)NULL)
#define AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) ((void*)NULL)
#define AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) ((void*)NULL)
#define AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) ((void*)NULL)
#define AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) ((void*)NULL)
#define AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) ((void*)NULL)
#define AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) ((void*)NULL)
#define AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) ((void*)NULL)
#define AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) ((void*)NULL)

#define AROS_LC0NR(t,n,bt,bn,o,s) ((void*)NULL)
#define AROS_LC1NR(t,n,a1,bt,bn,o,s) ((void*)NULL)
#define AROS_LC2NR(t,n,a1,a2,bt,bn,o,s) ((void*)NULL)
#define AROS_LC3NR(t,n,a1,a2,a3,bt,bn,o,s) ((void*)NULL)
#define AROS_LC4NR(t,n,a1,a2,a3,a4,bt,bn,o,s) ((void*)NULL)
#define AROS_LC5NR(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) ((void*)NULL)
#define AROS_LC6NR(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) ((void*)NULL)
#define AROS_LC7NR(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) ((void*)NULL)
#define AROS_LC8NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) ((void*)NULL)
#define AROS_LC9NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) ((void*)NULL)
#define AROS_LC10NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) ((void*)NULL)
#define AROS_LC11NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) ((void*)NULL)
#define AROS_LC12NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) ((void*)NULL)
#define AROS_LC13NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) ((void*)NULL)
#define AROS_LC14NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) ((void*)NULL)
#define AROS_LC15NR(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) ((void*)NULL)

#define AROS_LPQUAD1(t,n,a1,bt,bn,o,s)
#define AROS_LPQUAD2(t,n,a1,a2,bt,bn,o,s)

#define AROS_LCQUAD1(t,n,a1,bt,bn,o,s) ((void*)NULL)
#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s) ((void*)NULL)

#ifndef AROS_CALL0
#define AROS_CALL0(returntype,address,basetype,basename) ((void*)NULL)
#endif

#ifndef AROS_CALL0NR
#define AROS_CALL0NR(returntype,address,basetype,basename) ((void*)NULL)
#endif

#ifndef AROS_CALL1
#define AROS_CALL1(t,a,a1,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL1NR
#define AROS_CALL1NR(t,a,a1,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL2
#define AROS_CALL2(t,a,a1,a2,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL2NR
#define AROS_CALL2NR(t,a,a1,a2,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL3
#define AROS_CALL3(t,a,a1,a2,a3,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL3NR
#define AROS_CALL3NR(t,a,a1,a2,a3,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL4
#define AROS_CALL4(t,a,a1,a2,a3,a4,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL4NR
#define AROS_CALL4NR(t,a,a1,a2,a3,a4,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL5
#define AROS_CALL5(t,a,a1,a2,a3,a4,a5,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL5NR
#define AROS_CALL5NR(t,a,a1,a2,a3,a4,a5,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL6
#define AROS_CALL6(t,a,a1,a2,a3,a4,a5,a6,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL6NR
#define AROS_CALL6NR(t,a,a1,a2,a3,a4,a5,a6,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL7
#define AROS_CALL7(t,a,a1,a2,a3,a4,a5,a6,a7,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL7NR
#define AROS_CALL7NR(t,a,a1,a2,a3,a4,a5,a6,a7,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL8
#define AROS_CALL8(t,a,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL8NR
#define AROS_CALL8NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL9
#define AROS_CALL9(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL9NR
#define AROS_CALL9NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL10
#define AROS_CALL10(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_CALL10NR
#define AROS_CALL10NR(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL0
#define AROS_LVO_CALL0(returntype,basetype,basename,offset,system) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL0NR
#define AROS_LVO_CALL0NR(returntype,basetype,basename,offset,system) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL1
#define AROS_LVO_CALL1(t,a1,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL1NR
#define AROS_LVO_CALL1NR(t,a1,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL2
#define AROS_LVO_CALL2(t,a1,a2,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL2NR
#define AROS_LVO_CALL2NR(t,a1,a2,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL3
#define AROS_LVO_CALL3(t,a1,a2,a3,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL3NR
#define AROS_LVO_CALL3NR(t,a1,a2,a3,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL4
#define AROS_LVO_CALL4(t,a1,a2,a3,a4,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL4NR
#define AROS_LVO_CALL4NR(t,a1,a2,a3,a4,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL5
#define AROS_LVO_CALL5(t,a1,a2,a3,a4,a5,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL5NR
#define AROS_LVO_CALL5NR(t,a1,a2,a3,a4,a5,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL6
#define AROS_LVO_CALL6(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL6NR
#define AROS_LVO_CALL6NR(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL7
#define AROS_LVO_CALL7(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL7NR
#define AROS_LVO_CALL7NR(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL8
#define AROS_LVO_CALL8(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL8NR
#define AROS_LVO_CALL8NR(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL9
#define AROS_LVO_CALL9(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL9NR
#define AROS_LVO_CALL9NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL10
#define AROS_LVO_CALL10(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) ((void*)NULL)
#endif

#ifndef AROS_LVO_CALL10NR
#define AROS_LVO_CALL10NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) ((void*)NULL)
#endif

#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT

#define AROS_LIBBASE_EXT_DECL(a,b)

#  endif /* AROS_LIBCALL_H */
