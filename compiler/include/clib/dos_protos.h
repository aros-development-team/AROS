#ifndef CLIB_DOS_PROTOS_H
#define CLIB_DOS_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#ifndef DOSBase
extern struct DosLibrary *DOSBase;
#endif

/*
    Prototypes
*/
AROS_LP2(BOOL, AddBuffers,
    AROS_LPA(STRPTR, devicename, D1),
    AROS_LPA(LONG,   numbuffers, D2),
    struct DosLibrary *, DOSBase, 122, Dos)
#define AddBuffers(devicename, numbuffers) \
    AROS_LC2(BOOL, AddBuffers, \
    AROS_LCA(STRPTR, devicename, D1), \
    AROS_LCA(LONG,   numbuffers, D2), \
    struct DosLibrary *, DOSBase, 122, Dos)

AROS_LP1(LONG, AddDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 113, Dos)
#define AddDosEntry(dlist) \
    AROS_LC1(LONG, AddDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 113, Dos)

AROS_LP2(APTR, AllocDosObject,
    AROS_LPA(ULONG,            type, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 38, Dos)
#define AllocDosObject(type, tags) \
    AROS_LC2(APTR, AllocDosObject, \
    AROS_LCA(ULONG,            type, D1), \
    AROS_LCA(struct TagItem *, tags, D2), \
    struct DosLibrary *, DOSBase, 38, Dos)

AROS_LP2(BOOL, AssignLock,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR,   lock, D2),
    struct DosLibrary *, DOSBase, 102, Dos)
#define AssignLock(name, lock) \
    AROS_LC2(BOOL, AssignLock, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(BPTR,   lock, D2), \
    struct DosLibrary *, DOSBase, 102, Dos)

AROS_LP1(struct DosList *, AttemptLockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 111, Dos)
#define AttemptLockDosList(flags) \
    AROS_LC1(struct DosList *, AttemptLockDosList, \
    AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 111, Dos)

AROS_LP3(BOOL, ChangeMode,
    AROS_LPA(ULONG, type,    D1),
    AROS_LPA(BPTR,  object,  D2),
    AROS_LPA(ULONG, newmode, D3),
    struct DosLibrary *, DOSBase, 75, Dos)
#define ChangeMode(type, object, newmode) \
    AROS_LC3(BOOL, ChangeMode, \
    AROS_LCA(ULONG, type,    D1), \
    AROS_LCA(BPTR,  object,  D2), \
    AROS_LCA(ULONG, newmode, D3), \
    struct DosLibrary *, DOSBase, 75, Dos)

AROS_LP0(struct CommandLineInterface *, Cli,
    struct DosLibrary *, DOSBase, 82, Dos)
#define Cli() \
    AROS_LC0(struct CommandLineInterface *, Cli, \
    struct DosLibrary *, DOSBase, 82, Dos)

AROS_LP1(BOOL, Close,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 6, Dos)
#define Close(file) \
    AROS_LC1(BOOL, Close, \
    AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 6, Dos)

AROS_LP1(BOOL, UnLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 15, Dos)
#define UnLock(lock) \
    AROS_LC1(BOOL, UnLock, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 15, Dos)

AROS_LP1(BPTR, CreateDir,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 20, Dos)
#define CreateDir(name) \
    AROS_LC1(BPTR, CreateDir, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 20, Dos)

AROS_LP1(struct Process *, CreateNewProc,
    AROS_LPA(struct TagItem *, tags, D1),
    struct DosLibrary *, DOSBase, 83, Dos)
#define CreateNewProc(tags) \
    AROS_LC1(struct Process *, CreateNewProc, \
    AROS_LCA(struct TagItem *, tags, D1), \
    struct DosLibrary *, DOSBase, 83, Dos)

AROS_LP1(BPTR, CurrentDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 21, Dos)
#define CurrentDir(lock) \
    AROS_LC1(BPTR, CurrentDir, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 21, Dos)

AROS_LP1(BOOL, DateToStr,
    AROS_LPA(struct DateTime *, datetime, D1),
    struct DosLibrary *, DOSBase, 124, Dos)
#define DateToStr(datetime) \
    AROS_LC1(BOOL, DateToStr, \
    AROS_LCA(struct DateTime *, datetime, D1), \
    struct DosLibrary *, DOSBase, 124, Dos)

