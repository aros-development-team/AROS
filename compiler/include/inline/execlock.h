#ifndef INLINE_EXECLOCK_H
#define INLINE_EXECLOCK_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
*/

/*
    Desc: Inline function(s) for execlock
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <aros/symbolsets.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>


#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline int __inline_ExecLock_ObtainSystemLock(struct List * __arg1, ULONG __arg2, ULONG __arg3, APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    return AROS_LC3(int, ObtainSystemLock,
        AROS_LCA(struct List *,(__arg1),A0),
        AROS_LCA(ULONG,(__arg2),D0),
        AROS_LCA(ULONG,(__arg3),D1),
        struct Library *, (__ExecLockBase), 1, ExecLock    );
}

#define ObtainSystemLock(arg1, arg2, arg3) \
    __inline_ExecLock_ObtainSystemLock((arg1), (arg2), (arg3), __aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline void __inline_ExecLock_ReleaseSystemLock(struct List * __arg1, ULONG __arg2, APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    AROS_LC2NR(void, ReleaseSystemLock,
        AROS_LCA(struct List *,(__arg1),A0),
        AROS_LCA(ULONG,(__arg2),D1),
        struct Library *, (__ExecLockBase), 2, ExecLock    );
}

#define ReleaseSystemLock(arg1, arg2) \
    __inline_ExecLock_ReleaseSystemLock((arg1), (arg2), __aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#endif /* INLINE_EXECLOCK_H*/
