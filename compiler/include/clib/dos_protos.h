#ifndef CLIB_DOS_PROTOS_H
#define CLIB_DOS_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

/*
    Prototypes
*/
__AROS_LP2(BOOL, AddBuffers,
    __AROS_LPA(STRPTR, devicename, D1),
    __AROS_LPA(LONG,   numbuffers, D2),
    struct DosLibrary *, DOSBase, 122, Dos)
#define AddBuffers(devicename, numbuffers) \
    __AROS_LC2(BOOL, AddBuffers, \
    __AROS_LCA(STRPTR, devicename, D1), \
    __AROS_LCA(LONG,   numbuffers, D2), \
    struct DosLibrary *, DOSBase, 122, Dos)

__AROS_LP1(LONG, AddDosEntry,
    __AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 113, Dos)
#define AddDosEntry(dlist) \
    __AROS_LC1(LONG, AddDosEntry, \
    __AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 113, Dos)

__AROS_LP2(APTR, AllocDosObject,
    __AROS_LPA(ULONG,            type, D1),
    __AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 38, Dos)
#define AllocDosObject(type, tags) \
    __AROS_LC2(APTR, AllocDosObject, \
    __AROS_LCA(ULONG,            type, D1), \
    __AROS_LCA(struct TagItem *, tags, D2), \
    struct DosLibrary *, DOSBase, 38, Dos)

__AROS_LP2(BOOL, AssignLock,
    __AROS_LPA(STRPTR, name, D1),
    __AROS_LPA(BPTR,   lock, D2),
    struct DosLibrary *, DOSBase, 102, Dos)
#define AssignLock(name, lock) \
    __AROS_LC2(BOOL, AssignLock, \
    __AROS_LCA(STRPTR, name, D1), \
    __AROS_LCA(BPTR,   lock, D2), \
    struct DosLibrary *, DOSBase, 102, Dos)

__AROS_LP1(struct DosList *, AttemptLockDosList,
    __AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 111, Dos)
#define AttemptLockDosList(flags) \
    __AROS_LC1(struct DosList *, AttemptLockDosList, \
    __AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 111, Dos)

__AROS_LP3(BOOL, ChangeMode,
    __AROS_LPA(ULONG, type,    D1),
    __AROS_LPA(BPTR,  object,  D2),
    __AROS_LPA(ULONG, newmode, D3),
    struct DosLibrary *, DOSBase, 75, Dos)
#define ChangeMode(type, object, newmode) \
    __AROS_LC3(BOOL, ChangeMode, \
    __AROS_LCA(ULONG, type,    D1), \
    __AROS_LCA(BPTR,  object,  D2), \
    __AROS_LCA(ULONG, newmode, D3), \
    struct DosLibrary *, DOSBase, 75, Dos)

__AROS_LP0(struct CommandLineInterface *, Cli,
    struct DosLibrary *, DOSBase, 82, Dos)
#define Cli() \
    __AROS_LC0(struct CommandLineInterface *, Cli, \
    struct DosLibrary *, DOSBase, 82, Dos)

__AROS_LP1(BOOL, Close,
    __AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 6, Dos)
#define Close(file) \
    __AROS_LC1(BOOL, Close, \
    __AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 6, Dos)

__AROS_LP1(BOOL, UnLock,
    __AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 15, Dos)
#define UnLock(lock) \
    __AROS_LC1(BOOL, UnLock, \
    __AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 15, Dos)

__AROS_LP1(BPTR, CreateDir,
    __AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 20, Dos)
#define CreateDir(name) \
    __AROS_LC1(BPTR, CreateDir, \
    __AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 20, Dos)

__AROS_LP1(struct Process *, CreateNewProc,
    __AROS_LPA(struct TagItem *, tags, D1),
    struct DosLibrary *, DOSBase, 83, Dos)
