#ifndef CLIB_EXEC_PROTOS_H
#define CLIB_EXEC_PROTOS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#ifndef SysBase
extern struct ExecBase * SysBase;
#endif

/*
    Prototypes
*/
__AROS_LP3(void, CacheClearE,
    __AROS_LPA(APTR,  address, A0),
    __AROS_LPA(ULONG, length,  D0),
    __AROS_LPA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)
#define CacheClearE(address, length, caches) \
    __AROS_LC3(void, CacheClearE, \
    __AROS_LCA(APTR,  address, A0), \
    __AROS_LCA(ULONG, length,  D0), \
    __AROS_LCA(ULONG, caches,  D1), \
    struct ExecBase *, SysBase, 107, Exec)

__AROS_LP0(void, CacheClearU,
    struct ExecBase *, SysBase, 106, Exec)
#define CacheClearU() \
    __AROS_LC0(void, CacheClearU, \
    struct ExecBase *, SysBase, 106, Exec)

__AROS_LP0(void, CacheControl,
    struct ExecBase *, SysBase, 108, Exec)
#define CacheControl() \
    __AROS_LC0(void, CacheControl, \
    struct ExecBase *, SysBase, 108, Exec)

__AROS_LP0(void, CachePostDMA,
    struct ExecBase *, SysBase, 128, Exec)
#define CachePostDMA() \
    __AROS_LC0(void, CachePostDMA, \
    struct ExecBase *, SysBase, 128, Exec)

__AROS_LP0(void, CachePreDMA,
    struct ExecBase *, SysBase, 127, Exec)
#define CachePreDMA() \
    __AROS_LC0(void, CachePreDMA, \
    struct ExecBase *, SysBase, 127, Exec)

__AROS_LP0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
#define Disable() \
    __AROS_LC0(void, Disable, \
    struct ExecBase *, SysBase, 20, Exec)

__AROS_LP0(void, Dispatch,
    struct ExecBase *, SysBase, 7, Exec)
#define Dispatch() \
    __AROS_LC0(void, Dispatch, \
    struct ExecBase *, SysBase, 7, Exec)

__AROS_LP0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
#define Enable() \
    __AROS_LC0(void, Enable, \
    struct ExecBase *, SysBase, 21, Exec)

__AROS_LP0(void, Exception,
    struct ExecBase *, SysBase, 8, Exec)
#define Exception() \
    __AROS_LC0(void, Exception, \
    struct ExecBase *, SysBase, 8, Exec)

__AROS_LP0(void, Forbid,
    struct ExecBase *, SysBase, 22, Exec)
#define Forbid() \
    __AROS_LC0(void, Forbid, \
    struct ExecBase *, SysBase, 22, Exec)

__AROS_LP0(void, GetCC,
    struct ExecBase *, SysBase, 88, Exec)
#define GetCC() \
    __AROS_LC0(void, GetCC, \
    struct ExecBase *, SysBase, 88, Exec)

__AROS_LP0(void, Permit,
    struct ExecBase *, SysBase, 23, Exec)
#define Permit() \
    __AROS_LC0(void, Permit, \
    struct ExecBase *, SysBase, 23, Exec)

__AROS_LP0(void, SetSR,
    struct ExecBase *, SysBase, 24, Exec)
#define SetSR() \
    __AROS_LC0(void, SetSR, \
    struct ExecBase *, SysBase, 24, Exec)

__AROS_LP0(void, StackSwap,
    struct ExecBase *, SysBase, 122, Exec)
#define StackSwap() \
    __AROS_LC0(void, StackSwap, \
    struct ExecBase *, SysBase, 122, Exec)