AROS_LP1(void, Delay,
    AROS_LPA(ULONG, timeout, D1),
    struct DosLibrary *, DOSBase, 33, Dos)
#define Delay(timeout) \
    AROS_LC1(void, Delay, \
    AROS_LCA(ULONG, timeout, D1), \
    struct DosLibrary *, DOSBase, 33, Dos)

AROS_LP1(BOOL, DeleteFile,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 12, Dos)
#define DeleteFile(name) \
    AROS_LC1(BOOL, DeleteFile, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 12, Dos)

AROS_LP1(BPTR, DupLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 16, Dos)
#define DupLock(lock) \
    AROS_LC1(BPTR, DupLock, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 16, Dos)

AROS_LP1(BPTR, DupLockFromFH,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 62, Dos)
#define DupLockFromFH(fh) \
    AROS_LC1(BPTR, DupLockFromFH, \
    AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 62, Dos)

AROS_LP5(BOOL, ExAll,
    AROS_LPA(BPTR,                  lock,    D1),
    AROS_LPA(struct ExAllData *,    buffer,  D2),
    AROS_LPA(LONG,                  size,    D3),
    AROS_LPA(LONG,                  data,    D4),
    AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 72, Dos)
#define ExAll(lock, buffer, size, data, control) \
    AROS_LC5(BOOL, ExAll, \
    AROS_LCA(BPTR,                  lock,    D1), \
    AROS_LCA(struct ExAllData *,    buffer,  D2), \
    AROS_LCA(LONG,                  size,    D3), \
    AROS_LCA(LONG,                  data,    D4), \
    AROS_LCA(struct ExAllControl *, control, D5), \
    struct DosLibrary *, DOSBase, 72, Dos)

AROS_LP5(void, ExAllEnd,
    AROS_LPA(BPTR,                  lock,    D1),
    AROS_LPA(struct ExAllData *,    buffer,  D2),
    AROS_LPA(LONG,                  size,    D3),
    AROS_LPA(LONG,                  data,    D4),
    AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 165, Dos)
#define ExAllEnd(lock, buffer, size, data, control) \
    AROS_LC5(void, ExAllEnd, \
    AROS_LCA(BPTR,                  lock,    D1), \
    AROS_LCA(struct ExAllData *,    buffer,  D2), \
    AROS_LCA(LONG,                  size,    D3), \
    AROS_LCA(LONG,                  data,    D4), \
    AROS_LCA(struct ExAllControl *, control, D5), \
    struct DosLibrary *, DOSBase, 165, Dos)

AROS_LP2(BOOL, Examine,
    AROS_LPA(BPTR,                   lock, D1),
    AROS_LPA(struct FileInfoBlock *, fib,  D2),
    struct DosLibrary *, DOSBase, 17, Dos)
#define Examine(lock, fib) \
    AROS_LC2(BOOL, Examine, \
    AROS_LCA(BPTR,                   lock, D1), \
    AROS_LCA(struct FileInfoBlock *, fib,  D2), \
    struct DosLibrary *, DOSBase, 17, Dos)

AROS_LP2(BOOL, ExamineFH,
    AROS_LPA(BPTR                  , fh, D1),
    AROS_LPA(struct FileInfoBlock *, fib, D2),
    struct DosLibrary *, DOSBase, 65, Dos)
#define ExamineFH(fh, fib) \
    AROS_LC2(BOOL, ExamineFH, \
    AROS_LCA(BPTR                  , fh, D1), \
    AROS_LCA(struct FileInfoBlock *, fib, D2), \
    struct DosLibrary *, DOSBase, 65, Dos)

AROS_LP1(LONG, FGetC,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 51, Dos)
#define FGetC(file) \
    AROS_LC1(LONG, FGetC, \
    AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 51, Dos)

AROS_LP1(STRPTR, FilePart,
    AROS_LPA(STRPTR, path, D1),
    struct DosLibrary *, DOSBase, 145, Dos)
#define FilePart(path) \
    AROS_LC1(STRPTR, FilePart, \
    AROS_LCA(STRPTR, path, D1), \
    struct DosLibrary *, DOSBase, 145, Dos)

AROS_LP2(LONG, FindArg,
    AROS_LPA(STRPTR, template, D1),
    AROS_LPA(STRPTR, keyword,  D2),
    struct DosLibrary *, DOSBase, 134, Dos)
