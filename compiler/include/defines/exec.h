#ifndef DEFINES_EXEC_H
#define DEFINES_EXEC_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

/*
    Defines
*/
#define AbortIO(iORequest) \
    AROS_LC1I(LONG, AbortIO, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 80, Exec)

#define AddDevice(device) \
    AROS_LC1(void, AddDevice, \
    AROS_LCA(struct Device *, device,A1), \
    struct ExecBase *, SysBase, 72, Exec)

#define AddHead(list, node) \
    AROS_LC2I(void, AddHead, \
    AROS_LCA(struct List *, list, A0), \
    AROS_LCA(struct Node *, node, A1), \
    struct ExecBase *, SysBase, 40, Exec)

#define AddIntServer(intNumber, interrupt) \
    AROS_LC2(void, AddIntServer, \
    AROS_LCA(ULONG,              intNumber, D0), \
    AROS_LCA(struct Interrupt *, interrupt, A1), \
    struct ExecBase *, SysBase, 28, Exec)

#define AddLibrary(library) \
    AROS_LC1(void, AddLibrary, \
    AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 66, Exec)

#define AddMemHandler(memHandler) \
    AROS_LC1(void, AddMemHandler, \
    AROS_LCA(struct Interrupt *, memHandler, A1), \
    struct ExecBase *, SysBase, 129, Exec)

#define AddMemList(size, attributes, pri, base, name) \
    AROS_LC5(void, AddMemList, \
    AROS_LCA(ULONG,  size,       D0), \
    AROS_LCA(ULONG,  attributes, D1), \
    AROS_LCA(LONG,   pri,        D2), \
    AROS_LCA(APTR,   base,       A0), \
    AROS_LCA(STRPTR, name,       A1), \
    struct ExecBase *, SysBase, 103, Exec)

#define AddPort(port) \
    AROS_LC1(void, AddPort, \
    AROS_LCA(struct MsgPort *, port, A1), \
    struct ExecBase *, SysBase, 59, Exec)

#define AddResource(resource) \
    AROS_LC1(void, AddResource, \
    AROS_LCA(APTR, resource, A1), \
    struct ExecBase *, SysBase, 81, Exec)

#define AddSemaphore(sigSem) \
    AROS_LC1(void, AddSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A1), \
    struct ExecBase *, SysBase, 100, Exec)

#define AddTail(list, node) \
    AROS_LC2I(void, AddTail, \
    AROS_LCA(struct List *, list, A0), \
    AROS_LCA(struct Node *, node, A1), \
    struct ExecBase *, SysBase, 41, Exec)

#define AddTask(task, initialPC, finalPC) \
    AROS_LC3(APTR, AddTask, \
    AROS_LCA(struct Task *,     task,      A1), \
    AROS_LCA(APTR,              initialPC, A2), \
    AROS_LCA(APTR,              finalPC,   A3), \
    struct ExecBase *, SysBase, 47, Exec)

#define Alert(alertNum) \
    AROS_LC1(void, Alert, \
    AROS_LCA(ULONG, alertNum, D7), \
    struct ExecBase *, SysBase, 18, Exec)

#define AllocAbs(byteSize, location) \
    AROS_LC2(APTR, AllocAbs, \
    AROS_LCA(ULONG, byteSize, D0), \
    AROS_LCA(APTR,  location, D1), \
    struct ExecBase *, SysBase, 34, Exec)

#define Allocate(freeList, byteSize) \
    AROS_LC2(APTR, Allocate, \
    AROS_LCA(struct MemHeader *, freeList, A0), \
    AROS_LCA(ULONG,              byteSize, D0), \
    struct ExecBase *, SysBase, 31, Exec)

#define AllocEntry(entry) \
    AROS_LC1(struct MemList *, AllocEntry, \
    AROS_LCA(struct MemList *, entry, A0), \
    struct ExecBase *, SysBase, 37, Exec)

#define AllocMem(byteSize, requirements) \
    AROS_LC2(APTR, AllocMem, \
    AROS_LCA(ULONG, byteSize,     D0), \
    AROS_LCA(ULONG, requirements, D1), \
    struct ExecBase *, SysBase, 33, Exec)

#define AllocPooled(poolHeader, memSize) \
    AROS_LC2(APTR, AllocPooled, \
    AROS_LCA(APTR,  poolHeader, A0), \
    AROS_LCA(ULONG, memSize,    D0), \
    struct ExecBase *, SysBase, 118, Exec)

