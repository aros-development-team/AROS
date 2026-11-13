/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ram-handler multi-user support (security.library). Only active when
          security.library is resident and the system is configured (has a
          passwd file). Everything is a no-op otherwise, so a system without
          the library behaves exactly as before.

          RAM: is always enforced when active: the volume root is unowned
          (world writable), objects belong to their creator and carry the
          creator's default protection (umask). Objects created before the
          library became active are unowned too, i.e. accessible to all.
*/
#ifndef RAM_SECURITY_H
#define RAM_SECURITY_H

#include <exec/types.h>
#include <libraries/security.h>   /* ACTION_SET_OWNER, ACTION_IS_SECFS, secOWNER_#? */

struct Handler;
struct Object;
struct Lock;
struct DosPacket;

/* Access types (same values as secAt_#? in <libraries/security.h>) */
#define RAM_ACCESS_READ         1
#define RAM_ACCESS_WRITE        2
#define RAM_ACCESS_EXECUTE      4
#define RAM_ACCESS_DELETE       8

void  ramSecBeginPacket(struct Handler *handler, struct DosPacket *packet);
void  ramSecEndPacket(struct Handler *handler);
BOOL  ramSecIsActive(struct Handler *handler);
BOOL  ramSecIsPresent(struct Handler *handler);
LONG  ramSecCheckObject(struct Handler *handler, struct Object *object, LONG access);
LONG  ramSecCheckProperty(struct Handler *handler, struct Object *object);
ULONG ramSecNewOwner(struct Handler *handler);
ULONG ramSecNewProtection(struct Handler *handler);
void  ramSecFillOwner(struct Handler *handler, struct Object *object, UWORD *uid, UWORD *gid);
BOOL  CmdSetOwner(struct Handler *handler, struct Lock *lock, const TEXT *name, ULONG owner);

#endif /* RAM_SECURITY_H */