__AROS_LP0(void, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
#define SuperState() \
    __AROS_LC0(void, SuperState, \
    struct ExecBase *, SysBase, 25, Exec)

__AROS_LP0(void, Switch,
    struct ExecBase *, SysBase, 6, Exec)
#define Switch() \
    __AROS_LC0(void, Switch, \
    struct ExecBase *, SysBase, 6, Exec)

__AROS_LP0(void, Switch,
    struct ExecBase *, SysBase, 6, Exec)
#define Switch() \
    __AROS_LC0(void, Switch, \
    struct ExecBase *, SysBase, 6, Exec)

__AROS_LP0(void, UserState,
    struct ExecBase *, SysBase, 26, Exec)
#define UserState() \
    __AROS_LC0(void, UserState, \
    struct ExecBase *, SysBase, 26, Exec)

__AROS_LP3I(APTR, PrepareContext,
    __AROS_LPA(APTR, stackPointer, A0),
    __AROS_LPA(APTR, entryPoint,   A1),
    __AROS_LPA(APTR, fallBack,     A2),
    struct ExecBase *, SysBase, 9, Exec)
#define PrepareContext(stackPointer, entryPoint, fallBack) \
    __AROS_LC3I(APTR, PrepareContext, \
    __AROS_LCA(APTR, stackPointer, A0), \
    __AROS_LCA(APTR, entryPoint,   A1), \
    __AROS_LCA(APTR, fallBack,     A2), \
    struct ExecBase *, SysBase, 9, Exec)

__AROS_LP1I(LONG, AbortIO,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 80, Exec)
#define AbortIO(iORequest) \
    __AROS_LC1I(LONG, AbortIO, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 80, Exec)

__AROS_LP1(void, AddDevice,
    __AROS_LPA(struct Device *, device,A1),
    struct ExecBase *, SysBase, 72, Exec)
#define AddDevice(device) \
    __AROS_LC1(void, AddDevice, \
    __AROS_LCA(struct Device *, device,A1), \
    struct ExecBase *, SysBase, 72, Exec)

__AROS_LP2I(void, AddHead,
    __AROS_LPA(struct List *, list, A0),
    __AROS_LPA(struct Node *, node, A1),
    struct SysBase *, SysBase, 40, Exec)
#define AddHead(list, node) \
    __AROS_LC2I(void, AddHead, \
    __AROS_LCA(struct List *, list, A0), \
    __AROS_LCA(struct Node *, node, A1), \
    struct SysBase *, SysBase, 40, Exec)

__AROS_LP1(void, AddLibrary,
    __AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 66, Exec)
#define AddLibrary(library) \
    __AROS_LC1(void, AddLibrary, \
    __AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 66, Exec)

__AROS_LP1(void, AddMemHandler,
    __AROS_LPA(struct Interrupt *, memHandler, A1),
    struct ExecBase *, SysBase, 129, Exec)
#define AddMemHandler(memHandler) \
    __AROS_LC1(void, AddMemHandler, \
    __AROS_LCA(struct Interrupt *, memHandler, A1), \
    struct ExecBase *, SysBase, 129, Exec)

__AROS_LP5(void, AddMemList,
    __AROS_LPA(ULONG,  size,       D0),
    __AROS_LPA(ULONG,  attributes, D1),
    __AROS_LPA(LONG,   pri,        D2),
    __AROS_LPA(APTR,   base,       A0),
    __AROS_LPA(STRPTR, name,       A1),
    struct ExecBase *, SysBase, 103, Exec)
#define AddMemList(size, attributes, pri, base, name) \
    __AROS_LC5(void, AddMemList, \
    __AROS_LCA(ULONG,  size,       D0), \
    __AROS_LCA(ULONG,  attributes, D1), \
    __AROS_LCA(LONG,   pri,        D2), \
    __AROS_LCA(APTR,   base,       A0), \
    __AROS_LCA(STRPTR, name,       A1), \
    struct ExecBase *, SysBase, 103, Exec)

__AROS_LP1(void, AddPort,
    __AROS_LPA(struct MsgPort *, port, A1),
    struct ExecBase *, SysBase, 59, Exec)
#define AddPort(port) \
    __AROS_LC1(void, AddPort, \
    __AROS_LCA(struct MsgPort *, port, A1), \
    struct ExecBase *, SysBase, 59, Exec)

__AROS_LP1(void, AddResource,
    __AROS_LPA(APTR, resource, A1),
    struct ExecBase *, SysBase, 81, Exec)
#define AddResource(resource) \
    __AROS_LC1(void, AddResource, \
    __AROS_LCA(APTR, resource, A1), \
    struct ExecBase *, SysBase, 81, Exec)

__AROS_LP1(void, AddSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A1),
    struct ExecBase *, SysBase, 100, Exec)
#define AddSemaphore(sigSem) \
    __AROS_LC1(void, AddSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A1), \
    struct ExecBase *, SysBase, 100, Exec)

__AROS_LP2I(void, AddTail,
    __AROS_LPA(struct List *, list, A0),
    __AROS_LPA(struct Node *, node, A1),
    struct SysBase *, SysBase, 41, Exec)
#define AddTail(list, node) \
    __AROS_LC2I(void, AddTail, \
    __AROS_LCA(struct List *, list, A0), \
    __AROS_LCA(struct Node *, node, A1), \
    struct SysBase *, SysBase, 41, Exec)

__AROS_LP3(APTR, AddTask,
    __AROS_LPA(struct Task *,     task,      A1),
    __AROS_LPA(APTR,              initialPC, A2),
    __AROS_LPA(APTR,              finalPC,   A3),
    struct ExecBase *, SysBase, 47, Exec)
#define AddTask(task, initialPC, finalPC) \
    __AROS_LC3(APTR, AddTask, \
    __AROS_LCA(struct Task *,     task,      A1), \
    __AROS_LCA(APTR,              initialPC, A2), \
    __AROS_LCA(APTR,              finalPC,   A3), \
    struct ExecBase *, SysBase, 47, Exec)

__AROS_LP1(void, Alert,
    __AROS_LPA(unsigned long, alertNum, D7),
    struct ExecBase *, SysBase, 18, Exec)
#define Alert(alertNum) \
    __AROS_LC1(void, Alert, \
    __AROS_LCA(unsigned long, alertNum, D7), \
    struct ExecBase *, SysBase, 18, Exec)

__AROS_LP2(APTR, AllocAbs,
    __AROS_LPA(ULONG, byteSize, D0),
    __AROS_LPA(APTR,  location, D1),
    struct ExecBase *, SysBase, 34, Exec)
#define AllocAbs(byteSize, location) \
    __AROS_LC2(APTR, AllocAbs, \
    __AROS_LCA(ULONG, byteSize, D0), \
    __AROS_LCA(APTR,  location, D1), \
    struct ExecBase *, SysBase, 34, Exec)

__AROS_LP2(APTR, Allocate,
    __AROS_LPA(struct MemHeader *, freeList, A0),
    __AROS_LPA(ULONG,              byteSize, D0),
    struct ExecBase *, SysBase, 31, Exec)
