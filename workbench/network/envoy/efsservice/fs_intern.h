#ifndef FS_INTERN_H
#define FS_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - internal definitions. The wire protocol is
          re/spec/efs-protocol.md; section numbers in comments refer to it.
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/notify.h>
#include <devices/timer.h>
#include <envoy/nipc.h>
#include <envoy/accounts.h>
#include <aros/debug.h>

#define FS_SERVICE_NAME         "Filesystem"            /* SVCAttrs_Name                     */
#define FS_ENTITY_NAME          "Filesystem"            /* the one public entity (§5.1)      */
#define FS_MANAGER_ENTITY       "Services Manager"      /* services.library FindService target */
#define FS_DAEMON_PORT          "Envoy EFS Daemon"      /* single-instance marker port       */
#define FS_PREFS_ENV            "ENV:Envoy/EFS.prefs"
#define FS_PREFS_ENVARC         "ENVARC:Envoy/EFS.prefs"

/* VOLM export flags (§6.3) */
#define EXPF_SNAPSHOT           0x01    /* Disk.info may be changed                      */
#define EXPF_LEFTOUT            0x02    /* .backdrop visible                             */
#define EXPF_FULLSEC            0x04    /* "Mounts&Files": Full File Security            */
#define EXPF_NOSEC              0x08    /* "None": no login needed                       */
#define EXPF_EMULEXALL          0x10
#define EXPF_READONLY           0x20
#define EXPF_REMOVABLE          0x40

/* per-mount flags (§5.2) */
#define MNTF_HIDEBACKDROP       (1 << 0)
#define MNTF_PROTDISKINFO       (1 << 1)
#define MNTF_FULLSEC            (1 << 16)
#define MNTF_ABSPOS             (1 << 19)
#define MNTF_READONLY           (1 << 22)

/* transaction errors (§1.4, §1.7) */
#define EFSERR_UNKNOWNCMD       0x8000
#define EFSERR_REFUSED          0x8001

#define EFS_HDR                 52      /* the command-3 header (§2.1)                    */
#define EFS_MAXCHUNK            32768

/* EFS-private actions */
#define EFS_ACT_PROBE           31
#define EFS_ACT_EXNEXT          5678
#define EFS_ACT_DELETE          5679
#define EFS_ACT_VOLUME          5680
#define EFS_ACT_NOTIFYEVENT     5681
#define EFS_ACT_USERNAME_TO_UID 20000
#define EFS_ACT_GROUPNAME_TO_GID 20001
#define EFS_ACT_UID_TO_USERINFO 20002
#define EFS_ACT_GID_TO_GROUPINFO 20003
#define EFS_ACT_CHANGELOGIN     20004

/* rights (§5.3) */
#define RIGHT_R                 (1 << 0)
#define RIGHT_W                 (1 << 1)
#define RIGHT_E                 (1 << 2)
#define RIGHT_D                 (1 << 3)
#define RIGHTS_ALL              0x0F

/* Envoy IDs */
#define EFS_NOUSER              0xFFFF  /* "no valid login" on the wire                   */

/* The thin compatibility stub filesystem.service keeps the original library
 * layout; the server itself now runs as the standalone envoyfs daemon. */
struct FSServiceBase
{
    struct Library          fb_Lib;
    struct SignalSemaphore  fb_Sem;
};

struct Export
{
    struct MinNode  Node;
    char            Path[64];
    char            Name[64];
    ULONG           Flags;                  /* EXPF_#?                                    */
    ULONG           NumAccess;
    UWORD          *AccId;                  /* access list: IDs                           */
    UBYTE          *AccKind;                /* 0 = user, else group                       */
    BPTR            RootLock;               /* the export root; 0 if unusable             */
    BOOL            IsFS;
    ULONG           VolumeID;               /* opaque to the client (§1.5)                */
    struct DateStamp VolDate;
    char            VolName[108];
    char            RootPath[256];          /* NameFromLock() of the root                 */
};

struct LockRec
{
    struct MinNode  Node;
    ULONG           Handle;
    BPTR            Lock;
    BOOL            ReadOnly;               /* Disk.info rule, §5.5                       */
    struct ExAllControl *Eac;               /* an unfinished 5678/1033 scan              */
    APTR            EacBuf;
    ULONG           EacBufSize;
    STRPTR          EacPattern;
    LONG            EacType;
    BOOL            EacMore;
    struct ExAllData *EacNext;             /* for action 24: next entry to hand out      */
};

struct FileRec
{
    struct MinNode  Node;
    ULONG           Handle;
    BPTR            FH;
    LONG            Pos;
    BOOL            ReadOnly;               /* writes answered 223                        */
    BOOL            NoRead;                 /* reads answered 224                         */
    BOOL            Dirty;                  /* written since opened                       */
    char            Name[256];              /* full path, for the notify matching         */
};

struct NotifyRec
{
    struct MinNode  Node;
    ULONG           Handle;
    ULONG           ClientKey;
    struct NotifyRequest NR;
    BOOL            Started;
    struct Mount   *Mount;
    char            Name[256];
};

