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

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline APTR __inline_ExecLock_AllocLock(APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    return AROS_LC0(APTR, AllocLock,
        struct Library *, (__ExecLockBase), 3, ExecLock    );
}

#define AllocLock() \
    __inline_ExecLock_AllocLock(__aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline void __inline_ExecLock_FreeLock(APTR __arg1, APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    AROS_LC1NR(void, FreeLock,
        AROS_LCA(APTR,(__arg1),A0),
        struct Library *, (__ExecLockBase), 4, ExecLock    );
}

#define FreeLock(arg1, arg2) \
    __inline_ExecLock_FreeLock((arg1), __aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline int __inline_ExecLock_ObtainLock(APTR __arg1, ULONG __arg2, ULONG __arg3, APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    return AROS_LC3(int, ObtainLock,
        AROS_LCA(APTR,(__arg1),A0),
        AROS_LCA(ULONG,(__arg2),D0),
        AROS_LCA(ULONG,(__arg3),D1),
        struct Library *, (__ExecLockBase), 5, ExecLock    );
}

#define ObtainLock(arg1, arg2, arg3) \
    __inline_ExecLock_ObtainLock((arg1), (arg2), (arg3), __aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

static inline void __inline_ExecLock_ReleaseLock(APTR __arg1, ULONG __arg2, APTR __ExecLockBase)
{
    AROS_LIBREQ(ExecLockBase, 36)
    AROS_LC2NR(void, ReleaseLock,
        AROS_LCA(APTR,(__arg1),A0),
        AROS_LCA(ULONG,(__arg2),D1),
        struct Library *, (__ExecLockBase), 6, ExecLock    );
}

#define ReleaseLock(arg1, arg2) \
    __inline_ExecLock_ReleaseLock((arg1), (arg2), __aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#endif /* INLINE_EXECLOCK_H*/