#define AllocSignal(signalNum) \
    AROS_LC1(BYTE, AllocSignal, \
    AROS_LCA(LONG, signalNum, D0), \
    struct ExecBase *, SysBase, 55, Exec)

#define AllocTrap(trapNum) \
    AROS_LC1(LONG, AllocTrap, \
    AROS_LCA(long, trapNum, D0), \
    struct ExecBase *, SysBase, 57, Exec)

#define AllocVec(byteSize, requirements) \
    AROS_LC2(APTR, AllocVec, \
    AROS_LCA(ULONG, byteSize,     D0), \
    AROS_LCA(ULONG, requirements, D1), \
    struct ExecBase *, SysBase, 114, Exec)

#define AttemptSemaphore(sigSem) \
    AROS_LC1(ULONG, AttemptSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 96, Exec)

#define AttemptSemaphoreShared(sigSem) \
    AROS_LC1(ULONG, AttemptSemaphoreShared, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 120, Exec)

#define AvailMem(attributes) \
    AROS_LC1(ULONG, AvailMem, \
    AROS_LCA(ULONG, attributes, D1), \
    struct ExecBase *, SysBase, 36, Exec)

#define CacheClearE(address, length, caches) \
    AROS_LC3(void, CacheClearE, \
    AROS_LCA(APTR, address, A0), \
    AROS_LCA(ULONG, length, D0), \
    AROS_LCA(ULONG, caches, D1), \
    struct ExecBase *, SysBase, 107, Exec)

#define CacheClearU() \
    AROS_LC0(void, CacheClearU, \
    struct ExecBase *, SysBase, 106, Exec)

#define CacheControl(cacheBits, cacheMask) \
    AROS_LC2(ULONG, CacheControl, \
    AROS_LCA(ULONG, cacheBits, D0), \
    AROS_LCA(ULONG, cacheMask, D1), \
    struct ExecBase *, SysBase, 108, Exec)

#define CachePostDMA(address, length, flags) \
    AROS_LC3(void, CachePostDMA, \
    AROS_LCA(APTR,    address, A0), \
    AROS_LCA(ULONG *, length,  A1), \
    AROS_LCA(ULONG,   flags,  D0), \
    struct ExecBase *, SysBase, 128, Exec)

#define CachePreDMA(address, length, flags) \
    AROS_LC3(APTR, CachePreDMA, \
    AROS_LCA(APTR,    address, A0), \
    AROS_LCA(ULONG *, length,  A1), \
    AROS_LCA(ULONG,   flags,  D0), \
    struct ExecBase *, SysBase, 127, Exec)

#define Cause(softint) \
    AROS_LC1(void, Cause, \
    AROS_LCA(struct Interrupt *, softint, A1), \
    struct ExecBase *, SysBase, 30, Exec)

#define CheckIO(iORequest) \
    AROS_LC1I(struct IORequest *, CheckIO, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 78, Exec)

#define ChildFree(tid) \
    AROS_LC1(void, ChildFree, \
    AROS_LCA(APTR, tid, D0), \
    struct ExecBase *, SysBase, 123, Exec)

#define ChildOrphan(tid) \
    AROS_LC1(void, ChildOrphan, \
    AROS_LCA(APTR, tid, D0), \
    struct ExecBase *, SysBase, 124, Exec)

#define ChildStatus(tid) \
    AROS_LC1(void, ChildStatus, \
    AROS_LCA(APTR, tid, D0), \
    struct ExecBase *, SysBase, 125, Exec)

#define ChildWait(tid) \
    AROS_LC1(void, ChildWait, \
    AROS_LCA(APTR, tid, D0), \
    struct ExecBase *, SysBase, 126, Exec)

#define CloseDevice(iORequest) \
    AROS_LC1(void, CloseDevice, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 75, Exec)

#define CloseLibrary(library) \
    AROS_LC1(void, CloseLibrary, \
    AROS_LCA(struct Library *, library, A1), \
    struct ExecBase *, SysBase, 69, Exec)

#define ColdReboot() \
    AROS_LC0(void, ColdReboot, \
    struct ExecBase *, SysBase, 121, Exec)

#define CopyMem(source, dest, size) \
    AROS_LC3I(void, CopyMem, \
    AROS_LCA(APTR,  source, A0), \
    AROS_LCA(APTR,  dest,   A1), \
    AROS_LCA(ULONG, size,   D0), \
    struct ExecBase *, SysBase, 104, Exec)