#define Allocate(freeList, byteSize) \
    __AROS_LC2(APTR, Allocate, \
    __AROS_LCA(struct MemHeader *, freeList, A0), \
    __AROS_LCA(ULONG,              byteSize, D0), \
    struct ExecBase *, SysBase, 31, Exec)

__AROS_LP1(struct MemList *, AllocEntry,
    __AROS_LPA(struct MemList *, entry, A0),
    struct ExecBase *, SysBase, 37, Exec)
#define AllocEntry(entry) \
    __AROS_LC1(struct MemList *, AllocEntry, \
    __AROS_LCA(struct MemList *, entry, A0), \
    struct ExecBase *, SysBase, 37, Exec)

__AROS_LP2(APTR, AllocMem,
    __AROS_LPA(ULONG, byteSize,     D0),
    __AROS_LPA(ULONG, requirements, D1),
    struct ExecBase *, SysBase, 33, Exec)
#define AllocMem(byteSize, requirements) \
    __AROS_LC2(APTR, AllocMem, \
    __AROS_LCA(ULONG, byteSize,     D0), \
    __AROS_LCA(ULONG, requirements, D1), \
    struct ExecBase *, SysBase, 33, Exec)

__AROS_LP2(APTR, AllocPooled,
    __AROS_LPA(APTR,  poolHeader, A0),
    __AROS_LPA(ULONG, memSize,    D0),
    struct ExecBase *, SysBase, 118, Exec)
#define AllocPooled(poolHeader, memSize) \
    __AROS_LC2(APTR, AllocPooled, \
    __AROS_LCA(APTR,  poolHeader, A0), \
    __AROS_LCA(ULONG, memSize,    D0), \
    struct ExecBase *, SysBase, 118, Exec)

__AROS_LP1(BYTE, AllocSignal,
    __AROS_LPA(LONG, signalNum, D0),
    struct ExecBase *, SysBase, 55, Exec)
#define AllocSignal(signalNum) \
    __AROS_LC1(BYTE, AllocSignal, \
    __AROS_LCA(LONG, signalNum, D0), \
    struct ExecBase *, SysBase, 55, Exec)

__AROS_LP2(APTR, AllocVec,
    __AROS_LPA(ULONG, byteSize,     D0),
    __AROS_LPA(ULONG, requirements, D1),
    struct ExecBase *, SysBase, 114, Exec)
#define AllocVec(byteSize, requirements) \
    __AROS_LC2(APTR, AllocVec, \
    __AROS_LCA(ULONG, byteSize,     D0), \
    __AROS_LCA(ULONG, requirements, D1), \
    struct ExecBase *, SysBase, 114, Exec)

__AROS_LP1(ULONG, AttemptSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 96, Exec)
#define AttemptSemaphore(sigSem) \
    __AROS_LC1(ULONG, AttemptSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 96, Exec)

__AROS_LP1(ULONG, AttemptSemaphoreShared,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 120, Exec)
#define AttemptSemaphoreShared(sigSem) \
    __AROS_LC1(ULONG, AttemptSemaphoreShared, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 120, Exec)

__AROS_LP1(ULONG, AvailMem,
    __AROS_LPA(ULONG, attributes, D1),
    struct ExecBase *, SysBase, 36, Exec)
#define AvailMem(attributes) \
    __AROS_LC1(ULONG, AvailMem, \
    __AROS_LCA(ULONG, attributes, D1), \
    struct ExecBase *, SysBase, 36, Exec)

__AROS_LP1(void, Cause,
    __AROS_LPA(struct Interrupt *, interrupt, A1),
    struct ExecBase *, SysBase, 30, Exec)
#define Cause(interrupt) \
    __AROS_LC1(void, Cause, \
    __AROS_LCA(struct Interrupt *, interrupt, A1), \
    struct ExecBase *, SysBase, 30, Exec)

__AROS_LP1I(struct IORequest *, CheckIO,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 78, Exec)
#define CheckIO(iORequest) \
    __AROS_LC1I(struct IORequest *, CheckIO, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 78, Exec)

__AROS_LP1(void, CloseDevice,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 75, Exec)
#define CloseDevice(iORequest) \
    __AROS_LC1(void, CloseDevice, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 75, Exec)

__AROS_LP1(void, CloseLibrary,
    __AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 69, Exec)
#define CloseLibrary(library) \
    __AROS_LC1(void, CloseLibrary, \
    __AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 69, Exec)

__AROS_LP3I(void, CopyMem,
    __AROS_LPA(APTR,  source, A0),
    __AROS_LPA(APTR,  dest,   A1),
    __AROS_LPA(ULONG, size,   D0),
    struct ExecBase *, SysBase, 104, Exec)
#define CopyMem(source, dest, size) \
    __AROS_LC3I(void, CopyMem, \
    __AROS_LCA(APTR,  source, A0), \
    __AROS_LCA(APTR,  dest,   A1), \
    __AROS_LCA(ULONG, size,   D0), \
    struct ExecBase *, SysBase, 104, Exec)

__AROS_LP3I(void, CopyMemQuick,
    __AROS_LPA(APTR,  source, A0),
    __AROS_LPA(APTR,  dest,   A1),
    __AROS_LPA(ULONG, size,   D0),
    struct ExecBase *, SysBase, 105, Exec)
