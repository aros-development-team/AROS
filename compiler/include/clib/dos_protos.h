#ifndef CLIB_DOS_PROTOS_H
#define CLIB_DOS_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for dos.library
    Lang: english
*/

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
AROS_LP2(void, AbortPkt,
    AROS_LPA(struct MsgPort   *, port, D1),
    AROS_LPA(struct DosPacket *, pkt, D2),
    struct DosLibrary *, DOSBase, 44, Dos)

AROS_LP2(BOOL, AddBuffers,
    AROS_LPA(STRPTR, devicename, D1),
    AROS_LPA(LONG,   numbuffers, D2),
    struct DosLibrary *, DOSBase, 122, Dos)

AROS_LP1(LONG, AddDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 113, Dos)

AROS_LP3(BOOL, AddPart,
    AROS_LPA(STRPTR, dirname, D1),
    AROS_LPA(STRPTR, filename, D2),
    AROS_LPA(ULONG , size, D3),
    struct DosLibrary *, DOSBase, 147, Dos)

AROS_LP3(LONG, AddSegment,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , seg, D2),
    AROS_LPA(LONG  , system, D3),
    struct DosLibrary *, DOSBase, 129, Dos)

AROS_LP2(APTR, AllocDosObject,
    AROS_LPA(ULONG,            type, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 38, Dos)

AROS_LP2(BOOL, AssignAdd,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , lock, D2),
    struct DosLibrary *, DOSBase, 105, Dos)

AROS_LP2(BOOL, AssignLate,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, path, D2),
    struct DosLibrary *, DOSBase, 103, Dos)

AROS_LP2(BOOL, AssignLock,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR,   lock, D2),
    struct DosLibrary *, DOSBase, 102, Dos)

AROS_LP2(BOOL, AssignPath,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, path, D2),
    struct DosLibrary *, DOSBase, 104, Dos)

AROS_LP1(struct DosList *, AttemptLockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 111, Dos)

AROS_LP3(BOOL, ChangeMode,
    AROS_LPA(ULONG, type,    D1),
    AROS_LPA(BPTR,  object,  D2),
    AROS_LPA(ULONG, newmode, D3),
    struct DosLibrary *, DOSBase, 75, Dos)

AROS_LP1(LONG, CheckSignal,
    AROS_LPA(LONG, mask, D1),
    struct DosLibrary *, DOSBase, 132, Dos)

AROS_LP0(struct CommandLineInterface *, Cli,
    struct DosLibrary *, DOSBase, 82, Dos)

AROS_LP1(IPTR, CliInitNewcli,
    AROS_LPA(struct DosPacket *, dp, A0),
    struct DosLibrary *, DOSBase, 155, Dos)

AROS_LP1(IPTR, CliInitRun,
    AROS_LPA(struct DosPacket *, dp, A0),
    struct DosLibrary *, DOSBase, 156, Dos)

AROS_LP1(BOOL, Close,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 6, Dos)

AROS_LP1(BOOL, UnLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 15, Dos)

AROS_LP2(LONG, CompareDates,
    AROS_LPA(struct DateStamp *, date1, D1),
    AROS_LPA(struct DateStamp *, date2, D2),
    struct DosLibrary *, DOSBase, 123, Dos)

AROS_LP1(BPTR, CreateDir,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 20, Dos)

AROS_LP1(struct Process *, CreateNewProc,
    AROS_LPA(struct TagItem *, tags, D1),
    struct DosLibrary *, DOSBase, 83, Dos)

AROS_LP4(struct MsgPort *, CreateProc,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , pri, D2),
    AROS_LPA(BPTR  , segList, D3),
    AROS_LPA(LONG  , stackSize, D4),
    struct DosLibrary *, DOSBase, 23, Dos)

AROS_LP1(BPTR, CurrentDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 21, Dos)

AROS_LP1(struct DateStamp *, DateStamp,
    AROS_LPA(struct DateStamp *, date, D1),
    struct DosLibrary *, DOSBase, 32, Dos)