#define FindArg(template, keyword) \
    AROS_LC2(LONG, FindArg, \
    AROS_LCA(STRPTR, template, D1), \
    AROS_LCA(STRPTR, keyword,  D2), \
    struct DosLibrary *, DOSBase, 134, Dos)

AROS_LP3(struct DosList *, FindDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    AROS_LPA(STRPTR,           name,  D2),
    AROS_LPA(ULONG,            flags, D3),
    struct DosLibrary *, DOSBase, 114, Dos)
#define FindDosEntry(dlist, name, flags) \
    AROS_LC3(struct DosList *, FindDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    AROS_LCA(STRPTR,           name,  D2), \
    AROS_LCA(ULONG,            flags, D3), \
    struct DosLibrary *, DOSBase, 114, Dos)

AROS_LP1(LONG, Flush,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 60, Dos)
#define Flush(file) \
    AROS_LC1(LONG, Flush, \
    AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 60, Dos)

AROS_LP3(BOOL, Format,
    AROS_LPA(STRPTR, devicename, D1),
    AROS_LPA(STRPTR, volumename, D2),
    AROS_LPA(ULONG,  dostype,    D3),
    struct DosLibrary *, DOSBase, 119, Dos)
#define Format(devicename, volumename, dostype) \
    AROS_LC3(BOOL, Format, \
    AROS_LCA(STRPTR, devicename, D1), \
    AROS_LCA(STRPTR, volumename, D2), \
    AROS_LCA(ULONG,  dostype,    D3), \
    struct DosLibrary *, DOSBase, 119, Dos)

AROS_LP2(LONG, FPutC,
    AROS_LPA(BPTR, file,      D1),
    AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 52, Dos)
#define FPutC(file, character) \
    AROS_LC2(LONG, FPutC, \
    AROS_LCA(BPTR, file,      D1), \
    AROS_LCA(LONG, character, D2), \
    struct DosLibrary *, DOSBase, 52, Dos)

AROS_LP2(LONG, FPuts,
    AROS_LPA(BPTR,   file,   D1),
    AROS_LPA(STRPTR, string, D2),
    struct DosLibrary *, DOSBase, 56, Dos)
#define FPuts(file, string) \
    AROS_LC2(LONG, FPuts, \
    AROS_LCA(BPTR,   file,   D1), \
    AROS_LCA(STRPTR, string, D2), \
    struct DosLibrary *, DOSBase, 56, Dos)

AROS_LP4(LONG, FRead,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(APTR , block, D2),
    AROS_LPA(ULONG, blocklen, D3),
    AROS_LPA(ULONG, number, D4),
    struct DosLibrary *, DOSBase, 54, Dos)
#define FRead(fh, block, blocklen, number) \
    AROS_LC4(LONG, FRead, \
    AROS_LCA(BPTR , fh, D1), \
    AROS_LCA(APTR , block, D2), \
    AROS_LCA(ULONG, blocklen, D3), \
    AROS_LCA(ULONG, number, D4), \
    struct DosLibrary *, DOSBase, 54, Dos)

AROS_LP1(void, FreeArgs,
    AROS_LPA(struct RDArgs *, args, D1),
    struct DosLibrary *, DOSBase, 143, Dos)
#define FreeArgs(args) \
    AROS_LC1(void, FreeArgs, \
    AROS_LCA(struct RDArgs *, args, D1), \
    struct DosLibrary *, DOSBase, 143, Dos)

AROS_LP1(void, FreeDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 117, Dos)
#define FreeDosEntry(dlist) \
    AROS_LC1(void, FreeDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 117, Dos)

AROS_LP2(void, FreeDosObject,
    AROS_LPA(ULONG, type, D1),
    AROS_LPA(APTR,  ptr,  D2),
    struct DosLibrary *, DOSBase, 39, Dos)
#define FreeDosObject(type, ptr) \
    AROS_LC2(void, FreeDosObject, \
    AROS_LCA(ULONG, type, D1), \
    AROS_LCA(APTR,  ptr,  D2), \
    struct DosLibrary *, DOSBase, 39, Dos)

AROS_LP4(LONG, FWrite,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(APTR , block, D2),
    AROS_LPA(ULONG, blocklen, D3),
    AROS_LPA(ULONG, number, D4),
    struct DosLibrary *, DOSBase, 55, Dos)
