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
AROS_LP2(void, AbortPkt,
    AROS_LPA(struct MsgPort   *, port, D1),
    AROS_LPA(struct DosPacket *, pkt, D2),
    struct DosLibrary *, DOSBase, 44, Dos)
#define AbortPkt(port, pkt) \
    AROS_LC2(void, AbortPkt, \
    AROS_LCA(struct MsgPort   *, port, D1), \
    AROS_LCA(struct DosPacket *, pkt, D2), \
    struct DosLibrary *, DOSBase, 44, Dos)

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

AROS_LP3(BOOL, AddPart,
    AROS_LPA(STRPTR, dirname, D1),
    AROS_LPA(STRPTR, filename, D2),
    AROS_LPA(ULONG , size, D3),
    struct DosLibrary *, DOSBase, 147, Dos)
#define AddPart(dirname, filename, size) \
    AROS_LC3(BOOL, AddPart, \
    AROS_LCA(STRPTR, dirname, D1), \
    AROS_LCA(STRPTR, filename, D2), \
    AROS_LCA(ULONG , size, D3), \
    struct DosLibrary *, DOSBase, 147, Dos)

AROS_LP3(LONG, AddSegment,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , seg, D2),
    AROS_LPA(LONG  , system, D3),
    struct DosLibrary *, DOSBase, 129, Dos)
#define AddSegment(name, seg, system) \
    AROS_LC3(LONG, AddSegment, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(BPTR  , seg, D2), \
    AROS_LCA(LONG  , system, D3), \
    struct DosLibrary *, DOSBase, 129, Dos)

AROS_LP2(APTR, AllocDosObject,
    AROS_LPA(ULONG,            type, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 38, Dos)
#define AllocDosObject(type, tags) \
    AROS_LC2(APTR, AllocDosObject, \
    AROS_LCA(ULONG,            type, D1), \
    AROS_LCA(struct TagItem *, tags, D2), \
    struct DosLibrary *, DOSBase, 38, Dos)

AROS_LP2(BOOL, AssignAdd,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , lock, D2),
    struct DosLibrary *, DOSBase, 105, Dos)
#define AssignAdd(name, lock) \
    AROS_LC2(BOOL, AssignAdd, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(BPTR  , lock, D2), \
    struct DosLibrary *, DOSBase, 105, Dos)

AROS_LP2(BOOL, AssignLate,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, path, D2),
    struct DosLibrary *, DOSBase, 103, Dos)
#define AssignLate(name, path) \
    AROS_LC2(BOOL, AssignLate, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(STRPTR, path, D2), \
    struct DosLibrary *, DOSBase, 103, Dos)

AROS_LP2(BOOL, AssignLock,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR,   lock, D2),
    struct DosLibrary *, DOSBase, 102, Dos)
#define AssignLock(name, lock) \
    AROS_LC2(BOOL, AssignLock, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(BPTR,   lock, D2), \
    struct DosLibrary *, DOSBase, 102, Dos)

AROS_LP2(BOOL, AssignPath,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, path, D2),
    struct DosLibrary *, DOSBase, 104, Dos)
#define AssignPath(name, path) \
    AROS_LC2(BOOL, AssignPath, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(STRPTR, path, D2), \
    struct DosLibrary *, DOSBase, 104, Dos)

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

AROS_LP1(LONG, CheckSignal,
    AROS_LPA(LONG, mask, D1),
    struct DosLibrary *, DOSBase, 132, Dos)
#define CheckSignal(mask) \
    AROS_LC1(LONG, CheckSignal, \
    AROS_LCA(LONG, mask, D1), \
    struct DosLibrary *, DOSBase, 132, Dos)

AROS_LP0(struct CommandLineInterface *, Cli,
    struct DosLibrary *, DOSBase, 82, Dos)
#define Cli() \
    AROS_LC0(struct CommandLineInterface *, Cli, \
    struct DosLibrary *, DOSBase, 82, Dos)

AROS_LP1(IPTR, CliInitNewcli,
    AROS_LPA(struct DosPacket *, dp, A0),
    struct DosLibrary *, DOSBase, 155, Dos)
#define CliInitNewcli(dp) \
    AROS_LC1(IPTR, CliInitNewcli, \
    AROS_LCA(struct DosPacket *, dp, A0), \
    struct DosLibrary *, DOSBase, 155, Dos)

AROS_LP1(IPTR, CliInitRun,
    AROS_LPA(struct DosPacket *, dp, A0),
    struct DosLibrary *, DOSBase, 156, Dos)
#define CliInitRun(dp) \
    AROS_LC1(IPTR, CliInitRun, \
    AROS_LCA(struct DosPacket *, dp, A0), \
    struct DosLibrary *, DOSBase, 156, Dos)

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

AROS_LP2(LONG, CompareDates,
    AROS_LPA(struct DateStamp *, date1, D1),
    AROS_LPA(struct DateStamp *, date2, D2),
    struct DosLibrary *, DOSBase, 123, Dos)
#define CompareDates(date1, date2) \
    AROS_LC2(LONG, CompareDates, \
    AROS_LCA(struct DateStamp *, date1, D1), \
    AROS_LCA(struct DateStamp *, date2, D2), \
    struct DosLibrary *, DOSBase, 123, Dos)

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

