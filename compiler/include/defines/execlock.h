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

#define __ObtainSystemLock_WB(__ExecLockBase, __arg1, __arg2, __arg2) ({\
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

__END_DECLS

#endif /* DEFINES_EXECLOCK_H*/