#define FWrite(fh, block, blocklen, number) \
    AROS_LC4(LONG, FWrite, \
    AROS_LCA(BPTR , fh, D1), \
    AROS_LCA(APTR , block, D2), \
    AROS_LCA(ULONG, blocklen, D3), \
    AROS_LCA(ULONG, number, D4), \
    struct DosLibrary *, DOSBase, 55, Dos)

AROS_LP0(STRPTR, GetArgStr,
    struct DosLibrary *, DOSBase, 89, Dos)
#define GetArgStr() \
    AROS_LC0(STRPTR, GetArgStr, \
    struct DosLibrary *, DOSBase, 89, Dos)

AROS_LP0(BPTR, Input,
    struct DosLibrary *, DOSBase, 9, Dos)
#define Input() \
    AROS_LC0(BPTR, Input, \
    struct DosLibrary *, DOSBase, 9, Dos)

AROS_LP0(LONG, IoErr,
    struct DosLibrary *, DOSBase, 22, Dos)
#define IoErr() \
    AROS_LC0(LONG, IoErr, \
    struct DosLibrary *, DOSBase, 22, Dos)

AROS_LP1(BOOL, IsFileSystem,
    AROS_LPA(STRPTR, devicename, D1),
    struct DosLibrary *, DOSBase, 118, Dos)
#define IsFileSystem(devicename) \
    AROS_LC1(BOOL, IsFileSystem, \
    AROS_LCA(STRPTR, devicename, D1), \
    struct DosLibrary *, DOSBase, 118, Dos)

AROS_LP1(BOOL, IsInteractive,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 36, Dos)
#define IsInteractive(file) \
    AROS_LC1(BOOL, IsInteractive, \
    AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 36, Dos)

AROS_LP1(BPTR, LoadSeg,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 25, Dos)
#define LoadSeg(name) \
    AROS_LC1(BPTR, LoadSeg, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 25, Dos)

AROS_LP2(BPTR, Lock,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 14, Dos)
#define Lock(name, accessMode) \
    AROS_LC2(BPTR, Lock, \
    AROS_LCA(STRPTR, name,       D1), \
    AROS_LCA(LONG,   accessMode, D2), \
    struct DosLibrary *, DOSBase, 14, Dos)

AROS_LP1(struct DosList *, LockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 109, Dos)
#define LockDosList(flags) \
    AROS_LC1(struct DosList *, LockDosList, \
    AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 109, Dos)

AROS_LP2(struct DosList *, MakeDosEntry,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG,   type, D2),
    struct DosLibrary *, DOSBase, 116, Dos)
#define MakeDosEntry(name, type) \
    AROS_LC2(struct DosList *, MakeDosEntry, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(LONG,   type, D2), \
    struct DosLibrary *, DOSBase, 116, Dos)

AROS_LP2(BOOL, MatchPattern,
    AROS_LPA(STRPTR, pat, D1),
    AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 141, Dos)
#define MatchPattern(pat, str) \
    AROS_LC2(BOOL, MatchPattern, \
    AROS_LCA(STRPTR, pat, D1), \
    AROS_LCA(STRPTR, str, D2), \
    struct DosLibrary *, DOSBase, 141, Dos)

AROS_LP2(BOOL, MatchPatternNoCase,
    AROS_LPA(STRPTR, pat, D1),
    AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 162, Dos)
#define MatchPatternNoCase(pat, str) \
    AROS_LC2(BOOL, MatchPatternNoCase, \
    AROS_LCA(STRPTR, pat, D1), \
    AROS_LCA(STRPTR, str, D2), \
    struct DosLibrary *, DOSBase, 162, Dos)

AROS_LP0(ULONG, MaxCli,
    struct DosLibrary *, DOSBase, 92, Dos)
#define MaxCli() \
    AROS_LC0(ULONG, MaxCli, \
    struct DosLibrary *, DOSBase, 92, Dos)

AROS_LP3(BOOL, NameFromLock,
    AROS_LPA(BPTR,   lock,   D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG,   length, D3),
    struct DosLibrary *, DOSBase, 67, Dos)
#define NameFromLock(lock, buffer, length) \
    AROS_LC3(BOOL, NameFromLock, \
    AROS_LCA(BPTR,   lock,   D1), \
    AROS_LCA(STRPTR, buffer, D2), \
    AROS_LCA(LONG,   length, D3), \
    struct DosLibrary *, DOSBase, 67, Dos)