AROS_LP4(struct MsgPort *, CreateProc,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , pri, D2),
    AROS_LPA(BPTR  , segList, D3),
    AROS_LPA(LONG  , stackSize, D4),
    struct DosLibrary *, DOSBase, 23, Dos)
#define CreateProc(name, pri, segList, stackSize) \
    AROS_LC4(struct MsgPort *, CreateProc, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(LONG  , pri, D2), \
    AROS_LCA(BPTR  , segList, D3), \
    AROS_LCA(LONG  , stackSize, D4), \
    struct DosLibrary *, DOSBase, 23, Dos)

AROS_LP1(BPTR, CurrentDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 21, Dos)
#define CurrentDir(lock) \
    AROS_LC1(BPTR, CurrentDir, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 21, Dos)

AROS_LP1(struct DateStamp *, DateStamp,
    AROS_LPA(struct DateStamp *, date, D1),
    struct DosLibrary *, DOSBase, 32, Dos)
#define DateStamp(date) \
    AROS_LC1(struct DateStamp *, DateStamp, \
    AROS_LCA(struct DateStamp *, date, D1), \
    struct DosLibrary *, DOSBase, 32, Dos)

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

AROS_LP2(LONG, DeleteVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , flags, D2),
    struct DosLibrary *, DOSBase, 152, Dos)
#define DeleteVar(name, flags) \
    AROS_LC2(LONG, DeleteVar, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(ULONG , flags, D2), \
    struct DosLibrary *, DOSBase, 152, Dos)

AROS_LP1(struct MsgPort *, DeviceProc,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 29, Dos)
#define DeviceProc(name) \
    AROS_LC1(struct MsgPort *, DeviceProc, \
    AROS_LCA(STRPTR, name, D1), \
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
#define DoPkt(port, action, arg1, arg2, arg3, arg4, arg5) \
    AROS_LC7(LONG, DoPkt, \
    AROS_LCA(struct MsgPort *, port, D1), \
    AROS_LCA(LONG            , action, D2), \
    AROS_LCA(LONG            , arg1, D3), \
    AROS_LCA(LONG            , arg2, D4), \
    AROS_LCA(LONG            , arg3, D5), \
    AROS_LCA(LONG            , arg4, D6), \
    AROS_LCA(LONG            , arg5, D7), \
    struct DosLibrary *, DOSBase, 40, Dos)

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

AROS_LP1(void, EndNotify,
    AROS_LPA(struct NotifyRequest *, notify, D1),
    struct DosLibrary *, DOSBase, 149, Dos)
#define EndNotify(notify) \
    AROS_LC1(void, EndNotify, \
    AROS_LCA(struct NotifyRequest *, notify, D1), \
    struct DosLibrary *, DOSBase, 149, Dos)

AROS_LP4(LONG, ErrorReport,
    AROS_LPA(LONG            , code, D1),
    AROS_LPA(LONG            , type, D2),
    AROS_LPA(ULONG           , arg1, D3),
    AROS_LPA(struct MsgPort *, device, D4),
    struct DosLibrary *, DOSBase, 80, Dos)
#define ErrorReport(code, type, arg1, device) \
    AROS_LC4(LONG, ErrorReport, \
    AROS_LCA(LONG            , code, D1), \
    AROS_LCA(LONG            , type, D2), \
    AROS_LCA(ULONG           , arg1, D3), \
    AROS_LCA(struct MsgPort *, device, D4), \
    struct DosLibrary *, DOSBase, 80, Dos)

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

AROS_LP3(LONG, Execute,
    AROS_LPA(STRPTR, string, D1),
    AROS_LPA(BPTR  , file, D2),
    AROS_LPA(BPTR  , file2, D3),
    struct DosLibrary *, DOSBase, 37, Dos)
#define Execute(string, file, file2) \
    AROS_LC3(LONG, Execute, \
    AROS_LCA(STRPTR, string, D1), \
    AROS_LCA(BPTR  , file, D2), \
    AROS_LCA(BPTR  , file2, D3), \
    struct DosLibrary *, DOSBase, 37, Dos)

AROS_LP1(void, Exit,
    AROS_LPA(LONG, returnCode, D1),
    struct DosLibrary *, DOSBase, 24, Dos)
#define Exit(returnCode) \
    AROS_LC1(void, Exit, \
    AROS_LCA(LONG, returnCode, D1), \
    struct DosLibrary *, DOSBase, 24, Dos)

AROS_LP2(LONG, ExNext,
    AROS_LPA(BPTR                  , lock, D1),
    AROS_LPA(struct FileInfoBlock *, fileInfoBlock, D2),
    struct DosLibrary *, DOSBase, 18, Dos)
#define ExNext(lock, fileInfoBlock) \
    AROS_LC2(LONG, ExNext, \
    AROS_LCA(BPTR                  , lock, D1), \
    AROS_LCA(struct FileInfoBlock *, fileInfoBlock, D2), \
    struct DosLibrary *, DOSBase, 18, Dos)

AROS_LP4(BOOL, Fault,
    AROS_LPA(LONG  , code, D1),
    AROS_LPA(STRPTR, header, D2),
    AROS_LPA(STRPTR, buffer, D3),
    AROS_LPA(LONG  , len, D4),
    struct DosLibrary *, DOSBase, 78, Dos)
#define Fault(code, header, buffer, len) \
    AROS_LC4(BOOL, Fault, \
    AROS_LCA(LONG  , code, D1), \
    AROS_LCA(STRPTR, header, D2), \
    AROS_LCA(STRPTR, buffer, D3), \
    AROS_LCA(LONG  , len, D4), \
    struct DosLibrary *, DOSBase, 78, Dos)

AROS_LP1(LONG, FGetC,
    AROS_LPA(BPTR, file, D1),
    struct DosLibrary *, DOSBase, 51, Dos)
#define FGetC(file) \
    AROS_LC1(LONG, FGetC, \
    AROS_LCA(BPTR, file, D1), \
    struct DosLibrary *, DOSBase, 51, Dos)

AROS_LP3(STRPTR, FGets,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buf, D2),
    AROS_LPA(ULONG , buflen, D3),
    struct DosLibrary *, DOSBase, 56, Dos)
