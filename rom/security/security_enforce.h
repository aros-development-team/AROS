/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library filesystem enforcer
*/
#ifndef _SECURITY_ENFORCE_H
#define _SECURITY_ENFORCE_H

#include <dos/dosextens.h>
#include <libraries/security.h>

struct SecurityBase;
struct secVolume;

/* Flags for secVolume.FS_Flags */
#define secFSE_TRUE_MUFS                (0)     /* A true multi-user aware volume       */
#define secFSE_ENFORCED                 (1)     /* Enforced by the packet interceptor   */
#define secFSE_READONLY                 (2)     /* This FS is read only                 */
#define secFSE_NOSUID                   (4)     /* Do not allow setuid from this volume */

/* Default Protections/owner for the root directory of an enforced volume */
#define secFSE_DEF_ROOTPROTECTION       (FIBF_OTR_READ | FIBF_GRP_READ | FIBF_DELETE | FIBF_EXECUTE)
#define secFSE_DEF_ROOTOWNER            secOWNER_SYSTEM

typedef BOOL (*PKTFUNC)(struct SecurityBase *, struct secVolume *, struct DosPacket *, struct secExtOwner *);

struct secFSE_PktHandler
{
    LONG        action;
    PKTFUNC     func;
};

extern void ReadFSTab(struct SecurityBase *secBase);
extern BOOL BootStrapRendezvous(struct SecurityBase *secBase);
extern LONG IsAllowed(struct SecurityBase *secBase, struct secVolume *Vol, struct secExtOwner *task,
                      ULONG object, LONG prot, LONG access);

#endif /* _SECURITY_ENFORCE_H */
