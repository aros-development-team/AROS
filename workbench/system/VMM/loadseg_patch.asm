                    SECTION   CODE

                    INCLUDE   "exec/memory.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/semaphores.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "dos/dosextens.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "dos/dos_lib.i"
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"


                    XDEF      _LoadSegPatch
                    XDEF      _NewLoadSegPatch
                    IFD       DEBUG
                    XDEF      _CrashHandler
                    XDEF      _FindHunk
                    XDEF      _OrigFuncs
                    XDEF      _AlertPatch
                    XREF      _OrigAlert
                    INT_ABLES
                    ENDC

                    XREF      _OrigLoadSeg
                    XREF      _CodePagingAllowed
                    XREF      _AllocVM
                    XREF      _OrigAllocMem
                    XREF      _OrigFreeMem
                    XREF      _ChangeOwner
                    XREF      _MemTracking
                    XREF      _LoadingTasksList

                    IFD       _PHXASS_
                    MACHINE   MC68030
                    ENDC

* There's an undocumented feature of LoadSeg:
* If it's passed a NULL value in A1, it should do some kind of
* overlay loading. I don't know exactly how this works, so
* in this case the original LoadSeg routine is called

_LVOInternalLoadSeg EQU       -$2f4
_LVOFilePart        EQU       -$366
_LVONameFromLock    EQU       -$192

UseOrigLoadSeg      move.l    _OrigLoadSeg,a0
                    PRINT_DEB "LoadSeg into PUBLIC"
                    jmp       (a0)

_LoadSegPatch       * d1 : name (char*)
_NewLoadSegPatch    * a6 : DOSBase
                    *
                    * returns:
                    * d0 : SegList
                    * d1 : SegList

                    PRINT_DEB "LoadSegPatch called"
                    tst.l     d1
                    beq       UseOrigLoadSeg
                    move.l    d1,a0
                    tst.b     (a0)
                    beq       GetNameFromCurrentDir
CheckCodePaging
                    move.l    d1,-(sp)
                    jsr       _CodePagingAllowed
                    move.l    (sp)+,d1
                    tst.w     d0
                    beq       UseOrigLoadSeg

                    movem.l   d2-d5/a2-a4,-(sp)

                    * Register assignments for the following routine
                    * d3: pointer to input filename
                    * d4: filehandle received from Open ()
                    * d5: SegList received from InternalLoadSeg ()
                    * a3: LoadingTaskStruct
                    * a4: DOSBase

                    move.l    d1,d3                    ; store filename and DOSBase
                    move.l    a6,a4                    ; they are needed multiple times
                    suba.l    a3,a3                    ; default pointer to LoadingTaskStruct
                    tst.w     _MemTracking
                    beq       DontTrack

                    move.l    4,a6
                    moveq     #LT_SIZE,d0
                    moveq     #MEMF_PUBLIC,d1
                    move.l    _OrigAllocMem,a0         ; no need to go through patch
                    jsr       (a0)
                    move.l    ThisTask(a6),a0
                    tst.l     d0
                    move.l    d0,a3                    ; save pointer to LoadingTaskStruct
                    beq       DontTrack
                    move.l    d3,LoadfileName(a3)
                    move.l    a0,LoadingTask(a3)
                    lea       _LoadingTasksList,a0
                    move.l    a3,a1
                    FORBID
                    ADDHEAD                            ; d1 is not touched by this
                    PERMIT

DontTrack           moveq     #0,d5                    ; default result
                    move.l    a4,a6                    ; restore DOSBase
                    move.l    #MODE_OLDFILE,d2
                    move.l    d3,d1
                    jsr       _LVOOpen(a6)
                    tst.l     d0
                    beq       Finish
                    PRINT_DEB "Open successful"
                    move.l    d0,d4                    ; save fh

                    lea       FuncVMTable(pc),a1
                    PRINT_DEB "LoadSeg into VM"
                    suba.l    a0,a0          ; overlay table = NULL
                    lea       DummyStackSize(pc),a2
                    jsr       _LVOInternalLoadSeg(a6)
                    move.l    d0,d5
                    move.l    d4,d1
                    PRINT_DEB "LoadSeg: Closing file"
                    jsr       _LVOClose(a6)
                    PRINT_DEB "LoadSeg: File closed"

Finish              tst.l     a3                       ; check if LoadingTaskStruct has to be freed
                    beq       DontFree

                    move.l    4,a6
                    move.l    a3,a1
                    FORBID
                    REMOVE                             ; from LoadingTasksList
                    PERMIT
                    move.l    a3,a1
                    move.l    #LT_SIZE,d0
                    move.l    _OrigFreeMem,a0
                    jsr       (a0)
                    move.l    a4,a6                    ; restore DOSBase

