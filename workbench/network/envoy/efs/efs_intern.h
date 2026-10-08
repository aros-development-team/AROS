#ifndef EFS_INTERN_H
#define EFS_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy filesystem client (EFS) - internal definitions. Written from
          re/spec/efs-protocol.md: one handler process per mount, a public
          entity named after the DOS device, one 564-byte transaction for
          everything but file data, two chunk transactions for READ/WRITE.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/exall.h>
#include <dos/notify.h>
#include <utility/tagitem.h>
#include <aros/debug.h>

#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>

#define EFS_BUFSIZE         564         /* the general-purpose request/response buffer      */
#define EFS_HDR             52          /* the EFS packet header                            */
#define EFS_CHUNK           32768       /* READ/WRITE chunk                                 */
#define EFS_MAXPATH         256
#define EFS_VOLBLOCK        128         /* volume information block                         */
#define EFS_FIBSIZE         260         /* FileInfoBlock on the wire                        */
#define EFS_SCANBLOCK       512         /* directory block fetched by action 5678           */

/* header field offsets */
#define WH_MOUNTID          0
#define WH_COOKIE           4
#define WH_ACTION           8
#define WH_RES1             12
#define WH_RES2             16
#define WH_ARG1             20
#define WH_ARG2             24
#define WH_ARG3             28
#define WH_ARG4             32
#define WH_ARG5             36
#define WH_ARG6             40

/* transaction commands */
#define EFSCMD_MOUNT        1
#define EFSCMD_PACKET       3
#define EFSCMD_LIST         4

/* EFS-private actions */
#define EFSACT_PROBE        31
#define EFSACT_EXNEXT       5678
#define EFSACT_DELETE       5679
#define EFSACT_VOLINFO      5680
#define EFSACT_NOTIFY       5681
#define EFSACT_NAME2UID     20000
#define EFSACT_NAME2GID     20001
#define EFSACT_UID2INFO     20002
#define EFSACT_GID2INFO     20003
#define EFSACT_LOGIN        20004
#define EFSACT_QUERYMOUNT   64000

/* trans_Error values of the server */
#define EFSERR_UNKNOWNCMD   0x8000
#define EFSERR_REFUSED      0x8001

/* mount entry flags (fifth Unit field) */
#define EFSF_HIDEBACKDROP   (1 << 0)
#define EFSF_PROTECTDISKINFO (1 << 1)
#define EFSF_NOREQUESTER    (1 << 16)

/* volume information flags */
#define VOLF_INUSE          (1 << 0)
#define VOLF_BYID           (1 << 1)
#define VOLF_FILESYSTEM     (1 << 2)

struct EfsVolume
{
    struct Node         Node;
    ULONG               ID;             /* server's volume ID, 0 = none                     */
    ULONG               Flags;
    struct DateStamp    Date;
    char                Name[108];
    struct DosList      *DosList;       /* our DOS volume node                              */
};

struct EfsLock
{
    struct MinNode      Node;           /* first: lists are walked with ForeachNode()       */
    struct FileLock     FL;             /* what the application holds a BPTR to             */
    ULONG               Handle;         /* server lock handle                               */
    ULONG               VolID;
    UWORD               Gen;            /* connection generation it is valid for            */
    LONG                Mode;
    struct EfsVolume    *Vol;
    char                Path[EFS_MAXPATH];
    /* directory scan state for ExNext (action 5678 blocks) */
    UBYTE               *Scan;
    ULONG               ScanEntries;
    ULONG               ScanNext;       /* offset of the next record in the block, ~0 = none */
    ULONG               ScanType;
    BOOL                ScanMore;
    BOOL                ScanStarted;
};

struct EfsRecord
{
    struct MinNode      Node;
    ULONG               Offset, Length, Mode, Timeout;
};

struct EfsFile
{
    struct MinNode      Node;
    ULONG               Handle;         /* server file handle                               */
    ULONG               VolID;
    UWORD               Gen;
    ULONG               OpenAction;     /* 1004..1006 as sent                               */
    LONG                Pos;            /* our copy of the file position                    */
    struct EfsVolume    *Vol;
    char                Path[EFS_MAXPATH];
    struct MinList      Records;
};

struct EfsNotify
{
    struct MinNode          Node;
    struct NotifyRequest    *NR;
    ULONG                   Key;        /* 32-bit key sent to the server                    */
    ULONG                   Handle;     /* server notification handle                       */
    ULONG                   VolID;
};

struct EfsNotifyMsg
{
    struct NotifyMessage    NM;
    struct EfsNotify        *Owner;
};

struct Globals
{
    struct ExecBase         *gl_SysBase;
    struct DosLibrary       *gl_DOSBase;
    struct Library          *gl_UtilityBase;
    struct Library          *gl_NIPCBase;
    struct Library          *gl_ServicesBase;

    struct Process          *Proc;
    struct MsgPort          *Port;
    struct MsgPort          *NotifyPort;
    struct DeviceNode       *DevNode;
    BOOL                    Dismounted;     /* the node was taken out of the DOS list (and freed) */