#define CopyMemQuick(source, dest, size) \
    __AROS_LC3I(void, CopyMemQuick, \
    __AROS_LCA(APTR,  source, A0), \
    __AROS_LCA(APTR,  dest,   A1), \
    __AROS_LCA(ULONG, size,   D0), \
    struct ExecBase *, SysBase, 105, Exec)

__AROS_LP2(struct IORequest *, CreateIORequest,
    __AROS_LPA(struct MsgPort *, ioReplyPort, A0),
    __AROS_LPA(ULONG,            size,        D0),
    struct ExecBase *, SysBase, 109, Exec)
#define CreateIORequest(ioReplyPort, size) \
    __AROS_LC2(struct IORequest *, CreateIORequest, \
    __AROS_LCA(struct MsgPort *, ioReplyPort, A0), \
    __AROS_LCA(ULONG,            size,        D0), \
    struct ExecBase *, SysBase, 109, Exec)

__AROS_LP0(struct MsgPort *, CreateMsgPort,
    struct ExecBase *, SysBase, 111, Exec)
#define CreateMsgPort() \
    __AROS_LC0(struct MsgPort *, CreateMsgPort, \
    struct ExecBase *, SysBase, 111, Exec)

__AROS_LP3(APTR, CreatePool,
    __AROS_LPA(ULONG, requirements, D0),
    __AROS_LPA(ULONG, puddleSize,   D1),
    __AROS_LPA(ULONG, threshSize,   D2),
    struct ExecBase *, SysBase, 116, Exec)
#define CreatePool(requirements, puddleSize, threshSize) \
    __AROS_LC3(APTR, CreatePool, \
    __AROS_LCA(ULONG, requirements, D0), \
    __AROS_LCA(ULONG, puddleSize,   D1), \
    __AROS_LCA(ULONG, threshSize,   D2), \
    struct ExecBase *, SysBase, 116, Exec)

__AROS_LP3(void, Deallocate,
    __AROS_LPA(struct MemHeader *, freeList,    A0),
    __AROS_LPA(APTR,               memoryBlock, A1),
    __AROS_LPA(ULONG,              byteSize,    D0),
    struct ExecBase *, SysBase, 32, Exec)
#define Deallocate(freeList, memoryBlock, byteSize) \
    __AROS_LC3(void, Deallocate, \
    __AROS_LCA(struct MemHeader *, freeList,    A0), \
    __AROS_LCA(APTR,               memoryBlock, A1), \
    __AROS_LCA(ULONG,              byteSize,    D0), \
    struct ExecBase *, SysBase, 32, Exec)

__AROS_LP1(void, DeleteIORequest,
    __AROS_LPA(struct IORequest *, iorequest, A0),
    struct ExecBase *, SysBase, 110, Exec)
#define DeleteIORequest(iorequest) \
    __AROS_LC1(void, DeleteIORequest, \
    __AROS_LCA(struct IORequest *, iorequest, A0), \
    struct ExecBase *, SysBase, 110, Exec)

__AROS_LP1(void, DeleteMsgPort,
    __AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 112, Exec)
#define DeleteMsgPort(port) \
    __AROS_LC1(void, DeleteMsgPort, \
    __AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 112, Exec)

__AROS_LP1(void, DeletePool,
    __AROS_LPA(APTR, poolHeader, A0),
    struct ExecBase *, SysBase, 117, Exec)
#define DeletePool(poolHeader) \
    __AROS_LC1(void, DeletePool, \
    __AROS_LCA(APTR, poolHeader, A0), \
    struct ExecBase *, SysBase, 117, Exec)

__AROS_LP1(BYTE, DoIO,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 76, Exec)
#define DoIO(iORequest) \
    __AROS_LC1(BYTE, DoIO, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 76, Exec)

__AROS_LP2I(void, Enqueue,
    __AROS_LPA(struct List *, list, A0),
    __AROS_LPA(struct Node *, node, A1),
    struct SysBase *, SysBase, 45, Exec)
#define Enqueue(list, node) \
    __AROS_LC2I(void, Enqueue, \
    __AROS_LCA(struct List *, list, A0), \
    __AROS_LCA(struct Node *, node, A1), \
    struct SysBase *, SysBase, 45, Exec)

__AROS_LP2I(struct Node *, FindName,
    __AROS_LPA(struct List *, list, A0),
    __AROS_LPA(UBYTE       *, name, A1),
    struct SysBase *, SysBase, 46, Exec)
#define FindName(list, name) \
    __AROS_LC2I(struct Node *, FindName, \
    __AROS_LCA(struct List *, list, A0), \
    __AROS_LCA(UBYTE       *, name, A1), \
    struct SysBase *, SysBase, 46, Exec)

__AROS_LP1(struct MsgPort *, FindPort,
    __AROS_LPA(STRPTR, name,A1),
    struct ExecBase *, SysBase, 65, Exec)
#define FindPort(name) \
    __AROS_LC1(struct MsgPort *, FindPort, \
    __AROS_LCA(STRPTR, name,A1), \
    struct ExecBase *, SysBase, 65, Exec)

__AROS_LP1(struct SignalSemaphore *, FindSemaphore,
    __AROS_LPA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 99, Exec)
#define FindSemaphore(name) \
    __AROS_LC1(struct SignalSemaphore *, FindSemaphore, \
    __AROS_LCA(STRPTR, name, A1), \
    struct ExecBase *, SysBase, 99, Exec)