#define FGets(fh, buf, buflen) \
    AROS_LC3(STRPTR, FGets, \
    AROS_LCA(BPTR  , fh, D1), \
    AROS_LCA(STRPTR, buf, D2), \
    AROS_LCA(ULONG , buflen, D3), \
    struct DosLibrary *, DOSBase, 56, Dos)

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

AROS_LP1(struct Process *, FindCliProc,
    AROS_LPA(ULONG, num, D1),
    struct DosLibrary *, DOSBase, 91, Dos)
#define FindCliProc(num) \
    AROS_LC1(struct Process *, FindCliProc, \
    AROS_LCA(ULONG, num, D1), \
    struct DosLibrary *, DOSBase, 91, Dos)

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

AROS_LP3(struct Segment *, FindSegment,
    AROS_LPA(STRPTR          , name, D1),
    AROS_LPA(struct Segment *, seg, D2),
    AROS_LPA(LONG            , system, D3),
    struct DosLibrary *, DOSBase, 130, Dos)
#define FindSegment(name, seg, system) \
    AROS_LC3(struct Segment *, FindSegment, \
    AROS_LCA(STRPTR          , name, D1), \
    AROS_LCA(struct Segment *, seg, D2), \
    AROS_LCA(LONG            , system, D3), \
    struct DosLibrary *, DOSBase, 130, Dos)

AROS_LP2(struct LocalVar *, FindVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , type, D2),
    struct DosLibrary *, DOSBase, 153, Dos)
#define FindVar(name, type) \
    AROS_LC2(struct LocalVar *, FindVar, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(ULONG , type, D2), \
    struct DosLibrary *, DOSBase, 153, Dos)

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
    struct DosLibrary *, DOSBase, 57, Dos)
#define FPuts(file, string) \
    AROS_LC2(LONG, FPuts, \
    AROS_LCA(BPTR,   file,   D1), \
    AROS_LCA(STRPTR, string, D2), \
    struct DosLibrary *, DOSBase, 57, Dos)

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

AROS_LP1(void, FreeDeviceProc,
    AROS_LPA(struct DevProc *, dp, D1),
    struct DosLibrary *, DOSBase, 108, Dos)
#define FreeDeviceProc(dp) \
    AROS_LC1(void, FreeDeviceProc, \
    AROS_LCA(struct DevProc *, dp, D1), \
    struct DosLibrary *, DOSBase, 108, Dos)

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

AROS_LP0(struct MsgPort *, GetConsoleTask,
    struct DosLibrary *, DOSBase, 85, Dos)
#define GetConsoleTask() \
    AROS_LC0(struct MsgPort *, GetConsoleTask, \
    struct DosLibrary *, DOSBase, 85, Dos)

AROS_LP2(BOOL, GetCurrentDirName,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 94, Dos)
#define GetCurrentDirName(buf, len) \
    AROS_LC2(BOOL, GetCurrentDirName, \
    AROS_LCA(STRPTR, buf, D1), \
    AROS_LCA(LONG  , len, D2), \
    struct DosLibrary *, DOSBase, 94, Dos)

AROS_LP2(struct DevProc *, GetDeviceProc,
    AROS_LPA(STRPTR          , name, D1),
    AROS_LPA(struct DevProc *, dp, D2),
    struct DosLibrary *, DOSBase, 107, Dos)
#define GetDeviceProc(name, dp) \
    AROS_LC2(struct DevProc *, GetDeviceProc, \
    AROS_LCA(STRPTR          , name, D1), \
    AROS_LCA(struct DevProc *, dp, D2), \
    struct DosLibrary *, DOSBase, 107, Dos)

AROS_LP0(struct MsgPort *, GetFileSysTask,
    struct DosLibrary *, DOSBase, 87, Dos)
#define GetFileSysTask() \
    AROS_LC0(struct MsgPort *, GetFileSysTask, \
    struct DosLibrary *, DOSBase, 87, Dos)

AROS_LP0(BPTR, GetProgramDir,
    struct DosLibrary *, DOSBase, 100, Dos)