#define CopyMemQuick(source, dest, size) \
    AROS_LC3I(void, CopyMemQuick, \
    AROS_LCA(APTR,  source, A0), \
    AROS_LCA(APTR,  dest,   A1), \
    AROS_LCA(ULONG, size,   D0), \
    struct ExecBase *, SysBase, 105, Exec)

#define CreateIORequest(ioReplyPort, size) \
    AROS_LC2(struct IORequest *, CreateIORequest, \
    AROS_LCA(struct MsgPort *, ioReplyPort, A0), \
    AROS_LCA(ULONG,            size,        D0), \
    struct ExecBase *, SysBase, 109, Exec)

#define CreateMsgPort() \
    AROS_LC0(struct MsgPort *, CreateMsgPort, \
    struct ExecBase *, SysBase, 111, Exec)

#define CreatePool(requirements, puddleSize, threshSize) \
    AROS_LC3(APTR, CreatePool, \
    AROS_LCA(ULONG, requirements, D0), \
    AROS_LCA(ULONG, puddleSize,   D1), \
    AROS_LCA(ULONG, threshSize,   D2), \
    struct ExecBase *, SysBase, 116, Exec)

#define Deallocate(freeList, memoryBlock, byteSize) \
    AROS_LC3(void, Deallocate, \
    AROS_LCA(struct MemHeader *, freeList,    A0), \
    AROS_LCA(APTR,               memoryBlock, A1), \
    AROS_LCA(ULONG,              byteSize,    D0), \
    struct ExecBase *, SysBase, 32, Exec)

#define Debug(flags) \
    AROS_LC1(void, Debug, \
    AROS_LCA(unsigned long, flags, D0), \
    struct ExecBase *, SysBase, 19, Exec)

#define DeleteIORequest(iorequest) \
    AROS_LC1(void, DeleteIORequest, \
    AROS_LCA(struct IORequest *, iorequest, A0), \
    struct ExecBase *, SysBase, 110, Exec)

#define DeleteMsgPort(port) \
    AROS_LC1(void, DeleteMsgPort, \
    AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 112, Exec)

#define DeletePool(poolHeader) \
    AROS_LC1(void, DeletePool, \
    AROS_LCA(APTR, poolHeader, A0), \
    struct ExecBase *, SysBase, 117, Exec)

#define Disable() \
    AROS_LC0(void, Disable, \
    struct ExecBase *, SysBase, 20, Exec)

#define DoIO(iORequest) \
    AROS_LC1(BYTE, DoIO, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 76, Exec)

#define Enable() \
    AROS_LC0(void, Enable, \
    struct ExecBase *, SysBase, 21, Exec)

#define Enqueue(list, node) \
    AROS_LC2I(void, Enqueue, \
    AROS_LCA(struct List *, list, A0), \
    AROS_LCA(struct Node *, node, A1), \
    struct ExecBase *, SysBase, 45, Exec)

#define FindName(list, name) \
    AROS_LC2I(struct Node *, FindName, \
    AROS_LCA(struct List *, list, A0), \
    AROS_LCA(UBYTE       *, name, A1), \
    struct ExecBase *, SysBase, 46, Exec)

#define FindPort(name) \
    AROS_LC1(struct MsgPort *, FindPort, \
    AROS_LCA(STRPTR, name, A1), \
    struct ExecBase *, SysBase, 65, Exec)

#define FindResident(name) \
    AROS_LC1(struct Resident *, FindResident, \
    AROS_LCA(UBYTE *, name, A1), \
    struct ExecBase *, SysBase, 16, Exec)

#define FindSemaphore(name) \
    AROS_LC1(struct SignalSemaphore *, FindSemaphore, \
    AROS_LCA(STRPTR, name, A1), \
    struct ExecBase *, SysBase, 99, Exec)

#define FindTask(name) \
    AROS_LC1(struct Task *, FindTask, \
    AROS_LCA(STRPTR, name, A1), \
    struct ExecBase *, SysBase, 49, Exec)

#define Forbid() \
    AROS_LC0(void, Forbid, \
    struct ExecBase *, SysBase, 22, Exec)

#define FreeEntry(entry) \
    AROS_LC1(void, FreeEntry, \
    AROS_LCA(struct MemList *, entry,A0), \
    struct ExecBase *, SysBase, 38, Exec)

#define FreeMem(memoryBlock, byteSize) \
    AROS_LC2(void, FreeMem, \
    AROS_LCA(APTR,  memoryBlock, A1), \
    AROS_LCA(ULONG, byteSize,    D0), \
    struct ExecBase *, SysBase, 35, Exec)