__AROS_LP1(struct Task *, FindTask,
    __AROS_LPA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 49, Exec)
#define FindTask(name) \
    __AROS_LC1(struct Task *, FindTask, \
    __AROS_LCA(STRPTR, name, A1), \
    struct ExecBase *, SysBase, 49, Exec)

__AROS_LP1(void, FreeEntry,
    __AROS_LPA(struct MemList *, entry,A0),
    struct ExecBase *, SysBase, 38, Exec)
#define FreeEntry(entry) \
    __AROS_LC1(void, FreeEntry, \
    __AROS_LCA(struct MemList *, entry,A0), \
    struct ExecBase *, SysBase, 38, Exec)

__AROS_LP2(void, FreeMem,
    __AROS_LPA(APTR,  memoryBlock, A1),
    __AROS_LPA(ULONG, byteSize,    D0),
    struct ExecBase *, SysBase, 35, Exec)
#define FreeMem(memoryBlock, byteSize) \
    __AROS_LC2(void, FreeMem, \
    __AROS_LCA(APTR,  memoryBlock, A1), \
    __AROS_LCA(ULONG, byteSize,    D0), \
    struct ExecBase *, SysBase, 35, Exec)

__AROS_LP3(void,FreePooled,
    __AROS_LPA(APTR, poolHeader,A0),
    __AROS_LPA(APTR, memory,    A1),
    __AROS_LPA(ULONG,memSize,   D0),
    struct ExecBase *, SysBase, 119, Exec)
#define FreePooled(poolHeader, memory, memSize) \
    __AROS_LC3(void,FreePooled, \
    __AROS_LCA(APTR, poolHeader,A0), \
    __AROS_LCA(APTR, memory,    A1), \
    __AROS_LCA(ULONG,memSize,   D0), \
    struct ExecBase *, SysBase, 119, Exec)

__AROS_LP1(void, FreeSignal,
    __AROS_LPA(LONG, signalNum, D0),
    struct ExecBase *, SysBase, 56, Exec)
#define FreeSignal(signalNum) \
    __AROS_LC1(void, FreeSignal, \
    __AROS_LCA(LONG, signalNum, D0), \
    struct ExecBase *, SysBase, 56, Exec)

__AROS_LP1(void, FreeVec,
    __AROS_LPA(APTR, memoryBlock, A1),
    struct ExecBase *, SysBase, 115, Exec)
#define FreeVec(memoryBlock) \
    __AROS_LC1(void, FreeVec, \
    __AROS_LCA(APTR, memoryBlock, A1), \
    struct ExecBase *, SysBase, 115, Exec)

__AROS_LP1(struct Message *, GetMsg,
    __AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 62, Exec)
#define GetMsg(port) \
    __AROS_LC1(struct Message *, GetMsg, \
    __AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 62, Exec)

__AROS_LP2(APTR, InitResident,
    __AROS_LPA(struct Resident *, resident, A1),
    __AROS_LPA(BPTR,              segList,  D1),
    struct ExecBase *, SysBase, 17, Exec)
#define InitResident(resident, segList) \
    __AROS_LC2(APTR, InitResident, \
    __AROS_LCA(struct Resident *, resident, A1), \
    __AROS_LCA(BPTR,              segList,  D1), \
    struct ExecBase *, SysBase, 17, Exec)

__AROS_LP1I(void, InitSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 93, Exec)
#define InitSemaphore(sigSem) \
    __AROS_LC1I(void, InitSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 93, Exec)

__AROS_LP3(void, InitStruct,
    __AROS_LPA(APTR,  initTable, A1),
    __AROS_LPA(APTR,  memory,    A2),
    __AROS_LPA(ULONG, size,      D0),
    struct ExecBase *, SysBase, 13, Exec)
#define InitStruct(initTable, memory, size) \
    __AROS_LC3(void, InitStruct, \
    __AROS_LCA(APTR,  initTable, A1), \
    __AROS_LCA(APTR,  memory,    A2), \
    __AROS_LCA(ULONG, size,      D0), \
    struct ExecBase *, SysBase, 13, Exec)

__AROS_LP3I(void, Insert,
    __AROS_LPA(struct List *, list, A0),
    __AROS_LPA(struct Node *, node, A1),
    __AROS_LPA(struct Node *, pred, A2),
    struct SysBase *, SysBase, 39, Exec)
#define Insert(list, node, pred) \
    __AROS_LC3I(void, Insert, \
    __AROS_LCA(struct List *, list, A0), \
    __AROS_LCA(struct Node *, node, A1), \
    __AROS_LCA(struct Node *, pred, A2), \
    struct SysBase *, SysBase, 39, Exec)

__AROS_LP3(ULONG, MakeFunctions,
    __AROS_LPA(APTR, target,        A0),
    __AROS_LPA(APTR, functionArray, A1),
    __AROS_LPA(APTR, funcDispBase,  A2),
    struct ExecBase *, SysBase, 15, Exec)
#define MakeFunctions(target, functionArray, funcDispBase) \
    __AROS_LC3(ULONG, MakeFunctions, \
    __AROS_LCA(APTR, target,        A0), \
    __AROS_LCA(APTR, functionArray, A1), \
    __AROS_LCA(APTR, funcDispBase,  A2), \
    struct ExecBase *, SysBase, 15, Exec)