#define CreateNewProc(tags) \
    __AROS_LC1(struct Process *, CreateNewProc, \
    __AROS_LCA(struct TagItem *, tags, D1), \
    struct DosLibrary *, DOSBase, 83, Dos)

__AROS_LP1(BPTR, CurrentDir,
    __AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 21, Dos)
#define CurrentDir(lock) \
    __AROS_LC1(BPTR, CurrentDir, \
    __AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 21, Dos)

__AROS_LP1(BOOL, DateToStr,
    __AROS_LPA(struct DateTime *, datetime, D1),
    struct DosLibrary *, DOSBase, 124, Dos)
#define DateToStr(datetime) \
    __AROS_LC1(BOOL, DateToStr, \
    __AROS_LCA(struct DateTime *, datetime, D1), \
    struct DosLibrary *, DOSBase, 124, Dos)

__AROS_LP1(void, Delay,
    __AROS_LPA(ULONG, timeout, D1),
    struct DosLibrary *, DOSBase, 33, Dos)
#define Delay(timeout) \
    __AROS_LC1(void, Delay, \
    __AROS_LCA(ULONG, timeout, D1), \
    struct DosLibrary *, DOSBase, 33, Dos)

__AROS_LP1(BOOL, DeleteFile,
    __AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 12, Dos)
#define DeleteFile(name) \
    __AROS_LC1(BOOL, DeleteFile, \
    __AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 12, Dos)

__AROS_LP1(BPTR, DupLock,
    __AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 16, Dos)
#define DupLock(lock) \
    __AROS_LC1(BPTR, DupLock, \
    __AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 16, Dos)

__AROS_LP1(BPTR, DupLockFromFH,
    __AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 62, Dos)
#define DupLockFromFH(fh) \
    __AROS_LC1(BPTR, DupLockFromFH, \
    __AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 62, Dos)

__AROS_LP5(BOOL, ExAll,
    __AROS_LPA(BPTR,                  lock,    D1),
    __AROS_LPA(struct ExAllData *,    buffer,  D2),
    __AROS_LPA(LONG,                  size,    D3),
    __AROS_LPA(LONG,                  data,    D4),
    __AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 72, Dos)
#define ExAll(lock, buffer, size, data, control) \
    __AROS_LC5(BOOL, ExAll, \
    __AROS_LCA(BPTR,                  lock,    D1), \
    __AROS_LCA(struct ExAllData *,    buffer,  D2), \
    __AROS_LCA(LONG,                  size,    D3), \
    __AROS_LCA(LONG,                  data,    D4), \
    __AROS_LCA(struct ExAllControl *, control, D5), \
    struct DosLibrary *, DOSBase, 72, Dos)

__AROS_LP5(void, ExAllEnd,
    __AROS_LPA(BPTR,                  lock,    D1),
    __AROS_LPA(struct ExAllData *,    buffer,  D2),
    __AROS_LPA(LONG,                  size,    D3),
    __AROS_LPA(LONG,                  data,    D4),
    __AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 165, Dos)
#define ExAllEnd(lock, buffer, size, data, control) \
    __AROS_LC5(void, ExAllEnd, \
    __AROS_LCA(BPTR,                  lock,    D1), \
    __AROS_LCA(struct ExAllData *,    buffer,  D2), \
    __AROS_LCA(LONG,                  size,    D3), \
    __AROS_LCA(LONG,                  data,    D4), \
    __AROS_LCA(struct ExAllControl *, control, D5), \
    struct DosLibrary *, DOSBase, 165, Dos)

__AROS_LP2(BOOL, Examine,
    __AROS_LPA(BPTR,                   lock, D1),
    __AROS_LPA(struct FileInfoBlock *, fib,  D2),
    struct DosLibrary *, DOSBase, 17, Dos)
#define Examine(lock, fib) \
    __AROS_LC2(BOOL, Examine, \
    __AROS_LCA(BPTR,                   lock, D1), \
    __AROS_LCA(struct FileInfoBlock *, fib,  D2), \
    struct DosLibrary *, DOSBase, 17, Dos)

