#ifndef AROS_LIBCALL_H
#define AROS_LIBCALL_H

#define AROS_LHA(type,name,reg) type,name,reg
#define __AROS_LHA_GENMOD(type,name,reg) type name ## _ ## reg

#define AROS_LH0(t,n,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o (void)
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1))
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2))
#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3))
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4))
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5))
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6))
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7))
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
    __AROS_LHA_GENMOD(a1),\
    __AROS_LHA_GENMOD(a2),\
    __AROS_LHA_GENMOD(a3),\
    __AROS_LHA_GENMOD(a4),\
    __AROS_LHA_GENMOD(a5),\
    __AROS_LHA_GENMOD(a6),\
    __AROS_LHA_GENMOD(a7),\
    __AROS_LHA_GENMOD(a8))
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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
    t AROS_LH_ ## n ## _ ## o(\
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

#define AROS_LP0I(t,n,bt,bn,o,s)
#define AROS_LP1I(t,n,a1,bt,bn,o,s)
#define AROS_LP2I(t,n,a1,a2,bt,bn,o,s)
#define AROS_LP3I(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LP4I(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LP5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LP6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LP7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LP8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LP9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LP10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LP11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LP12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LP13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LP14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LP15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

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

#define AROS_LC0I(t,n,bt,bn,o,s) ((void*)NULL)
#define AROS_LC1I(t,n,a1,bt,bn,o,s) ((void*)NULL)
#define AROS_LC2I(t,n,a1,a2,bt,bn,o,s) ((void*)NULL)
#define AROS_LC3I(t,n,a1,a2,a3,bt,bn,o,s) ((void*)NULL)
#define AROS_LC4I(t,n,a1,a2,a3,a4,bt,bn,o,s) ((void*)NULL)
#define AROS_LC5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) ((void*)NULL)
#define AROS_LC6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) ((void*)NULL)
#define AROS_LC7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) ((void*)NULL)
#define AROS_LC8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) ((void*)NULL)
#define AROS_LC9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) ((void*)NULL)
#define AROS_LC10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) ((void*)NULL)
#define AROS_LC11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) ((void*)NULL)
#define AROS_LC12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) ((void*)NULL)
#define AROS_LC13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) ((void*)NULL)
#define AROS_LC14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) ((void*)NULL)
#define AROS_LC15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) ((void*)NULL)

#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT

#  endif /* AROS_LIBCALL_H */