AROS_LP1(BOOL, DateToStr,
    AROS_LPA(struct DateTime *, datetime, D1),
    struct DosLibrary *, DOSBase, 124, Dos)

AROS_LP1(void, Delay,
    AROS_LPA(ULONG, timeout, D1),
    struct DosLibrary *, DOSBase, 33, Dos)

AROS_LP1(BOOL, DeleteFile,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 12, Dos)

AROS_LP2(LONG, DeleteVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , flags, D2),
    struct DosLibrary *, DOSBase, 152, Dos)

AROS_LP1(struct MsgPort *, DeviceProc,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 29, Dos)

AROS_LP7(LONG, DoPkt,
    AROS_LPA(struct MsgPort *, port, D1),
    AROS_LPA(LONG            , action, D2),
    AROS_LPA(LONG            , arg1, D3),
    AROS_LPA(LONG            , arg2, D4),
    AROS_LPA(LONG            , arg3, D5),
    AROS_LPA(LONG            , arg4, D6),
    AROS_LPA(LONG            , arg5, D7),
    struct DosLibrary *, DOSBase, 40, Dos)

AROS_LP1(BPTR, DupLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 16, Dos)

AROS_LP1(BPTR, DupLockFromFH,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 62, Dos)

AROS_LP1(void, EndNotify,
    AROS_LPA(struct NotifyRequest *, notify, D1),
    struct DosLibrary *, DOSBase, 149, Dos)

AROS_LP4(LONG, ErrorReport,
    AROS_LPA(LONG            , code, D1),
    AROS_LPA(LONG            , type, D2),
    AROS_LPA(ULONG           , arg1, D3),
    AROS_LPA(struct MsgPort *, device, D4),
    struct DosLibrary *, DOSBase, 80, Dos)

AROS_LP5(BOOL, ExAll,
    AROS_LPA(BPTR,                  lock,    D1),
    AROS_LPA(struct ExAllData *,    buffer,  D2),
    AROS_LPA(LONG,                  size,    D3),
    AROS_LPA(LONG,                  data,    D4),
    AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 72, Dos)

AROS_LP5(void, ExAllEnd,
    AROS_LPA(BPTR,                  lock,    D1),
    AROS_LPA(struct ExAllData *,    buffer,  D2),
    AROS_LPA(LONG,                  size,    D3),
    AROS_LPA(LONG,                  data,    D4),
    AROS_LPA(struct ExAllControl *, control, D5),
    struct DosLibrary *, DOSBase, 165, Dos)

AROS_LP2(BOOL, Examine,
    AROS_LPA(BPTR,                   lock, D1),
    AROS_LPA(struct FileInfoBlock *, fib,  D2),
    struct DosLibrary *, DOSBase, 17, Dos)

AROS_LP2(BOOL, ExamineFH,
    AROS_LPA(BPTR                  , fh, D1),
    AROS_LPA(struct FileInfoBlock *, fib, D2),
    struct DosLibrary *, DOSBase, 65, Dos)

AROS_LP3(LONG, Execute,
    AROS_LPA(STRPTR, string, D1),
    AROS_LPA(BPTR  , file, D2),
    AROS_LPA(BPTR  , file2, D3),
    struct DosLibrary *, DOSBase, 37, Dos)

AROS_LP1(void, Exit,
    AROS_LPA(LONG, returnCode, D1),
    struct DosLibrary *, DOSBase, 24, Dos)

AROS_LP2(LONG, ExNext,
    AROS_LPA(BPTR                  , lock, D1),
    AROS_LPA(struct FileInfoBlock *, fileInfoBlock, D2),
    struct DosLibrary *, DOSBase, 18, Dos)

AROS_LP4(BOOL, Fault,
    AROS_LPA(LONG  , code, D1),
    AROS_LPA(STRPTR, header, D2),
    AROS_LPA(STRPTR, buffer, D3),
    AROS_LPA(LONG  , len, D4),
    struct DosLibrary *, DOSBase, 78, Dos)

