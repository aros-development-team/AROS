#ifndef PROTO_DOS_H
#define PROTO_DOS_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DOS_BASE_NAME
#define DOS_BASE_NAME DOSBase
#endif

#define AbortPkt(port, pkt) \
	LP2NR(0xb0, AbortPkt, struct MsgPort *, port, struct DosPacket *, pkt, \
	, DOS_BASE_NAME)

#define AddBuffers(name, number) \
	LP2(0x1e8, LONG, AddBuffers, STRPTR, name, long, number, \
	, DOS_BASE_NAME)

#define AddDosEntry(dlist) \
	LP1(0x1c4, LONG, AddDosEntry, struct DosList *, dlist, \
	, DOS_BASE_NAME)

#define AddPart(dirname, filename, size) \
	LP3(0x24c, BOOL, AddPart, STRPTR, dirname, STRPTR, filename, unsigned long, size, \
	, DOS_BASE_NAME)

#define AddSegment(name, seg, system) \
	LP3(0x204, LONG, AddSegment, STRPTR, name, BPTR, seg, long, system, \
	, DOS_BASE_NAME)

#define AllocDosObject(type, tags) \
	LP2(0x98, APTR, AllocDosObject, unsigned long, type, struct TagItem *, tags, \
	, DOS_BASE_NAME)

#define AllocDosObjectTagList(a0, a1) AllocDosObject ((a0), (a1))