#define GetProgramDir() \
    AROS_LC0(BPTR, GetProgramDir, \
    struct DosLibrary *, DOSBase, 100, Dos)

AROS_LP2(BOOL, GetProgramName,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 96, Dos)
#define GetProgramName(buf, len) \
    AROS_LC2(BOOL, GetProgramName, \
    AROS_LCA(STRPTR, buf, D1), \
    AROS_LCA(LONG  , len, D2), \
    struct DosLibrary *, DOSBase, 96, Dos)

AROS_LP2(BOOL, GetPrompt,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(LONG  , len, D2),
    struct DosLibrary *, DOSBase, 98, Dos)
#define GetPrompt(buf, len) \
    AROS_LC2(BOOL, GetPrompt, \
    AROS_LCA(STRPTR, buf, D1), \
    AROS_LCA(LONG  , len, D2), \
    struct DosLibrary *, DOSBase, 98, Dos)

AROS_LP4(LONG, GetVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , size, D3),
    AROS_LPA(LONG  , flags, D4),
    struct DosLibrary *, DOSBase, 151, Dos)
#define GetVar(name, buffer, size, flags) \
    AROS_LC4(LONG, GetVar, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(STRPTR, buffer, D2), \
    AROS_LCA(LONG  , size, D3), \
    AROS_LCA(LONG  , flags, D4), \
    struct DosLibrary *, DOSBase, 151, Dos)

AROS_LP2(LONG, Info,
    AROS_LPA(BPTR             , lock, D1),
    AROS_LPA(struct InfoData *, parameterBlock, D2),
    struct DosLibrary *, DOSBase, 19, Dos)
#define Info(lock, parameterBlock) \
    AROS_LC2(LONG, Info, \
    AROS_LCA(BPTR             , lock, D1), \
    AROS_LCA(struct InfoData *, parameterBlock, D2), \
    struct DosLibrary *, DOSBase, 19, Dos)

AROS_LP2(LONG, Inhibit,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , onoff, D2),
    struct DosLibrary *, DOSBase, 121, Dos)
#define Inhibit(name, onoff) \
    AROS_LC2(LONG, Inhibit, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(LONG  , onoff, D2), \
    struct DosLibrary *, DOSBase, 121, Dos)

AROS_LP0(BPTR, Input,
    struct DosLibrary *, DOSBase, 9, Dos)
#define Input() \
    AROS_LC0(BPTR, Input, \
    struct DosLibrary *, DOSBase, 9, Dos)

AROS_LP4(BPTR, InternalLoadSeg,
    AROS_LPA(BPTR  , fh, D0),
    AROS_LPA(BPTR  , table, A0),
    AROS_LPA(LONG *, funcarray, A1),
    AROS_LPA(LONG *, stack, A2),
    struct DosLibrary *, DOSBase, 126, Dos)
#define InternalLoadSeg(fh, table, funcarray, stack) \
    AROS_LC4(BPTR, InternalLoadSeg, \
    AROS_LCA(BPTR  , fh, D0), \
    AROS_LCA(BPTR  , table, A0), \
    AROS_LCA(LONG *, funcarray, A1), \
    AROS_LCA(LONG *, stack, A2), \
    struct DosLibrary *, DOSBase, 126, Dos)

AROS_LP2(BOOL, InternalUnLoadSeg,
    AROS_LPA(BPTR     , seglist, D1),
    AROS_LPA(VOID_FUNC, freefunc, A1),
    struct DosLibrary *, DOSBase, 127, Dos)
#define InternalUnLoadSeg(seglist, freefunc) \
    AROS_LC2(BOOL, InternalUnLoadSeg, \
    AROS_LCA(BPTR     , seglist, D1), \
    AROS_LCA(VOID_FUNC, freefunc, A1), \
    struct DosLibrary *, DOSBase, 127, Dos)

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

AROS_LP5(BOOL, LockRecord,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(ULONG, offset, D2),
    AROS_LPA(ULONG, length, D3),
    AROS_LPA(ULONG, mode, D4),
    AROS_LPA(ULONG, timeout, D5),
    struct DosLibrary *, DOSBase, 45, Dos)
#define LockRecord(fh, offset, length, mode, timeout) \
    AROS_LC5(BOOL, LockRecord, \
    AROS_LCA(BPTR , fh, D1), \
    AROS_LCA(ULONG, offset, D2), \
    AROS_LCA(ULONG, length, D3), \
    AROS_LCA(ULONG, mode, D4), \
    AROS_LCA(ULONG, timeout, D5), \
    struct DosLibrary *, DOSBase, 45, Dos)

AROS_LP2(BOOL, LockRecords,
    AROS_LPA(struct RecordLock *, recArray, D1),
    AROS_LPA(ULONG              , timeout, D2),
    struct DosLibrary *, DOSBase, 46, Dos)
#define LockRecords(recArray, timeout) \
    AROS_LC2(BOOL, LockRecords, \
    AROS_LCA(struct RecordLock *, recArray, D1), \
    AROS_LCA(ULONG              , timeout, D2), \
    struct DosLibrary *, DOSBase, 46, Dos)