AROS_LP1(LONG, FGetC,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 51, Dos)

AROS_LP3(STRPTR, FGets,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buf, D2),
    AROS_LPA(ULONG , buflen, D3),
    struct DosLibrary *, DOSBase, 56, Dos)

AROS_LP1(STRPTR, FilePart,
    AROS_LPA(STRPTR, path, D1),
    struct DosLibrary *, DOSBase, 145, Dos)

AROS_LP2(LONG, FindArg,
    AROS_LPA(STRPTR, template, D1),
    AROS_LPA(STRPTR, keyword,  D2),
    struct DosLibrary *, DOSBase, 134, Dos)

AROS_LP1(struct Process *, FindCliProc,
    AROS_LPA(ULONG, num, D1),
    struct DosLibrary *, DOSBase, 91, Dos)

AROS_LP3(struct DosList *, FindDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    AROS_LPA(STRPTR,           name,  D2),
    AROS_LPA(ULONG,            flags, D3),
    struct DosLibrary *, DOSBase, 114, Dos)

AROS_LP3(struct Segment *, FindSegment,
    AROS_LPA(STRPTR          , name, D1),
    AROS_LPA(struct Segment *, seg, D2),
    AROS_LPA(LONG            , system, D3),
    struct DosLibrary *, DOSBase, 130, Dos)

AROS_LP2(struct LocalVar *, FindVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , type, D2),
    struct DosLibrary *, DOSBase, 153, Dos)

AROS_LP1(LONG, Flush,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 60, Dos)

AROS_LP3(BOOL, Format,
    AROS_LPA(STRPTR, devicename, D1),
    AROS_LPA(STRPTR, volumename, D2),
    AROS_LPA(ULONG,  dostype,    D3),
    struct DosLibrary *, DOSBase, 119, Dos)

AROS_LP2(LONG, FPutC,
    AROS_LPA(BPTR, file,      D1),
    AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 52, Dos)

AROS_LP2(LONG, FPuts,
    AROS_LPA(BPTR,   file,   D1),
    AROS_LPA(STRPTR, string, D2),
    struct DosLibrary *, DOSBase, 57, Dos)

AROS_LP4(LONG, FRead,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(APTR , block, D2),
    AROS_LPA(ULONG, blocklen, D3),
    AROS_LPA(ULONG, number, D4),
    struct DosLibrary *, DOSBase, 54, Dos)

AROS_LP1(void, FreeArgs,
    AROS_LPA(struct RDArgs *, args, D1),
    struct DosLibrary *, DOSBase, 143, Dos)

AROS_LP1(void, FreeDeviceProc,
    AROS_LPA(struct DevProc *, dp, D1),
    struct DosLibrary *, DOSBase, 108, Dos)

AROS_LP1(void, FreeDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 117, Dos)

AROS_LP2(void, FreeDosObject,
    AROS_LPA(ULONG, type, D1),
    AROS_LPA(APTR,  ptr,  D2),
    struct DosLibrary *, DOSBase, 39, Dos)

AROS_LP4(LONG, FWrite,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(APTR , block, D2),
    AROS_LPA(ULONG, blocklen, D3),
    AROS_LPA(ULONG, number, D4),
    struct DosLibrary *, DOSBase, 55, Dos)

AROS_LP0(STRPTR, GetArgStr,
    struct DosLibrary *, DOSBase, 89, Dos)

AROS_LP0(struct MsgPort *, GetConsoleTask,
    struct DosLibrary *, DOSBase, 85, Dos)

AROS_LP2(BOOL, GetCurrentDirName,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 94, Dos)

AROS_LP2(struct DevProc *, GetDeviceProc,
    AROS_LPA(STRPTR          , name, D1),
    AROS_LPA(struct DevProc *, dp, D2),
    struct DosLibrary *, DOSBase, 107, Dos)

AROS_LP0(struct MsgPort *, GetFileSysTask,
    struct DosLibrary *, DOSBase, 87, Dos)