__AROS_LP5(struct Library *, MakeLibrary,
    __AROS_LPA(APTR,       funcInit,   A0),
    __AROS_LPA(APTR,       structInit, A1),
    __AROS_LPA(ULONG_FUNC, libInit,    A2),
    __AROS_LPA(ULONG,      dataSize,   D0),
    __AROS_LPA(BPTR,       segList,    D1),
    struct ExecBase *, SysBase, 14, Exec)
#define MakeLibrary(funcInit, structInit, libInit, dataSize, segList) \
    __AROS_LC5(struct Library *, MakeLibrary, \
    __AROS_LCA(APTR,       funcInit,   A0), \
    __AROS_LCA(APTR,       structInit, A1), \
    __AROS_LCA(ULONG_FUNC, libInit,    A2), \
    __AROS_LCA(ULONG,      dataSize,   D0), \
    __AROS_LCA(BPTR,       segList,    D1), \
    struct ExecBase *, SysBase, 14, Exec)

__AROS_LP1(ULONG, ObtainQuickVector,
    __AROS_LPA(APTR, interruptCode, A0),
    struct ExecBase *, SysBase, 131, Exec)
#define ObtainQuickVector(interruptCode) \
    __AROS_LC1(ULONG, ObtainQuickVector, \
    __AROS_LCA(APTR, interruptCode, A0), \
    struct ExecBase *, SysBase, 131, Exec)

__AROS_LP1(void, ObtainSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 94, Exec)
#define ObtainSemaphore(sigSem) \
    __AROS_LC1(void, ObtainSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 94, Exec)

__AROS_LP1(void, ObtainSemaphoreList,
    __AROS_LPA(struct List *, sigSem, A0),
    struct ExecBase *, SysBase, 97, Exec)
#define ObtainSemaphoreList(sigSem) \
    __AROS_LC1(void, ObtainSemaphoreList, \
    __AROS_LCA(struct List *, sigSem, A0), \
    struct ExecBase *, SysBase, 97, Exec)

__AROS_LP1(void, ObtainSemaphoreShared,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 113, Exec)
#define ObtainSemaphoreShared(sigSem) \
    __AROS_LC1(void, ObtainSemaphoreShared, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 113, Exec)

__AROS_LP1(struct Library *, OldOpenLibrary,
    __AROS_LPA(UBYTE *, libName, A1),
    struct ExecBase *, SysBase, 68, Exec)
#define OldOpenLibrary(libName) \
    __AROS_LC1(struct Library *, OldOpenLibrary, \
    __AROS_LCA(UBYTE *, libName, A1), \
    struct ExecBase *, SysBase, 68, Exec)

__AROS_LP4(BYTE, OpenDevice,
    __AROS_LPA(STRPTR,             devName,    A0),
    __AROS_LPA(ULONG,              unitNumber, D0),
    __AROS_LPA(struct IORequest *, iORequest,  A1),
    __AROS_LPA(ULONG,              flags,      D1),
    struct ExecBase *, SysBase, 74, Exec)
#define OpenDevice(devName, unitNumber, iORequest, flags) \
    __AROS_LC4(BYTE, OpenDevice, \
    __AROS_LCA(STRPTR,             devName,    A0), \
    __AROS_LCA(ULONG,              unitNumber, D0), \
    __AROS_LCA(struct IORequest *, iORequest,  A1), \
    __AROS_LCA(ULONG,              flags,      D1), \
    struct ExecBase *, SysBase, 74, Exec)

__AROS_LP2(struct Library *, OpenLibrary,
    __AROS_LPA(UBYTE *, libName, A1),
    __AROS_LPA(ULONG,   version, D0),
    struct ExecBase *, SysBase, 92, Exec)
#define OpenLibrary(libName, version) \
    __AROS_LC2(struct Library *, OpenLibrary, \
    __AROS_LCA(UBYTE *, libName, A1), \
    __AROS_LCA(ULONG,   version, D0), \
    struct ExecBase *, SysBase, 92, Exec)

__AROS_LP1(APTR, OpenResource,
    __AROS_LPA(STRPTR, resName, A1),
    struct ExecBase *, SysBase, 83, Exec)
#define OpenResource(resName) \
    __AROS_LC1(APTR, OpenResource, \
    __AROS_LCA(STRPTR, resName, A1), \
    struct ExecBase *, SysBase, 83, Exec)

__AROS_LP2(ULONG, Procure,
    __AROS_LPA(struct SignalSemaphore  *, sigSem, A0),
    __AROS_LPA(struct SemaphoreMessage *, bidMsg, A1),
    struct ExecBase *, SysBase, 90, Exec)
#define Procure(sigSem, bidMsg) \
    __AROS_LC2(ULONG, Procure, \
    __AROS_LCA(struct SignalSemaphore  *, sigSem, A0), \
    __AROS_LCA(struct SemaphoreMessage *, bidMsg, A1), \
    struct ExecBase *, SysBase, 90, Exec)

__AROS_LP2(void, PutMsg,
    __AROS_LPA(struct MsgPort *, port,    A0),
    __AROS_LPA(struct Message *, message, A1),
    struct ExecBase *, SysBase, 61, Exec)
#define PutMsg(port, message) \
    __AROS_LC2(void, PutMsg, \
    __AROS_LCA(struct MsgPort *, port,    A0), \
    __AROS_LCA(struct Message *, message, A1), \
    struct ExecBase *, SysBase, 61, Exec)