__AROS_LP2(BOOL, ExamineFH,
    __AROS_LPA(BPTR                  , fh, D1),
    __AROS_LPA(struct FileInfoBlock *, fib, D2),
    struct DosLibrary *, DOSBase, 65, Dos)
#define ExamineFH(fh, fib) \
    __AROS_LC2(BOOL, ExamineFH, \
    __AROS_LCA(BPTR                  , fh, D1), \
    __AROS_LCA(struct FileInfoBlock *, fib, D2), \
    struct DosLibrary *, DOSBase, 65, Dos)

__AROS_LP1(LONG, FGetC,
    __AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 51, Dos)
#define FGetC(file) \
    __AROS_LC1(LONG, FGetC, \
    __AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 51, Dos)

__AROS_LP1(STRPTR, FilePart,
    __AROS_LPA(STRPTR, path, D1),
    struct DosLibrary *, DOSBase, 145, Dos)
#define FilePart(path) \
    __AROS_LC1(STRPTR, FilePart, \
    __AROS_LCA(STRPTR, path, D1), \
    struct DosLibrary *, DOSBase, 145, Dos)

__AROS_LP2(LONG, FindArg,
    __AROS_LPA(STRPTR, template, D1),
    __AROS_LPA(STRPTR, keyword,  D2),
    struct DosLibrary *, DOSBase, 134, Dos)
#define FindArg(template, keyword) \
    __AROS_LC2(LONG, FindArg, \
    __AROS_LCA(STRPTR, template, D1), \
    __AROS_LCA(STRPTR, keyword,  D2), \
    struct DosLibrary *, DOSBase, 134, Dos)

__AROS_LP3(struct DosList *, FindDosEntry,
    __AROS_LPA(struct DosList *, dlist, D1),
    __AROS_LPA(STRPTR,           name,  D2),
    __AROS_LPA(ULONG,            flags, D3),
    struct DosLibrary *, DOSBase, 114, Dos)
#define FindDosEntry(dlist, name, flags) \
    __AROS_LC3(struct DosList *, FindDosEntry, \
    __AROS_LCA(struct DosList *, dlist, D1), \
    __AROS_LCA(STRPTR,           name,  D2), \
    __AROS_LCA(ULONG,            flags, D3), \
    struct DosLibrary *, DOSBase, 114, Dos)

__AROS_LP1(LONG, Flush,
    __AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 60, Dos)
#define Flush(file) \
    __AROS_LC1(LONG, Flush, \
    __AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 60, Dos)

__AROS_LP3(BOOL, Format,
    __AROS_LPA(STRPTR, devicename, D1),
    __AROS_LPA(STRPTR, volumename, D2),
    __AROS_LPA(ULONG,  dostype,    D3),
    struct DosLibrary *, DOSBase, 119, Dos)
#define Format(devicename, volumename, dostype) \
    __AROS_LC3(BOOL, Format, \
    __AROS_LCA(STRPTR, devicename, D1), \
    __AROS_LCA(STRPTR, volumename, D2), \
    __AROS_LCA(ULONG,  dostype,    D3), \
    struct DosLibrary *, DOSBase, 119, Dos)

__AROS_LP2(LONG, FPutC,
    __AROS_LPA(BPTR, file,      D1),
    __AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 52, Dos)
#define FPutC(file, character) \
    __AROS_LC2(LONG, FPutC, \
    __AROS_LCA(BPTR, file,      D1), \
    __AROS_LCA(LONG, character, D2), \
    struct DosLibrary *, DOSBase, 52, Dos)

__AROS_LP2(LONG, FPuts,
    __AROS_LPA(BPTR,   file,   D1),
    __AROS_LPA(STRPTR, string, D2),
    struct DosLibrary *, DOSBase, 56, Dos)