AROS_LP0(BPTR, GetProgramDir,
    struct DosLibrary *, DOSBase, 100, Dos)

AROS_LP2(BOOL, GetProgramName,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 96, Dos)

AROS_LP2(BOOL, GetPrompt,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 98, Dos)

AROS_LP4(LONG, GetVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , size, D3),
    AROS_LPA(LONG  , flags, D4),
    struct DosLibrary *, DOSBase, 151, Dos)

AROS_LP2(LONG, Info,
    AROS_LPA(BPTR             , lock, D1),
    AROS_LPA(struct InfoData *, parameterBlock, D2),
    struct DosLibrary *, DOSBase, 19, Dos)

AROS_LP2(LONG, Inhibit,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , onoff, D2),
    struct DosLibrary *, DOSBase, 121, Dos)

AROS_LP0(BPTR, Input,
    struct DosLibrary *, DOSBase, 9, Dos)

AROS_LP4(BPTR, InternalLoadSeg,
    AROS_LPA(BPTR  , fh, D0),
    AROS_LPA(BPTR  , table, A0),
    AROS_LPA(LONG *, funcarray, A1),
    AROS_LPA(LONG *, stack, A2),
    struct DosLibrary *, DOSBase, 126, Dos)

AROS_LP2(BOOL, InternalUnLoadSeg,
    AROS_LPA(BPTR     , seglist, D1),
    AROS_LPA(VOID_FUNC, freefunc, A1),
    struct DosLibrary *, DOSBase, 127, Dos)

AROS_LP0(LONG, IoErr,
    struct DosLibrary *, DOSBase, 22, Dos)

AROS_LP1(BOOL, IsFileSystem,
    AROS_LPA(STRPTR, devicename, D1),
    struct DosLibrary *, DOSBase, 118, Dos)

AROS_LP1(BOOL, IsInteractive,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 36, Dos)

AROS_LP1(BPTR, LoadSeg,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 25, Dos)

AROS_LP2(BPTR, Lock,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 14, Dos)

AROS_LP1(struct DosList *, LockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 109, Dos)

AROS_LP5(BOOL, LockRecord,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(ULONG, offset, D2),
    AROS_LPA(ULONG, length, D3),
    AROS_LPA(ULONG, mode, D4),
    AROS_LPA(ULONG, timeout, D5),
    struct DosLibrary *, DOSBase, 45, Dos)

AROS_LP2(BOOL, LockRecords,
    AROS_LPA(struct RecordLock *, recArray, D1),
    AROS_LPA(ULONG              , timeout, D2),
    struct DosLibrary *, DOSBase, 46, Dos)

AROS_LP2(struct DosList *, MakeDosEntry,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG,   type, D2),
    struct DosLibrary *, DOSBase, 116, Dos)

AROS_LP3(LONG, MakeLink,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , dest, D2),
    AROS_LPA(LONG  , soft, D3),
    struct DosLibrary *, DOSBase, 74, Dos)

AROS_LP1(void, MatchEnd,
    AROS_LPA(struct AnchorPath *, anchor, D1),
    struct DosLibrary *, DOSBase, 139, Dos)

AROS_LP2(LONG, MatchFirst,
    AROS_LPA(STRPTR             , pat, D1),
    AROS_LPA(struct AnchorPath *, anchor, D2),
    struct DosLibrary *, DOSBase, 137, Dos)

AROS_LP1(LONG, MatchNext,
    AROS_LPA(struct AnchorPath *, anchor, D1),
    struct DosLibrary *, DOSBase, 138, Dos)

AROS_LP2(BOOL, MatchPattern,
    AROS_LPA(STRPTR, pat, D1),
    AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 141, Dos)

AROS_LP2(BOOL, MatchPatternNoCase,
    AROS_LPA(STRPTR, pat, D1),
    AROS_LPA(STRPTR, str, D2),
    struct DosLibrary *, DOSBase, 162, Dos)

