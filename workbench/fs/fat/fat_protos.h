/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef FAT_HANDLER_PROTO_H
#define FAT_HANDLER_PROTO_H

/* direntry */
LONG GetDirCacheEntry(struct FSSuper *sb, struct DirCache *dc, LONG entry, struct DirEntry **de);
LONG SetupDirCache(struct FSSuper *sb, struct DirCache *dc, struct Extent *ext, ULONG cluster);
LONG FreeDirCache(struct FSSuper *sb, struct DirCache *dc);

void ConvertDate(UWORD date, UWORD time, struct DateStamp *ds);

LONG ReadNextDirEntry(struct ExtFileLock *fl, struct FileInfoBlock *fib);
LONG FillFIB (struct ExtFileLock *fl, struct FileInfoBlock *fib);
LONG FindEntryByPath(ULONG start_cluster, UBYTE *path, LONG pathlen, ULONG *dst_cluster, ULONG *dst_entry);

/* fat */
LONG ReadFATSuper (struct FSSuper *s);
void FreeFATSuper(struct FSSuper *s);
LONG CompareFATSuper(struct FSSuper *s1, struct FSSuper *s2);

LONG ReadVolumeName(struct FSSuper *sb, UBYTE *dest);

LONG InitExtent(struct FSSuper *sb, struct Extent *ext, ULONG start_cluster);
LONG NextExtent(struct FSSuper *sb, struct Extent *ext);
LONG SeekExtent(struct FSSuper *sb, struct Extent *ext, ULONG dst_sector);

void CountFreeClusters(struct FSSuper *sb);

/* file */
LONG File_Read(struct ExtFileLock *fl, ULONG togo, void *buffer, LONG *result);

/* names.c */
LONG GetLongName(struct FSSuper *sb, struct DirCache *dc, struct DirEntry *de, ULONG entry, STRPTR dest, UBYTE *dlen);
LONG GetShortName(struct DirEntry *de, STRPTR dest, UBYTE *dlen);

/* diskchange */
void ProcessDiskChange (void);
void DoDiskInsert();
void DoDiskRemove();
void SendVolumePacket(struct DosList *vol, ULONG action);

/* diskio */
LONG InitDevice(struct FileSysStartupMsg *fssm, LONG blocksize);
LONG DoRawRead (ULONG n, void *buff);
LONG InitDiskHandler (struct FileSysStartupMsg *fssm, ULONG *ressigbit);
void CleanupDiskHandler(ULONG diskchgsig_bit);

/* info.c */
void FillDiskInfo (struct InfoData *id);
 
/* lock.c */
LONG TryLockObj(struct ExtFileLock *fl, UBYTE *name, LONG namelen, LONG access, BPTR *result);
LONG LockFile(ULONG entry, ULONG cluster, LONG axs, BPTR *res);
LONG LockRoot(LONG axs, BPTR *res);
LONG LockParent(struct ExtFileLock *ld, LONG axs, BPTR *res);
LONG IsLockable(ULONG ino, LONG mode);
void FreeLock(struct ExtFileLock *fl);
LONG CopyLock(struct ExtFileLock *src_fl, BPTR *res);

/* packet.c */
void ProcessPackets(void);

#include "fat_inlines.h"

#endif