#define FPuts(file, string) \
    __AROS_LC2(LONG, FPuts, \
    __AROS_LCA(BPTR,   file,   D1), \
    __AROS_LCA(STRPTR, string, D2), \
    struct DosLibrary *, DOSBase, 56, Dos)

__AROS_LP1(void, FreeArgs,
    __AROS_LPA(struct RDArgs *, args, D1),
    struct DosLibrary *, DOSBase, 143, Dos)
#define FreeArgs(args) \
    __AROS_LC1(void, FreeArgs, \
    __AROS_LCA(struct RDArgs *, args, D1), \
    struct DosLibrary *, DOSBase, 143, Dos)

__AROS_LP1(void, FreeDosEntry,
    __AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 117, Dos)
#define FreeDosEntry(dlist) \
    __AROS_LC1(void, FreeDosEntry, \
    __AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 117, Dos)

__AROS_LP2(void, FreeDosObject,
    __AROS_LPA(ULONG, type, D1),
    __AROS_LPA(APTR,  ptr,  D2),
    struct DosLibrary *, DOSBase, 39, Dos)
#define FreeDosObject(type, ptr) \
    __AROS_LC2(void, FreeDosObject, \
    __AROS_LCA(ULONG, type, D1), \
    __AROS_LCA(APTR,  ptr,  D2), \
    struct DosLibrary *, DOSBase, 39, Dos)

__AROS_LP0(STRPTR, GetArgStr,
    struct DosLibrary *, DOSBase, 89, Dos)
#define GetArgStr() \
    __AROS_LC0(STRPTR, GetArgStr, \
    struct DosLibrary *, DOSBase, 89, Dos)

__AROS_LP0(BPTR, Input,
    struct DosLibrary *, DOSBase, 9, Dos)
#define Input() \
    __AROS_LC0(BPTR, Input, \
    struct DosLibrary *, DOSBase, 9, Dos)

__AROS_LP0(LONG, IoErr,
    struct DosLibrary *, DOSBase, 22, Dos)
#define IoErr() \
    __AROS_LC0(LONG, IoErr, \
    struct DosLibrary *, DOSBase, 22, Dos)

__AROS_LP1(BOOL, IsFileSystem,
    __AROS_LPA(STRPTR, devicename, D1),
    struct DosLibrary *, DOSBase, 118, Dos)
#define IsFileSystem(devicename) \
    __AROS_LC1(BOOL, IsFileSystem, \
    __AROS_LCA(STRPTR, devicename, D1), \
    struct DosLibrary *, DOSBase, 118, Dos)

__AROS_LP1(BOOL, IsInteractive,
    __AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 36, Dos)
#define IsInteractive(file) \
    __AROS_LC1(BOOL, IsInteractive, \
    __AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 36, Dos)

__AROS_LP1(BPTR, LoadSeg,
    __AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 25, Dos)
#define LoadSeg(name) \
    __AROS_LC1(BPTR, LoadSeg, \
    __AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 25, Dos)

__AROS_LP2(BPTR, Lock,
    __AROS_LPA(STRPTR, name,       D1),
    __AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 14, Dos)
#define Lock(name, accessMode) \
    __AROS_LC2(BPTR, Lock, \
    __AROS_LCA(STRPTR, name,       D1), \
    __AROS_LCA(LONG,   accessMode, D2), \
    struct DosLibrary *, DOSBase, 14, Dos)

__AROS_LP1(struct DosList *, LockDosList,
    __AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 109, Dos)
#define LockDosList(flags) \
    __AROS_LC1(struct DosList *, LockDosList, \
    __AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 109, Dos)

__AROS_LP2(struct DosList *, MakeDosEntry,
    __AROS_LPA(STRPTR, name, D1),
    __AROS_LPA(LONG,   type, D2),
    struct DosLibrary *, DOSBase, 116, Dos)
#define MakeDosEntry(name, type) \
    __AROS_LC2(struct DosList *, MakeDosEntry, \
    __AROS_LCA(STRPTR, name, D1), \
    __AROS_LCA(LONG,   type, D2), \
    struct DosLibrary *, DOSBase, 116, Dos)