AROS_LP2(struct DosList *, MakeDosEntry,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG,   type, D2),
    struct DosLibrary *, DOSBase, 116, Dos)
#define MakeDosEntry(name, type) \
    AROS_LC2(struct DosList *, MakeDosEntry, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(LONG,   type, D2), \
    struct DosLibrary *, DOSBase, 116, Dos)

AROS_LP3(LONG, MakeLink,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(LONG  , dest, D2),
    AROS_LPA(LONG  , soft, D3),
    struct DosLibrary *, DOSBase, 74, Dos)
#define MakeLink(name, dest, soft) \
    AROS_LC3(LONG, MakeLink, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(LONG  , dest, D2), \
    AROS_LCA(LONG  , soft, D3), \
    struct DosLibrary *, DOSBase, 74, Dos)

AROS_LP1(void, MatchEnd,
    AROS_LPA(struct AnchorPath *, anchor, D1),
    struct DosLibrary *, DOSBase, 139, Dos)
#define MatchEnd(anchor) \
    AROS_LC1(void, MatchEnd, \
    AROS_LCA(struct AnchorPath *, anchor, D1), \
    struct DosLibrary *, DOSBase, 139, Dos)

AROS_LP2(LONG, MatchFirst,
    AROS_LPA(STRPTR             , pat, D1),
    AROS_LPA(struct AnchorPath *, anchor, D2),
    struct DosLibrary *, DOSBase, 137, Dos)
#define MatchFirst(pat, anchor) \
    AROS_LC2(LONG, MatchFirst, \
    AROS_LCA(STRPTR             , pat, D1), \
    AROS_LCA(struct AnchorPath *, anchor, D2), \
    struct DosLibrary *, DOSBase, 137, Dos)

AROS_LP1(LONG, MatchNext,
    AROS_LPA(struct AnchorPath *, anchor, D1),
    struct DosLibrary *, DOSBase, 138, Dos)
#define MatchNext(anchor) \
    AROS_LC1(LONG, MatchNext, \
    AROS_LCA(struct AnchorPath *, anchor, D1), \
    struct DosLibrary *, DOSBase, 138, Dos)

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