#ifndef NO_INLINE_STDARG
#define AllocDosObjectTags(a0, tags...) \
	({ULONG _tags[] = { tags }; AllocDosObject((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AssignAdd(name, lock) \
	LP2(0x1a4, BOOL, AssignAdd, STRPTR, name, BPTR, lock, \
	, DOS_BASE_NAME)

#define AssignLate(name, path) \
	LP2(0x19c, BOOL, AssignLate, STRPTR, name, STRPTR, path, \
	, DOS_BASE_NAME)

#define AssignLock(name, lock) \
	LP2(0x198, LONG, AssignLock, STRPTR, name, BPTR, lock, \
	, DOS_BASE_NAME)

#define AssignPath(name, path) \
	LP2(0x1a0, BOOL, AssignPath, STRPTR, name, STRPTR, path, \
	, DOS_BASE_NAME)

#define AttemptLockDosList(flags) \
	LP1(0x1bc, struct DosList *, AttemptLockDosList, unsigned long, flags, \
	, DOS_BASE_NAME)

#define ChangeMode(type, fh, newmode) \
	LP3(0x12c, LONG, ChangeMode, long, type, BPTR, fh, long, newmode, \
	, DOS_BASE_NAME)

#define CheckSignal(mask) \
	LP1(0x210, LONG, CheckSignal, long, mask, \
	, DOS_BASE_NAME)

#define Cli() \
	LP0(0x148, struct CommandLineInterface *, Cli, \
	, DOS_BASE_NAME)

#define CliInitNewcli(dp) \
	LP1(0x2bc, LONG, CliInitNewcli, struct DosPacket *, dp, \
	, DOS_BASE_NAME)

#define CliInitRun(dp) \
	LP1(0x270, LONG, CliInitRun, struct DosPacket *, dp, \
	, DOS_BASE_NAME)

#define Close(file) \
	LP1(0x18, LONG, Close, BPTR, file, \
	, DOS_BASE_NAME)

#define CompareDates(date1, date2) \
	LP2(0x1ec, LONG, CompareDates, struct DateStamp *, date1, struct DateStamp *, date2, \
	, DOS_BASE_NAME)

#define CreateDir(name) \
	LP1(0x50, BPTR, CreateDir, STRPTR, name, \
	, DOS_BASE_NAME)

#define CreateNewProc(tags) \
	LP1(0x14c, struct Process *, CreateNewProc, struct TagItem *, tags, \
	, DOS_BASE_NAME)

#define CreateNewProcTagList(a0) CreateNewProc ((a0))

#ifndef NO_INLINE_STDARG
#define CreateNewProcTags(tags...) \
	({ULONG _tags[] = { tags }; CreateNewProc((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define CreateProc(name, pri, segList, stackSize) \
	LP4(0x5c, struct MsgPort *, CreateProc, STRPTR, name, long, pri, BPTR, segList, long, stackSize, \
	, DOS_BASE_NAME)

#define CurrentDir(lock) \
	LP1(0x54, BPTR, CurrentDir, BPTR, lock, \
	, DOS_BASE_NAME)

#define DateStamp(date) \
	LP1(0x80, struct DateStamp *, DateStamp, struct DateStamp *, date, \
	, DOS_BASE_NAME)

#define DateToStr(datetime) \
	LP1(0x1f0, LONG, DateToStr, struct DateTime *, datetime, \
	, DOS_BASE_NAME)

#define Delay(timeout) \
	LP1NR(0x84, Delay, long, timeout, \
	, DOS_BASE_NAME)

#define DeleteFile(name) \
	LP1(0x30, LONG, DeleteFile, STRPTR, name, \
	, DOS_BASE_NAME)

#define DeleteVar(name, flags) \
	LP2(0x260, LONG, DeleteVar, STRPTR, name, unsigned long, flags, \
	, DOS_BASE_NAME)

#define DeviceProc(name) \
	LP1(0x74, struct MsgPort *, DeviceProc, STRPTR, name, \
	, DOS_BASE_NAME)

#define DoPkt(port, action, arg1, arg2, arg3, arg4, arg5) \
	LP7(0xa0, LONG, DoPkt, struct MsgPort *, port, long, action, long, arg1, long, arg2, long, arg3, long, arg4, long, arg5, \
	, DOS_BASE_NAME)

#define DoPkt0(port, action) \
	LP2(0xa0, LONG, DoPkt0, struct MsgPort *, port, long, action, \
	, DOS_BASE_NAME)

#define DoPkt1(port, action, arg1) \
	LP3(0xa0, LONG, DoPkt1, struct MsgPort *, port, long, action, long, arg1, \
	, DOS_BASE_NAME)

#define DoPkt2(port, action, arg1, arg2) \
	LP4(0xa0, LONG, DoPkt2, struct MsgPort *, port, long, action, long, arg1, long, arg2, \
	, DOS_BASE_NAME)

#define DoPkt3(port, action, arg1, arg2, arg3) \
	LP5(0xa0, LONG, DoPkt3, struct MsgPort *, port, long, action, long, arg1, long, arg2, long, arg3, \
	, DOS_BASE_NAME)

#define DoPkt4(port, action, arg1, arg2, arg3, arg4) \
	LP6(0xa0, LONG, DoPkt4, struct MsgPort *, port, long, action, long, arg1, long, arg2, long, arg3, long, arg4, \
	, DOS_BASE_NAME)

#define DupLock(lock) \
	LP1(0x40, BPTR, DupLock, BPTR, lock, \
	, DOS_BASE_NAME)

#define DupLockFromFH(fh) \
	LP1(0xf8, BPTR, DupLockFromFH, BPTR, fh, \
	, DOS_BASE_NAME)

#define EndNotify(notify) \
	LP1NR(0x254, EndNotify, struct NotifyRequest *, notify, \
	, DOS_BASE_NAME)

#define ErrorReport(code, type, arg1, device) \
	LP4(0x140, LONG, ErrorReport, long, code, long, type, unsigned long, arg1, struct MsgPort *, device, \
	, DOS_BASE_NAME)

#define ExAll(lock, buffer, size, data, control) \
	LP5(0x120, LONG, ExAll, BPTR, lock, struct ExAllData *, buffer, long, size, long, data, struct ExAllControl *, control, \
	, DOS_BASE_NAME)

#define ExAllEnd(lock, buffer, size, data, control) \
	LP5NR(0x294, ExAllEnd, BPTR, lock, struct ExAllData *, buffer, long, size, long, data, struct ExAllControl *, control, \
	, DOS_BASE_NAME)

#define ExNext(lock, fileInfoBlock) \
	LP2(0x48, LONG, ExNext, BPTR, lock, struct FileInfoBlock *, fileInfoBlock, \
	, DOS_BASE_NAME)

#define Examine(lock, fileInfoBlock) \
	LP2(0x44, LONG, Examine, BPTR, lock, struct FileInfoBlock *, fileInfoBlock, \
	, DOS_BASE_NAME)

#define ExamineFH(fh, fib) \
	LP2(0x104, BOOL, ExamineFH, BPTR, fh, struct FileInfoBlock *, fib, \
	, DOS_BASE_NAME)

#define Execute(string, file, file2) \
	LP3(0x94, LONG, Execute, STRPTR, string, BPTR, file, BPTR, file2, \
	, DOS_BASE_NAME)

#define Exit(returnCode) \
	LP1NR(0x60, Exit, long, returnCode, \
	, DOS_BASE_NAME)

#define FGetC(fh) \
	LP1(0xcc, LONG, FGetC, BPTR, fh, \
	, DOS_BASE_NAME)

#define FGets(fh, buf, buflen) \
	LP3(0xe0, STRPTR, FGets, BPTR, fh, STRPTR, buf, unsigned long, buflen, \
	, DOS_BASE_NAME)

#define FPutC(fh, ch) \
	LP2(0xd0, LONG, FPutC, BPTR, fh, long, ch, \
	, DOS_BASE_NAME)

#define FPuts(fh, str) \
	LP2(0xe4, LONG, FPuts, BPTR, fh, STRPTR, str, \
	, DOS_BASE_NAME)

#define FRead(fh, block, blocklen, number) \
	LP4(0xd8, LONG, FRead, BPTR, fh, APTR, block, unsigned long, blocklen, unsigned long, number, \
	, DOS_BASE_NAME)

#define FWrite(fh, block, blocklen, number) \
	LP4(0xdc, LONG, FWrite, BPTR, fh, APTR, block, unsigned long, blocklen, unsigned long, number, \
	, DOS_BASE_NAME)

#define Fault(code, header, buffer, len) \
	LP4(0x138, BOOL, Fault, long, code, STRPTR, header, STRPTR, buffer, long, len, \
	, DOS_BASE_NAME)

#define FilePart(path) \
	LP1(0x244, STRPTR, FilePart, STRPTR, path, \
	, DOS_BASE_NAME)

#define FindArg(keyword, arg_template) \
	LP2(0x218, LONG, FindArg, STRPTR, keyword, STRPTR, arg_template, \
	, DOS_BASE_NAME)

#define FindCliProc(num) \
	LP1(0x16c, struct Process *, FindCliProc, unsigned long, num, \
	, DOS_BASE_NAME)

#define FindDosEntry(dlist, name, flags) \
	LP3(0x1c8, struct DosList *, FindDosEntry, struct DosList *, dlist, STRPTR, name, unsigned long, flags, \
	, DOS_BASE_NAME)

#define FindSegment(name, seg, system) \
	LP3(0x208, struct Segment *, FindSegment, STRPTR, name, struct Segment *, seg, long, system, \
	, DOS_BASE_NAME)

#define FindVar(name, type) \
	LP2(0x264, struct LocalVar *, FindVar, STRPTR, name, unsigned long, type, \
	, DOS_BASE_NAME)

#define Flush(fh) \
	LP1(0xf0, LONG, Flush, BPTR, fh, \
	, DOS_BASE_NAME)

#define Format(filesystem, volumename, dostype) \
	LP3(0x1dc, BOOL, Format, STRPTR, filesystem, STRPTR, volumename, unsigned long, dostype, \
	, DOS_BASE_NAME)

#define FreeArgs(args) \
	LP1NR(0x23c, FreeArgs, struct RDArgs *, args, \
	, DOS_BASE_NAME)

#define FreeDeviceProc(dp) \
	LP1NR(0x1b0, FreeDeviceProc, struct DevProc *, dp, \
	, DOS_BASE_NAME)

#define FreeDosEntry(dlist) \
	LP1NR(0x1d4, FreeDosEntry, struct DosList *, dlist, \
	, DOS_BASE_NAME)

#define FreeDosObject(type, ptr) \
	LP2NR(0x9c, FreeDosObject, unsigned long, type, APTR, ptr, \
	, DOS_BASE_NAME)

#define GetArgStr() \
	LP0(0x164, STRPTR, GetArgStr, \
	, DOS_BASE_NAME)

#define GetConsoleTask() \
	LP0(0x154, struct MsgPort *, GetConsoleTask, \
	, DOS_BASE_NAME)

#define GetCurrentDirName(buf, len) \
	LP2(0x178, BOOL, GetCurrentDirName, STRPTR, buf, long, len, \
	, DOS_BASE_NAME)

#define GetDeviceProc(name, dp) \
	LP2(0x1ac, struct DevProc *, GetDeviceProc, STRPTR, name, struct DevProc *, dp, \
	, DOS_BASE_NAME)

#define GetFileSysTask() \
	LP0(0x15c, struct MsgPort *, GetFileSysTask, \
	, DOS_BASE_NAME)

#define GetProgramDir() \
	LP0(0x190, BPTR, GetProgramDir, \
	, DOS_BASE_NAME)

#define GetProgramName(buf, len) \
	LP2(0x180, BOOL, GetProgramName, STRPTR, buf, long, len, \
	, DOS_BASE_NAME)

#define GetPrompt(buf, len) \
	LP2(0x188, BOOL, GetPrompt, STRPTR, buf, long, len, \
	, DOS_BASE_NAME)

#define GetVar(name, buffer, size, flags) \
	LP4(0x25c, LONG, GetVar, STRPTR, name, STRPTR, buffer, long, size, long, flags, \
	, DOS_BASE_NAME)

#define Info(lock, parameterBlock) \
	LP2(0x4c, LONG, Info, BPTR, lock, struct InfoData *, parameterBlock, \
	, DOS_BASE_NAME)

#define Inhibit(name, onoff) \
	LP2(0x1e4, LONG, Inhibit, STRPTR, name, long, onoff, \
	, DOS_BASE_NAME)

#define Input() \
	LP0(0x24, BPTR, Input, \
	, DOS_BASE_NAME)

#define InternalLoadSeg(fh, table, funcarray, stack) \
	LP4(0x1f8, BPTR, InternalLoadSeg, BPTR, fh, d0, BPTR, table, LONG *, funcarray, LONG *, stack, \
	, DOS_BASE_NAME)

#define InternalUnLoadSeg(seglist, freefunc) \
	LP2FP(0x1fc, BOOL, InternalUnLoadSeg, BPTR, seglist, __fpt, freefunc, \
	, DOS_BASE_NAME, void (*__fpt)())

#define IoErr() \
	LP0(0x58, LONG, IoErr, \
	, DOS_BASE_NAME)

#define IsFileSystem(name) \
	LP1(0x1d8, BOOL, IsFileSystem, STRPTR, name, \
	, DOS_BASE_NAME)

#define IsInteractive(file) \
	LP1(0x90, LONG, IsInteractive, BPTR, file, \
	, DOS_BASE_NAME)

#define LoadSeg(name) \
	LP1(0x64, BPTR, LoadSeg, STRPTR, name, \
	, DOS_BASE_NAME)

#define Lock(name, type) \
	LP2(0x38, BPTR, Lock, STRPTR, name, long, type, \
	, DOS_BASE_NAME)

#define LockDosList(flags) \
	LP1(0x1b4, struct DosList *, LockDosList, unsigned long, flags, \
	, DOS_BASE_NAME)

#define LockRecord(fh, offset, length, mode, timeout) \
	LP5(0xb4, BOOL, LockRecord, BPTR, fh, unsigned long, offset, unsigned long, length, unsigned long, mode, unsigned long, timeout, \
	, DOS_BASE_NAME)

#define LockRecords(recArray, timeout) \
	LP2(0xb8, BOOL, LockRecords, struct RecordLock *, recArray, unsigned long, timeout, \
	, DOS_BASE_NAME)

#define MakeDosEntry(name, type) \
	LP2(0x1d0, struct DosList *, MakeDosEntry, STRPTR, name, long, type, \
	, DOS_BASE_NAME)

#define MakeLink(name, dest, soft) \
	LP3(0x128, LONG, MakeLink, STRPTR, name, long, dest, long, soft, \
	, DOS_BASE_NAME)

#define MatchEnd(anchor) \
	LP1NR(0x22c, MatchEnd, struct AnchorPath *, anchor, \
	, DOS_BASE_NAME)

#define MatchFirst(pat, anchor) \
	LP2(0x224, LONG, MatchFirst, STRPTR, pat, struct AnchorPath *, anchor, \
	, DOS_BASE_NAME)

#define MatchNext(anchor) \
	LP1(0x228, LONG, MatchNext, struct AnchorPath *, anchor, \
	, DOS_BASE_NAME)

#define MatchPattern(pat, str) \
	LP2(0x234, BOOL, MatchPattern, STRPTR, pat, STRPTR, str, \
	, DOS_BASE_NAME)

#define MatchPatternNoCase(pat, str) \
	LP2(0x288, BOOL, MatchPatternNoCase, STRPTR, pat, STRPTR, str, \
	, DOS_BASE_NAME)

#define MaxCli() \
	LP0(0x170, ULONG, MaxCli, \
	, DOS_BASE_NAME)

#define NameFromFH(fh, buffer, len) \
	LP3(0x110, LONG, NameFromFH, BPTR, fh, STRPTR, buffer, long, len, \
	, DOS_BASE_NAME)

#define NameFromLock(lock, buffer, len) \
	LP3(0x10c, LONG, NameFromLock, BPTR, lock, STRPTR, buffer, long, len, \
	, DOS_BASE_NAME)

#define NewLoadSeg(file, tags) \
	LP2(0x200, BPTR, NewLoadSeg, STRPTR, file, struct TagItem *, tags, \
	, DOS_BASE_NAME)

#define NewLoadSegTagList(a0, a1) NewLoadSeg ((a0), (a1))

#ifndef NO_INLINE_STDARG
#define NewLoadSegTags(a0, tags...) \
	({ULONG _tags[] = { tags }; NewLoadSeg((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define NextDosEntry(dlist, flags) \
	LP2(0x1cc, struct DosList *, NextDosEntry, struct DosList *, dlist, unsigned long, flags, \
	, DOS_BASE_NAME)

#define Open(name, accessMode) \
	LP2(0x14, BPTR, Open, STRPTR, name, long, accessMode, \
	, DOS_BASE_NAME)

#define OpenFromLock(lock) \
	LP1(0xfc, BPTR, OpenFromLock, BPTR, lock, \
	, DOS_BASE_NAME)

#define Output() \
	LP0(0x28, BPTR, Output, \
	, DOS_BASE_NAME)

#define ParentDir(lock) \
	LP1(0x8c, BPTR, ParentDir, BPTR, lock, \
	, DOS_BASE_NAME)

#define ParentOfFH(fh) \
	LP1(0x100, BPTR, ParentOfFH, BPTR, fh, \
	, DOS_BASE_NAME)

#define ParsePattern(pat, buf, buflen) \
	LP3(0x230, LONG, ParsePattern, STRPTR, pat, STRPTR, buf, long, buflen, \
	, DOS_BASE_NAME)

#define ParsePatternNoCase(pat, buf, buflen) \
	LP3(0x284, LONG, ParsePatternNoCase, STRPTR, pat, STRPTR, buf, long, buflen, \
	, DOS_BASE_NAME)

#define PathPart(path) \
	LP1(0x248, STRPTR, PathPart, STRPTR, path, \
	, DOS_BASE_NAME)

#define PrintFault(code, header) \
	LP2(0x13c, BOOL, PrintFault, long, code, STRPTR, header, \
	, DOS_BASE_NAME)

#define PutStr(str) \
	LP1(0x278, LONG, PutStr, STRPTR, str, \
	, DOS_BASE_NAME)

#define Read(file, buffer, length) \
	LP3(0x1c, LONG, Read, BPTR, file, APTR, buffer, long, length, \
	, DOS_BASE_NAME)

#define ReadArgs(arg_template, array, args) \
	LP3(0x214, struct RDArgs *, ReadArgs, STRPTR, arg_template, LONG *, array, struct RDArgs *, args, \
	, DOS_BASE_NAME)

#define ReadItem(name, maxchars, cSource) \
	LP3(0x21c, LONG, ReadItem, STRPTR, name, long, maxchars, struct CSource *, cSource, \
	, DOS_BASE_NAME)

#define ReadLink(port, lock, path, buffer, size) \
	LP5(0x124, LONG, ReadLink, struct MsgPort *, port, BPTR, lock, STRPTR, path, STRPTR, buffer, unsigned long, size, \
	, DOS_BASE_NAME)

#define Relabel(drive, newname) \
	LP2(0x1e0, LONG, Relabel, STRPTR, drive, STRPTR, newname, \
	, DOS_BASE_NAME)

#define RemAssignList(name, lock) \
	LP2(0x1a8, LONG, RemAssignList, STRPTR, name, BPTR, lock, \
	, DOS_BASE_NAME)

#define RemDosEntry(dlist) \
	LP1(0x1c0, BOOL, RemDosEntry, struct DosList *, dlist, \
	, DOS_BASE_NAME)

#define RemSegment(seg) \
	LP1(0x20c, LONG, RemSegment, struct Segment *, seg, \
	, DOS_BASE_NAME)

#define Rename(oldName, newName) \
	LP2(0x34, LONG, Rename, STRPTR, oldName, STRPTR, newName, \
	, DOS_BASE_NAME)

#define ReplyPkt(dp, res1, res2) \
	LP3NR(0xac, ReplyPkt, struct DosPacket *, dp, long, res1, long, res2, \
	, DOS_BASE_NAME)

#define RunCommand(seg, stack, paramptr, paramlen) \
	LP4(0x150, LONG, RunCommand, BPTR, seg, long, stack, STRPTR, paramptr, long, paramlen, \
	, DOS_BASE_NAME)

#define SameDevice(lock1, lock2) \
	LP2(0x290, BOOL, SameDevice, BPTR, lock1, BPTR, lock2, \
	, DOS_BASE_NAME)

#define SameLock(lock1, lock2) \
	LP2(0x118, LONG, SameLock, BPTR, lock1, BPTR, lock2, \
	, DOS_BASE_NAME)

#define Seek(file, position, offset) \
	LP3(0x2c, LONG, Seek, BPTR, file, long, position, long, offset, \
	, DOS_BASE_NAME)

#define SelectInput(fh) \
	LP1(0xc4, BPTR, SelectInput, BPTR, fh, \
	, DOS_BASE_NAME)

#define SelectOutput(fh) \
	LP1(0xc8, BPTR, SelectOutput, BPTR, fh, \
	, DOS_BASE_NAME)

#define SendPkt(dp, port, replyport) \
	LP3NR(0xa4, SendPkt, struct DosPacket *, dp, struct MsgPort *, port, struct MsgPort *, replyport, \
	, DOS_BASE_NAME)

#define SetArgStr(string) \
	LP1(0x168, BOOL, SetArgStr, STRPTR, string, \
	, DOS_BASE_NAME)

#define SetComment(name, comment) \
	LP2(0x78, LONG, SetComment, STRPTR, name, STRPTR, comment, \
	, DOS_BASE_NAME)

#define SetConsoleTask(task) \
	LP1(0x158, struct MsgPort *, SetConsoleTask, struct MsgPort *, task, \
	, DOS_BASE_NAME)

#define SetCurrentDirName(name) \
	LP1(0x174, BOOL, SetCurrentDirName, STRPTR, name, \
	, DOS_BASE_NAME)

#define SetFileDate(name, date) \
	LP2(0x108, LONG, SetFileDate, STRPTR, name, struct DateStamp *, date, \
	, DOS_BASE_NAME)

#define SetFileSize(fh, pos, mode) \
	LP3(0x130, LONG, SetFileSize, BPTR, fh, long, pos, long, mode, \
	, DOS_BASE_NAME)

#define SetFileSysTask(task) \
	LP1(0x160, struct MsgPort *, SetFileSysTask, struct MsgPort *, task, \
	, DOS_BASE_NAME)

#define SetIoErr(result) \
	LP1(0x134, LONG, SetIoErr, long, result, \
	, DOS_BASE_NAME)

#define SetMode(fh, mode) \
	LP2(0x11c, LONG, SetMode, BPTR, fh, long, mode, \
	, DOS_BASE_NAME)

#define SetOwner(name, owner_info) \
	LP2(0x298, BOOL, SetOwner, STRPTR, name, long, owner_info, \
	, DOS_BASE_NAME)

#define SetProgramDir(lock) \
	LP1(0x18c, BPTR, SetProgramDir, BPTR, lock, \
	, DOS_BASE_NAME)

#define SetProgramName(name) \
	LP1(0x17c, BOOL, SetProgramName, STRPTR, name, \
	, DOS_BASE_NAME)

#define SetPrompt(name) \
	LP1(0x184, BOOL, SetPrompt, STRPTR, name, \
	, DOS_BASE_NAME)

#define SetProtection(name, protect) \
	LP2(0x7c, LONG, SetProtection, STRPTR, name, long, protect, \
	, DOS_BASE_NAME)

#define SetVBuf(fh, buff, type, size) \
	LP4(0xf4, LONG, SetVBuf, BPTR, fh, STRPTR, buff, long, type, long, size, \
	, DOS_BASE_NAME)

#define SetVar(name, buffer, size, flags) \
	LP4(0x258, BOOL, SetVar, STRPTR, name, STRPTR, buffer, long, size, long, flags, \
	, DOS_BASE_NAME)

#define SplitName(name, seperator, buf, oldpos, size) \
	LP5(0x114, WORD, SplitName, STRPTR, name, unsigned long, seperator, STRPTR, buf, long, oldpos, long, size, \
	, DOS_BASE_NAME)

#define StartNotify(notify) \
	LP1(0x250, BOOL, StartNotify, struct NotifyRequest *, notify, \
	, DOS_BASE_NAME)

#define StrToDate(datetime) \
	LP1(0x1f4, LONG, StrToDate, struct DateTime *, datetime, \
	, DOS_BASE_NAME)

#define StrToLong(string, value) \
	LP2(0x220, LONG, StrToLong, STRPTR, string, LONG *, value, \
	, DOS_BASE_NAME)

#define SystemTagList(command, tags) \
	LP2(0x194, LONG, SystemTagList, STRPTR, command, struct TagItem *, tags, \
	, DOS_BASE_NAME)

#define System(a0, a1) SystemTagList ((a0), (a1))

#ifndef NO_INLINE_STDARG
#define SystemTags(a0, tags...) \
	({ULONG _tags[] = { tags }; SystemTagList((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define UnGetC(fh, character) \
	LP2(0xd4, LONG, UnGetC, BPTR, fh, long, character, \
	, DOS_BASE_NAME)

#define UnLoadSeg(seglist) \
	LP1NR(0x68, UnLoadSeg, BPTR, seglist, \
	, DOS_BASE_NAME)

#define UnLock(lock) \
	LP1NR(0x3c, UnLock, BPTR, lock, \
	, DOS_BASE_NAME)

#define UnLockDosList(flags) \
	LP1NR(0x1b8, UnLockDosList, unsigned long, flags, \
	, DOS_BASE_NAME)

#define UnLockRecord(fh, offset, length) \
	LP3(0xbc, BOOL, UnLockRecord, BPTR, fh, unsigned long, offset, unsigned long, length, \
	, DOS_BASE_NAME)

#define UnLockRecords(recArray) \
	LP1(0xc0, BOOL, UnLockRecords, struct RecordLock *, recArray, \
	, DOS_BASE_NAME)

#define VFPrintf(fh, format, argarray) \
	LP3(0xec, LONG, VFPrintf, BPTR, fh, STRPTR, format, APTR, argarray, \
	, DOS_BASE_NAME)

#define VFWritef(fh, format, argarray) \
	LP3NR(0xe8, VFWritef, BPTR, fh, STRPTR, format, LONG *, argarray, \
	, DOS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define FWritef(a0, tags...) \
	({ULONG _tags[] = { tags }; VFWritef((a0), (a1), (LONG *)_tags);})
#endif /* !NO_INLINE_STDARG */

#ifndef NO_INLINE_STDARG
#define Printf(a0, tags...) \
	({ULONG _tags[] = { tags }; VPrintf((a0), (APTR)_tags);})
#endif /* !NO_INLINE_STDARG */

#define VPrintf(format, argarray) \
	LP2(0x27c, LONG, VPrintf, STRPTR, format, APTR, argarray, \
	, DOS_BASE_NAME)

#define WaitForChar(file, timeout) \
	LP2(0x88, LONG, WaitForChar, BPTR, file, long, timeout, \
	, DOS_BASE_NAME)

#define WaitPkt() \
	LP0(0xa8, struct DosPacket *, WaitPkt, \
	, DOS_BASE_NAME)

#define Write(file, buffer, length) \
	LP3(0x20, LONG, Write, BPTR, file, APTR, buffer, long, length, \
	, DOS_BASE_NAME)

#define WriteChars(buf, buflen) \
	LP2(0x274, LONG, WriteChars, STRPTR, buf, unsigned long, buflen, \
	, DOS_BASE_NAME)

#endif /* PROTO_DOS_H */
