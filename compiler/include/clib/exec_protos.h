#ifndef CLIB_EXEC_PROTOS_H
#define CLIB_EXEC_PROTOS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALLDEFS_H
#   include <aros/libcall.h>
#endif

#include <aros/config/gnuc/exec_defines.h>
#ifdef __SASC
#   include <sasc/exec_pragmas.h>
#endif

LP0(void, Switch, struct ExecBase *, SysBase, -6, Exec)

LP3(APTR, PrepareContext,
    APTR, stackPointer, A0,
    APTR, entryPoint, A1,
    APTR, fallBack, A2,
    struct ExecBase *, SysBase, -9, Exec)

LP3(void, InitStruct,
    APTR, initTable, A1,
    APTR, memory, A2,
    ULONG, size, D0,
    struct ExecBase *, SysBase, -13, Exec)

LP1(void, Alert,
    ULONG, alertNum, D7,
    struct ExecBase *, SysBase, -18, Exec)

LP0(void, Disable, struct ExecBase *, SysBase, -20, Exec)

LP0(void, Enable, struct ExecBase *, SysBase, -21, Exec)

LP0(void, Forbid, struct ExecBase *, SysBase, -22, Exec)

LP0(void, Permit, struct ExecBase *, SysBase, -23, Exec)

LP1(void, Cause,
    struct Interrupt *, interrupt, A1,
    struct ExecBase *, SysBase, -30, Exec)

LP2(APTR, Allocate,
    struct MemHeader *, freeList, A0,
    ULONG, byteSize, D0,
    struct ExecBase *, SysBase, -31, Exec)

LP2(APTR, AllocMem,
    ULONG, byteSize, D0,
    ULONG, requirements, D1,
    struct ExecBase *, SysBase, -33, Exec)

LP2(void, FreeMem,
    APTR, memoryBlock, A1,
    ULONG, byteSize, D0,
    struct ExecBase *, SysBase, -35, Exec)

LP1(void, FreeEntry,
    struct MemList *, entry, A0,
    struct ExecBase *, SysBase, -38, Exec)

LP2(void, AddHead,
    struct List *, list, A0,
    struct Node *, node, A1,
    struct ExecBase *, SysBase, -40, Exec)

LP2(void, AddTail,
    struct List *, list, A0,
    struct Node *, node, A1,
    struct ExecBase *, SysBase, -41, Exec)

LP1(void, Remove,
    struct Node *, node, A1,
    struct ExecBase *, SysBase, -42, Exec)

LP1(struct Node *, RemHead,
    struct List *, list, A0,
    struct ExecBase *, SysBase, -43, Exec)

LP2(void, Enqueue,
    struct List *, list, A0,
    struct Node *, node, A1,
    struct ExecBase *, SysBase, -45, Exec)

LP2(struct Node *, FindName,
    struct List *, list, A0,
    STRPTR, name, A1,
    struct ExecBase *, SysBase, -46, Exec)

LP3(struct Task *, AddTask,
    struct Task *, task, A1,
    APTR, initPC, A2,
    APTR, finalPC, A3,
    struct ExecBase *, SysBase, -47, Exec)

LP1(void, RemTask,
    struct Task *, task, A1,
    struct ExecBase *, SysBase, -48, Exec)

LP1(struct Task *, FindTask,
    STRPTR, name, A1,
    struct ExecBase *, SysBase, -49, Exec)

LP2(BYTE, SetTaskPri,
    struct Task *, task, A1,
    LONG, priority, D0,
    struct ExecBase *, SysBase, -50, Exec)

LP2(ULONG, SetSignal,
    ULONG, newSignals, D0,
    ULONG, signalSet, D1,
    struct ExecBase *, SysBase, -51, Exec)

LP2(ULONG, SetExcept,
    ULONG, newSignals, D0,
    ULONG, signalSet,  D1,
    struct ExecBase *, SysBase, -52, Exec)

LP1(ULONG, Wait,
    ULONG, signalSet, D0,
    struct ExecBase *, SysBase, -53, Exec)

LP2(void, Signal,
    struct Task *, task, A1,
    ULONG, signalSet, D0,
    struct ExecBase *, SysBase, -54, Exec)

LP1(BYTE, AllocSignal,
    LONG, signalNum, D0,
    struct ExecBase *, SysBase, -55, Exec)

LP1(void, FreeSignal,
    LONG, signalNum, D0,
    struct ExecBase *, SysBase, -56, Exec)

LP1(void, AddPort,
    struct MsgPort *, port, A1,
    struct ExecBase *, SysBase, -59, Exec)

LP1(void, RemPort,
    struct MsgPort *, port, A1,
    struct ExecBase *, SysBase, -60, Exec)

LP2(void, PutMsg,
    struct MsgPort *, port,    A0,
    struct Message *, message, A1,
    struct ExecBase *, SysBase, -61, Exec)

LP1(struct Message *, GetMsg,
    struct MsgPort *, port, A0,
    struct ExecBase *, SysBase, -62, Exec)

LP1(void, ReplyMsg,
    struct Message *, message, A1,
    struct ExecBase *, SysBase, -63, Exec)

LP1(struct Message *, WaitPort,
    struct MsgPort *, port, A0,
    struct ExecBase *, SysBase, -64, Exec)

LP1(struct MsgPort *, FindPort,
    STRPTR, name,A1,
    struct ExecBase *, SysBase, -65, Exec)

LP2(struct IORequest *, CreateIORequest,
    struct MsgPort *, ioReplyPort, A0,
    ULONG, size, D0,
    struct ExecBase *, SysBase, -109, Exec)

LP1(void, DeleteIORequest,
    struct IORequest *, iorequest, A0,
    struct ExecBase *, SysBase, -110, Exec)

LP0(struct MsgPort *, CreateMsgPort, struct ExecBase *, SysBase, -111, Exec)

LP1(void, DeleteMsgPort,
    struct MsgPort *, port, A0,
    struct ExecBase *, SysBase, -112, Exec)

LP1(void, AddMemHandler,
    struct Interrupt *, memHandler, A1,
    struct ExecBase *, SysBase, -129, Exec)

#endif