AROS_LP2(BPTR, NewLoadSeg,
    AROS_LPA(STRPTR          , file, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 128, Dos)
#define NewLoadSeg(file, tags) \
    AROS_LC2(BPTR, NewLoadSeg, \
    AROS_LCA(STRPTR          , file, D1), \
    AROS_LCA(struct TagItem *, tags, D2), \
    struct DosLibrary *, DOSBase, 128, Dos)

AROS_LP2I(struct DosList *, NextDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    AROS_LPA(ULONG           , flags, D2),
    struct DosLibrary *, DOSBase, 115, Dos)
#define NextDosEntry(dlist, flags) \
    AROS_LC2I(struct DosList *, NextDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    AROS_LCA(ULONG           , flags, D2), \
    struct DosLibrary *, DOSBase, 115, Dos)

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

AROS_LP1(BPTR, ParentDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 35, Dos)
#define ParentDir(lock) \
    AROS_LC1(BPTR, ParentDir, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 35, Dos)

AROS_LP1(BPTR, ParentOfFH,
    AROS_LPA(BPTR, fh, D1),
    struct DosLibrary *, DOSBase, 64, Dos)
#define ParentOfFH(fh) \
    AROS_LC1(BPTR, ParentOfFH, \
    AROS_LCA(BPTR, fh, D1), \
    struct DosLibrary *, DOSBase, 64, Dos)

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

AROS_LP5(LONG, ReadLink,
    AROS_LPA(struct MsgPort *, port, D1),
    AROS_LPA(BPTR            , lock, D2),
    AROS_LPA(STRPTR          , path, D3),
    AROS_LPA(STRPTR          , buffer, D4),
    AROS_LPA(ULONG           , size, D5),
    struct DosLibrary *, DOSBase, 73, Dos)
#define ReadLink(port, lock, path, buffer, size) \
    AROS_LC5(LONG, ReadLink, \
    AROS_LCA(struct MsgPort *, port, D1), \
    AROS_LCA(BPTR            , lock, D2), \
    AROS_LCA(STRPTR          , path, D3), \
    AROS_LCA(STRPTR          , buffer, D4), \
    AROS_LCA(ULONG           , size, D5), \
    struct DosLibrary *, DOSBase, 73, Dos)

AROS_LP2(LONG, Relabel,
    AROS_LPA(STRPTR, drive, D1),
    AROS_LPA(STRPTR, newname, D2),
    struct DosLibrary *, DOSBase, 120, Dos)
#define Relabel(drive, newname) \
    AROS_LC2(LONG, Relabel, \
    AROS_LCA(STRPTR, drive, D1), \
    AROS_LCA(STRPTR, newname, D2), \
    struct DosLibrary *, DOSBase, 120, Dos)

AROS_LP2(LONG, RemAssignList,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(BPTR  , lock, D2),
    struct DosLibrary *, DOSBase, 106, Dos)
#define RemAssignList(name, lock) \
    AROS_LC2(LONG, RemAssignList, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(BPTR  , lock, D2), \
    struct DosLibrary *, DOSBase, 106, Dos)

AROS_LP1(LONG, RemDosEntry,
    AROS_LPA(struct DosList *, dlist, D1),
    struct DosLibrary *, DOSBase, 112, Dos)
#define RemDosEntry(dlist) \
    AROS_LC1(LONG, RemDosEntry, \
    AROS_LCA(struct DosList *, dlist, D1), \
    struct DosLibrary *, DOSBase, 112, Dos)

AROS_LP1(LONG, RemSegment,
    AROS_LPA(struct Segment *, seg, D1),
    struct DosLibrary *, DOSBase, 131, Dos)
#define RemSegment(seg) \
    AROS_LC1(LONG, RemSegment, \
    AROS_LCA(struct Segment *, seg, D1), \
    struct DosLibrary *, DOSBase, 131, Dos)

AROS_LP2(LONG, Rename,
    AROS_LPA(STRPTR, oldName, D1),
    AROS_LPA(STRPTR, newName, D2),
    struct DosLibrary *, DOSBase, 13, Dos)
#define Rename(oldName, newName) \
    AROS_LC2(LONG, Rename, \
    AROS_LCA(STRPTR, oldName, D1), \
    AROS_LCA(STRPTR, newName, D2), \
    struct DosLibrary *, DOSBase, 13, Dos)

AROS_LP3(void, ReplyPkt,
    AROS_LPA(struct DosPacket *, dp, D1),
    AROS_LPA(LONG              , res1, D2),
    AROS_LPA(LONG              , res2, D3),
    struct DosLibrary *, DOSBase, 43, Dos)
#define ReplyPkt(dp, res1, res2) \
    AROS_LC3(void, ReplyPkt, \
    AROS_LCA(struct DosPacket *, dp, D1), \
    AROS_LCA(LONG              , res1, D2), \
    AROS_LCA(LONG              , res2, D3), \
    struct DosLibrary *, DOSBase, 43, Dos)

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

AROS_LP2(BOOL, SameDevice,
    AROS_LPA(BPTR, lock1, D1),
    AROS_LPA(BPTR, lock2, D2),
    struct DosLibrary *, DOSBase, 164, Dos)
#define SameDevice(lock1, lock2) \
    AROS_LC2(BOOL, SameDevice, \
    AROS_LCA(BPTR, lock1, D1), \
    AROS_LCA(BPTR, lock2, D2), \
    struct DosLibrary *, DOSBase, 164, Dos)

AROS_LP2(LONG, SameLock,
    AROS_LPA(BPTR, lock1, D1),
    AROS_LPA(BPTR, lock2, D2),
    struct DosLibrary *, DOSBase, 70, Dos)
#define SameLock(lock1, lock2) \
    AROS_LC2(LONG, SameLock, \
    AROS_LCA(BPTR, lock1, D1), \
    AROS_LCA(BPTR, lock2, D2), \
    struct DosLibrary *, DOSBase, 70, Dos)

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

AROS_LP3(void, SendPkt,
    AROS_LPA(struct DosPacket *, dp, D1),
    AROS_LPA(struct MsgPort   *, port, D2),
    AROS_LPA(struct MsgPort   *, replyport, D3),
    struct DosLibrary *, DOSBase, 41, Dos)
#define SendPkt(dp, port, replyport) \
    AROS_LC3(void, SendPkt, \
    AROS_LCA(struct DosPacket *, dp, D1), \
    AROS_LCA(struct MsgPort   *, port, D2), \
    AROS_LCA(struct MsgPort   *, replyport, D3), \
    struct DosLibrary *, DOSBase, 41, Dos)

AROS_LP1(BOOL, SetArgStr,
    AROS_LPA(STRPTR, string, D1),
    struct DosLibrary *, DOSBase, 90, Dos)
#define SetArgStr(string) \
    AROS_LC1(BOOL, SetArgStr, \
    AROS_LCA(STRPTR, string, D1), \
    struct DosLibrary *, DOSBase, 90, Dos)

AROS_LP2(BOOL, SetComment,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(STRPTR, comment, D2),
    struct DosLibrary *, DOSBase, 30, Dos)
#define SetComment(name, comment) \
    AROS_LC2(BOOL, SetComment, \
    AROS_LCA(STRPTR, name,    D1), \
    AROS_LCA(STRPTR, comment, D2), \
    struct DosLibrary *, DOSBase, 30, Dos)

AROS_LP1(struct MsgPort *, SetConsoleTask,
    AROS_LPA(struct MsgPort *, task, D1),
    struct DosLibrary *, DOSBase, 86, Dos)
#define SetConsoleTask(task) \
    AROS_LC1(struct MsgPort *, SetConsoleTask, \
    AROS_LCA(struct MsgPort *, task, D1), \
    struct DosLibrary *, DOSBase, 86, Dos)

AROS_LP1(BOOL, SetCurrentDirName,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 93, Dos)
#define SetCurrentDirName(name) \
    AROS_LC1(BOOL, SetCurrentDirName, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 93, Dos)

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

AROS_LP1(struct MsgPort *, SetFileSysTask,
    AROS_LPA(struct MsgPort *, task, D1),
    struct DosLibrary *, DOSBase, 88, Dos)
#define SetFileSysTask(task) \
    AROS_LC1(struct MsgPort *, SetFileSysTask, \
    AROS_LCA(struct MsgPort *, task, D1), \
    struct DosLibrary *, DOSBase, 88, Dos)

AROS_LP1(LONG, SetIoErr,
    AROS_LPA(LONG, result, D1),
    struct DosLibrary *, DOSBase, 77, Dos)
#define SetIoErr(result) \
    AROS_LC1(LONG, SetIoErr, \
    AROS_LCA(LONG, result, D1), \
    struct DosLibrary *, DOSBase, 77, Dos)

AROS_LP2(LONG, SetMode,
    AROS_LPA(BPTR, fh, D1),
    AROS_LPA(LONG, mode, D2),
    struct DosLibrary *, DOSBase, 71, Dos)
#define SetMode(fh, mode) \
    AROS_LC2(LONG, SetMode, \
    AROS_LCA(BPTR, fh, D1), \
    AROS_LCA(LONG, mode, D2), \
    struct DosLibrary *, DOSBase, 71, Dos)

AROS_LP2(BOOL, SetOwner,
    AROS_LPA(STRPTR, name,       D1),
    AROS_LPA(ULONG,  owner_info, D2),
    struct DosLibrary *, DOSBase, 166, Dos)
#define SetOwner(name, owner_info) \
    AROS_LC2(BOOL, SetOwner, \
    AROS_LCA(STRPTR, name,       D1), \
    AROS_LCA(ULONG,  owner_info, D2), \
    struct DosLibrary *, DOSBase, 166, Dos)

AROS_LP1(BPTR, SetProgramDir,
    AROS_LPA(BPTR, lock, D1),
    struct DosLibrary *, DOSBase, 99, Dos)
#define SetProgramDir(lock) \
    AROS_LC1(BPTR, SetProgramDir, \
    AROS_LCA(BPTR, lock, D1), \
    struct DosLibrary *, DOSBase, 99, Dos)

AROS_LP1(BOOL, SetProgramName,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 95, Dos)
#define SetProgramName(name) \
    AROS_LC1(BOOL, SetProgramName, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 95, Dos)

AROS_LP1(BOOL, SetPrompt,
    AROS_LPA(STRPTR, name, D1),
    struct DosLibrary *, DOSBase, 97, Dos)
#define SetPrompt(name) \
    AROS_LC1(BOOL, SetPrompt, \
    AROS_LCA(STRPTR, name, D1), \
    struct DosLibrary *, DOSBase, 97, Dos)

AROS_LP2(BOOL, SetProtection,
    AROS_LPA(STRPTR, name,    D1),
    AROS_LPA(ULONG,  protect, D2),
    struct DosLibrary *, DOSBase, 31, Dos)
#define SetProtection(name, protect) \
    AROS_LC2(BOOL, SetProtection, \
    AROS_LCA(STRPTR, name,    D1), \
    AROS_LCA(ULONG,  protect, D2), \
    struct DosLibrary *, DOSBase, 31, Dos)

AROS_LP4(BOOL, SetVar,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(STRPTR, buffer, D2),
    AROS_LPA(LONG  , size, D3),
    AROS_LPA(LONG  , flags, D4),
    struct DosLibrary *, DOSBase, 150, Dos)
#define SetVar(name, buffer, size, flags) \
    AROS_LC4(BOOL, SetVar, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(STRPTR, buffer, D2), \
    AROS_LCA(LONG  , size, D3), \
    AROS_LCA(LONG  , flags, D4), \
    struct DosLibrary *, DOSBase, 150, Dos)

AROS_LP4(LONG, SetVBuf,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, buff, D2),
    AROS_LPA(LONG  , type, D3),
    AROS_LPA(LONG  , size, D4),
    struct DosLibrary *, DOSBase, 61, Dos)
