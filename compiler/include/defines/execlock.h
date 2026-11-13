#ifndef DEFINES_EXECLOCK_H
#define DEFINES_EXECLOCK_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
*/

/*
    Desc: Defines for execlock
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <aros/symbolsets.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>

__BEGIN_DECLS


#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __ObtainSystemLock_WB(__ExecLockBase, __arg1, __arg2, __arg3) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC3(int, ObtainSystemLock, \
                  AROS_LCA(struct List *,(__arg1),A0), \
                  AROS_LCA(ULONG,(__arg2),D0), \
                  AROS_LCA(ULONG,(__arg3),D1), \
        struct Library *, (__ExecLockBase), 1, ExecLock);\
})

#define ObtainSystemLock(arg1, arg2, arg3) \
    __ObtainSystemLock_WB(__aros_getbase_ExecLockBase(), (arg1), (arg2), (arg3))

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __ReleaseSystemLock_WB(__ExecLockBase, __arg1, __arg2) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC2NR(void, ReleaseSystemLock, \
                  AROS_LCA(struct List *,(__arg1),A0), \
                  AROS_LCA(ULONG,(__arg2),D1), \
        struct Library *, (__ExecLockBase), 2, ExecLock);\
})

#define ReleaseSystemLock(arg1, arg2) \
    __ReleaseSystemLock_WB(__aros_getbase_ExecLockBase(), (arg1), (arg2))

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __AllocLock_WB(__ExecLockBase) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC0(APTR, AllocLock, \
        struct Library *, (__ExecLockBase), 3, ExecLock);\
})

#define AllocLock() \
    __AllocLock_WB(__aros_getbase_ExecLockBase())

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __FreeLock_WB(__ExecLockBase, __arg1) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC1NR(void, FreeLock, \
                  AROS_LCA(APTR,(__arg1),A0), \
        struct Library *, (__ExecLockBase), 4, ExecLock);\
})

#define FreeLock(arg1) \
    __FreeLock_WB(__aros_getbase_ExecLockBase(), (arg1))

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __ObtainLock_WB(__ExecLockBase, __arg1, __arg2, __arg3) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC3(int, ObtainLock, \
                  AROS_LCA(APTR,(__arg1),A0), \
                  AROS_LCA(ULONG,(__arg2),D0), \
                  AROS_LCA(ULONG,(__arg3),D1), \
        struct Library *, (__ExecLockBase), 5, ExecLock);\
})

#define ObtainLock(arg1, arg2, arg3) \
    __ObtainLock_WB(__aros_getbase_ExecLockBase(), (arg1), (arg2), (arg3))

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)

#define __ReleaseLock_WB(__ExecLockBase, __arg1, __arg2) ({\
        AROS_LIBREQ(ExecLockBase,36)\
        AROS_LC2NR(void, ReleaseLock, \
                  AROS_LCA(APTR,(__arg1),A0), \
                  AROS_LCA(ULONG,(__arg2),D1), \
        struct Library *, (__ExecLockBase), 6, ExecLock);\
})

#define ReleaseLock(arg1, arg2) \
    __ReleaseLock_WB(__aros_getbase_ExecLockBase(), (arg1), (arg2))

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

__END_DECLS

#endif /* DEFINES_EXECLOCK_H*/
