#ifndef CLIB_EXECLOCK_PROTOS_H
#define CLIB_EXECLOCK_PROTOS_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>

#include <resources/execlock.h>

__BEGIN_DECLS

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP3(int, ObtainSystemLock,
         AROS_LPA(struct List *, systemList, A0),
         AROS_LPA(ULONG, mode, D0),
         AROS_LPA(ULONG, flags, D1),
         LIBBASETYPEPTR, ExecLockBase, 1, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP2(void, ReleaseSystemLock,
         AROS_LPA(struct List *, systemList, A0),
         AROS_LPA(ULONG, flags, D1),
         LIBBASETYPEPTR, ExecLockBase, 2, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP0(APTR, AllocLock,
         LIBBASETYPEPTR, ExecLockBase, 3, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP1(void, FreeLock,
         AROS_LPA(APTR, lock, A0),
         LIBBASETYPEPTR, ExecLockBase, 4, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP3(int, ObtainLock,
         AROS_LPA(APTR, lock, A0),
         AROS_LPA(ULONG, mode, D0),
         AROS_LPA(ULONG, flags, D1),
         LIBBASETYPEPTR, ExecLockBase, 5, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

#if !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__)
AROS_LP2(void, ReleaseLock,
         AROS_LPA(APTR, lock, A0),
         AROS_LPA(ULONG, flags, D1),
         LIBBASETYPEPTR, ExecLockBase, 6, ExecLock
);

#endif /* !defined(__EXECLOCK_LIBAPI__) || (36 <= __EXECLOCK_LIBAPI__) */

__END_DECLS

#endif /* CLIB_EXECLOCK_PROTOS_H */