__AROS_LP2(BOOL, MatchPattern,
    __AROS_LPA(STRPTR, pat, D1),
    __AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 141, Dos)
#define MatchPattern(pat, str) \
    __AROS_LC2(BOOL, MatchPattern, \
    __AROS_LCA(STRPTR, pat, D1), \
    __AROS_LCA(STRPTR, str, D2), \
    struct DosLibrary *, DOSBase, 141, Dos)

__AROS_LP2(BOOL, MatchPatternNoCase,
    __AROS_LPA(STRPTR, pat, D1),
    __AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 162, Dos)
#define MatchPatternNoCase(pat, str) \
    __AROS_LC2(BOOL, MatchPatternNoCase, \
    __AROS_LCA(STRPTR, pat, D1), \
    __AROS_LCA(STRPTR, str, D2), \
    struct DosLibrary *, DOSBase, 162, Dos)

__AROS_LP0(BPTR, MaxCli,
    struct DosLibrary *, DOSBase, 92, Dos)
#define MaxCli() \
    __AROS_LC0(BPTR, MaxCli, \
    struct DosLibrary *, DOSBase, 92, Dos)

__AROS_LP3(BOOL, NameFromLock,
    __AROS_LPA(BPTR,   lock,   D1),
    __AROS_LPA(STRPTR, buffer, D2),
    __AROS_LPA(LONG,   length, D3),
    struct DosLibrary *, DOSBase, 67, Dos)
#define NameFromLock(lock, buffer, length) \
    __AROS_LC3(BOOL, NameFromLock, \
    __AROS_LCA(BPTR,   lock,   D1), \
    __AROS_LCA(STRPTR, buffer, D2), \
    __AROS_LCA(LONG,   length, D3), \
    struct DosLibrary *, DOSBase, 67, Dos)

__AROS_LP3(LONG, NameFromFH,
    __AROS_LPA(BPTR  , fh, D1),
    __AROS_LPA(STRPTR, buffer, D2),
    __AROS_LPA(long  , len, D3),
    struct DosLibrary *, DOSBase, 68, Dos)
#define NameFromFH(fh, buffer, len) \
    __AROS_LC3(LONG, NameFromFH, \
    __AROS_LCA(BPTR  , fh, D1), \
    __AROS_LCA(STRPTR, buffer, D2), \
    __AROS_LCA(long  , len, D3), \
    struct DosLibrary *, DOSBase, 68, Dos)

__AROS_LP2(BPTR, Open,
    __AROS_LPA(STRPTR, name,       D1),
    __AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 5, Dos)
#define Open(name, accessMode) \
    __AROS_LC2(BPTR, Open, \
    __AROS_LCA(STRPTR, name,       D1), \
    __AROS_LCA(LONG,   accessMode, D2), \
    struct DosLibrary *, DOSBase, 5, Dos)

__AROS_LP1(BPTR, OpenFromLock,
    __AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 63, Dos)
#define OpenFromLock(lock) \
    __AROS_LC1(BPTR, OpenFromLock, \
    __AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 63, Dos)

__AROS_LP0(BPTR, Output,
    struct DosLibrary *, DOSBase, 10, Dos)
#define Output() \
    __AROS_LC0(BPTR, Output, \
    struct DosLibrary *, DOSBase, 10, Dos)

__AROS_LP3(LONG, ParsePattern,
    __AROS_LPA(STRPTR, Source,      D1),
    __AROS_LPA(STRPTR, Dest,        D2),
    __AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 140, Dos)
#define ParsePattern(Source, Dest, DestLength) \
    __AROS_LC3(LONG, ParsePattern, \
    __AROS_LCA(STRPTR, Source,      D1), \
    __AROS_LCA(STRPTR, Dest,        D2), \
    __AROS_LCA(LONG,   DestLength,  D3), \
    struct DosLibrary *, DOSBase, 140, Dos)