AROS_LP0(ULONG, MaxCli,
    struct DosLibrary *, DOSBase, 92, Dos)

AROS_LP3(BOOL, NameFromLock,
    AROS_LPA(BPTR,   lock,   D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG,   length, D3),
    struct DosLibrary *, DOSBase, 67, Dos)

AROS_LP3(LONG, NameFromFH,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , len, D3),
    struct DosLibrary *, DOSBase, 68, Dos)

AROS_LP2(BPTR, NewLoadSeg,
    AROS_LPA(STRPTR          , file, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 128, Dos)

AROS_LP2I(struct DosList *, NextDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    AROS_LPA(ULONG           , flags, D2),
    struct DosLibrary *, DOSBase, 115, Dos)

AROS_LP2(BPTR, Open,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(LONG,   accessMode, D2),
    struct DosLibrary *, DOSBase, 5, Dos)

AROS_LP1(BPTR, OpenFromLock,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 63, Dos)

AROS_LP0(BPTR, Output,
    struct DosLibrary *, DOSBase, 10, Dos)

AROS_LP1(BPTR, ParentDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 35, Dos)

AROS_LP1(BPTR, ParentOfFH,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 64, Dos)

AROS_LP3(LONG, ParsePattern,
    AROS_LPA(STRPTR, Source,      D1),
    AROS_LPA(STRPTR, Dest,        D2),
    AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 140, Dos)

AROS_LP3(LONG, ParsePatternNoCase,
    AROS_LPA(STRPTR, Source,      D1),
    AROS_LPA(STRPTR, Dest,        D2),
    AROS_LPA(LONG,   DestLength,  D3),
    struct DosLibrary *, DOSBase, 161, Dos)

AROS_LP1(STRPTR, PathPart,
    AROS_LPA(STRPTR, path, D1),
    struct DosLibrary *, DOSBase, 146, Dos)

AROS_LP2(BOOL, PrintFault,
    AROS_LPA(LONG,   code,   D1),
    AROS_LPA(STRPTR, header, D2),
    struct DosLibrary *, DOSBase, 79, Dos)

AROS_LP1(LONG, PutStr,
    AROS_LPA(STRPTR, string, D1),
    struct DosLibrary *, DOSBase, 158, Dos)

AROS_LP3(LONG, Read,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(APTR, buffer, D2),
    AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 7, Dos)

AROS_LP3(struct RDArgs *, ReadArgs,
    AROS_LPA(STRPTR,          template, D1),
    AROS_LPA(IPTR *,          array,    D2),
    AROS_LPA(struct RDArgs *, rdargs,   D3),
    struct DosLibrary *, DOSBase, 133, Dos)

AROS_LP3(LONG, ReadItem,
    AROS_LPA(STRPTR,           buffer,   D1),
    AROS_LPA(LONG,             maxchars, D2),
    AROS_LPA(struct CSource *, input,    D3),
    struct DosLibrary *, DOSBase, 135, Dos)

AROS_LP5(LONG, ReadLink,
    AROS_LPA(struct MsgPort *, port, D1),
    AROS_LPA(BPTR            , lock, D2),
    AROS_LPA(STRPTR          , path, D3),
    AROS_LPA(STRPTR          , buffer, D4),
    AROS_LPA(ULONG           , size, D5),
    struct DosLibrary *, DOSBase, 73, Dos)

AROS_LP2(LONG, Relabel,
    AROS_LPA(STRPTR, drive, D1),
    AROS_LPA(STRPTR, newname, D2),
    struct DosLibrary *, DOSBase, 120, Dos)

AROS_LP2(LONG, RemAssignList,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , lock, D2),
    struct DosLibrary *, DOSBase, 106, Dos)

AROS_LP1(LONG, RemDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 112, Dos)

AROS_LP1(LONG, RemSegment,
    AROS_LPA(struct Segment *, seg, D1),
    struct DosLibrary *, DOSBase, 131, Dos)

