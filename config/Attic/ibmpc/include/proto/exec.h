/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    
    Desc: IBMPC prototypes.
    Lang: English
*/

#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
#endif

#define AbortIO(ioRequest) \
	LP1NR(0x140, AbortIO, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define AddDevice(device) \
	LP1NR(0x120, AddDevice, struct Device *, device, \
	, EXEC_BASE_NAME)

#define AddHead(list, node) \
	LP2NR(0xa0, AddHead, struct List *, list, struct Node *, node, \
	, EXEC_BASE_NAME)

#define AddIntServer(intNumber, interrupt) \
	LP2NR(0x70, AddIntServer, long, intNumber, struct Interrupt *, interrupt, \
	, EXEC_BASE_NAME)

#define AddLibrary(library) \
	LP1NR(0x108, AddLibrary, struct Library *, library, \
	, EXEC_BASE_NAME)

#define AddMemHandler(memhand) \
	LP1NR(0x204, AddMemHandler, struct Interrupt *, memhand, \
	, EXEC_BASE_NAME)

#define AddMemList(size, attributes, pri, base, name) \
	LP5NR(0x19c, AddMemList, unsigned long, size, unsigned long, attributes, long, pri, APTR, base, UBYTE *, name, \
	, EXEC_BASE_NAME)

#define AddPort(port) \
	LP1NR(0xec, AddPort, struct MsgPort *, port, \
	, EXEC_BASE_NAME)

#define AddResource(resource) \
	LP1NR(0x144, AddResource, APTR, resource, \
	, EXEC_BASE_NAME)

#define AddSemaphore(sigSem) \
	LP1NR(0x190, AddSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define AddTail(list, node) \
	LP2NR(0xa4, AddTail, struct List *, list, struct Node *, node, \
	, EXEC_BASE_NAME)

#define AddTask(task, initPC, finalPC) \
	LP3(0xbc, APTR, AddTask, struct Task *, task, APTR, initPC, APTR, finalPC, \
	, EXEC_BASE_NAME)

#define Alert(alertNum) \
	LP1NR(0x48, Alert, unsigned long, alertNum, \
	, EXEC_BASE_NAME)

#define AllocAbs(byteSize, location) \
	LP2(0x88, APTR, AllocAbs, unsigned long, byteSize, APTR, location, \
	, EXEC_BASE_NAME)

#define AllocEntry(entry) \
	LP1(0x94, struct MemList *, AllocEntry, struct MemList *, entry, \
	, EXEC_BASE_NAME)

#define AllocMem(byteSize, requirements) \
	LP2(0x84, APTR, AllocMem, unsigned long, byteSize, unsigned long, requirements, \
	, EXEC_BASE_NAME)

#define AllocPooled(poolHeader, memSize) \
	LP2(0x1d8, APTR, AllocPooled, APTR, poolHeader, unsigned long, memSize, \
	, EXEC_BASE_NAME)

#define AllocSignal(signalNum) \
	LP1(0xdc, BYTE, AllocSignal, long, signalNum, \
	, EXEC_BASE_NAME)

#define AllocTrap(trapNum) \
	LP1(0xe4, LONG, AllocTrap, long, trapNum, \
	, EXEC_BASE_NAME)

#define AllocVec(byteSize, requirements) \
	LP2(0x1c8, APTR, AllocVec, unsigned long, byteSize, unsigned long, requirements, \
	, EXEC_BASE_NAME)

#define Allocate(freeList, byteSize) \
	LP2(0x7c, APTR, Allocate, struct MemHeader *, freeList, unsigned long, byteSize, \
	, EXEC_BASE_NAME)

#define AttemptSemaphore(sigSem) \
	LP1(0x180, ULONG, AttemptSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define AttemptSemaphoreShared(sigSem) \
	LP1(0x1e0, ULONG, AttemptSemaphoreShared, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define AvailMem(requirements) \
	LP1(0x90, ULONG, AvailMem, unsigned long, requirements, \
	, EXEC_BASE_NAME)

#define CacheClearE(address, length, caches) \
	LP3NR(0x1ac, CacheClearE, APTR, address, unsigned long, length, unsigned long, caches, \
	, EXEC_BASE_NAME)

#define CacheClearU() \
	LP0NR(0x1a8, CacheClearU, \
	, EXEC_BASE_NAME)

#define CacheControl(cacheBits, cacheMask) \
	LP2(0x1b0, ULONG, CacheControl, unsigned long, cacheBits, unsigned long, cacheMask, \
	, EXEC_BASE_NAME)

#define CachePostDMA(address, length, flags) \
	LP3NR(0x200, CachePostDMA, APTR, address, ULONG *, length, unsigned long, flags, \
	, EXEC_BASE_NAME)

#define CachePreDMA(address, length, flags) \
	LP3(0x1fc, APTR, CachePreDMA, APTR, address, ULONG *, length, unsigned long, flags, \
	, EXEC_BASE_NAME)

#define Cause(interrupt) \
	LP1NR(0x78, Cause, struct Interrupt *, interrupt, \
	, EXEC_BASE_NAME)

#define CheckIO(ioRequest) \
	LP1(0x138, struct IORequest *, CheckIO, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define ChildFree(tid) \
	LP1NR(0x1ec, ChildFree, APTR, tid, \
	, EXEC_BASE_NAME)

#define ChildOrphan(tid) \
	LP1NR(0x1f0, ChildOrphan, APTR, tid, \
	, EXEC_BASE_NAME)

#define ChildStatus(tid) \
	LP1NR(0x1f4, ChildStatus, APTR, tid, \
	, EXEC_BASE_NAME)

#define ChildWait(tid) \
	LP1NR(0x1f8, ChildWait, APTR, tid, \
	, EXEC_BASE_NAME)

#define CloseDevice(ioRequest) \
	LP1NR(0x12c, CloseDevice, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define CloseLibrary(library) \
	LP1NR(0x114, CloseLibrary, struct Library *, library, \
	, EXEC_BASE_NAME)

#define ColdReboot() \
	LP0NR(0x1e4, ColdReboot, \
	, EXEC_BASE_NAME)

#define CopyMem(source, dest, size) \
	LP3NR(0x1a0, CopyMem, APTR, source, APTR, dest, unsigned long, size, \
	, EXEC_BASE_NAME)

#define CopyMemQuick(source, dest, size) \
	LP3NR(0x1a4, CopyMemQuick, APTR, source, APTR, dest, unsigned long, size, \
	, EXEC_BASE_NAME)

#define CreateIORequest(port, size) \
	LP2(0x1b4, APTR, CreateIORequest, struct MsgPort *, port, unsigned long, size, \
	, EXEC_BASE_NAME)

#define CreateMsgPort() \
	LP0(0x1bc, struct MsgPort *, CreateMsgPort, \
	, EXEC_BASE_NAME)

#define CreatePool(requirements, puddleSize, threshSize) \
	LP3(0x1d0, APTR, CreatePool, unsigned long, requirements, unsigned long, puddleSize, unsigned long, threshSize, \
	, EXEC_BASE_NAME)

#define Deallocate(freeList, memoryBlock, byteSize) \
	LP3NR(0x80, Deallocate, struct MemHeader *, freeList, APTR, memoryBlock, unsigned long, byteSize, \
	, EXEC_BASE_NAME)

#define Debug(flags) \
	LP1NR(0x4c, Debug, unsigned long, flags, \
	, EXEC_BASE_NAME)

#define DeleteIORequest(iorequest) \
	LP1NR(0x1b8, DeleteIORequest, APTR, iorequest, \
	, EXEC_BASE_NAME)

#define DeleteMsgPort(port) \
	LP1NR(0x1c0, DeleteMsgPort, struct MsgPort *, port, \
	, EXEC_BASE_NAME)

#define DeletePool(poolHeader) \
	LP1NR(0x1d4, DeletePool, APTR, poolHeader, \
	, EXEC_BASE_NAME)

#define Disable() \
	LP0NR(0x50, Disable, \
	, EXEC_BASE_NAME)

#define DoIO(ioRequest) \
	LP1(0x130, BYTE, DoIO, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define Enable() \
	LP0NR(0x54, Enable, \
	, EXEC_BASE_NAME)

#define Enqueue(list, node) \
	LP2NR(0xb4, Enqueue, struct List *, list, struct Node *, node, \
	, EXEC_BASE_NAME)

#define FindName(list, name) \
	LP2(0xb8, struct Node *, FindName, struct List *, list, UBYTE *, name, \
	, EXEC_BASE_NAME)

#define FindPort(name) \
	LP1(0x104, struct MsgPort *, FindPort, UBYTE *, name, \
	, EXEC_BASE_NAME)

#define FindResident(name) \
	LP1(0x40, struct Resident *, FindResident, UBYTE *, name, \
	, EXEC_BASE_NAME)

#define FindSemaphore(sigSem) \
	LP1(0x18c, struct SignalSemaphore *, FindSemaphore, UBYTE *, sigSem, \
	, EXEC_BASE_NAME)

#define FindTask(name) \
	LP1(0xc4, struct Task *, FindTask, UBYTE *, name, \
	, EXEC_BASE_NAME)

#define Forbid() \
	LP0NR(0x58, Forbid, \
	, EXEC_BASE_NAME)

#define FreeEntry(entry) \
	LP1NR(0x98, FreeEntry, struct MemList *, entry, \
	, EXEC_BASE_NAME)

#define FreeMem(memoryBlock, byteSize) \
	LP2NR(0x8c, FreeMem, APTR, memoryBlock, unsigned long, byteSize, \
	, EXEC_BASE_NAME)

#define FreePooled(poolHeader, memory, memSize) \
	LP3NR(0x1dc, FreePooled, APTR, poolHeader, APTR, memory, unsigned long, memSize, \
	, EXEC_BASE_NAME)

#define FreeSignal(signalNum) \
	LP1NR(0xe0, FreeSignal, long, signalNum, \
	, EXEC_BASE_NAME)

#define FreeTrap(trapNum) \
	LP1NR(0xe8, FreeTrap, long, trapNum, \
	, EXEC_BASE_NAME)

#define FreeVec(memoryBlock) \
	LP1NR(0x1cc, FreeVec, APTR, memoryBlock, \
	, EXEC_BASE_NAME)

#define GetCC() \
	LP0(0x160, ULONG, GetCC, \
	, EXEC_BASE_NAME)

#define GetMsg(port) \
	LP1(0xf8, struct Message *, GetMsg, struct MsgPort *, port, \
	, EXEC_BASE_NAME)

#define InitCode(startClass, version) \
	LP2NR(0x30, InitCode, unsigned long, startClass, unsigned long, version, \
	, EXEC_BASE_NAME)

#define InitResident(resident, segList) \
	LP2(0x44, APTR, InitResident, struct Resident *, resident, unsigned long, segList, \
	, EXEC_BASE_NAME)

#define InitSemaphore(sigSem) \
	LP1NR(0x174, InitSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define InitStruct(initTable, memory, size) \
	LP3NR(0x34, InitStruct, APTR, initTable, APTR, memory, unsigned long, size, \
	, EXEC_BASE_NAME)

#define Insert(list, node, pred) \
	LP3NR(0x9c, Insert, struct List *, list, struct Node *, node, struct Node *, pred, \
	, EXEC_BASE_NAME)

#define MakeFunctions(target, functionArray, funcDispBase) \
	LP3NR(0x3c, MakeFunctions, APTR, target, APTR, functionArray, APTR, funcDispBase, \
	, EXEC_BASE_NAME)

#define MakeLibrary(funcInit, structInit, libInit, dataSize, segList) \
	LP5FP(0x38, struct Library *, MakeLibrary, APTR, funcInit, APTR, structInit, __fpt, libInit, unsigned long, dataSize, APTR, segList, \
	, EXEC_BASE_NAME, unsigned long (*__fpt)())

#define ObtainQuickVector(interruptCode) \
	LP1(0x20c, ULONG, ObtainQuickVector, APTR, interruptCode, \
	, EXEC_BASE_NAME)

#define ObtainSemaphore(sigSem) \
	LP1NR(0x178, ObtainSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define ObtainSemaphoreList(sigSem) \
	LP1NR(0x184, ObtainSemaphoreList, struct List *, sigSem, \
	, EXEC_BASE_NAME)

#define ObtainSemaphoreShared(sigSem) \
	LP1NR(0x1c4, ObtainSemaphoreShared, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define OldOpenLibrary(libName) \
	LP1(0x110, struct Library *, OldOpenLibrary, UBYTE *, libName, \
	, EXEC_BASE_NAME)

#define OpenDevice(devName, unit, ioRequest, flags) \
	LP4(0x128, BYTE, OpenDevice, UBYTE *, devName, unsigned long, unit, struct IORequest *, ioRequest, unsigned long, flags, \
	, EXEC_BASE_NAME)

#define OpenLibrary(libName, version) \
	LP2(0x170, struct Library *, OpenLibrary, UBYTE *, libName, unsigned long, version, \
	, EXEC_BASE_NAME)

#define OpenResource(resName) \
	LP1(0x14c, APTR, OpenResource, UBYTE *, resName, \
	, EXEC_BASE_NAME)

#define Permit() \
	LP0NR(0x5c, Permit, \
	, EXEC_BASE_NAME)

#define Procure(sigSem, bidMsg) \
	LP2(0x168, ULONG, Procure, struct SignalSemaphore *, sigSem, struct SemaphoreMessage *, bidMsg, \
	, EXEC_BASE_NAME)

#define PutMsg(port, message) \
	LP2NR(0xf4, PutMsg, struct MsgPort *, port, struct Message *, message, \
	, EXEC_BASE_NAME)

#define RawDoFmt(formatString, dataStream, putChProc, putChData) \
	LP4FP(0x15c, APTR, RawDoFmt, UBYTE *, formatString, APTR, dataStream, __fpt, putChProc, APTR, putChData, \
	, EXEC_BASE_NAME, void (*__fpt)())

#define RawPutChar(char) \
	LP1NR(0x158, RawPutChar, UBYTE, char, \
	, EXEC_BASE_NAME)

#define ReleaseSemaphore(sigSem) \
	LP1NR(0x17c, ReleaseSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define ReleaseSemaphoreList(sigSem) \
	LP1NR(0x188, ReleaseSemaphoreList, struct List *, sigSem, \
	, EXEC_BASE_NAME)

#define RemDevice(device) \
	LP1NR(0x124, RemDevice, struct Device *, device, \
	, EXEC_BASE_NAME)

#define RemHead(list) \
	LP1(0xac, struct Node *, RemHead, struct List *, list, \
	, EXEC_BASE_NAME)

#define RemIntServer(intNumber, interrupt) \
	LP2NR(0x74, RemIntServer, long, intNumber, struct Interrupt *, interrupt, \
	, EXEC_BASE_NAME)

#define RemLibrary(library) \
	LP1NR(0x10c, RemLibrary, struct Library *, library, \
	, EXEC_BASE_NAME)

#define RemMemHandler(memhand) \
	LP1NR(0x208, RemMemHandler, struct Interrupt *, memhand, \
	, EXEC_BASE_NAME)

#define RemPort(port) \
	LP1NR(0xf0, RemPort, struct MsgPort *, port, \
	, EXEC_BASE_NAME)

#define RemResource(resource) \
	LP1NR(0x148, RemResource, APTR, resource, \
	, EXEC_BASE_NAME)

#define RemSemaphore(sigSem) \
	LP1NR(0x194, RemSemaphore, struct SignalSemaphore *, sigSem, \
	, EXEC_BASE_NAME)

#define RemTail(list) \
	LP1(0xb0, struct Node *, RemTail, struct List *, list, \
	, EXEC_BASE_NAME)

#define RemTask(task) \
	LP1NR(0xc0, RemTask, struct Task *, task, \
	, EXEC_BASE_NAME)

#define Remove(node) \
	LP1NR(0xa8, Remove, struct Node *, node, \
	, EXEC_BASE_NAME)

#define ReplyMsg(message) \
	LP1NR(0xfc, ReplyMsg, struct Message *, message, \
	, EXEC_BASE_NAME)

#define SendIO(ioRequest) \
	LP1NR(0x134, SendIO, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define SetExcept(newSignals, signalSet) \
	LP2(0xd0, ULONG, SetExcept, unsigned long, newSignals, unsigned long, signalSet, \
	, EXEC_BASE_NAME)

#define SetFunction(library, funcOffset, newFunction) \
	LP3FP(0x118, APTR, SetFunction, struct Library *, library, long, funcOffset, __fpt, newFunction, \
	, EXEC_BASE_NAME, unsigned long (*__fpt)())

#define SetIntVector(intNumber, interrupt) \
	LP2(0x6c, struct Interrupt *, SetIntVector, long, intNumber, struct Interrupt *, interrupt, \
	, EXEC_BASE_NAME)

#define SetSR(newSR, mask) \
	LP2(0x60, ULONG, SetSR, unsigned long, newSR, unsigned long, mask, \
	, EXEC_BASE_NAME)

#define SetSignal(newSignals, signalSet) \
	LP2(0xcc, ULONG, SetSignal, unsigned long, newSignals, unsigned long, signalSet, \
	, EXEC_BASE_NAME)

#define SetTaskPri(task, priority) \
	LP2(0xc8, BYTE, SetTaskPri, struct Task *, task, long, priority, \
	, EXEC_BASE_NAME)

#define Signal(task, signalSet) \
	LP2NR(0xd8, Signal, struct Task *, task, unsigned long, signalSet, \
	, EXEC_BASE_NAME)

#define StackSwap(newStack) \
	LP1NR(0x1e8, StackSwap, struct StackSwapStruct *, newStack, \
	, EXEC_BASE_NAME)

#define SumKickData() \
	LP0(0x198, ULONG, SumKickData, \
	, EXEC_BASE_NAME)

#define SumLibrary(library) \
	LP1NR(0x11c, SumLibrary, struct Library *, library, \
	, EXEC_BASE_NAME)

#define SuperState() \
	LP0(0x64, APTR, SuperState, \
	, EXEC_BASE_NAME)

#define Supervisor(userFunction) \
	LP1A5FP(0x14, ULONG, Supervisor, __fpt, userFunction, \
	, EXEC_BASE_NAME, unsigned long (*__fpt)())

#define TypeOfMem(address) \
	LP1(0x164, ULONG, TypeOfMem, APTR, address, \
	, EXEC_BASE_NAME)

#define UserState(sysStack) \
	LP1NR(0x68, UserState, APTR, sysStack, \
	, EXEC_BASE_NAME)

#define Vacate(sigSem, bidMsg) \
	LP2NR(0x16c, Vacate, struct SignalSemaphore *, sigSem, struct SemaphoreMessage *, bidMsg, \
	, EXEC_BASE_NAME)

#define Wait(signalSet) \
	LP1(0xd4, ULONG, Wait, unsigned long, signalSet, \
	, EXEC_BASE_NAME)

#define WaitIO(ioRequest) \
	LP1(0x13c, BYTE, WaitIO, struct IORequest *, ioRequest, \
	, EXEC_BASE_NAME)

#define WaitPort(port) \
	LP1(0x100, struct Message *, WaitPort, struct MsgPort *, port, \
	, EXEC_BASE_NAME)

#endif /* PROTO_EXEC_H */