#define SetVBuf(fh, buff, type, size) \
    AROS_LC4(LONG, SetVBuf, \
    AROS_LCA(BPTR  , fh, D1), \
    AROS_LCA(STRPTR, buff, D2), \
    AROS_LCA(LONG  , type, D3), \
    AROS_LCA(LONG  , size, D4), \
    struct DosLibrary *, DOSBase, 61, Dos)

AROS_LP5(WORD, SplitName,
    AROS_LPA(STRPTR, name, D1),
    AROS_LPA(ULONG , seperator, D2),
    AROS_LPA(STRPTR, buf, D3),
    AROS_LPA(LONG  , oldpos, D4),
    AROS_LPA(LONG  , size, D5),
    struct DosLibrary *, DOSBase, 69, Dos)
#define SplitName(name, seperator, buf, oldpos, size) \
    AROS_LC5(WORD, SplitName, \
    AROS_LCA(STRPTR, name, D1), \
    AROS_LCA(ULONG , seperator, D2), \
    AROS_LCA(STRPTR, buf, D3), \
    AROS_LCA(LONG  , oldpos, D4), \
    AROS_LCA(LONG  , size, D5), \
    struct DosLibrary *, DOSBase, 69, Dos)

AROS_LP1(BOOL, StartNotify,
    AROS_LPA(struct NotifyRequest *, notify, D1),
    struct DosLibrary *, DOSBase, 148, Dos)