__AROS_LP3(LONG, ParsePatternNoCase,
    __AROS_LPA(STRPTR, Source,      D1),
    __AROS_LPA(STRPTR, Dest,        D2),
    __AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 161, Dos)
#define ParsePatternNoCase(Source, Dest, DestLength) \
    __AROS_LC3(LONG, ParsePatternNoCase, \
    __AROS_LCA(STRPTR, Source,      D1), \
    __AROS_LCA(STRPTR, Dest,        D2), \
    __AROS_LCA(LONG,   DestLength,  D3), \
    struct DosLibrary *, DOSBase, 161, Dos)

__AROS_LP2(BOOL, PrintFault,
    __AROS_LPA(LONG,   code,   D1),
    __AROS_LPA(STRPTR, header, D2),
    struct DosLibrary *, DOSBase, 79, Dos)
#define PrintFault(code, header) \
    __AROS_LC2(BOOL, PrintFault, \
    __AROS_LCA(LONG,   code,   D1), \
    __AROS_LCA(STRPTR, header, D2), \
    struct DosLibrary *, DOSBase, 79, Dos)

__AROS_LP1(LONG, PutStr,
    __AROS_LPA(STRPTR, string, D1),
    struct DosLibrary *, DOSBase, 158, Dos)
#define PutStr(string) \
    __AROS_LC1(LONG, PutStr, \
    __AROS_LCA(STRPTR, string, D1), \
    struct DosLibrary *, DOSBase, 158, Dos)

__AROS_LP3(LONG, Read,
    __AROS_LPA(BPTR, file,   D1),
    __AROS_LPA(APTR, buffer, D2),
    __AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 7, Dos)
#define Read(file, buffer, length) \
    __AROS_LC3(LONG, Read, \
    __AROS_LCA(BPTR, file,   D1), \
    __AROS_LCA(APTR, buffer, D2), \
    __AROS_LCA(LONG, length, D3), \
    struct DosLibrary *, DOSBase, 7, Dos)

__AROS_LP3(struct RDArgs *, ReadArgs,
    __AROS_LPA(STRPTR,          template, D1),
    __AROS_LPA(LONG *,          array,    D2),
    __AROS_LPA(struct RDArgs *, rdargs,   D3),
    struct DosLibrary *, DOSBase, 133, Dos)
#define ReadArgs(template, array, rdargs) \
    __AROS_LC3(struct RDArgs *, ReadArgs, \
    __AROS_LCA(STRPTR,          template, D1), \
    __AROS_LCA(LONG *,          array,    D2), \
    __AROS_LCA(struct RDArgs *, rdargs,   D3), \
    struct DosLibrary *, DOSBase, 133, Dos)

__AROS_LP3(LONG, ReadItem,
    __AROS_LPA(STRPTR,           buffer,   D1),
    __AROS_LPA(LONG,             maxchars, D2),
    __AROS_LPA(struct CSource *, input,    D3),
    struct DosLibrary *, DOSBase, 135, Dos)
#define ReadItem(buffer, maxchars, input) \
    __AROS_LC3(LONG, ReadItem, \
    __AROS_LCA(STRPTR,           buffer,   D1), \
    __AROS_LCA(LONG,             maxchars, D2), \
    __AROS_LCA(struct CSource *, input,    D3), \
    struct DosLibrary *, DOSBase, 135, Dos)

__AROS_LP1(LONG, RemDosEntry,
    __AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 112, Dos)
#define RemDosEntry(dlist) \
    __AROS_LC1(LONG, RemDosEntry, \
    __AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 112, Dos)

__AROS_LP4(LONG, RunCommand,
    __AROS_LPA(BPTR,   segList,   D1),
    __AROS_LPA(ULONG,  stacksize, D2),
    __AROS_LPA(STRPTR, argptr,    D3),
    __AROS_LPA(ULONG,  argsize,   D4),
    struct DosLibrary *, DOSBase, 84, Dos)
