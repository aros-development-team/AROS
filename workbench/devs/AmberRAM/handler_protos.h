/*

File: handler_protos.h
Author: Neil Cafferkey
Copyright (C) 2001-2008 Neil Cafferkey

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this file; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef HANDLER_PROTOS_H
#define HANDLER_PROTOS_H


#include "handler.h"


/* Function prototypes */

BOOL CmdStartup(struct Handler *h, STRPTR name, struct DeviceNode *dev_node,
   struct MsgPort *proc_port);
BOOL CmdDie(struct Handler *handler);
BOOL CmdIsFileSystem();
BOOL CmdFind(struct Handler *handler, struct FileHandle *handle,
   struct Lock *lock, const TEXT *name, ULONG mode);
BOOL CmdFHFromLock(struct Handler *handler, struct FileHandle *handle,
   struct Lock *lock);
BOOL CmdEnd(struct Handler *handler, struct Opening *opening);
UPINT CmdRead(struct Handler *handler, struct Opening *opening, UBYTE *buffer, UPINT length);
UPINT CmdWrite(struct Handler *handler, struct Opening *opening,
   UBYTE *buffer, UPINT length);
UPINT CmdSeek(struct Handler *handler, struct Opening *opening, PINT offset, LONG mode);
PINT CmdSetFileSize(struct Handler *handler, struct Opening *opening,
   PINT offset, LONG mode);
struct Lock *CmdLocateObject(struct Handler *handler,
   struct Lock *lock, const TEXT *name, ULONG mode);
BOOL CmdFreeLock(struct Handler *handler, struct Lock *lock);
struct Lock *CmdCopyDir(struct Handler *handler, struct Lock *lock);
struct Lock *CmdCopyDirFH(struct Handler *handler,
   struct Opening *opening);
struct Lock *CmdParent(struct Handler *handler, struct Lock *lock);
struct Lock *CmdParentFH(struct Handler *handler,
   struct Opening *opening);
BOOL CmdSameLock(struct Handler *handler, struct Lock *lock1, struct Lock *lock2);
struct Lock *CmdCreateDir(struct Handler *handler,
   struct Lock *lock, const TEXT *name);
BOOL CmdExamineObject(struct Handler *handler, struct Lock *lock,
   struct FileInfoBlock *info);
BOOL CmdExamineFH(struct Handler *handler, struct Opening *opening,
   struct FileInfoBlock *info);
BOOL CmdExamineNext(struct Handler *handler, struct FileInfoBlock *info);
BOOL CmdExamineAll(struct Handler *handler, struct Lock *lock,
   UBYTE *buffer, ULONG size, ULONG type, struct ExAllControl *control);
VOID CmdExamineAllEnd(struct Handler *handler, struct Lock *lock,
   UBYTE *buffer, ULONG size, ULONG type, struct ExAllControl *control);
BOOL CmdInfo(struct Handler *handler, struct InfoData *info_data);
BOOL CmdSetProtect(struct Handler *handler, struct Lock *lock,
   const TEXT *name, ULONG flags);
BOOL CmdSetComment(struct Handler *handler, struct Lock *lock,
   const TEXT *name, const TEXT *comment);
BOOL CmdRenameObject(struct Handler *handler, struct Lock *old_lock,
   STRPTR old_name, struct Lock *new_lock, STRPTR new_name);
BOOL CmdRenameDisk(struct Handler *handler, STRPTR new_name);
BOOL CmdSetDate(struct Handler *handler, struct Lock *lock, STRPTR name,
   struct DateStamp *date);
BOOL CmdDeleteObject(struct Handler *handler, struct Lock *lock,
   STRPTR name);
struct DosList *CmdCurrentVolume(struct Handler *handler);
BOOL CmdChangeMode(struct Handler *handler, ULONG type, APTR thing, ULONG new_mode);
BOOL CmdMakeLink(struct Handler *handler, struct Lock *lock, STRPTR name,
   APTR reference, LONG link_type);
LONG CmdReadLink(struct Handler *handler, struct Lock *lock,
   const TEXT *name, TEXT *buffer, LONG buffer_size);
BOOL CmdWriteProtect(struct Handler *handler, BOOL on, ULONG key);
BOOL CmdFlush();
BOOL CmdAddNotify(struct Handler *handler, struct NotifyRequest *request);
BOOL CmdRemoveNotify(struct Handler *handler,
   struct NotifyRequest *request);

VOID DeleteHandler(struct Handler *handler);
struct Object *CreateObject(struct Handler *handler, const TEXT *name,
   BYTE type, struct Object *parent);
BOOL AttemptDeleteObject(struct Handler *handler, struct Object *object,
   BOOL notify);
VOID DeleteObject(struct Handler *handler, struct Object *object);
struct Object *GetHardObject(struct Handler *handler, struct Lock *lock,
   const TEXT *name, struct Object **parent);
struct Object *GetObject(struct Handler *handler, struct Lock *lock,
   const TEXT *name, struct Object **parent, LONG *remainder_pos);
PINT ChangeFileSize(struct Handler *handler, struct Opening *opening,
   PINT offset, LONG mode);
UPINT ReadData(struct Opening *opening, UBYTE *buffer, UPINT length);
UPINT WriteData(struct Handler *handler, struct Opening *opening,
   UBYTE *buffer, UPINT length);
struct Lock *LockObject(struct Handler *handler,
   struct Object *object, LONG access);
BOOL ExamineObject(struct Handler *handler, struct Object *object,
   struct FileInfoBlock *info);
VOID AdjustExaminations(struct Handler *handler, struct Object *object);
BOOL SetName(struct Handler *handler, struct Object *object,
   const TEXT *name);
UPINT GetBlockLength(struct Object *file, struct Block *block);
struct Object *GetRealObject(struct Object *object);

VOID MatchNotifyRequests(struct Handler *handler);
VOID UnmatchNotifyRequests(struct Handler *handler, struct Object *object);
VOID NotifyAll(struct Handler *handler, struct Object *object,
   BOOL notify_links);
VOID ReceiveNotifyReply(struct Handler *handler,
   struct NotifyMessage *message);
struct Notification *FindNotification(struct Handler *handler,
   struct NotifyRequest *request);
VOID Notify(struct Handler *handler, struct Notification *notification);

UBYTE *MkBStr(struct Handler *h, STRPTR str);

PINT SetString(struct Handler *handler, TEXT **field, const TEXT *new_str);
PINT SwapStrings(TEXT **field1, TEXT **field2);
UPINT StrLen(const TEXT *s);
UPINT StrSize(const TEXT *s);
struct Node *FindNameNoCase(struct Handler *handler, struct List *start, const TEXT *name);
struct DosList *MyMakeDosEntry(struct Handler *handler, const TEXT *name, LONG type);
VOID MyFreeDosEntry(struct Handler *handler, struct DosList *entry);
BOOL MyRenameDosEntry(struct Handler *handler, struct DosList *entry, const TEXT *name);


#endif


