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
AROS_LP3(void, CacheClearE,
    AROS_LPA(APTR,  address, A0),
    AROS_LPA(ULONG, length,  D0),
    AROS_LPA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)

AROS_LP0(void, CacheClearU,
    struct ExecBase *, SysBase, 106, Exec)

AROS_LP0(void, CacheControl,
    struct ExecBase *, SysBase, 108, Exec)

AROS_LP0(void, CachePostDMA,
    struct ExecBase *, SysBase, 128, Exec)

AROS_LP0(void, CachePreDMA,
    struct ExecBase *, SysBase, 127, Exec)

AROS_LP0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)

AROS_LP0(void, Dispatch,
    struct ExecBase *, SysBase, 10, Exec)

AROS_LP0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)

AROS_LP0(void, Exception,
    struct ExecBase *, SysBase, 11, Exec)

AROS_LP0(void, Forbid,
    struct ExecBase *, SysBase, 22, Exec)

AROS_LP0(void, GetCC,
    struct ExecBase *, SysBase, 88, Exec)

AROS_LP0(void, Permit,
    struct ExecBase *, SysBase, 23, Exec)

AROS_LP0(void, SetSR,
    struct ExecBase *, SysBase, 24, Exec)

AROS_LP0(void, StackSwap,
    struct ExecBase *, SysBase, 122, Exec)

AROS_LP0(void, SuperState,
    struct ExecBase *, SysBase, 25, Exec)

AROS_LP1(void, Supervisor,
    AROS_LPA(ULONG_FUNC, userFunction, A5),
    struct ExecBase *, SysBase, 5, Exec)

AROS_LP0(void, Switch,
    struct ExecBase *, SysBase, 9, Exec)

AROS_LP0(void, UserState,
    struct ExecBase *, SysBase, 26, Exec)

AROS_LP3I(APTR, PrepareContext,
    AROS_LPA(APTR, stackPointer, A0),
    AROS_LPA(APTR, entryPoint,   A1),
    AROS_LPA(APTR, fallBack,     A2),
    struct ExecBase *, SysBase, 6, Exec)

AROS_LP1I(LONG, AbortIO,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 80, Exec)

AROS_LP1(void, AddDevice,
    AROS_LPA(struct Device *, device,A1),
    struct ExecBase *, SysBase, 72, Exec)

AROS_LP2I(void, AddHead,
    AROS_LPA(struct List *, list, A0),
    AROS_LPA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 40, Exec)

AROS_LP2(void, AddIntServer,
    AROS_LPA(ULONG,              intNumber, D0),
    AROS_LPA(struct Interrupt *, interrupt, A1),
    struct ExecBase *, SysBase, 28, Exec)

AROS_LP1(void, AddLibrary,
    AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 66, Exec)

AROS_LP1(void, AddMemHandler,
    AROS_LPA(struct Interrupt *, memHandler, A1),
    struct ExecBase *, SysBase, 129, Exec)

AROS_LP5(void, AddMemList,
    AROS_LPA(ULONG,  size,       D0),
    AROS_LPA(ULONG,  attributes, D1),
    AROS_LPA(LONG,   pri,        D2),
    AROS_LPA(APTR,   base,       A0),
    AROS_LPA(STRPTR, name,       A1),
    struct ExecBase *, SysBase, 103, Exec)

AROS_LP1(void, AddPort,
    AROS_LPA(struct MsgPort *, port, A1),
    struct ExecBase *, SysBase, 59, Exec)

AROS_LP1(void, AddResource,
    AROS_LPA(APTR, resource, A1),
    struct ExecBase *, SysBase, 81, Exec)

AROS_LP1(void, AddSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A1),
    struct ExecBase *, SysBase, 100, Exec)

AROS_LP2I(void, AddTail,
    AROS_LPA(struct List *, list, A0),
    AROS_LPA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 41, Exec)

AROS_LP3(APTR, AddTask,
    AROS_LPA(struct Task *,     task,      A1),
    AROS_LPA(APTR,              initialPC, A2),
    AROS_LPA(APTR,              finalPC,   A3),
    struct ExecBase *, SysBase, 47, Exec)

