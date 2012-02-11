/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#ifndef NTFS_PROTOS_H
#define NTFS_PROTOS_H

/* disk.c */
void ProcessDiskChange (void);
void DoDiskInsert();
BOOL AttemptDestroyVolume(struct FSData *);
void DoDiskRemove();
void SendVolumePacket(struct DosList *, ULONG);

LONG InitDiskHandler(struct FileSysStartupMsg *);
void CleanupDiskHandler(void);
void UpdateDisk(void);
void Probe_64bit_support(void);
ULONG AccessDisk(BOOL, ULONG, ULONG, ULONG, UBYTE *);

/* info.c */
void FillDiskInfo (struct InfoData *);
 
/* packet.c */
void ProcessPackets(void);
void ReplyPacket(struct DosPacket *);

/* direntry.c */
LONG InitDirHandle(struct FSData *, struct DirHandle *, BOOL);
LONG ReleaseDirHandle(struct DirHandle *);

LONG GetDirEntry(struct DirHandle *, ULONG, struct DirEntry *);
LONG GetNextDirEntry(struct DirHandle *, struct DirEntry *, BOOL);
LONG GetDirEntryByCluster(struct DirHandle *, ULONG, struct DirEntry *);

LONG GetDirEntryByName(struct DirHandle *, STRPTR, ULONG, struct DirEntry *);
LONG GetDirEntryByPath(struct DirHandle *, STRPTR, ULONG, struct DirEntry *);

LONG GetParentDir(struct DirHandle *, struct DirEntry *);

LONG UpdateDirEntry(struct DirEntry *);

LONG FillFIB(struct ExtFileLock *, struct FileInfoBlock *);

/* names.c */
LONG SetDirEntryName(struct DirEntry *, STRPTR, ULONG);

/* ntfs.c */
LONG ReadBootSector (struct FSData *);
void FreeBootSector(struct FSData *);
ULONG PostProcessMFTRecord(struct FSData *, struct MFTRecordEntry *, int, UBYTE *);
ULONG PreProcessMFTRecord(struct FSData *, struct MFTRecordEntry *, int);
IPTR InitMFTEntry(struct NTFSMFTEntry *, ULONG);
IPTR ReadMFTAttribData(struct NTFSMFTAttr *, struct MFTAttr *, UBYTE *, UQUAD, ULONG, int);
IPTR ReadMFTAttrib(struct NTFSMFTAttr *, UBYTE *, UQUAD, ULONG, int);
struct MFTAttr *MapMFTAttrib (struct NTFSMFTAttr *, struct NTFSMFTEntry *, UBYTE);
struct MFTAttr *FindMFTAttrib(struct NTFSMFTAttr *, UBYTE);
void FreeMFTAttrib(struct NTFSMFTAttr *);
LONG ProcessFSEntry(struct NTFSMFTEntry *, struct DirEntry *, ULONG **);

/* ops.c */
LONG OpLockFile(struct ExtFileLock *, UBYTE *, ULONG, LONG, struct ExtFileLock **);
void OpUnlockFile(struct ExtFileLock *);
LONG OpCopyLock(struct ExtFileLock *, struct ExtFileLock **);
LONG OpLockParent(struct ExtFileLock *, struct ExtFileLock **);
LONG OpOpenFile(struct ExtFileLock *, UBYTE *, ULONG, LONG, struct ExtFileLock **);
LONG OpDeleteFile(struct ExtFileLock *, UBYTE *, ULONG);
LONG OpRenameFile(struct ExtFileLock *, UBYTE *, ULONG, struct ExtFileLock *, UBYTE *, ULONG);
LONG OpCreateDir(struct ExtFileLock *, UBYTE *, ULONG, struct ExtFileLock **);
LONG OpRead(struct ExtFileLock *, UBYTE *, UQUAD, UQUAD *);
LONG OpWrite(struct ExtFileLock *, UBYTE *, UQUAD, UQUAD *);
LONG OpSetFileSize(struct ExtFileLock *, UQUAD, LONG, UQUAD *);
LONG OpSetProtect(struct ExtFileLock *, UBYTE *, ULONG, ULONG);
LONG OpSetDate(struct ExtFileLock *, UBYTE *, ULONG, struct DateStamp *);
LONG OpAddNotify(struct NotifyRequest *);
LONG OpRemoveNotify(struct NotifyRequest *);

/* lock.c */
LONG TestLock(struct ExtFileLock *);
LONG LockFileByName(struct ExtFileLock *, UBYTE *, LONG, LONG, struct ExtFileLock **);
LONG LockFile(struct DirEntry *, LONG, struct ExtFileLock **) ;
LONG LockRoot(LONG, struct ExtFileLock **);
LONG CopyLock(struct ExtFileLock *, struct ExtFileLock **);
void FreeLock(struct ExtFileLock *);

/* notify.c */
void SendNotify(struct NotifyRequest *);
void SendNotifyByLock(struct FSData *, struct GlobalLock *);
void SendNotifyByDirEntry(struct FSData *, struct DirEntry *);
void ProcessNotify(void); 

/* timer.c */
LONG InitTimer(void);
void CleanupTimer(void);
void RestartTimer(void);
void HandleTimer(void);

/* support.c */
int ilog2(ULONG);
void NTFS2DateStamp(UQUAD *, struct DateStamp *);
#endif