AROS_LP2(LONG, Rename,
    AROS_LPA(STRPTR, oldName, D1),
    AROS_LPA(STRPTR, newName, D2),
    struct DosLibrary *, DOSBase, 13, Dos)

AROS_LP3(void, ReplyPkt,
    AROS_LPA(struct DosPacket *, dp, D1),
    AROS_LPA(LONG              , res1, D2),
    AROS_LPA(LONG              , res2, D3),
    struct DosLibrary *, DOSBase, 43, Dos)

AROS_LP4(LONG, RunCommand,
    AROS_LPA(BPTR,   segList,   D1),
    AROS_LPA(ULONG,  stacksize, D2),
    AROS_LPA(STRPTR, argptr,    D3),
    AROS_LPA(ULONG,  argsize,   D4),
    struct DosLibrary *, DOSBase, 84, Dos)

AROS_LP2(BOOL, SameDevice,
    AROS_LPA(BPTR, lock1, D1),
    AROS_LPA(BPTR, lock2, D2),
    struct DosLibrary *, DOSBase, 164, Dos)

AROS_LP2(LONG, SameLock,
    AROS_LPA(BPTR, lock1, D1),
    AROS_LPA(BPTR, lock2, D2),
    struct DosLibrary *, DOSBase, 70, Dos)

AROS_LP3(LONG, Seek,
    AROS_LPA(BPTR, file,     D1),
    AROS_LPA(LONG, position, D2),
    AROS_LPA(LONG, mode,     D3),
    struct DosLibrary *, DOSBase, 11, Dos)

AROS_LP1(BPTR, SelectInput,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 49, Dos)

AROS_LP1(BPTR, SelectOutput,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 50, Dos)

AROS_LP3(void, SendPkt,
    AROS_LPA(struct DosPacket *, dp, D1),
    AROS_LPA(struct MsgPort   *, port, D2),
    AROS_LPA(struct MsgPort   *, replyport, D3),
    struct DosLibrary *, DOSBase, 41, Dos)

AROS_LP1(BOOL, SetArgStr,
    AROS_LPA(STRPTR, string, D1),
    struct DosLibrary *, DOSBase, 90, Dos)

AROS_LP2(BOOL, SetComment,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(STRPTR, comment, D2),
    struct DosLibrary *, DOSBase, 30, Dos)

AROS_LP1(struct MsgPort *, SetConsoleTask,
    AROS_LPA(struct MsgPort *, task, D1),
    struct DosLibrary *, DOSBase, 86, Dos)

AROS_LP1(BOOL, SetCurrentDirName,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 93, Dos)

AROS_LP2(BOOL, SetFileDate,
    AROS_LPA(STRPTR,             name, D1),
    AROS_LPA(struct DateStamp *, date, D2),
    struct DosLibrary *, DOSBase, 66, Dos)

AROS_LP3(LONG, SetFileSize,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(LONG, offset, D2),
    AROS_LPA(LONG, mode,   D3),
    struct DosLibrary *, DOSBase, 76, Dos)

AROS_LP1(struct MsgPort *, SetFileSysTask,
    AROS_LPA(struct MsgPort *, task, D1),
    struct DosLibrary *, DOSBase, 88, Dos)

AROS_LP1(LONG, SetIoErr,
    AROS_LPA(LONG, result, D1),
    struct DosLibrary *, DOSBase, 77, Dos)

AROS_LP2(LONG, SetMode,
    AROS_LPA(BPTR, fh, D1),
    AROS_LPA(LONG, mode, D2),
    struct DosLibrary *, DOSBase, 71, Dos)

AROS_LP2(BOOL, SetOwner,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(ULONG,  owner_info, D2),
    struct DosLibrary *, DOSBase, 166, Dos)

AROS_LP1(BPTR, SetProgramDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 99, Dos)

AROS_LP1(BOOL, SetProgramName,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 95, Dos)

AROS_LP1(BOOL, SetPrompt,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 97, Dos)