#define FreePooled(poolHeader, memory, memSize) \
    AROS_LC3(void,FreePooled, \
    AROS_LCA(APTR, poolHeader,A0), \
    AROS_LCA(APTR, memory,    A1), \
    AROS_LCA(ULONG,memSize,   D0), \
    struct ExecBase *, SysBase, 119, Exec)

#define FreeSignal(signalNum) \
    AROS_LC1(void, FreeSignal, \
    AROS_LCA(LONG, signalNum, D0), \
    struct ExecBase *, SysBase, 56, Exec)

#define FreeTrap(trapNum) \
    AROS_LC1(void, FreeTrap, \
    AROS_LCA(long, trapNum, D0), \
    struct ExecBase *, SysBase, 58, Exec)

#define FreeVec(memoryBlock) \
    AROS_LC1(void, FreeVec, \
    AROS_LCA(APTR, memoryBlock, A1), \
    struct ExecBase *, SysBase, 115, Exec)

#define GetCC() \
    AROS_LC0(UWORD, GetCC, \
    struct ExecBase *, SysBase, 88, Exec)

#define GetMsg(port) \
    AROS_LC1(struct Message *, GetMsg, \
    AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 62, Exec)

#define InitCode(startClass, version) \
    AROS_LC2(void, InitCode, \
    AROS_LCA(ULONG, startClass, D0), \
    AROS_LCA(ULONG, version, D1), \
    struct ExecBase *, SysBase, 12, Exec)

#define InitResident(resident, segList) \
    AROS_LC2(APTR, InitResident, \
    AROS_LCA(struct Resident *, resident, A1), \
    AROS_LCA(BPTR,              segList,  D1), \
    struct ExecBase *, SysBase, 17, Exec)

#define InitSemaphore(sigSem) \
    AROS_LC1I(void, InitSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 93, Exec)

#define InitStruct(initTable, memory, size) \
    AROS_LC3(void, InitStruct, \
    AROS_LCA(APTR,  initTable, A1), \
    AROS_LCA(APTR,  memory,    A2), \
    AROS_LCA(ULONG, size,      D0), \
    struct ExecBase *, SysBase, 13, Exec)

#define Insert(list, node, pred) \
    AROS_LC3I(void, Insert, \
    AROS_LCA(struct List *, list, A0), \
    AROS_LCA(struct Node *, node, A1), \
    AROS_LCA(struct Node *, pred, A2), \
    struct ExecBase *, SysBase, 39, Exec)

#define MakeFunctions(target, functionArray, funcDispBase) \
    AROS_LC3(ULONG, MakeFunctions, \
    AROS_LCA(APTR, target,        A0), \
    AROS_LCA(APTR, functionArray, A1), \
    AROS_LCA(APTR, funcDispBase,  A2), \
    struct ExecBase *, SysBase, 15, Exec)

#define MakeLibrary(funcInit, structInit, libInit, dataSize, segList) \
    AROS_LC5(struct Library *, MakeLibrary, \
    AROS_LCA(APTR,       funcInit,   A0), \
    AROS_LCA(APTR,       structInit, A1), \
    AROS_LCA(ULONG_FUNC, libInit,    A2), \
    AROS_LCA(ULONG,      dataSize,   D0), \
    AROS_LCA(BPTR,       segList,    D1), \
    struct ExecBase *, SysBase, 14, Exec)

#define ObtainQuickVector(interruptCode) \
    AROS_LC1(ULONG, ObtainQuickVector, \
    AROS_LCA(APTR, interruptCode, A0), \
    struct ExecBase *, SysBase, 131, Exec)

#define ObtainSemaphore(sigSem) \
    AROS_LC1(void, ObtainSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 94, Exec)

#define ObtainSemaphoreList(sigSem) \
    AROS_LC1(void, ObtainSemaphoreList, \
    AROS_LCA(struct List *, sigSem, A0), \
    struct ExecBase *, SysBase, 97, Exec)

#define ObtainSemaphoreShared(sigSem) \
    AROS_LC1(void, ObtainSemaphoreShared, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 113, Exec)

#define OldOpenLibrary(libName) \
    AROS_LC1(struct Library *, OldOpenLibrary, \
    AROS_LCA(UBYTE *, libName, A1), \
    struct ExecBase *, SysBase, 68, Exec)

