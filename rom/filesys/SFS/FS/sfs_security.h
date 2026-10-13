/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SFS multi-user support (security.library). Active only when the
          library is resident and configured and the volume is listed as
          NATIVE in the security fstab. Everything is a no-op otherwise.
*/
#ifndef SFS_SECURITY_H
#define SFS_SECURITY_H

#include <exec/types.h>
#include <dos/dosextens.h>
#include <libraries/security.h>

struct CacheBuffer;
struct fsObject;
struct ExtFileLock;

/* Access types (same values as secAt_#? in <libraries/security.h>) */
#define SFS_ACCESS_READ         1
#define SFS_ACCESS_WRITE        2
#define SFS_ACCESS_EXECUTE      4
#define SFS_ACCESS_DELETE       8

void  sfsSecBeginPacket(struct DosPacket *dp);
void  sfsSecEndPacket(void);
BOOL  sfsSecIsActive(void);
void  sfsSecVolumeChanged(void);
void  sfsSecVolumeOnline(void);
LONG  sfsSecCheckObject(struct fsObject *o, LONG access);
LONG  sfsSecCheckProperty(struct fsObject *o);
LONG  sfsSecCheckPath(struct ExtFileLock *lock, UBYTE *path, LONG access);
LONG  sfsSecCheckParentPath(struct ExtFileLock *lock, UBYTE *path, LONG access);
LONG  sfsSecCheckPropertyPath(struct ExtFileLock *lock, UBYTE *path);
LONG  sfsSecCheckSetOwner(struct fsObject *o, ULONG newowner);
ULONG sfsSecNewOwner(void);
/* Protection to store (SFS representation) for a new object, or default */
ULONG sfsSecNewStoredProtection(ULONG def);

#endif /* SFS_SECURITY_H */