DontFree            move.l    d5,d0
                    move.l    d5,d1
                    movem.l   (sp)+,d2-d5/a2-a4
                    rts

GetNameFromCurrentDir:
                    PRINT_DEB "LoadSegPatch: Reading loadfile from current dir"
                    movem.l   d2-d3/a4,-(sp)
                    move.l    a6,a4
                    sub.w     #128,sp
                    move.l    4,a6
                    move.l    ThisTask(a6),a0
                    move.l    a4,a6
                    move.l    pr_CurrentDir(a0),d1
                    move.l    sp,d2
                    moveq     #127,d3
                    jsr       _LVONameFromLock(a6)
                    move.l    sp,d1
                    tst.w     d0
                    bne       GotName
                    PRINT_DEB "LoadSegPatch: Couldn't get name from lock"
                    clr.b     (sp)
GotName:            bsr       CheckCodePaging
                    add.w     #128,sp
                    movem.l   (sp)+,d2-d3/a4
                    rts

*********************************************************************

ReadFunc:           jmp       _LVORead(a6)
FreeFunc:           jmp       _LVOFreeMem(a6)

AllocFunc           jsr       _AllocVM
                    tst.w     _MemTracking
                    beq       AllocReady

                    move.l    d0,-(sp)            ; pass buffer to ChangeOwner
                    jsr       _ChangeOwner
                    move.l    (sp)+,d0
AllocReady          rts

*********************************************************************

                    IFD       DEBUG

_CrashHandler:      sub.w     #4,sp               ; reserve space for orig handler
                    movem.l   d0-d1/a0-a1,-(sp)
                    move.l    sp,a0
                    PRINT_RANGE
                    moveq     #0,d0
                    move.w    5*4+6(sp),d0       ; Vector offset and frame id
                    and.w     #$0fff,d0
                    lea       _OrigFuncs,a0
                    move.l    (a0,d0.w),4*4(sp)
                    asr.w     #2,d0
                    PRINT_DEB "Trap %lx occurred.",d0
                    move.l    5*4+2(sp),d0
                    PRINT_DEB "Stacked PC = %lx",d0
                    move.l    d0,-(sp)
                    bsr       _FindHunk
                    add.w     #4,sp
                    movem.l   (sp)+,d0-d1/a0-a1
                    rts                           ; call orig func
                    

*********************************************************************

_FindHunk:          * called with an address on the stack
                    * prints the information received from SegTracker
                    * to the debug file

                    movem.l   a2-a3/a6,-(sp)
                    move.l    4,a6
                    DISABLE
                    lea       SegTrackerName(pc),a1
                    lea       SemaphoreList(a6),a0
                    jsr       _LVOFindName(a6)
                    tst.l     d0
                    bne       SegTrackerRunning
                    PRINT_DEB "SegTracker is not running"
                    bra       EndFindSegment

SegTrackerRunning   move.l    d0,a0
                    move.l    SS_SIZE(a0),a3      ; function ptr
                    move.l    4*4(sp),a0          ; address
                    PRINT_DEB "SegTracker for address: %lx",a0
                    sub.w     #8,sp
                    lea       (sp),a1             ; ptr to SegNum
                    lea       4(sp),a2            ; ptr to offset
                    jsr       (a3)                ; returns name in d0
                    tst.l     d0
                    beq       NoHunk
                    pea       0
                    move.l    d0,-(sp)
                    jsr       _PrintDebugMsg      ; print seg name
                    add.w     #8,sp
                    move.l    (sp),d0
                    PRINT_DEB "Hunk: %ld",d0
                    move.l    4(sp),d0
                    PRINT_DEB "Offset: %lx",d0
NoHunk              add.w     #8,sp

EndFindSegment      ENABLE
                    movem.l   (sp)+,a2-a3/a6
                    rts

*********************************************************************

_AlertPatch:        PRINT_DEB "Alert %lx called",d7

                    move.l    _OrigAlert,-(sp)
                    rts

*********************************************************************

SegTrackerName:     dc.b      "SegTracker",0
                    ds.w      0

_OrigFuncs          ds.l      60

                    ENDC      ; DEBUG

*********************************************************************

DummyStackSize      dc.l      0

FuncVMTable         dc.l      ReadFunc
                    dc.l      AllocFunc
                    dc.l      FreeFunc

                    end
