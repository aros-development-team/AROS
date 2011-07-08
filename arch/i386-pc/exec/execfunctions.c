/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec vector table
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Exec,1)(void);
void AROS_SLIB_ENTRY(close,Exec,2)(void);
void AROS_SLIB_ENTRY(null,Exec,3)(void);
void AROS_SLIB_ENTRY(null,Exec,4)(void);
void AROS_SLIB_ENTRY(Supervisor,Exec,5)(void);
void AROS_SLIB_ENTRY(PrepareContext,Exec,6)(void);
void AROS_SLIB_ENTRY(Reschedule,Exec,8)(void);
void AROS_SLIB_ENTRY(Switch,Exec,9)(void);
void AROS_SLIB_ENTRY(Dispatch,Exec,10)(void);
void AROS_SLIB_ENTRY(Exception,Exec,11)(void);
void AROS_SLIB_ENTRY(InitCode,Exec,12)(void);
void AROS_SLIB_ENTRY(InitStruct,Exec,13)(void);
void AROS_SLIB_ENTRY(MakeLibrary,Exec,14)(void);
void AROS_SLIB_ENTRY(MakeFunctions,Exec,15)(void);
void AROS_SLIB_ENTRY(FindResident,Exec,16)(void);
void AROS_SLIB_ENTRY(InitResident,Exec,17)(void);
void AROS_SLIB_ENTRY(Alert,Exec,18)(void);
void AROS_SLIB_ENTRY(Debug,Exec,19)(void);
void AROS_SLIB_ENTRY(Disable,Exec,20)(void);
void AROS_SLIB_ENTRY(Enable,Exec,21)(void);
void AROS_SLIB_ENTRY(Forbid,Exec,22)(void);
void AROS_SLIB_ENTRY(Permit,Exec,23)(void);
void AROS_SLIB_ENTRY(SetSR,Exec,24)(void);
void AROS_SLIB_ENTRY(SuperState,Exec,25)(void);
void AROS_SLIB_ENTRY(UserState,Exec,26)(void);
void AROS_SLIB_ENTRY(SetIntVector,Exec,27)(void);
void AROS_SLIB_ENTRY(AddIntServer,Exec,28)(void);
void AROS_SLIB_ENTRY(RemIntServer,Exec,29)(void);
void AROS_SLIB_ENTRY(Cause,Exec,30)(void);
void AROS_SLIB_ENTRY(Allocate,Exec,31)(void);
void AROS_SLIB_ENTRY(Deallocate,Exec,32)(void);
void AROS_SLIB_ENTRY(AllocMem,Exec,33)(void);
void AROS_SLIB_ENTRY(AllocAbs,Exec,34)(void);
void AROS_SLIB_ENTRY(FreeMem,Exec,35)(void);
void AROS_SLIB_ENTRY(AvailMem,Exec,36)(void);
void AROS_SLIB_ENTRY(AllocEntry,Exec,37)(void);
void AROS_SLIB_ENTRY(FreeEntry,Exec,38)(void);
void AROS_SLIB_ENTRY(Insert,Exec,39)(void);
void AROS_SLIB_ENTRY(AddHead,Exec,40)(void);
void AROS_SLIB_ENTRY(AddTail,Exec,41)(void);
void AROS_SLIB_ENTRY(Remove,Exec,42)(void);
void AROS_SLIB_ENTRY(RemHead,Exec,43)(void);
void AROS_SLIB_ENTRY(RemTail,Exec,44)(void);
void AROS_SLIB_ENTRY(Enqueue,Exec,45)(void);
void AROS_SLIB_ENTRY(FindName,Exec,46)(void);
void AROS_SLIB_ENTRY(AddTask,Exec,47)(void);
void AROS_SLIB_ENTRY(RemTask,Exec,48)(void);
void AROS_SLIB_ENTRY(FindTask,Exec,49)(void);
void AROS_SLIB_ENTRY(SetTaskPri,Exec,50)(void);
void AROS_SLIB_ENTRY(SetSignal,Exec,51)(void);
void AROS_SLIB_ENTRY(SetExcept,Exec,52)(void);
void AROS_SLIB_ENTRY(Wait,Exec,53)(void);
void AROS_SLIB_ENTRY(Signal,Exec,54)(void);
void AROS_SLIB_ENTRY(AllocSignal,Exec,55)(void);
void AROS_SLIB_ENTRY(FreeSignal,Exec,56)(void);
void AROS_SLIB_ENTRY(AllocTrap,Exec,57)(void);
void AROS_SLIB_ENTRY(FreeTrap,Exec,58)(void);
void AROS_SLIB_ENTRY(AddPort,Exec,59)(void);
void AROS_SLIB_ENTRY(RemPort,Exec,60)(void);
void AROS_SLIB_ENTRY(PutMsg,Exec,61)(void);
void AROS_SLIB_ENTRY(GetMsg,Exec,62)(void);
void AROS_SLIB_ENTRY(ReplyMsg,Exec,63)(void);
void AROS_SLIB_ENTRY(WaitPort,Exec,64)(void);
void AROS_SLIB_ENTRY(FindPort,Exec,65)(void);
void AROS_SLIB_ENTRY(AddLibrary,Exec,66)(void);
void AROS_SLIB_ENTRY(RemLibrary,Exec,67)(void);
void AROS_SLIB_ENTRY(OldOpenLibrary,Exec,68)(void);
void AROS_SLIB_ENTRY(CloseLibrary,Exec,69)(void);
void AROS_SLIB_ENTRY(SetFunction,Exec,70)(void);
void AROS_SLIB_ENTRY(SumLibrary,Exec,71)(void);
void AROS_SLIB_ENTRY(AddDevice,Exec,72)(void);
void AROS_SLIB_ENTRY(RemDevice,Exec,73)(void);
void AROS_SLIB_ENTRY(OpenDevice,Exec,74)(void);
void AROS_SLIB_ENTRY(CloseDevice,Exec,75)(void);
void AROS_SLIB_ENTRY(DoIO,Exec,76)(void);
void AROS_SLIB_ENTRY(SendIO,Exec,77)(void);
void AROS_SLIB_ENTRY(CheckIO,Exec,78)(void);
void AROS_SLIB_ENTRY(WaitIO,Exec,79)(void);
void AROS_SLIB_ENTRY(AbortIO,Exec,80)(void);
void AROS_SLIB_ENTRY(AddResource,Exec,81)(void);
void AROS_SLIB_ENTRY(RemResource,Exec,82)(void);
void AROS_SLIB_ENTRY(OpenResource,Exec,83)(void);
void AROS_SLIB_ENTRY(RawIOInit,Exec,84)(void);
void AROS_SLIB_ENTRY(RawMayGetChar,Exec,85)(void);
void AROS_SLIB_ENTRY(RawPutChar,Exec,86)(void);
void AROS_SLIB_ENTRY(RawDoFmt,Exec,87)(void);
void AROS_SLIB_ENTRY(GetCC,Exec,88)(void);
void AROS_SLIB_ENTRY(TypeOfMem,Exec,89)(void);
void AROS_SLIB_ENTRY(Procure,Exec,90)(void);
void AROS_SLIB_ENTRY(Vacate,Exec,91)(void);
void AROS_SLIB_ENTRY(OpenLibrary,Exec,92)(void);
void AROS_SLIB_ENTRY(InitSemaphore,Exec,93)(void);
void AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94)(void);
void AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95)(void);
void AROS_SLIB_ENTRY(AttemptSemaphore,Exec,96)(void);
void AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec,97)(void);
void AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec,99)(void);
void AROS_SLIB_ENTRY(FindSemaphore,Exec,99)(void);
void AROS_SLIB_ENTRY(AddSemaphore,Exec,100)(void);
void AROS_SLIB_ENTRY(RemSemaphore,Exec,101)(void);
void AROS_SLIB_ENTRY(SumKickData,Exec,102)(void);
void AROS_SLIB_ENTRY(AddMemList,Exec,103)(void);
void AROS_SLIB_ENTRY(CopyMem,Exec,104)(void);
void AROS_SLIB_ENTRY(CopyMemQuick,Exec,105)(void);
void AROS_SLIB_ENTRY(CacheClearU,Exec,106)(void);
void AROS_SLIB_ENTRY(CacheClearE,Exec,107)(void);
void AROS_SLIB_ENTRY(CacheControl,Exec,108)(void);
void AROS_SLIB_ENTRY(CreateIORequest,Exec,109)(void);
void AROS_SLIB_ENTRY(DeleteIORequest,Exec,110)(void);
void AROS_SLIB_ENTRY(CreateMsgPort,Exec,111)(void);
void AROS_SLIB_ENTRY(DeleteMsgPort,Exec,112)(void);
void AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113)(void);
void AROS_SLIB_ENTRY(AllocVec,Exec,114)(void);
void AROS_SLIB_ENTRY(FreeVec,Exec,115)(void);
void AROS_SLIB_ENTRY(CreatePool,Exec,116)(void);
void AROS_SLIB_ENTRY(DeletePool,Exec,117)(void);
void AROS_SLIB_ENTRY(AllocPooled,Exec,118)(void);
void AROS_SLIB_ENTRY(FreePooled,Exec,119)(void);
void AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec,120)(void);
void AROS_SLIB_ENTRY(ColdReboot,Exec,121)(void);
void AROS_SLIB_ENTRY(StackSwap,Exec,122)(void);
void AROS_SLIB_ENTRY(ChildFree,Exec,123)(void);
void AROS_SLIB_ENTRY(ChildOrphan,Exec,124)(void);
void AROS_SLIB_ENTRY(ChildStatus,Exec,125)(void);
void AROS_SLIB_ENTRY(ChildWait,Exec,126)(void);
void AROS_SLIB_ENTRY(CachePreDMA,Exec,127)(void);
void AROS_SLIB_ENTRY(CachePostDMA,Exec,128)(void);
void AROS_SLIB_ENTRY(AddMemHandler,Exec,129)(void);
void AROS_SLIB_ENTRY(RemMemHandler,Exec,130)(void);
void AROS_SLIB_ENTRY(ObtainQuickVector,Exec,131)(void);
void AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec,135)(void);
void AROS_SLIB_ENTRY(AllocVecPooled,Exec,149)(void);
void AROS_SLIB_ENTRY(FreeVecPooled,Exec,150)(void);
void AROS_SLIB_ENTRY(NewAllocEntry,Exec,151)(void);
void AROS_SLIB_ENTRY(NewAddTask,Exec,152)(void);