    char                    Host[80];
    char                    Export[80];
    char                    User[80];
    char                    Password[80];
    char                    DevName[80];
    ULONG                   Flags;

    struct Entity           *Entity;
    ULONG                   EntSig;
    struct Entity           *Service;

    struct Transaction      *Trans;         /* the 564-byte transaction                      */
    UBYTE                   *Buf;           /* its buffer                                    */
    struct Transaction      *Chunk[2];      /* READ/WRITE chunks                             */

    ULONG                   MountID;
    UWORD                   Gen;
    BOOL                    Connected;
    ULONG                   LastError;      /* trans_Error of the last failed mount          */
    ULONG                   ExtraTimeout;   /* X of §4.1                                     */
    ULONG                   OkCount;
    ULONG                   FailedMounts;
    ULONG                   NextCookie;
    ULONG                   NextKey;

    struct MinList          Locks;
    struct MinList          Files;
    struct MinList          Notifies;
    struct List             Volumes;
    struct EfsVolume        *CurVol;

    BOOL                    WriteProtect;
    BOOL                    Quit;
    BOOL                    Retry;          /* a transport failure asks for the packet to be redone */
    BOOL                    InReplay;
};

#define SysBase         (glob->gl_SysBase)
#define DOSBase         (glob->gl_DOSBase)
#define UtilityBase     (glob->gl_UtilityBase)
#define NIPCBase        (glob->gl_NIPCBase)
#define ServicesBase    (glob->gl_ServicesBase)

#define EFSLOG(...)     do { D(bug("[envoyfs] " __VA_ARGS__);) } while (0)

/* efs_wire.c */
void   PutL(UBYTE *p, ULONG off, ULONG v);
ULONG  GetL(const UBYTE *p, ULONG off);
void   PutW(UBYTE *p, ULONG off, UWORD v);
UWORD  GetW(const UBYTE *p, ULONG off);
void   HdrSet(struct Globals *glob, UBYTE *buf, ULONG action, ULONG cookie, ULONG a1, ULONG a2, ULONG a3, ULONG a4, ULONG a5, ULONG a6);
ULONG  PutBName(UBYTE *buf, ULONG off, CONST_STRPTR s);
ULONG  PutCName(UBYTE *buf, ULONG off, CONST_STRPTR s);
ULONG  Align4(ULONG off);
void   GetBstrArg(struct Globals *glob, BSTR b, char *dst, ULONG size);
CONST_STRPTR StripDevice(CONST_STRPTR name);
void   SetBString(UBYTE *dst, ULONG max, const UBYTE *src, ULONG len);
void   WireToFib(struct Globals *glob, const UBYTE *w, struct FileInfoBlock *fib);
void   ExRecToFib(struct Globals *glob, const UBYTE *block, ULONG off, ULONG type, struct FileInfoBlock *fib);
void   CopyStr(char *dst, ULONG size, CONST_STRPTR src);

/* efs_mount.c */
BOOL   ParseUnit(struct Globals *glob, CONST_STRPTR unit);
BOOL   FindFilesystem(struct Globals *glob);
BOOL   DoMount(struct Globals *glob);
BOOL   Reconnect(struct Globals *glob);
BOOL   Forward(struct Globals *glob, ULONG action, ULONG reqlen, ULONG tmult, LONG *res1, LONG *res2);
void   ProcessVolumeBlock(struct Globals *glob, const UBYTE *blk);
void   ReplayVolume(struct Globals *glob, struct EfsVolume *v);
BOOL   RefreshLock(struct Globals *glob, struct EfsLock *l);
BOOL   RefreshFile(struct Globals *glob, struct EfsFile *f);
void   RemoveVolumes(struct Globals *glob);
LONG   MountErrorToDos(ULONG err);

/* efs_ops.c */
void   HandlePacket(struct Globals *glob, struct DosPacket *dp);
void   EfsReplyPkt(struct Globals *glob, struct DosPacket *dp);
struct EfsLock *LockFromBPTR(struct Globals *glob, BPTR b);
struct EfsLock *NewLock(struct Globals *glob, ULONG handle, ULONG volid, LONG mode, CONST_STRPTR path);
void   FreeLockRec(struct Globals *glob, struct EfsLock *l);
void   ResetScan(struct Globals *glob, struct EfsLock *l);
struct EfsFile *FileFromArg(struct Globals *glob, SIPTR arg);
void   FreeFileRec(struct Globals *glob, struct EfsFile *f);

/* efs_rw.c */
void   DoReadWrite(struct Globals *glob, struct DosPacket *dp, BOOL write);
void   FreeChunks(struct Globals *glob);

/* efs_server.c */
void   HandleEntity(struct Globals *glob);
BOOL   StartTrans(struct Globals *glob, struct Transaction *t);
ULONG  WaitFor(struct Globals *glob, struct Transaction *t);
ULONG  DoTrans(struct Globals *glob, struct Transaction *t);
void   HandleNotifyReplies(struct Globals *glob);
void   FreeNotifies(struct Globals *glob);

#endif /* EFS_INTERN_H */
