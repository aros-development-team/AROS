/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: afs multi-user support (security.library). Only active when
          security.library is resident, configured, and the volume carries
          a muFS dostype (muF\0). Everything is a no-op otherwise.
*/
#ifndef AFS_SECURITY_H
#define AFS_SECURITY_H

struct BlockCache;
struct AfsHandle;
struct AFSBase;
struct Volume;
struct DosPacket;

/* Access types (same values as secAt_#? in <libraries/security.h>) */
#define AFS_ACCESS_READ         1
#define AFS_ACCESS_WRITE        2
#define AFS_ACCESS_EXECUTE      4
#define AFS_ACCESS_DELETE       8

#ifdef __AROS__
#include "os.h"
#include "afshandler.h"
#include "volumes.h"
#include <libraries/security.h>

void  afsSecBeginPacket(struct AFSBase *afsbase, struct Volume *volume, struct DosPacket *dp);
void  afsSecEndPacket(struct AFSBase *afsbase);
BOOL  afsSecIsActive(struct AFSBase *afsbase);
LONG  afsSecCheckBlock(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *bb, LONG access);
LONG  afsSecCheckProperty(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *bb);
LONG  afsSecCheckName(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, LONG access);
LONG  afsSecCheckParent(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, LONG access);
LONG  afsSecCheckNameProperty(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name);
ULONG afsSecNewOwner(struct AFSBase *afsbase);
ULONG afsSecNewProtection(struct AFSBase *afsbase, ULONG def);
ULONG afsSecSetOwner(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, ULONG owner);
#else
#define afsSecBeginPacket(a,v,d)            do {} while (0)
#define afsSecEndPacket(a)                  do {} while (0)
#define afsSecIsActive(a)                   (0)
#define afsSecCheckBlock(a,v,b,x)           (0)
#define afsSecCheckProperty(a,v,b)          (0)
#define afsSecCheckName(a,d,n,x)            (0)
#define afsSecCheckParent(a,d,n,x)          (0)
#define afsSecCheckNameProperty(a,d,n)      (0)
#define afsSecNewOwner(a)                   (0)
#define afsSecNewProtection(a,d)            (d)
#define afsSecSetOwner(a,d,n,o)             (ERROR_ACTION_NOT_KNOWN)
#endif

#endif /* AFS_SECURITY_H */
