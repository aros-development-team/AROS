/*
    (C) 1997-1998 AROS - Amiga Replacement OS
    
*/

#ifndef _AROS_LIBCALL_H
#define _AROS_LIBCALL_H

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

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

#ifndef __AROS_SLIB_ENTRY
#   define __AROS_SLIB_ENTRY(n,s)	s ## _ ## n
#endif
#ifndef AROS_SLIB_ENTRY
#   define AROS_SLIB_ENTRY(n,s) __AROS_SLIB_ENTRY(n,s)
#endif

/* Library functions which need the libbase */

#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)( \
	__AROS_LHQUAD(a1),\
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)( \
	__AROS_LHQUAD(a1),\
	__AROS_LHQUAD(a2),\
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LH0(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LH_BASE(bt,bn))

#define AROS_LH1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LH_BASE(bt,bn)\
	)
	
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
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
	__AROS_LH_BASE(bt,bn)\
	)

#define AROS_LH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
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
	__AROS_LH_BASE(bt,bn)\
	)

/* Library functions which don't need library base */

#define AROS_LH0I(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(void)

#define AROS_LH1I(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1)\
	)

#define AROS_LH2I(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2)\
	)

#define AROS_LH3I(t,n,a1,a2,a3,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3)\
	)

#define AROS_LH4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4)\
	)

#define AROS_LH5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5)\
	)

#define AROS_LH6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6)\
	)

#define AROS_LH7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7)\
	)

#define AROS_LH8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8)\
	)

#define AROS_LH9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9)\
	)

#define AROS_LH10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LHA(a10)\
	)

#define AROS_LH11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
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
	__AROS_LHA(a11)\
	)

#define AROS_LHA(type,name,reg) type,name,reg
#define AROS_LPA(type,name,reg) type,name,reg
#define AROS_LCA(type,name,reg) type,name,reg
#define AROS_LDA(type,name,reg) type,name,reg

#define AROS_LD0(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) (__AROS_LD_BASE(bt,bn))

#define AROS_LD1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) ( \
	__AROS_LDA(a1), __AROS_LD_BASE(bt,bn))

#define AROS_LD2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), __AROS_LD_BASE(bt,bn))

#ifndef AROS_LVO_CALL0
#define AROS_LVO_CALL0(t, bt, bn, o, s) \
    LP0((o*4), t, LVOCall0, bt, bn)
#endif

#ifndef AROS_LVO_CALL0NR
#define AROS_LVO_CALL0NR(bt, bn, o, s) \
    LP0NR((o*4), LVOCall0NR, bt, bn)
#endif

#ifndef AROS_LVO_CALL1
#define AROS_LVO_CALL1(t, a1, bt, bn, o, s) \
    LP1((o*4), t, LVOCall1, __AROS_LDA(a1), __AROS_LCA(a1), bt, bn)
#endif

#ifndef AROS_LVO_CALL1NR
#define AROS_LVO_CALL1NR(a1, bt, bn, o, s) \
    LP1NR((o*4), LVOCall1NR, __AROS_LDA(a1), __AROS_LCA(a1), bt, bn)
#endif

#ifndef AROS_LVO_CALL2
#define AROS_LVO_CALL2(t, a1, a2, bt, bn, o, s) \
    LP2((o*4), t, LVOCall2, __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), bt, bn)
#endif

#ifndef AROS_LVO_CALL2NR
#define AROS_LVO_CALL2NR(a1, a2, bt, bn, o, s) \
    LP2NR((o*4), LVOCall2NR, __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), bt, bn)
#endif

#ifndef AROS_LVO_CALL3
#define AROS_LVO_CALL3(t, a1, a2, a3, bt, bn, o, s) \
    LP3((o*4), t, LVOCall3, __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), __AROS_LDA(a3), __AROS_LCA(a3), bt, bn)
#endif

#ifndef AROS_LVO_CALL3NR
#define AROS_LVO_CALL3NR(a1, a2, a3, bt, bn, o, s) \
    LP3NR((o*4), LVOCall3NR, __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), __AROS_LDA(a3), __AROS_LCA(a3), bt, bn)
#endif

#ifndef __INLINE_MACROS_H
#   include <inline/macros.h>
#endif

#ifndef AROS_LIBFUNC_INIT
#   define AROS_LIBFUNC_INIT {
#endif

#ifndef AROS_LIBFUNC_EXIT
#   define AROS_LIBFUNC_EXIT }
#endif

#ifndef AROS_LIBBASE_EXT_DECL
#   define AROS_LIBBASE_EXT_DECL(a,b)
#endif

#endif /* _AROS_LIBCALL_H */