AROS_LP3(LONG, NameFromFH,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , len, D3),
    struct DosLibrary *, DOSBase, 68, Dos)
#define NameFromFH(fh, buffer, len) \
    AROS_LC3(LONG, NameFromFH, \
    AROS_LCA(BPTR  , fh, D1), \
    AROS_LCA(STRPTR, buffer, D2), \
    AROS_LCA(LONG  , len, D3), \
    struct DosLibrary *, DOSBase, 68, Dos)

AROS_LP2(BPTR, Open,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 5, Dos)
#define Open(name, accessMode) \
    AROS_LC2(BPTR, Open, \
    AROS_LCA(STRPTR, name,       D1), \
    AROS_LCA(LONG,   accessMode, D2), \
    struct DosLibrary *, DOSBase, 5, Dos)

AROS_LP1(BPTR, OpenFromLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 63, Dos)
#define OpenFromLock(lock) \
    AROS_LC1(BPTR, OpenFromLock, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 63, Dos)

AROS_LP0(BPTR, Output,
    struct DosLibrary *, DOSBase, 10, Dos)
#define Output() \
    AROS_LC0(BPTR, Output, \
    struct DosLibrary *, DOSBase, 10, Dos)

AROS_LP3(LONG, ParsePattern,
    AROS_LPA(STRPTR, Source,      D1),
    AROS_LPA(STRPTR, Dest,        D2),
    AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 140, Dos)
#define ParsePattern(Source, Dest, DestLength) \
    AROS_LC3(LONG, ParsePattern, \
    AROS_LCA(STRPTR, Source,      D1), \
    AROS_LCA(STRPTR, Dest,        D2), \
    AROS_LCA(LONG,   DestLength,  D3), \
    struct DosLibrary *, DOSBase, 140, Dos)

AROS_LP3(LONG, ParsePatternNoCase,
    AROS_LPA(STRPTR, Source,      D1),
    AROS_LPA(STRPTR, Dest,        D2),
    AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 161, Dos)
#define ParsePatternNoCase(Source, Dest, DestLength) \
    AROS_LC3(LONG, ParsePatternNoCase, \
    AROS_LCA(STRPTR, Source,      D1), \
    AROS_LCA(STRPTR, Dest,        D2), \
    AROS_LCA(LONG,   DestLength,  D3), \
    struct DosLibrary *, DOSBase, 161, Dos)

AROS_LP1(STRPTR, PathPart,
    AROS_LPA(STRPTR, path, D1),
    struct DosLibrary *, DOSBase, 146, Dos)
#define PathPart(path) \
    AROS_LC1(STRPTR, PathPart, \
    AROS_LCA(STRPTR, path, D1), \
    struct DosLibrary *, DOSBase, 146, Dos)

AROS_LP2(BOOL, PrintFault,
    AROS_LPA(LONG,   code,   D1),
    AROS_LPA(STRPTR, header, D2),
    struct DosLibrary *, DOSBase, 79, Dos)
#define PrintFault(code, header) \
    AROS_LC2(BOOL, PrintFault, \
    AROS_LCA(LONG,   code,   D1), \
    AROS_LCA(STRPTR, header, D2), \
    struct DosLibrary *, DOSBase, 79, Dos)

AROS_LP1(LONG, PutStr,
    AROS_LPA(STRPTR, string, D1),
    struct DosLibrary *, DOSBase, 158, Dos)
#define PutStr(string) \
    AROS_LC1(LONG, PutStr, \
    AROS_LCA(STRPTR, string, D1), \
    struct DosLibrary *, DOSBase, 158, Dos)

AROS_LP3(LONG, Read,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(APTR, buffer, D2),
    AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 7, Dos)
#define Read(file, buffer, length) \
    AROS_LC3(LONG, Read, \
    AROS_LCA(BPTR, file,   D1), \
    AROS_LCA(APTR, buffer, D2), \
    AROS_LCA(LONG, length, D3), \
    struct DosLibrary *, DOSBase, 7, Dos)

AROS_LP3(struct RDArgs *, ReadArgs,
    AROS_LPA(STRPTR,          template, D1),
    AROS_LPA(IPTR *,          array,    D2),
    AROS_LPA(struct RDArgs *, rdargs,   D3),
    struct DosLibrary *, DOSBase, 133, Dos)