AROS_LP1(void, Alert,
    AROS_LPA(ULONG, alertNum, D7),
    struct ExecBase *, SysBase, 18, Exec)

AROS_LP2(APTR, AllocAbs,
    AROS_LPA(ULONG, byteSize, D0),
    AROS_LPA(APTR,  location, D1),
    struct ExecBase *, SysBase, 34, Exec)

AROS_LP2(APTR, Allocate,
    AROS_LPA(struct MemHeader *, freeList, A0),
    AROS_LPA(ULONG,              byteSize, D0),
    struct ExecBase *, SysBase, 31, Exec)

AROS_LP1(struct MemList *, AllocEntry,
    AROS_LPA(struct MemList *, entry, A0),
    struct ExecBase *, SysBase, 37, Exec)

AROS_LP2(APTR, AllocMem,
    AROS_LPA(ULONG, byteSize,     D0),
    AROS_LPA(ULONG, requirements, D1),
    struct ExecBase *, SysBase, 33, Exec)

AROS_LP2(APTR, AllocPooled,
    AROS_LPA(APTR,  poolHeader, A0),
    AROS_LPA(ULONG, memSize,    D0),
    struct ExecBase *, SysBase, 118, Exec)

AROS_LP1(BYTE, AllocSignal,
    AROS_LPA(LONG, signalNum, D0),
    struct ExecBase *, SysBase, 55, Exec)

AROS_LP1(LONG, AllocTrap,
    AROS_LPA(long, trapNum, D0),
    struct ExecBase *, SysBase, 57, Exec)

AROS_LP2(APTR, AllocVec,
    AROS_LPA(ULONG, byteSize,     D0),
    AROS_LPA(ULONG, requirements, D1),
    struct ExecBase *, SysBase, 114, Exec)

AROS_LP1(ULONG, AttemptSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 96, Exec)

AROS_LP1(ULONG, AttemptSemaphoreShared,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 120, Exec)

AROS_LP1(ULONG, AvailMem,
    AROS_LPA(ULONG, attributes, D1),
    struct ExecBase *, SysBase, 36, Exec)

AROS_LP1(void, Cause,
    AROS_LPA(struct Interrupt *, interrupt, A1),
    struct ExecBase *, SysBase, 30, Exec)

AROS_LP1I(struct IORequest *, CheckIO,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 78, Exec)

AROS_LP1(void, ChildFree,
    AROS_LPA(APTR, tid, D0),
    struct ExecBase *, SysBase, 123, Exec)

AROS_LP1(void, ChildOrphan,
    AROS_LPA(APTR, tid, D0),
    struct ExecBase *, SysBase, 124, Exec)

AROS_LP1(void, ChildStatus,
    AROS_LPA(APTR, tid, D0),
    struct ExecBase *, SysBase, 125, Exec)

AROS_LP1(void, ChildWait,
    AROS_LPA(APTR, tid, D0),
    struct ExecBase *, SysBase, 126, Exec)

AROS_LP1(void, CloseDevice,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 75, Exec)

AROS_LP1(void, CloseLibrary,
    AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 69, Exec)

AROS_LP0(void, ColdReboot,
    struct ExecBase *, SysBase, 121, Exec)

AROS_LP3I(void, CopyMem,
    AROS_LPA(APTR,  source, A0),
    AROS_LPA(APTR,  dest,   A1),
    AROS_LPA(ULONG, size,   D0),
    struct ExecBase *, SysBase, 104, Exec)

AROS_LP3I(void, CopyMemQuick,
    AROS_LPA(APTR,  source, A0),
    AROS_LPA(APTR,  dest,   A1),
    AROS_LPA(ULONG, size,   D0),
    struct ExecBase *, SysBase, 105, Exec)

AROS_LP2(struct IORequest *, CreateIORequest,
    AROS_LPA(struct MsgPort *, ioReplyPort, A0),
    AROS_LPA(ULONG,            size,        D0),
    struct ExecBase *, SysBase, 109, Exec)

AROS_LP0(struct MsgPort *, CreateMsgPort,
    struct ExecBase *, SysBase, 111, Exec)

