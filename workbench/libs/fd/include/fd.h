#ifndef FD_LIBRARY_H
#define FD_LIBRARY_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UBYTE fd_owner_t;

#define FD_OWNER_NONE      ((fd_owner_t)0)
#define FD_OWNER_POSIXC    ((fd_owner_t)1)
#define FD_OWNER_BSDSOCKET ((fd_owner_t)2)

LONG FD_Alloc(LONG startfd, fd_owner_t owner, APTR data, LONG *outfd);
LONG FD_Reserve(LONG fd, fd_owner_t owner, APTR data);
LONG FD_Free(LONG fd, fd_owner_t owner);
LONG FD_Check(LONG fd);
fd_owner_t FD_GetOwner(LONG fd);
APTR FD_GetData(LONG fd);
LONG FD_SetData(LONG fd, fd_owner_t owner, APTR data);

#ifdef __cplusplus
}
#endif

#endif /* FD_LIBRARY_H */