#define OpenDevice(devName, unitNumber, iORequest, flags) \
    AROS_LC4(BYTE, OpenDevice, \
    AROS_LCA(STRPTR,             devName,    A0), \
    AROS_LCA(ULONG,              unitNumber, D0), \
    AROS_LCA(struct IORequest *, iORequest,  A1), \
    AROS_LCA(ULONG,              flags,      D1), \
    struct ExecBase *, SysBase, 74, Exec)

#define OpenLibrary(libName, version) \
    AROS_LC2(struct Library *, OpenLibrary, \
    AROS_LCA(UBYTE *, libName, A1), \
    AROS_LCA(ULONG,   version, D0), \
    struct ExecBase *, SysBase, 92, Exec)

#define OpenResource(resName) \
    AROS_LC1(APTR, OpenResource, \
    AROS_LCA(STRPTR, resName, A1), \
    struct ExecBase *, SysBase, 83, Exec)

#define Permit() \
    AROS_LC0(void, Permit, \
    struct ExecBase *, SysBase, 23, Exec)

#define Procure(sigSem, bidMsg) \
    AROS_LC2(ULONG, Procure, \
    AROS_LCA(struct SignalSemaphore  *, sigSem, A0), \
    AROS_LCA(struct SemaphoreMessage *, bidMsg, A1), \
    struct ExecBase *, SysBase, 90, Exec)

#define PutMsg(port, message) \
    AROS_LC2(void, PutMsg, \
    AROS_LCA(struct MsgPort *, port,    A0), \
    AROS_LCA(struct Message *, message, A1), \
    struct ExecBase *, SysBase, 61, Exec)

#define RawDoFmt(FormatString, DataStream, PutChProc, PutChData) \
    AROS_LC4I(APTR,RawDoFmt, \
    AROS_LCA(STRPTR,    FormatString, A0), \
    AROS_LCA(APTR,      DataStream,   A1), \
    AROS_LCA(VOID_FUNC, PutChProc,    A2), \
    AROS_LCA(APTR,      PutChData,    A3), \
    struct ExecBase *, SysBase, 87, Exec)

#define ReleaseSemaphore(sigSem) \
    AROS_LC1(void, ReleaseSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 95, Exec)

#define ReleaseSemaphoreList(sigSem) \
    AROS_LC1(void, ReleaseSemaphoreList, \
    AROS_LCA(struct List *, sigSem, A0), \
    struct ExecBase *, SysBase, 98, Exec)

#define RemDevice(device) \
    AROS_LC1(void, RemDevice, \
    AROS_LCA(struct Device *, device,A1), \
    struct ExecBase *, SysBase, 73, Exec)

#define RemHead(list) \
    AROS_LC1I(struct Node *, RemHead, \
    AROS_LCA(struct List *, list, A0), \
    struct ExecBase *, SysBase, 43, Exec)

#define RemIntServer(intNumber, interrupt) \
    AROS_LC2(void, RemIntServer, \
    AROS_LCA(ULONG,              intNumber, D0), \
    AROS_LCA(struct Interrupt *, interrupt, A1), \
    struct ExecBase *, SysBase, 29, Exec)

#define RemLibrary(library) \
    AROS_LC1(void, RemLibrary, \
    AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 67, Exec)

#define RemMemHandler(memHandler) \
    AROS_LC1(void, RemMemHandler, \
    AROS_LCA(struct Interrupt *, memHandler, A1), \
    struct ExecBase *, SysBase, 130, Exec)

#define Remove(node) \
    AROS_LC1I(void, Remove, \
    AROS_LCA(struct Node *, node, A1), \
    struct ExecBase *, SysBase, 42, Exec)

#define RemPort(port) \
    AROS_LC1(void, RemPort, \
    AROS_LCA(struct MsgPort *, port, A1), \
    struct ExecBase *, SysBase, 60, Exec)

#define RemResource(resource) \
    AROS_LC1(void, RemResource, \
    AROS_LCA(APTR, resource,A1), \
    struct ExecBase *, SysBase, 82, Exec)

#define RemSemaphore(sigSem) \
    AROS_LC1(void, RemSemaphore, \
    AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 101, Exec)

#define RemTail(list) \
    AROS_LC1I(struct Node *, RemTail, \
    AROS_LCA(struct List *, list, A0), \
    struct ExecBase *, SysBase, 44, Exec)

#define RemTask(task) \
    AROS_LC1(void, RemTask, \
    AROS_LCA(struct Task *,     task, A1), \
    struct ExecBase *, SysBase, 48, Exec)