AROS_LP3(APTR, CreatePool,
    AROS_LPA(ULONG, requirements, D0),
    AROS_LPA(ULONG, puddleSize,   D1),
    AROS_LPA(ULONG, threshSize,   D2),
    struct ExecBase *, SysBase, 116, Exec)

AROS_LP3(void, Deallocate,
    AROS_LPA(struct MemHeader *, freeList,    A0),
    AROS_LPA(APTR,               memoryBlock, A1),
    AROS_LPA(ULONG,              byteSize,    D0),
    struct ExecBase *, SysBase, 32, Exec)

AROS_LP1(void, Debug,
    AROS_LPA(unsigned long, flags, D0),
    struct ExecBase *, SysBase, 19, Exec)

AROS_LP1(void, DeleteIORequest,
    AROS_LPA(struct IORequest *, iorequest, A0),
    struct ExecBase *, SysBase, 110, Exec)

AROS_LP1(void, DeleteMsgPort,
    AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 112, Exec)

AROS_LP1(void, DeletePool,
    AROS_LPA(APTR, poolHeader, A0),
    struct ExecBase *, SysBase, 117, Exec)

AROS_LP1(BYTE, DoIO,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 76, Exec)

AROS_LP2I(void, Enqueue,
    AROS_LPA(struct List *, list, A0),
    AROS_LPA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 45, Exec)

AROS_LP2I(struct Node *, FindName,
    AROS_LPA(struct List *, list, A0),
    AROS_LPA(UBYTE       *, name, A1),
    struct ExecBase *, SysBase, 46, Exec)

AROS_LP1(struct MsgPort *, FindPort,
    AROS_LPA(STRPTR, name,A1),
    struct ExecBase *, SysBase, 65, Exec)

AROS_LP1(struct Resident *, FindResident,
    AROS_LPA(UBYTE *, name, A1),
    struct ExecBase *, SysBase, 16, Exec)

AROS_LP1(struct SignalSemaphore *, FindSemaphore,
    AROS_LPA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 99, Exec)

AROS_LP1(struct Task *, FindTask,
    AROS_LPA(STRPTR, name, A1),
    struct ExecBase *, SysBase, 49, Exec)

AROS_LP1(void, FreeEntry,
    AROS_LPA(struct MemList *, entry,A0),
    struct ExecBase *, SysBase, 38, Exec)

AROS_LP2(void, FreeMem,
    AROS_LPA(APTR,  memoryBlock, A1),
    AROS_LPA(ULONG, byteSize,    D0),
    struct ExecBase *, SysBase, 35, Exec)

AROS_LP3(void,FreePooled,
    AROS_LPA(APTR, poolHeader,A0),
    AROS_LPA(APTR, memory,    A1),
    AROS_LPA(ULONG,memSize,   D0),
    struct ExecBase *, SysBase, 119, Exec)

AROS_LP1(void, FreeSignal,
    AROS_LPA(LONG, signalNum, D0),
    struct ExecBase *, SysBase, 56, Exec)

AROS_LP1(void, FreeTrap,
    AROS_LPA(long, trapNum, D0),
    struct ExecBase *, SysBase, 58, Exec)

AROS_LP1(void, FreeVec,
    AROS_LPA(APTR, memoryBlock, A1),
    struct ExecBase *, SysBase, 115, Exec)

AROS_LP1(struct Message *, GetMsg,
    AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 62, Exec)

AROS_LP2(void, InitCode,
    AROS_LPA(unsigned long, startClass, D0),
    AROS_LPA(unsigned long, version, D1),
    struct ExecBase *, SysBase, 12, Exec)

AROS_LP2(APTR, InitResident,
    AROS_LPA(struct Resident *, resident, A1),
    AROS_LPA(BPTR,              segList,  D1),
    struct ExecBase *, SysBase, 17, Exec)

AROS_LP1I(void, InitSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 93, Exec)

AROS_LP3(void, InitStruct,
    AROS_LPA(APTR,  initTable, A1),
    AROS_LPA(APTR,  memory,    A2),
    AROS_LPA(ULONG, size,      D0),
    struct ExecBase *, SysBase, 13, Exec)