#define ReadArgs(template, array, rdargs) \
    AROS_LC3(struct RDArgs *, ReadArgs, \
    AROS_LCA(STRPTR,          template, D1), \
    AROS_LCA(IPTR *,          array,    D2), \
    AROS_LCA(struct RDArgs *, rdargs,   D3), \
    struct DosLibrary *, DOSBase, 133, Dos)

AROS_LP3(LONG, ReadItem,
    AROS_LPA(STRPTR,           buffer,   D1),
    AROS_LPA(LONG,             maxchars, D2),
    AROS_LPA(struct CSource *, input,    D3),
    struct DosLibrary *, DOSBase, 135, Dos)
#define ReadItem(buffer, maxchars, input) \
    AROS_LC3(LONG, ReadItem, \
    AROS_LCA(STRPTR,           buffer,   D1), \
    AROS_LCA(LONG,             maxchars, D2), \
    AROS_LCA(struct CSource *, input,    D3), \
    struct DosLibrary *, DOSBase, 135, Dos)

AROS_LP1(LONG, RemDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 112, Dos)
#define RemDosEntry(dlist) \
    AROS_LC1(LONG, RemDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 112, Dos)

AROS_LP4(LONG, RunCommand,
    AROS_LPA(BPTR,   segList,   D1),
    AROS_LPA(ULONG,  stacksize, D2),
    AROS_LPA(STRPTR, argptr,    D3),
    AROS_LPA(ULONG,  argsize,   D4),
    struct DosLibrary *, DOSBase, 84, Dos)
#define RunCommand(segList, stacksize, argptr, argsize) \
    AROS_LC4(LONG, RunCommand, \
    AROS_LCA(BPTR,   segList,   D1), \
    AROS_LCA(ULONG,  stacksize, D2), \
    AROS_LCA(STRPTR, argptr,    D3), \
    AROS_LCA(ULONG,  argsize,   D4), \
    struct DosLibrary *, DOSBase, 84, Dos)

AROS_LP3(LONG, Seek,
    AROS_LPA(BPTR, file,     D1),
    AROS_LPA(LONG, position, D2),
    AROS_LPA(LONG, mode,     D3),
    struct DosLibrary *, DOSBase, 11, Dos)
#define Seek(file, position, mode) \
    AROS_LC3(LONG, Seek, \
    AROS_LCA(BPTR, file,     D1), \
    AROS_LCA(LONG, position, D2), \
    AROS_LCA(LONG, mode,     D3), \
    struct DosLibrary *, DOSBase, 11, Dos)

AROS_LP1(BPTR, SelectInput,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 49, Dos)
#define SelectInput(fh) \
    AROS_LC1(BPTR, SelectInput, \
    AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 49, Dos)

AROS_LP1(BPTR, SelectOutput,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 50, Dos)
#define SelectOutput(fh) \
    AROS_LC1(BPTR, SelectOutput, \
    AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 50, Dos)

AROS_LP2(BOOL, SetComment,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(STRPTR, comment, D2),
    struct DosLibrary *, DOSBase, 30, Dos)
#define SetComment(name, comment) \
    AROS_LC2(BOOL, SetComment, \
    AROS_LCA(STRPTR, name,    D1), \
    AROS_LCA(STRPTR, comment, D2), \
    struct DosLibrary *, DOSBase, 30, Dos)

AROS_LP2(BOOL, SetFileDate,
    AROS_LPA(STRPTR,             name, D1),
    AROS_LPA(struct DateStamp *, date, D2),
    struct DosLibrary *, DOSBase, 66, Dos)
#define SetFileDate(name, date) \
    AROS_LC2(BOOL, SetFileDate, \
    AROS_LCA(STRPTR,             name, D1), \
    AROS_LCA(struct DateStamp *, date, D2), \
    struct DosLibrary *, DOSBase, 66, Dos)

AROS_LP3(LONG, SetFileSize,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(LONG, offset, D2),
    AROS_LPA(LONG, mode,   D3),
    struct DosLibrary *, DOSBase, 76, Dos)
#define SetFileSize(file, offset, mode) \
    AROS_LC3(LONG, SetFileSize, \
    AROS_LCA(BPTR, file,   D1), \
    AROS_LCA(LONG, offset, D2), \
    AROS_LCA(LONG, mode,   D3), \
    struct DosLibrary *, DOSBase, 76, Dos)