__AROS_LP4I(APTR,RawDoFmt,
    __AROS_LPA(STRPTR,    FormatString, A0),
    __AROS_LPA(APTR,      DataStream,   A1),
    __AROS_LPA(VOID_FUNC, PutChProc,    A2),
    __AROS_LPA(APTR,      PutChData,    A3),
    struct ExecBase *, SysBase, 87, Exec)
#define RawDoFmt(FormatString, DataStream, PutChProc, PutChData) \
    __AROS_LC4I(APTR,RawDoFmt, \
    __AROS_LCA(STRPTR,    FormatString, A0), \
    __AROS_LCA(APTR,      DataStream,   A1), \
    __AROS_LCA(VOID_FUNC, PutChProc,    A2), \
    __AROS_LCA(APTR,      PutChData,    A3), \
    struct ExecBase *, SysBase, 87, Exec)

__AROS_LP1(ULONG, RawPutChar,
    __AROS_LPA(ULONG, character, D0),
    struct ExecBase *, SysBase, 86, Exec)
#define RawPutChar(character) \
    __AROS_LC1(ULONG, RawPutChar, \
    __AROS_LCA(ULONG, character, D0), \
    struct ExecBase *, SysBase, 86, Exec)

__AROS_LP1(void, ReleaseSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 95, Exec)
#define ReleaseSemaphore(sigSem) \
    __AROS_LC1(void, ReleaseSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 95, Exec)

__AROS_LP1(void, ReleaseSemaphoreList,
    __AROS_LPA(struct List *, sigSem, A0),
    struct ExecBase *, SysBase, 98, Exec)
#define ReleaseSemaphoreList(sigSem) \
    __AROS_LC1(void, ReleaseSemaphoreList, \
    __AROS_LCA(struct List *, sigSem, A0), \
    struct ExecBase *, SysBase, 98, Exec)

__AROS_LP1(void, RemDevice,
    __AROS_LPA(struct Device *, device,A1),
    struct ExecBase *, SysBase, 73, Exec)
#define RemDevice(device) \
    __AROS_LC1(void, RemDevice, \
    __AROS_LCA(struct Device *, device,A1), \
    struct ExecBase *, SysBase, 73, Exec)

__AROS_LP1I(struct Node *, RemHead,
    __AROS_LPA(struct List *, list, A0),
    struct SysBase *, SysBase, 43, Exec)
#define RemHead(list) \
    __AROS_LC1I(struct Node *, RemHead, \
    __AROS_LCA(struct List *, list, A0), \
    struct SysBase *, SysBase, 43, Exec)

__AROS_LP1(void, RemLibrary,
    __AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 67, Exec)
#define RemLibrary(library) \
    __AROS_LC1(void, RemLibrary, \
    __AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 67, Exec)

__AROS_LP1(void, RemMemHandler,
    __AROS_LPA(struct Interrupt *, memHandler, A1),
    struct ExecBase *, SysBase, 130, Exec)
#define RemMemHandler(memHandler) \
    __AROS_LC1(void, RemMemHandler, \
    __AROS_LCA(struct Interrupt *, memHandler, A1), \
    struct ExecBase *, SysBase, 130, Exec)

__AROS_LP1I(void, Remove,
    __AROS_LPA(struct Node *, node, A1),
    struct SysBase *, SysBase, 42, Exec)
#define Remove(node) \
    __AROS_LC1I(void, Remove, \
    __AROS_LCA(struct Node *, node, A1), \
    struct SysBase *, SysBase, 42, Exec)

__AROS_LP1(void, RemPort,
    __AROS_LPA(struct MsgPort *, port, A1),
    struct ExecBase *, SysBase, 60, Exec)
#define RemPort(port) \
    __AROS_LC1(void, RemPort, \
    __AROS_LCA(struct MsgPort *, port, A1), \
    struct ExecBase *, SysBase, 60, Exec)

__AROS_LP1(void, RemResource,
    __AROS_LPA(APTR, resource,A1),
    struct ExecBase *, SysBase, 82, Exec)
#define RemResource(resource) \
    __AROS_LC1(void, RemResource, \
    __AROS_LCA(APTR, resource,A1), \
    struct ExecBase *, SysBase, 82, Exec)

__AROS_LP1(void, RemSemaphore,
    __AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 101, Exec)
#define RemSemaphore(sigSem) \
    __AROS_LC1(void, RemSemaphore, \
    __AROS_LCA(struct SignalSemaphore *, sigSem, A0), \
    struct ExecBase *, SysBase, 101, Exec)

__AROS_LP1I(struct Node *, RemTail,
    __AROS_LPA(struct List *, list, A0),
    struct SysBase *, SysBase, 44, Exec)
#define RemTail(list) \
    __AROS_LC1I(struct Node *, RemTail, \
    __AROS_LCA(struct List *, list, A0), \
    struct SysBase *, SysBase, 44, Exec)

__AROS_LP1(void, RemTask,
    __AROS_LPA(struct Task *,     task, A1),
    struct ExecBase *, SysBase, 48, Exec)
#define RemTask(task) \
    __AROS_LC1(void, RemTask, \
    __AROS_LCA(struct Task *,     task, A1), \
    struct ExecBase *, SysBase, 48, Exec)

__AROS_LP1(void, ReplyMsg,
    __AROS_LPA(struct Message *, message, A1),
    struct ExecBase *, SysBase, 63, Exec)