struct Mount
{
    struct MinNode  Node;
    ULONG           ID;
    struct Entity  *Source;                 /* the server link that carried the mount     */
    char            Host[80];
    char            DevName[256];
    char            User[36];
    UWORD           Uid, Gid;
    ULONG           AccFlags;               /* UFLAGF_#?                                  */
    BOOL            Authenticated;
    ULONG           Flags;                  /* MNTF_#?                                    */
    ULONG           ClientFlags;            /* low word of the mount entry flags (§1.2)   */
    struct Export  *Export;
    struct MinList  Locks, Files, Notifies;
    ULONG           Idle;                   /* seconds since the last request (§4.5)      */
    struct Entity  *Client;                 /* link to the client's public entity         */
    BOOL            PingOut;                /* a 5680 is on its way                       */
    BOOL            PingIsIdle;             /* ... because of the idle counter            */
};

/* a server -> client transaction in flight */
struct Event
{
    struct MinNode      Node;
    struct Transaction *Trans;
    struct Mount       *Mount;
    ULONG               Action;
};

struct FSServer
{
    struct Library      *NipcLib;
    struct Library      *AccLib;
    struct Library      *UtilLib;
    struct Library      *SecLib;                /* security.library, optional          */
    BOOL                 IsRoot;                /* configured security and we are root */
    ULONG                OwnOwner;              /* our own uid<<16|gid                 */
    struct Entity       *Ent;
    ULONG                EntSig;
    struct Entity       *MgrEnt;                /* "Services Manager" responder - only
                                                 * when no real manager owns the name  */
    ULONG                MgrSig;
    BOOL                 Paused;                /* inside a stack reconfigure fence    */
    struct MsgPort      *NotifyPort;
    struct MsgPort      *TimerPort;
    struct timerequest  *Timer;
    BOOL                 TimerOpen, TimerPending;
    struct NotifyRequest PrefsNR;
    BOOL                 PrefsWatched;
    struct MinList       Exports, Mounts, Events;
    ULONG                NextHandle;
    ULONG                NextVolID;
    char                 Path[512], Path2[512], Name[300];
    BOOL                 Verbose;
};

#define FSLOG(srv, ...)   do { if ((srv)->Verbose) bug("[filesystem.service] " __VA_ARGS__); } while (0)

/* fs_wire.c */
ULONG  EfsGet32(const UBYTE *p);
void   EfsPut32(UBYTE *p, ULONG v);
UWORD  EfsGet16(const UBYTE *p);
void   EfsPut16(UBYTE *p, UWORD v);
void   EfsPutCStr(UBYTE *dst, ULONG size, CONST_STRPTR s);
BOOL   EfsGetBSTR(const UBYTE *buf, ULONG buflen, ULONG off, STRPTR dst, ULONG size);
BOOL   EfsGetCStr(const UBYTE *buf, ULONG buflen, ULONG off, STRPTR dst, ULONG size);
void   EfsPutFIB(UBYTE *dst, const struct FileInfoBlock *fib);
void   EfsPutInfoData(UBYTE *dst, const struct InfoData *id);
ULONG  EfsPutExAll(UBYTE *dst, ULONG size, const struct ExAllData *ed, LONG type, ULONG *count);
void   EfsPutUserInfo(UBYTE *dst, const struct UserInfo *ui);
void   EfsPutGroupInfo(UBYTE *dst, const struct GroupInfo *gi);

/* fs_config.c */
void   ConfigLoad(struct FSServer *srv);
void   ConfigFree(struct FSServer *srv);
struct Export *ConfigFindExport(struct FSServer *srv, CONST_STRPTR name);

/* fs_auth.c */
BOOL   AuthVerify(struct FSServer *srv, CONST_STRPTR user, CONST_STRPTR password, struct UserInfo *ui);
struct Export *AuthSelectExport(struct FSServer *srv, CONST_STRPTR name, CONST_STRPTR user, CONST_STRPTR password,
                                struct UserInfo *ui, BOOL *authenticated, BOOL *credfail);
BOOL   AuthMayMount(struct FSServer *srv, struct Export *e, struct UserInfo *ui, BOOL authenticated);
ULONG  AuthRights(struct FSServer *srv, struct Mount *m, const struct FileInfoBlock *fib, BOOL *isowner);
LONG   AuthRewriteProtection(struct FSServer *srv, struct Mount *m, const struct FileInfoBlock *fib);

/* fs_actions.c */
void   ActionsHandle(struct FSServer *srv, struct Mount *m, struct Transaction *t);

/* fs_server.c - the daemon body: returns 0 on clean exit, nonzero when the
 * server could not come up.  The three masks are the netservices reconfigure
 * begin/end and stop signal masks (0 when not registered). */
int    ServerMain(ULONG stopmask, ULONG beginmask, ULONG endmask);
void   ServerImpersonate(struct FSServer *srv, struct Mount *m);
void   ServerUnimpersonate(struct FSServer *srv);
void   MountDestroy(struct FSServer *srv, struct Mount *m);
void   ServerSendEvent(struct FSServer *srv, struct Mount *m, ULONG action, ULONG key, BOOL idle);
void   ServerNotifyChanged(struct FSServer *srv, struct Mount *m, CONST_STRPTR fullname);
ULONG  ServerNewHandle(struct FSServer *srv);
void   ServerBuildVolumeInfo(struct FSServer *srv, struct Mount *m, UBYTE *dst);
void   LockRecFree(struct FSServer *srv, struct LockRec *l);
void   FileRecFree(struct FSServer *srv, struct FileRec *f);
void   NotifyRecFree(struct FSServer *srv, struct NotifyRec *n);

#endif /* FS_INTERN_H */