AROS_LP1(LONG, SetIoErr,
    AROS_LPA(LONG, result, D1),
    struct DosLibrary *, DOSBase, 77, Dos)
#define SetIoErr(result) \
    AROS_LC1(LONG, SetIoErr, \
    AROS_LCA(LONG, result, D1), \
    struct DosLibrary *, DOSBase, 77, Dos)

AROS_LP2(BOOL, SetOwner,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(ULONG,  owner_info, D2),
    struct DosLibrary *, DOSBase, 166, Dos)
#define SetOwner(name, owner_info) \
    AROS_LC2(BOOL, SetOwner, \
    AROS_LCA(STRPTR, name,       D1), \
    AROS_LCA(ULONG,  owner_info, D2), \
    struct DosLibrary *, DOSBase, 166, Dos)

AROS_LP2(BOOL, SetProtection,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(ULONG,  protect, D2),
    struct DosLibrary *, DOSBase, 31, Dos)
#define SetProtection(name, protect) \
    AROS_LC2(BOOL, SetProtection, \
    AROS_LCA(STRPTR, name,    D1), \
    AROS_LCA(ULONG,  protect, D2), \
    struct DosLibrary *, DOSBase, 31, Dos)

AROS_LP2I(LONG, StrToLong,
    AROS_LPA(STRPTR, string, D1),
    AROS_LPA(LONG *, value,  D2),
    struct DosLibrary *, DOSBase, 136, Dos)
#define StrToLong(string, value) \
    AROS_LC2I(LONG, StrToLong, \
    AROS_LCA(STRPTR, string, D1), \
    AROS_LCA(LONG *, value,  D2), \
    struct DosLibrary *, DOSBase, 136, Dos)

AROS_LP2(LONG, UnGetC,
    AROS_LPA(BPTR, file,      D1),
    AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 53, Dos)
#define UnGetC(file, character) \
    AROS_LC2(LONG, UnGetC, \
    AROS_LCA(BPTR, file,      D1), \
    AROS_LCA(LONG, character, D2), \
    struct DosLibrary *, DOSBase, 53, Dos)

AROS_LP1(void, UnLoadSeg,
    AROS_LPA(BPTR, seglist, D1),
    struct DosLibrary *, DOSBase, 26, Dos)
#define UnLoadSeg(seglist) \
    AROS_LC1(void, UnLoadSeg, \
    AROS_LCA(BPTR, seglist, D1), \
    struct DosLibrary *, DOSBase, 26, Dos)

AROS_LP1(void, UnLockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 110, Dos)
#define UnLockDosList(flags) \
    AROS_LC1(void, UnLockDosList, \
    AROS_LCA(ULONG, flags, D1), \
    struct DosLibrary *, DOSBase, 110, Dos)

AROS_LP3(LONG, VFPrintf,
    AROS_LPA(BPTR,   file,     D1),
    AROS_LPA(STRPTR, format,   D2),
    AROS_LPA(IPTR *, argarray, D3),
    struct DosLibrary *, DOSBase, 59, Dos)
#define VFPrintf(file, format, argarray) \
    AROS_LC3(LONG, VFPrintf, \
    AROS_LCA(BPTR,   file,     D1), \
    AROS_LCA(STRPTR, format,   D2), \
    AROS_LCA(IPTR *, argarray, D3), \
    struct DosLibrary *, DOSBase, 59, Dos)

AROS_LP2(LONG, VPrintf,
    AROS_LPA(STRPTR, format,   D1),
    AROS_LPA(IPTR *, argarray, D2),
    struct DosLibrary *, DOSBase, 159, Dos)
#define VPrintf(format, argarray) \
    AROS_LC2(LONG, VPrintf, \
    AROS_LCA(STRPTR, format,   D1), \
    AROS_LCA(IPTR *, argarray, D2), \
    struct DosLibrary *, DOSBase, 159, Dos)

AROS_LP3(LONG, Write,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(APTR, buffer, D2),
    AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 8, Dos)
#define Write(file, buffer, length) \
    AROS_LC3(LONG, Write, \
    AROS_LCA(BPTR, file,   D1), \
    AROS_LCA(APTR, buffer, D2), \
    AROS_LCA(LONG, length, D3), \
    struct DosLibrary *, DOSBase, 8, Dos)


#endif /* CLIB_DOS_PROTOS_H */