#define StartNotify(notify) \
    AROS_LC1(BOOL, StartNotify, \
    AROS_LCA(struct NotifyRequest *, notify, D1), \
    struct DosLibrary *, DOSBase, 148, Dos)

AROS_LP1(LONG, StrToDate,
    AROS_LPA(struct DateTime *, datetime, D1),
    struct DosLibrary *, DOSBase, 125, Dos)
#define StrToDate(datetime) \
    AROS_LC1(LONG, StrToDate, \
    AROS_LCA(struct DateTime *, datetime, D1), \
    struct DosLibrary *, DOSBase, 125, Dos)

AROS_LP2I(LONG, StrToLong,
    AROS_LPA(STRPTR, string, D1),
    AROS_LPA(LONG *, value,  D2),
    struct DosLibrary *, DOSBase, 136, Dos)
#define StrToLong(string, value) \
    AROS_LC2I(LONG, StrToLong, \
    AROS_LCA(STRPTR, string, D1), \
    AROS_LCA(LONG *, value,  D2), \
    struct DosLibrary *, DOSBase, 136, Dos)

AROS_LP2(LONG, SystemTagList,
    AROS_LPA(STRPTR          , command, D1),
    AROS_LPA(struct TagItem *, tags, D2),
    struct DosLibrary *, DOSBase, 101, Dos)
#define SystemTagList(command, tags) \
    AROS_LC2(LONG, SystemTagList, \
    AROS_LCA(STRPTR          , command, D1), \
    AROS_LCA(struct TagItem *, tags, D2), \
    struct DosLibrary *, DOSBase, 101, Dos)

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

AROS_LP3(BOOL, UnLockRecord,
    AROS_LPA(BPTR , fh, D1),
    AROS_LPA(ULONG, offset, D2),
    AROS_LPA(ULONG, length, D3),
    struct DosLibrary *, DOSBase, 47, Dos)
#define UnLockRecord(fh, offset, length) \
    AROS_LC3(BOOL, UnLockRecord, \
    AROS_LCA(BPTR , fh, D1), \
    AROS_LCA(ULONG, offset, D2), \
    AROS_LCA(ULONG, length, D3), \
    struct DosLibrary *, DOSBase, 47, Dos)

AROS_LP1(BOOL, UnLockRecords,
    AROS_LPA(struct RecordLock *, recArray, D1),
    struct DosLibrary *, DOSBase, 48, Dos)
#define UnLockRecords(recArray) \
    AROS_LC1(BOOL, UnLockRecords, \
    AROS_LCA(struct RecordLock *, recArray, D1), \
    struct DosLibrary *, DOSBase, 48, Dos)

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

AROS_LP3(void, VFWritef,
    AROS_LPA(BPTR  , fh, D1),
    AROS_LPA(STRPTR, format, D2),
    AROS_LPA(LONG *, argarray, D3),
    struct DosLibrary *, DOSBase, 58, Dos)
#define VFWritef(fh, format, argarray) \
    AROS_LC3(void, VFWritef, \
    AROS_LCA(BPTR  , fh, D1), \
    AROS_LCA(STRPTR, format, D2), \
    AROS_LCA(LONG *, argarray, D3), \
    struct DosLibrary *, DOSBase, 58, Dos)

AROS_LP2(LONG, VPrintf,
    AROS_LPA(STRPTR, format,   D1),
    AROS_LPA(IPTR *, argarray, D2),
    struct DosLibrary *, DOSBase, 159, Dos)
#define VPrintf(format, argarray) \
    AROS_LC2(LONG, VPrintf, \
    AROS_LCA(STRPTR, format,   D1), \
    AROS_LCA(IPTR *, argarray, D2), \
    struct DosLibrary *, DOSBase, 159, Dos)

AROS_LP2(LONG, WaitForChar,
    AROS_LPA(BPTR, file, D1),
    AROS_LPA(LONG, timeout, D2),
    struct DosLibrary *, DOSBase, 34, Dos)
#define WaitForChar(file, timeout) \
    AROS_LC2(LONG, WaitForChar, \
    AROS_LCA(BPTR, file, D1), \
    AROS_LCA(LONG, timeout, D2), \
    struct DosLibrary *, DOSBase, 34, Dos)

AROS_LP0(struct DosPacket *, WaitPkt,
    struct DosLibrary *, DOSBase, 42, Dos)
#define WaitPkt() \
    AROS_LC0(struct DosPacket *, WaitPkt, \
    struct DosLibrary *, DOSBase, 42, Dos)

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

AROS_LP2(LONG, WriteChars,
    AROS_LPA(STRPTR, buf, D1),
    AROS_LPA(ULONG , buflen, D2),
    struct DosLibrary *, DOSBase, 157, Dos)
#define WriteChars(buf, buflen) \
    AROS_LC2(LONG, WriteChars, \
    AROS_LCA(STRPTR, buf, D1), \
    AROS_LCA(ULONG , buflen, D2), \
    struct DosLibrary *, DOSBase, 157, Dos)


#endif /* CLIB_DOS_PROTOS_H */