#define ReplyMsg(message) \
    __AROS_LC1(void, ReplyMsg, \
    __AROS_LCA(struct Message *, message, A1), \
    struct ExecBase *, SysBase, 63, Exec)

__AROS_LP1(void, SendIO,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 77, Exec)
#define SendIO(iORequest) \
    __AROS_LC1(void, SendIO, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 77, Exec)

__AROS_LP2(ULONG, SetExcept,
    __AROS_LPA(ULONG, newSignals, D0),
    __AROS_LPA(ULONG, signalSet,  D1),
    struct ExecBase *, SysBase, 52, Exec)
#define SetExcept(newSignals, signalSet) \
    __AROS_LC2(ULONG, SetExcept, \
    __AROS_LCA(ULONG, newSignals, D0), \
    __AROS_LCA(ULONG, signalSet,  D1), \
    struct ExecBase *, SysBase, 52, Exec)

__AROS_LP3(APTR, SetFunction,
    __AROS_LPA(struct Library *, library,     A1),
    __AROS_LPA(LONG,             funcOffset,  A0),
    __AROS_LPA(APTR,             newFunction, D0),
    struct ExecBase *, SysBase, 70, Exec)
#define SetFunction(library, funcOffset, newFunction) \
    __AROS_LC3(APTR, SetFunction, \
    __AROS_LCA(struct Library *, library,     A1), \
    __AROS_LCA(LONG,             funcOffset,  A0), \
    __AROS_LCA(APTR,             newFunction, D0), \
    struct ExecBase *, SysBase, 70, Exec)

__AROS_LP2(ULONG, SetSignal,
    __AROS_LPA(ULONG, newSignals, D0),
    __AROS_LPA(ULONG, signalSet,  D1),
    struct ExecBase *, SysBase, 51, Exec)
#define SetSignal(newSignals, signalSet) \
    __AROS_LC2(ULONG, SetSignal, \
    __AROS_LCA(ULONG, newSignals, D0), \
    __AROS_LCA(ULONG, signalSet,  D1), \
    struct ExecBase *, SysBase, 51, Exec)

__AROS_LP2(BYTE, SetTaskPri,
    __AROS_LPA(struct Task *, task,      A1),
    __AROS_LPA(LONG,          priority,  D0),
    struct ExecBase *, SysBase, 50, Exec)
#define SetTaskPri(task, priority) \
    __AROS_LC2(BYTE, SetTaskPri, \
    __AROS_LCA(struct Task *, task,      A1), \
    __AROS_LCA(LONG,          priority,  D0), \
    struct ExecBase *, SysBase, 50, Exec)

__AROS_LP2(void, Signal,
    __AROS_LPA(struct Task *,     task,      A1),
    __AROS_LPA(ULONG,             signalSet, D0),
    struct ExecBase *, SysBase, 54, Exec)
#define Signal(task, signalSet) \
    __AROS_LC2(void, Signal, \
    __AROS_LCA(struct Task *,     task,      A1), \
    __AROS_LCA(ULONG,             signalSet, D0), \
    struct ExecBase *, SysBase, 54, Exec)

__AROS_LP1(void, SumLibrary,
    __AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 71, Exec)
#define SumLibrary(library) \
    __AROS_LC1(void, SumLibrary, \
    __AROS_LCA(struct Library *, library,A1), \
    struct ExecBase *, SysBase, 71, Exec)

__AROS_LP1(ULONG, TypeOfMem,
    __AROS_LPA(APTR, address, A1),
    struct ExecBase *, SysBase, 89, Exec)
#define TypeOfMem(address) \
    __AROS_LC1(ULONG, TypeOfMem, \
    __AROS_LCA(APTR, address, A1), \
    struct ExecBase *, SysBase, 89, Exec)

__AROS_LP2(void, Vacate,
    __AROS_LPA(struct SignalSemaphore  *, sigSem, A0),
    __AROS_LPA(struct SemaphoreMessage *, bidMsg, A1),
    struct ExecBase *, SysBase, 91, Exec)
#define Vacate(sigSem, bidMsg) \
    __AROS_LC2(void, Vacate, \
    __AROS_LCA(struct SignalSemaphore  *, sigSem, A0), \
    __AROS_LCA(struct SemaphoreMessage *, bidMsg, A1), \
    struct ExecBase *, SysBase, 91, Exec)

__AROS_LP1(ULONG, Wait,
    __AROS_LPA(ULONG, signalSet, D0),
    struct ExecBase *, SysBase, 53, Exec)
#define Wait(signalSet) \
    __AROS_LC1(ULONG, Wait, \
    __AROS_LCA(ULONG, signalSet, D0), \
    struct ExecBase *, SysBase, 53, Exec)

__AROS_LP1(BYTE, WaitIO,
    __AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 79, Exec)
#define WaitIO(iORequest) \
    __AROS_LC1(BYTE, WaitIO, \
    __AROS_LCA(struct IORequest *, iORequest, A1), \
    struct ExecBase *, SysBase, 79, Exec)

__AROS_LP1(struct Message *, WaitPort,
    __AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 64, Exec)
#define WaitPort(port) \
    __AROS_LC1(struct Message *, WaitPort, \
    __AROS_LCA(struct MsgPort *, port, A0), \
    struct ExecBase *, SysBase, 64, Exec)


#endif /* CLIB_EXEC_PROTOS_H */
