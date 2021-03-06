/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2006 Marek Szyprowski
 * Copyright (C) 2007-2015 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef FAT_HANDLER_PROTO_H
#define FAT_HANDLER_PROTO_H

/* disk.c */
void ProcessDiskChange (struct Globals *glob);
void UpdateDisk(struct Globals *glob);
void Probe64BitSupport(struct Globals *glob);

/* packet.c */
void ProcessPackets(struct Globals *glob);
void ReplyPacket(struct DosPacket *pkt, struct ExecBase *sysbase);

/* direntry.c */
void InitDir(struct FSSuper *sb, ULONG cluster, struct DirEntry *de);
LONG InitDirHandle(struct FSSuper *sb, ULONG cluster, struct DirHandle *dh,
    BOOL reuse, struct Globals *glob);
LONG ReleaseDirHandle(struct DirHandle *dh, struct Globals *glob);
LONG GetDirEntry(struct DirHandle *dh, ULONG index, struct DirEntry *de,
    struct Globals *glob);
LONG GetNextDirEntry(struct DirHandle *dh, struct DirEntry *de,
    struct Globals *glob);
LONG GetDirEntryByCluster(struct DirHandle *dh, ULONG cluster,
    struct DirEntry *de, struct Globals *glob);
LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG namelen,
    struct DirEntry *de, struct Globals *glob);
LONG GetDirEntryByPath(struct DirHandle *dh, STRPTR path, ULONG pathlen,
    struct DirEntry *de, struct Globals *glob);
LONG GetParentDir(struct DirHandle *dh, struct DirEntry *de,
    struct Globals *glob);
LONG UpdateDirEntry(struct DirEntry *de, struct Globals *glob);
LONG AllocDirEntry(struct DirHandle *dh, ULONG gap, struct DirEntry *de,
    struct Globals *glob);
LONG CreateDirEntry(struct DirHandle *dh, STRPTR name, ULONG namelen,
    UBYTE attr, ULONG cluster, struct DirEntry *de, struct Globals *glob);
void FillDirEntry(struct DirEntry *de, UBYTE attr, ULONG cluster,
    struct Globals *glob);
LONG DeleteDirEntry(struct DirEntry *de, struct Globals *glob);
LONG FillFIB(struct ExtFileLock *fl, struct FileInfoBlock *fib,
    struct Globals *glob);

/* names.c */
LONG GetDirEntryShortName(struct DirEntry *de, STRPTR name, ULONG *len,
    struct Globals *glob);
LONG GetDirEntryLongName(struct DirEntry *de, STRPTR name, ULONG *len);
LONG SetDirEntryName(struct DirEntry *de, STRPTR name, ULONG len);
ULONG NumLongNameEntries(STRPTR name, ULONG len);

/* fat.c */
ULONG GetFat12Entry(struct FSSuper *sb, ULONG n);
ULONG GetFat16Entry(struct FSSuper *sb, ULONG n);
ULONG GetFat32Entry(struct FSSuper *sb, ULONG n);
BOOL SetFat12Entry(struct FSSuper *sb, ULONG n, ULONG val);
BOOL SetFat16Entry(struct FSSuper *sb, ULONG n, ULONG val);
BOOL SetFat32Entry(struct FSSuper *sb, ULONG n, ULONG val);
LONG FindFreeCluster(struct FSSuper *sb, ULONG *rcluster);
void CountFreeClusters(struct FSSuper *sb);
void AllocCluster(struct FSSuper *sb, ULONG cluster);
void FreeCluster(struct FSSuper *sb, ULONG cluster);

/* volume.c */
LONG ReadFATSuper(struct FSSuper *s);
LONG FormatFATVolume(const UBYTE *name, UWORD len, struct Globals *glob);
LONG CompareFATSuper(struct FSSuper *s1, struct FSSuper *s2);
LONG SetVolumeName(struct FSSuper *sb, UBYTE *name, UWORD len);
void FillDiskInfo(struct InfoData *id, struct Globals *glob);
void DoDiskInsert(struct Globals *glob);
BOOL AttemptDestroyVolume(struct FSSuper *sb);
void DoDiskRemove(struct Globals *glob);

/* date.c */
void ConvertFATDate(UWORD date, UWORD time, struct DateStamp *ds,
    struct Globals *glob);
void ConvertDOSDate(struct DateStamp *ds, UWORD *date, UWORD *time,
    struct Globals *glob);

/* file.c */
LONG ReadFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant,
    UBYTE *data, ULONG *nread);
LONG WriteFileChunk(struct IOHandle *ioh, ULONG file_pos, ULONG nwant,
    UBYTE *data, ULONG *nwritten);

/* ops.c */
LONG OpLockFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    LONG access, struct ExtFileLock **filelock, struct Globals *glob);
void OpUnlockFile(struct ExtFileLock *lock, struct Globals *glob);
LONG OpCopyLock(struct ExtFileLock *lock, struct ExtFileLock **copy,
    struct Globals *glob);
LONG OpLockParent(struct ExtFileLock *lock, struct ExtFileLock **parent,
    struct Globals *glob);
LONG OpOpenFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    LONG action, struct ExtFileLock **filelock, struct Globals *glob);
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct Globals *glob);
LONG OpRenameFile(struct ExtFileLock *sdirlock, UBYTE *sname, ULONG snamelen,
    struct ExtFileLock *ddirlock, UBYTE *dname, ULONG dnamelen,
    struct Globals *glob);
LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct ExtFileLock **newdirlock, struct Globals *glob);
LONG OpRead(struct ExtFileLock *lock, UBYTE *data, ULONG want, ULONG *read,
    struct Globals *glob);
LONG OpWrite(struct ExtFileLock *lock, UBYTE *data, ULONG want,
    ULONG *written, struct Globals *glob);
LONG OpSetFileSize(struct ExtFileLock *lock, LONG offset, LONG mode,
    LONG *newsize, struct Globals *glob);
LONG OpSetProtect(struct ExtFileLock *lock, UBYTE *name, ULONG namelen,
    ULONG prot, struct Globals *glob);
LONG OpSetDate(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct DateStamp *ds, struct Globals *glob);
LONG OpAddNotify(struct NotifyRequest *nr, struct Globals *glob);
LONG OpRemoveNotify(struct NotifyRequest *nr, struct Globals *glob);

/* lock.c */
LONG TestLock(struct ExtFileLock *fl, struct Globals *glob);
LONG LockFileByName(struct ExtFileLock *fl, UBYTE *name, LONG namelen,
    LONG access, struct ExtFileLock **lock, struct Globals *glob);
LONG LockFile(ULONG cluster, ULONG entry, LONG access,
    struct ExtFileLock **lock, struct Globals *glob);
LONG LockRoot(LONG access, struct ExtFileLock **lock, struct Globals *glob);
LONG CopyLock(struct ExtFileLock *fl, struct ExtFileLock **lock,
    struct Globals *glob);
void FreeLock(struct ExtFileLock *fl, struct Globals *glob);

/* notify.c */
void SendNotify(struct NotifyRequest *nr, struct Globals *glob);
void SendNotifyByLock(struct FSSuper *sb, struct GlobalLock *gl);
void SendNotifyByDirEntry(struct FSSuper *sb, struct DirEntry *de);
void ProcessNotify(struct Globals *glob);

/* timer.c */
LONG InitTimer(struct Globals *glob);
void CleanupTimer(struct Globals *glob);
void RestartTimer(struct Globals *glob);
void HandleTimer(struct Globals *glob);

#endif