const void *ExecFunctions[] __attribute__((section(".rodata"))) =
{
/*  1 */&AROS_SLIB_ENTRY(open,Exec,1),
	&AROS_SLIB_ENTRY(close,Exec,2),
	&AROS_SLIB_ENTRY(null,Exec,3),
	&AROS_SLIB_ENTRY(null,Exec,4),
	&AROS_SLIB_ENTRY(Supervisor,Exec,5),
	&AROS_SLIB_ENTRY(PrepareContext,Exec,6),
	NULL,		/* Private2 */
	&AROS_SLIB_ENTRY(Reschedule,Exec,8),
	&AROS_SLIB_ENTRY(Switch,Exec,9),
/* 10 */&AROS_SLIB_ENTRY(Dispatch,Exec,10),
	&AROS_SLIB_ENTRY(Exception,Exec,11),
	&AROS_SLIB_ENTRY(InitCode,Exec,12),
	&AROS_SLIB_ENTRY(InitStruct,Exec,13),
	&AROS_SLIB_ENTRY(MakeLibrary,Exec,14),
	&AROS_SLIB_ENTRY(MakeFunctions,Exec,15),
	&AROS_SLIB_ENTRY(FindResident,Exec,16),
	&AROS_SLIB_ENTRY(InitResident,Exec,17),
	&AROS_SLIB_ENTRY(Alert,Exec,18),
	&AROS_SLIB_ENTRY(Debug,Exec,19),
/* 20 */&AROS_SLIB_ENTRY(Disable,Exec,20),
	&AROS_SLIB_ENTRY(Enable,Exec,21),
	&AROS_SLIB_ENTRY(Forbid,Exec,22),
	&AROS_SLIB_ENTRY(Permit,Exec,23),
	&AROS_SLIB_ENTRY(SetSR,Exec,24),
	&AROS_SLIB_ENTRY(SuperState,Exec,25),
	&AROS_SLIB_ENTRY(UserState,Exec,26),
	&AROS_SLIB_ENTRY(SetIntVector,Exec,27),
	&AROS_SLIB_ENTRY(AddIntServer,Exec,28),
	&AROS_SLIB_ENTRY(RemIntServer,Exec,29),
/* 30 */&AROS_SLIB_ENTRY(Cause,Exec,30),
	&AROS_SLIB_ENTRY(Allocate,Exec,31),
	&AROS_SLIB_ENTRY(Deallocate,Exec,32),
	&AROS_SLIB_ENTRY(AllocMem,Exec,33),
	&AROS_SLIB_ENTRY(AllocAbs,Exec,34),
	&AROS_SLIB_ENTRY(FreeMem,Exec,35),
	&AROS_SLIB_ENTRY(AvailMem,Exec,36),
	&AROS_SLIB_ENTRY(AllocEntry,Exec,37),
	&AROS_SLIB_ENTRY(FreeEntry,Exec,38),
	&AROS_SLIB_ENTRY(Insert,Exec,39),
/* 40 */&AROS_SLIB_ENTRY(AddHead,Exec,40),
	&AROS_SLIB_ENTRY(AddTail,Exec,41),
	&AROS_SLIB_ENTRY(Remove,Exec,42),
	&AROS_SLIB_ENTRY(RemHead,Exec,43),
	&AROS_SLIB_ENTRY(RemTail,Exec,44),
	&AROS_SLIB_ENTRY(Enqueue,Exec,45),
	&AROS_SLIB_ENTRY(FindName,Exec,46),
	&AROS_SLIB_ENTRY(AddTask,Exec,47),
	&AROS_SLIB_ENTRY(RemTask,Exec,48),
	&AROS_SLIB_ENTRY(FindTask,Exec,49),
/* 50 */&AROS_SLIB_ENTRY(SetTaskPri,Exec,50),
	&AROS_SLIB_ENTRY(SetSignal,Exec,51),
	&AROS_SLIB_ENTRY(SetExcept,Exec,52),
	&AROS_SLIB_ENTRY(Wait,Exec,53),
	&AROS_SLIB_ENTRY(Signal,Exec,54),
	&AROS_SLIB_ENTRY(AllocSignal,Exec,55),
	&AROS_SLIB_ENTRY(FreeSignal,Exec,56),
	&AROS_SLIB_ENTRY(AllocTrap,Exec,57),
	&AROS_SLIB_ENTRY(FreeTrap,Exec,58),
	&AROS_SLIB_ENTRY(AddPort,Exec,59),
/* 60 */&AROS_SLIB_ENTRY(RemPort,Exec,60),
	&AROS_SLIB_ENTRY(PutMsg,Exec,61),
	&AROS_SLIB_ENTRY(GetMsg,Exec,62),
	&AROS_SLIB_ENTRY(ReplyMsg,Exec,63),
	&AROS_SLIB_ENTRY(WaitPort,Exec,64),
	&AROS_SLIB_ENTRY(FindPort,Exec,65),
	&AROS_SLIB_ENTRY(AddLibrary,Exec,66),
	&AROS_SLIB_ENTRY(RemLibrary,Exec,67),
	&AROS_SLIB_ENTRY(OldOpenLibrary,Exec,68),
	&AROS_SLIB_ENTRY(CloseLibrary,Exec,69),
/* 70 */&AROS_SLIB_ENTRY(SetFunction,Exec,70),
	&AROS_SLIB_ENTRY(SumLibrary,Exec,71),
	&AROS_SLIB_ENTRY(AddDevice,Exec,72),
	&AROS_SLIB_ENTRY(RemDevice,Exec,73),
	&AROS_SLIB_ENTRY(OpenDevice,Exec,74),
	&AROS_SLIB_ENTRY(CloseDevice,Exec,75),
	&AROS_SLIB_ENTRY(DoIO,Exec,76),
	&AROS_SLIB_ENTRY(SendIO,Exec,77),
	&AROS_SLIB_ENTRY(CheckIO,Exec,78),
	&AROS_SLIB_ENTRY(WaitIO,Exec,79),
/* 80 */&AROS_SLIB_ENTRY(AbortIO,Exec,80),
	&AROS_SLIB_ENTRY(AddResource,Exec,81),
	&AROS_SLIB_ENTRY(RemResource,Exec,82),
	&AROS_SLIB_ENTRY(OpenResource,Exec,83),
	&AROS_SLIB_ENTRY(RawIOInit,Exec,84), /* Private7 */
	&AROS_SLIB_ENTRY(RawMayGetChar,Exec,85), /* Private8 */
	&AROS_SLIB_ENTRY(RawPutChar,Exec,86),
	&AROS_SLIB_ENTRY(RawDoFmt,Exec,87),
	&AROS_SLIB_ENTRY(GetCC,Exec,88),
	&AROS_SLIB_ENTRY(TypeOfMem,Exec,89),
/* 90 */&AROS_SLIB_ENTRY(Procure,Exec,90),
	&AROS_SLIB_ENTRY(Vacate,Exec,91),
	&AROS_SLIB_ENTRY(OpenLibrary,Exec,92),
	&AROS_SLIB_ENTRY(InitSemaphore,Exec,93),
	&AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94),
	&AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95),
	&AROS_SLIB_ENTRY(AttemptSemaphore,Exec,96),
	&AROS_SLIB_ENTRY(ObtainSemaphoreList,Exec,97),
	&AROS_SLIB_ENTRY(ReleaseSemaphoreList,Exec,99),
	&AROS_SLIB_ENTRY(FindSemaphore,Exec,99),