#define RunCommand(segList, stacksize, argptr, argsize) \
    __AROS_LC4(LONG, RunCommand, \
    __AROS_LCA(BPTR,   segList,   D1), \
    __AROS_LCA(ULONG,  stacksize, D2), \
    __AROS_LCA(STRPTR, argptr,    D3), \
    __AROS_LCA(ULONG,  argsize,   D4), \
    struct DosLibrary *, DOSBase, 84, Dos)

__AROS_LP3(LONG, Seek,
    __AROS_LPA(BPTR, file,     D1),
    __AROS_LPA(LONG, position, D2),
    __AROS_LPA(LONG, mode,     D3),
    struct DosLibrary *, DOSBase, 11, Dos)
#define Seek(file, position, mode) \
    __AROS_LC3(LONG, Seek, \
    __AROS_LCA(BPTR, file,     D1), \
    __AROS_LCA(LONG, position, D2), \
    __AROS_LCA(LONG, mode,     D3), \
    struct DosLibrary *, DOSBase, 11, Dos)

__AROS_LP1(BPTR, SelectInput,
    __AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 49, Dos)
#define SelectInput(fh) \
    __AROS_LC1(BPTR, SelectInput, \
    __AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 49, Dos)

__AROS_LP1(BPTR, SelectOutput,
    __AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 50, Dos)
#define SelectOutput(fh) \
    __AROS_LC1(BPTR, SelectOutput, \
    __AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 50, Dos)

__AROS_LP2(BOOL, SetComment,
    __AROS_LPA(STRPTR, name,    D1),
    __AROS_LPA(STRPTR, comment, D2),
    struct DosLibrary *, DOSBase, 30, Dos)
#define SetComment(name, comment) \
    __AROS_LC2(BOOL, SetComment, \
    __AROS_LCA(STRPTR, name,    D1), \
    __AROS_LCA(STRPTR, comment, D2), \
    struct DosLibrary *, DOSBase, 30, Dos)

__AROS_LP2(BOOL, SetFileDate,
    __AROS_LPA(STRPTR,             name, D1),
    __AROS_LPA(struct DateStamp *, date, D2),
    struct DosLibrary *, DOSBase, 66, Dos)
#define SetFileDate(name, date) \
    __AROS_LC2(BOOL, SetFileDate, \
    __AROS_LCA(STRPTR,             name, D1), \
    __AROS_LCA(struct DateStamp *, date, D2), \
    struct DosLibrary *, DOSBase, 66, Dos)

__AROS_LP3(LONG, SetFileSize,
    __AROS_LPA(BPTR, file,   D1),
    __AROS_LPA(LONG, offset, D2),
    __AROS_LPA(LONG, mode,   D3),
    struct DosLibrary *, DOSBase, 76, Dos)
#define SetFileSize(file, offset, mode) \
    __AROS_LC3(LONG, SetFileSize, \
    __AROS_LCA(BPTR, file,   D1), \
    __AROS_LCA(LONG, offset, D2), \
    __AROS_LCA(LONG, mode,   D3), \
    struct DosLibrary *, DOSBase, 76, Dos)

__AROS_LP1(LONG, SetIoErr,
    __AROS_LPA(LONG, result, D1),
    struct DosLibrary *, DOSBase, 77, Dos)
#define SetIoErr(result) \
    __AROS_LC1(LONG, SetIoErr, \
    __AROS_LCA(LONG, result, D1), \
    struct DosLibrary *, DOSBase, 77, Dos)

__AROS_LP2(BOOL, SetOwner,
    __AROS_LPA(STRPTR, name,       D1),
    __AROS_LPA(ULONG,  owner_info, D2),
    struct DosLibrary *, DOSBase, 166, Dos)
