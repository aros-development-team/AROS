/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library volume tracking
*/
#ifndef _SECURITY_VOLUMES_H
#define _SECURITY_VOLUMES_H

#include <exec/lists.h>
#include <dos/dosextens.h>
#include <libraries/security.h>

#ifndef TASKHASHVALUE
#define TASKHASHVALUE 23
#endif

struct SecurityBase;

/*
 * A volume known to the library: either a native muFS volume (FS_Flags == 0)
 * or one enforced by the packet interceptor.
 */
struct secVolume
{
    struct secVolume    *Next;
    struct DosList      *DosList;               /* DosList for this Volume                          */
    struct MsgPort      *Process;               /* Handler port for this Volume                     */

    LONG                FS_Flags;               /* secFSE_#?; 0 = true muFS volume                  */
    LONG                RootProtection;         /* Permissions for the root dir                     */
    ULONG               RootOwner;              /* UID:GID of owner of root dir                     */

    STRPTR              FS_Name;                /* Volume name (owned copy)                         */
    struct MsgPort      *OrigProc;              /* The real FS                                      */
    struct MsgPort      *RepPort;               /* For talking with the real FS                     */
    struct FileInfoBlock *fib;
    LONG                PassKey;                /* passkey for ACTION_WRITE_PROTECT, if any         */

    struct MinList      ProxyLocks;             /* proxy enforcer state                             */
    struct MinList      ProxyHandles[TASKHASHVALUE];
    struct DeviceNode   *ProxyDosList;
    struct DeviceList   *ProxyDosListVolume;
    ULONG               LockCount;
};

/* A volume whose handler enforces ownership itself (fstab NATIVE) */
struct secNativeVolume
{
    struct MinNode      Node;
    char                Name[1];                /* variable length */
};

extern BOOL IsSecFSDosType(ULONG dostype);
extern BOOL InitVolumes(struct SecurityBase *secBase);
extern void FreeVolumes(struct SecurityBase *secBase);
extern BOOL IsSecFSVolume(struct SecurityBase *secBase, struct MsgPort *port);

#endif /* _SECURITY_VOLUMES_H */