/*100 */&AROS_SLIB_ENTRY(AddSemaphore,Exec,100),
	&AROS_SLIB_ENTRY(RemSemaphore,Exec,101),
	&AROS_SLIB_ENTRY(SumKickData,Exec,102),
	&AROS_SLIB_ENTRY(AddMemList,Exec,103),
	&AROS_SLIB_ENTRY(CopyMem,Exec,104),
	&AROS_SLIB_ENTRY(CopyMemQuick,Exec,105),
	&AROS_SLIB_ENTRY(CacheClearU,Exec,106),
	&AROS_SLIB_ENTRY(CacheClearE,Exec,107),
	&AROS_SLIB_ENTRY(CacheControl,Exec,108),
	&AROS_SLIB_ENTRY(CreateIORequest,Exec,109),
/*110 */&AROS_SLIB_ENTRY(DeleteIORequest,Exec,110),
	&AROS_SLIB_ENTRY(CreateMsgPort,Exec,111),
	&AROS_SLIB_ENTRY(DeleteMsgPort,Exec,112),
	&AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113),
	&AROS_SLIB_ENTRY(AllocVec,Exec,114),
	&AROS_SLIB_ENTRY(FreeVec,Exec,115),
	&AROS_SLIB_ENTRY(CreatePool,Exec,116),
	&AROS_SLIB_ENTRY(DeletePool,Exec,117),
	&AROS_SLIB_ENTRY(AllocPooled,Exec,118),
	&AROS_SLIB_ENTRY(FreePooled,Exec,119),