#define SetOwner(name, owner_info) \
    __AROS_LC2(BOOL, SetOwner, \
    __AROS_LCA(STRPTR, name,       D1), \
    __AROS_LCA(ULONG,  owner_info, D2), \
    struct DosLibrary *, DOSBase, 166, Dos)

__AROS_LP2(BOOL, SetProtection,
    __AROS_LPA(STRPTR, name,    D1),
    __AROS_LPA(ULONG,  protect, D2),
    struct DosLibrary *, DOSBase, 31, Dos)
#define SetProtection(name, protect) \
    __AROS_LC2(BOOL, SetProtection, \
    __AROS_LCA(STRPTR, name,    D1), \
    __AROS_LCA(ULONG,  protect, D2), \
    struct DosLibrary *, DOSBase, 31, Dos)

__AROS_LP2I(LONG, StrToLong,
    __AROS_LPA(STRPTR, string, D1),
    __AROS_LPA(LONG *, value,  D2),
    struct DosLibrary *, DOSBase, 136, Dos)
#define StrToLong(string, value) \
    __AROS_LC2I(LONG, StrToLong, \
    __AROS_LCA(STRPTR, string, D1), \
    __AROS_LCA(LONG *, value,  D2), \
    struct DosLibrary *, DOSBase, 136, Dos)

__AROS_LP2(LONG, UnGetC,
    __AROS_LPA(BPTR, file,      D1),
    __AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 53, Dos)
#define UnGetC(file, character) \
    __AROS_LC2(LONG, UnGetC, \
    __AROS_LCA(BPTR, file,      D1), \
    __AROS_LCA(LONG, character, D2), \
    struct DosLibrary *, DOSBase, 53, Dos)

__AROS_LP1(void, UnLoadSeg,
    __AROS_LPA(BPTR, seglist, D1),
    struct DosLibrary *, DOSBase, 26, Dos)
#define UnLoadSeg(seglist) \
    __AROS_LC1(void, UnLoadSeg, \
    __AROS_LCA(BPTR, seglist, D1), \
    struct DosLibrary *, DOSBase, 26, Dos)

__AROS_LP1(void, UnLockDosList,
    __AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 110, Dos)
#define UnLockDosList(flags) \
    __AROS_LC1(void, UnLockDosList, \
    __AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 110, Dos)

__AROS_LP3(LONG, VFPrintf,
    __AROS_LPA(BPTR,   file,     D1),
    __AROS_LPA(STRPTR, format,   D2),
    __AROS_LPA(LONG *, argarray, D3),
    struct DosLibrary *, DOSBase, 59, Dos)
#define VFPrintf(file, format, argarray) \
    __AROS_LC3(LONG, VFPrintf, \
    __AROS_LCA(BPTR,   file,     D1), \
    __AROS_LCA(STRPTR, format,   D2), \
    __AROS_LCA(LONG *, argarray, D3), \
    struct DosLibrary *, DOSBase, 59, Dos)

__AROS_LP2(LONG, VPrintf,
    __AROS_LPA(STRPTR, format,   D1),
    __AROS_LPA(LONG *, argarray, D2),
    struct DosLibrary *, DOSBase, 159, Dos)
#define VPrintf(format, argarray) \
    __AROS_LC2(LONG, VPrintf, \
    __AROS_LCA(STRPTR, format,   D1), \
    __AROS_LCA(LONG *, argarray, D2), \
    struct DosLibrary *, DOSBase, 159, Dos)

__AROS_LP3(LONG, Write,
    __AROS_LPA(BPTR, file,   D1),
    __AROS_LPA(APTR, buffer, D2),
    __AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 8, Dos)
#define Write(file, buffer, length) \
    __AROS_LC3(LONG, Write, \
    __AROS_LCA(BPTR, file,   D1), \
    __AROS_LCA(APTR, buffer, D2), \
    __AROS_LCA(LONG, length, D3), \
    struct DosLibrary *, DOSBase, 8, Dos)


#endif /* CLIB_DOS_PROTOS_H */