AROS_LP3I(void, Insert,
    AROS_LPA(struct List *, list, A0),
    AROS_LPA(struct Node *, node, A1),
    AROS_LPA(struct Node *, pred, A2),
    struct ExecBase *, SysBase, 39, Exec)

AROS_LP3(ULONG, MakeFunctions,
    AROS_LPA(APTR, target,        A0),
    AROS_LPA(APTR, functionArray, A1),
    AROS_LPA(APTR, funcDispBase,  A2),
    struct ExecBase *, SysBase, 15, Exec)

AROS_LP5(struct Library *, MakeLibrary,
    AROS_LPA(APTR,       funcInit,   A0),
    AROS_LPA(APTR,       structInit, A1),
    AROS_LPA(ULONG_FUNC, libInit,    A2),
    AROS_LPA(ULONG,      dataSize,   D0),
    AROS_LPA(BPTR,       segList,    D1),
    struct ExecBase *, SysBase, 14, Exec)

AROS_LP1(ULONG, ObtainQuickVector,
    AROS_LPA(APTR, interruptCode, A0),
    struct ExecBase *, SysBase, 131, Exec)

AROS_LP1(void, ObtainSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 94, Exec)

AROS_LP1(void, ObtainSemaphoreList,
    AROS_LPA(struct List *, sigSem, A0),
    struct ExecBase *, SysBase, 97, Exec)

AROS_LP1(void, ObtainSemaphoreShared,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 113, Exec)

AROS_LP1(struct Library *, OldOpenLibrary,
    AROS_LPA(UBYTE *, libName, A1),
    struct ExecBase *, SysBase, 68, Exec)

AROS_LP4(BYTE, OpenDevice,
    AROS_LPA(STRPTR,             devName,    A0),
    AROS_LPA(ULONG,              unitNumber, D0),
    AROS_LPA(struct IORequest *, iORequest,  A1),
    AROS_LPA(ULONG,              flags,      D1),
    struct ExecBase *, SysBase, 74, Exec)

AROS_LP2(struct Library *, OpenLibrary,
    AROS_LPA(UBYTE *, libName, A1),
    AROS_LPA(ULONG,   version, D0),
    struct ExecBase *, SysBase, 92, Exec)

AROS_LP1(APTR, OpenResource,
    AROS_LPA(STRPTR, resName, A1),
    struct ExecBase *, SysBase, 83, Exec)

AROS_LP2(ULONG, Procure,
    AROS_LPA(struct SignalSemaphore  *, sigSem, A0),
    AROS_LPA(struct SemaphoreMessage *, bidMsg, A1),
    struct ExecBase *, SysBase, 90, Exec)

AROS_LP2(void, PutMsg,
    AROS_LPA(struct MsgPort *, port,    A0),
    AROS_LPA(struct Message *, message, A1),
    struct ExecBase *, SysBase, 61, Exec)

AROS_LP4I(APTR,RawDoFmt,
    AROS_LPA(STRPTR,    FormatString, A0),
    AROS_LPA(APTR,      DataStream,   A1),
    AROS_LPA(VOID_FUNC, PutChProc,    A2),
    AROS_LPA(APTR,      PutChData,    A3),
    struct ExecBase *, SysBase, 87, Exec)

AROS_LP1(ULONG, RawPutChar,
    AROS_LPA(ULONG, character, D0),
    struct ExecBase *, SysBase, 86, Exec)

AROS_LP1(void, ReleaseSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 95, Exec)

AROS_LP1(void, ReleaseSemaphoreList,
    AROS_LPA(struct List *, sigSem, A0),
    struct ExecBase *, SysBase, 98, Exec)

AROS_LP1(void, RemDevice,
    AROS_LPA(struct Device *, device,A1),
    struct ExecBase *, SysBase, 73, Exec)

AROS_LP1I(struct Node *, RemHead,
    AROS_LPA(struct List *, list, A0),
    struct ExecBase *, SysBase, 43, Exec)