AROS_LP2(BOOL, SetProtection,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(ULONG,  protect, D2),
    struct DosLibrary *, DOSBase, 31, Dos)

AROS_LP4(BOOL, SetVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , size, D3),
    AROS_LPA(LONG  , flags, D4),
    struct DosLibrary *, DOSBase, 150, Dos)

AROS_LP4(LONG, SetVBuf,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buff, D2),
    AROS_LPA(LONG  , type, D3),
    AROS_LPA(LONG  , size, D4),
    struct DosLibrary *, DOSBase, 61, Dos)

AROS_LP5(WORD, SplitName,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , seperator, D2),
    AROS_LPA(STRPTR, buf, D3),
    AROS_LPA(LONG  , oldpos, D4),
    AROS_LPA(LONG  , size, D5),
    struct DosLibrary *, DOSBase, 69, Dos)

AROS_LP1(BOOL, StartNotify,
    AROS_LPA(struct NotifyRequest *, notify, D1),
    struct DosLibrary *, DOSBase, 148, Dos)

AROS_LP1(LONG, StrToDate,
    AROS_LPA(struct DateTime *, datetime, D1),
    struct DosLibrary *, DOSBase, 125, Dos)

AROS_LP2I(LONG, StrToLong,
    AROS_LPA(STRPTR, string, D1),
    AROS_LPA(LONG *, value,  D2),
    struct DosLibrary *, DOSBase, 136, Dos)

AROS_LP2(LONG, SystemTagList,
    AROS_LPA(STRPTR          , command, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 101, Dos)

AROS_LP2(LONG, UnGetC,
    AROS_LPA(BPTR, file,      D1),
    AROS_LPA(LONG, character, D2),
    struct DosLibrary *, DOSBase, 53, Dos)

AROS_LP1(void, UnLoadSeg,
    AROS_LPA(BPTR, seglist, D1),
    struct DosLibrary *, DOSBase, 26, Dos)

AROS_LP1(void, UnLockDosList,
    AROS_LPA(ULONG, flags, D1),
    struct DosLibrary *, DOSBase, 110, Dos)

AROS_LP3(BOOL, UnLockRecord,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(ULONG, offset, D2),
    AROS_LPA(ULONG, length, D3),
    struct DosLibrary *, DOSBase, 47, Dos)

AROS_LP1(BOOL, UnLockRecords,
    AROS_LPA(struct RecordLock *, recArray, D1),
    struct DosLibrary *, DOSBase, 48, Dos)

AROS_LP3(LONG, VFPrintf,
    AROS_LPA(BPTR,   file,     D1),
    AROS_LPA(STRPTR, format,   D2),
    AROS_LPA(IPTR *, argarray, D3),
    struct DosLibrary *, DOSBase, 59, Dos)

AROS_LP3(void, VFWritef,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, format, D2),
    AROS_LPA(LONG *, argarray, D3),
    struct DosLibrary *, DOSBase, 58, Dos)

AROS_LP2(LONG, VPrintf,
    AROS_LPA(STRPTR, format,   D1),
    AROS_LPA(IPTR *, argarray, D2),
    struct DosLibrary *, DOSBase, 159, Dos)

AROS_LP2(LONG, WaitForChar,
    AROS_LPA(BPTR, file, D1),
    AROS_LPA(LONG, timeout, D2),
    struct DosLibrary *, DOSBase, 34, Dos)

AROS_LP0(struct DosPacket *, WaitPkt,
    struct DosLibrary *, DOSBase, 42, Dos)

AROS_LP3(LONG, Write,
    AROS_LPA(BPTR, file,   D1),
    AROS_LPA(APTR, buffer, D2),
    AROS_LPA(LONG, length, D3),
    struct DosLibrary *, DOSBase, 8, Dos)

AROS_LP2(LONG, WriteChars,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(ULONG , buflen, D2),
    struct DosLibrary *, DOSBase, 157, Dos)


#endif /* CLIB_DOS_PROTOS_H */