#define ReplyMsg(message) \
    AROS_LC1(void, ReplyMsg, \
    AROS_LCA(struct Message *, message, A1), \
    struct ExecBase *, SysBase, 63, Exec)

#define SendIO(iORequest) \
    AROS_LC1(void, SendIO, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 77, Exec)

#define SetExcept(newSignals, signalSet) \
    AROS_LC2(ULONG, SetExcept, \
    AROS_LCA(ULONG, newSignals, D0), \
    AROS_LCA(ULONG, signalSet,  D1), \
    struct ExecBase *, SysBase, 52, Exec)

#define SetFunction(library, funcOffset, newFunction) \
    AROS_LC3(APTR, SetFunction, \
    AROS_LCA(struct Library *, library,     A1), \
    AROS_LCA(LONG,             funcOffset,  A0), \
    AROS_LCA(APTR,             newFunction, D0), \
    struct ExecBase *, SysBase, 70, Exec)

#define SetIntVector(intNumber, interrupt) \
    AROS_LC2(struct Interrupt *, SetIntVector, \
    AROS_LCA(ULONG,              intNumber, D0), \
    AROS_LCA(struct Interrupt *, interrupt, A1), \
    struct ExecBase *, SysBase, 27, Exec)

#define SetSignal(newSignals, signalSet) \
    AROS_LC2(ULONG, SetSignal, \
    AROS_LCA(ULONG, newSignals, D0), \
    AROS_LCA(ULONG, signalSet,  D1), \
    struct ExecBase *, SysBase, 51, Exec)

#define SetSR(newSR, mask) \
    AROS_LC2(ULONG, SetSR, \
    AROS_LCA(ULONG, newSR, D0), \
    AROS_LCA(ULONG, mask,  D1), \
    struct ExecBase *, SysBase, 24, Exec)

#define SetTaskPri(task, priority) \
    AROS_LC2(BYTE, SetTaskPri, \
    AROS_LCA(struct Task *, task,      A1), \
    AROS_LCA(LONG,          priority,  D0), \
    struct ExecBase *, SysBase, 50, Exec)

#define Signal(task, signalSet) \
    AROS_LC2(void, Signal, \
    AROS_LCA(struct Task *,     task,      A1), \
    AROS_LCA(ULONG,             signalSet, D0), \
    struct ExecBase *, SysBase, 54, Exec)

#define StackSwap(sss) \
    AROS_LC1(void, StackSwap, \
    AROS_LCA(struct StackSwapStruct *,  sss, A0), \
    struct ExecBase *, SysBase, 122, Exec)

#define SumKickData() \
    AROS_LC0(ULONG, SumKickData, \
    struct ExecBase *, SysBase, 102, Exec)

#define SumLibrary(library) \
    AROS_LC1(void, SumLibrary, \
    AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 71, Exec)

#define SuperState() \
    AROS_LC0(APTR, SuperState, \
    struct ExecBase *, SysBase, 25, Exec)

#define Supervisor(userFunction) \
    AROS_LC1(void, Supervisor, \
    AROS_LCA(ULONG_FUNC, userFunction, A5), \
    struct ExecBase *, SysBase, 5, Exec)

#define Switch() \
    AROS_LC0(void, Switch, \
    struct ExecBase *, SysBase, 9, Exec)

#define TypeOfMem(address) \
    AROS_LC1(ULONG, TypeOfMem, \
    AROS_LCA(APTR, address, A1), \
    struct ExecBase *, SysBase, 89, Exec)

#define UserState(sysStack) \
    AROS_LC1(void, UserState, \
    AROS_LCA(APTR, sysStack, D0), \
    struct ExecBase *, SysBase, 26, Exec)

#define Vacate(sigSem, bidMsg) \
    AROS_LC2(void, Vacate, \
    AROS_LCA(struct SignalSemaphore  *, sigSem, A0), \
    AROS_LCA(struct SemaphoreMessage *, bidMsg, A1), \
    struct ExecBase *, SysBase, 91, Exec)

#define Wait(signalSet) \
    AROS_LC1(ULONG, Wait, \
    AROS_LCA(ULONG, signalSet, D0), \
    struct ExecBase *, SysBase, 53, Exec)

#define WaitIO(iORequest) \
    AROS_LC1(BYTE, WaitIO, \
    AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 79, Exec)

#define WaitPort(port) \
    AROS_LC1(struct Message *, WaitPort, \
    AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 64, Exec)


#endif /* DEFINES_EXEC_H */