AROS_LP2(void, RemIntServer,
    AROS_LPA(ULONG,              intNumber, D0),
    AROS_LPA(struct Interrupt *, interrupt, A1),
    struct ExecBase *, SysBase, 29, Exec)

AROS_LP1(void, RemLibrary,
    AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 67, Exec)

AROS_LP1(void, RemMemHandler,
    AROS_LPA(struct Interrupt *, memHandler, A1),
    struct ExecBase *, SysBase, 130, Exec)

AROS_LP1I(void, Remove,
    AROS_LPA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 42, Exec)

AROS_LP1(void, RemPort,
    AROS_LPA(struct MsgPort *, port, A1),
    struct ExecBase *, SysBase, 60, Exec)

AROS_LP1(void, RemResource,
    AROS_LPA(APTR, resource,A1),
    struct ExecBase *, SysBase, 82, Exec)

AROS_LP1(void, RemSemaphore,
    AROS_LPA(struct SignalSemaphore *, sigSem, A0),
    struct ExecBase *, SysBase, 101, Exec)

AROS_LP1I(struct Node *, RemTail,
    AROS_LPA(struct List *, list, A0),
    struct ExecBase *, SysBase, 44, Exec)

AROS_LP1(void, RemTask,
    AROS_LPA(struct Task *,     task, A1),
    struct ExecBase *, SysBase, 48, Exec)

AROS_LP1(void, ReplyMsg,
    AROS_LPA(struct Message *, message, A1),
    struct ExecBase *, SysBase, 63, Exec)

AROS_LP1(void, SendIO,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 77, Exec)

AROS_LP2(ULONG, SetExcept,
    AROS_LPA(ULONG, newSignals, D0),
    AROS_LPA(ULONG, signalSet,  D1),
    struct ExecBase *, SysBase, 52, Exec)

AROS_LP3(APTR, SetFunction,
    AROS_LPA(struct Library *, library,     A1),
    AROS_LPA(LONG,             funcOffset,  A0),
    AROS_LPA(APTR,             newFunction, D0),
    struct ExecBase *, SysBase, 70, Exec)

AROS_LP2(struct Interrupt *, SetIntVector,
    AROS_LPA(ULONG,              intNumber, D0),
    AROS_LPA(struct Interrupt *, interrupt, A1),
    struct ExecBase *, SysBase, 27, Exec)

AROS_LP2(ULONG, SetSignal,
    AROS_LPA(ULONG, newSignals, D0),
    AROS_LPA(ULONG, signalSet,  D1),
    struct ExecBase *, SysBase, 51, Exec)

AROS_LP2(BYTE, SetTaskPri,
    AROS_LPA(struct Task *, task,      A1),
    AROS_LPA(LONG,          priority,  D0),
    struct ExecBase *, SysBase, 50, Exec)

AROS_LP2(void, Signal,
    AROS_LPA(struct Task *,     task,      A1),
    AROS_LPA(ULONG,             signalSet, D0),
    struct ExecBase *, SysBase, 54, Exec)

AROS_LP0(ULONG, SumKickData,
    struct ExecBase *, SysBase, 102, Exec)

AROS_LP1(void, SumLibrary,
    AROS_LPA(struct Library *, library,A1),
    struct ExecBase *, SysBase, 71, Exec)

AROS_LP1(ULONG, TypeOfMem,
    AROS_LPA(APTR, address, A1),
    struct ExecBase *, SysBase, 89, Exec)

AROS_LP2(void, Vacate,
    AROS_LPA(struct SignalSemaphore  *, sigSem, A0),
    AROS_LPA(struct SemaphoreMessage *, bidMsg, A1),
    struct ExecBase *, SysBase, 91, Exec)

AROS_LP1(ULONG, Wait,
    AROS_LPA(ULONG, signalSet, D0),
    struct ExecBase *, SysBase, 53, Exec)

AROS_LP1(BYTE, WaitIO,
    AROS_LPA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 79, Exec)

AROS_LP1(struct Message *, WaitPort,
    AROS_LPA(struct MsgPort *, port, A0),
    struct ExecBase *, SysBase, 64, Exec)


#endif /* CLIB_EXEC_PROTOS_H */