/*120 */&AROS_SLIB_ENTRY(AttemptSemaphoreShared,Exec,120),
	&AROS_SLIB_ENTRY(ColdReboot,Exec,121),
	&AROS_SLIB_ENTRY(StackSwap,Exec,122),
	&AROS_SLIB_ENTRY(ChildFree,Exec,123),
	&AROS_SLIB_ENTRY(ChildOrphan,Exec,124),
	&AROS_SLIB_ENTRY(ChildStatus,Exec,125),
	&AROS_SLIB_ENTRY(ChildWait,Exec,126),
	&AROS_SLIB_ENTRY(CachePreDMA,Exec,127),
	&AROS_SLIB_ENTRY(CachePostDMA,Exec,128),
	&AROS_SLIB_ENTRY(AddMemHandler,Exec,129),
/*130 */&AROS_SLIB_ENTRY(RemMemHandler,Exec,130),
	&AROS_SLIB_ENTRY(ObtainQuickVector,Exec,131),
	NULL,
	NULL,
	NULL,
	&AROS_SLIB_ENTRY(TaggedOpenLibrary,Exec,135),
	NULL,
	NULL, /* 137 */
        NULL,
        NULL,
        NULL, /* 140 */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL, /* 145 */
        NULL,
        NULL,
        NULL,
        &AROS_SLIB_ENTRY(AllocVecPooled,Exec,149), /* 149 */
        &AROS_SLIB_ENTRY(FreeVecPooled,Exec,150),  /* 150 */
        &AROS_SLIB_ENTRY(NewAllocEntry,Exec,151),  /* 151 */
        &AROS_SLIB_ENTRY(NewAddTask,Exec,152),     /* 152 */
	(APTR)-1
};
