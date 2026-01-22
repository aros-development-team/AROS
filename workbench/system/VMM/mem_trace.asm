                    INCLUDE   "exec/memory.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "dos/dosextens.i"
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"

                    XDEF      _AllocVM
                    XDEF      _AllocMemPatch
                    XREF      _OrigAllocMem
                    XREF      _AllocMemUsers

                    XDEF      _FreeMemPatch
                    XREF      _OrigFreeMem

                    XDEF      _AvailMemPatch
                    XREF      _OrigAvailMem

                    XDEF      _FreeMemAfterExit
                    XDEF      _FreeMemAfterExitEnd
                    XDEF      _ExecFreeMem
                    XDEF      _PatchLoc1
                    XDEF      _PatchLoc2
                    XDEF      _PatchLoc3
                    XDEF      _RemainingBytes
                    XDEF      _RootTableContents

                    XDEF      _DoOrigAllocMem
                    XDEF      _DoOrigAvailMem

                    XREF      _HashTab
                    XREF      _VMPort
                    XREF      _VirtAddrStart
                    XREF      _VirtAddrEnd
                    XREF      _VMToBeFreed
                    XREF      _VMFreeRecycling
                    XREF      _FreeVMSignal
                    XREF      _PrePagerTask
                    XREF      _NumPageFrames
                    XREF      _PageReq
                    XREF      _Free
                    XREF      _PageFaultSignal
                    XREF      _PageHandlerTask
                    XREF      _LowMem
                    XREF      _PageHandlerPort
                    XREF      _VMD_NestCnt
                    XREF      _MinVMAlloc
                    XREF      _MemTracking
                    XREF      _CreateTrackInfo
                    XREF      _FreeTrackInfo
                    XREF      _MemTrackList
                    XREF      _CurrentConfig
                    XREF      _EmptyPageCollector
                    XREF      _VMFreeCounter
                    XREF      _CreateTask
                    IFD       USE_OWN_SEMAPHORES
                    XREF      VMMObtainSemaphore
                    XREF      VMMReleaseSemaphore
                    ENDC
                    

****************************************************************
* Memory tracking: Each allocation is prefixed by a standard 
* header, so it is possible to figure out which task allocated
* the block. The header looks like this:
*    Offset     Value
*      0        Ptr to MemTrackStruct
*      4        MAGIC
*      8        Start of buffer returned to requesting task
*
* AllocMem builds such a header for each request in VM if TrackMem
* is set.
* FreeMem checks for the magic longword on every deallocation.
* If it is found the header and the corresponding node is freed also.


                    IFD       _PHXASS_
                    MACHINE   MC68030
                    ENDC

                    SECTION   CODE

                    ALIGN_LONG

_FreeMemPatch:      * a1 : location
                    * d0 : size
                    * a6 : SysBase
                    IN_VM     a1,NoVirtMemFree

                    PRINT_MEM "FreeMem: size %ld",d0
                    PRINT_MEM "FreeMem: location %lx",a1

                    subq.l    #1,_VMFreeCounter
                    bne       NoEmptyPageCollection

                    movem.l   a1/d0,-(sp)
                    PRINT_DEB "Creating garbage collector"
                    addq.w    #1,_VMD_NestCnt               ; disable allocation of VM
                    move.l    #2000,-(sp)                   ; stacksize
                    lea       _EmptyPageCollector,a0        ; task routine
                    move.l    a0,-(sp)
                    clr.l     -(sp)                         ; priority
                    lea       GarbageCollName,a0
                    move.l    a0,-(sp)                      ; taskname
                    jsr       _CreateTask
                    add.w     #4*4,sp
                    subq.w    #1,_VMD_NestCnt               ; enable allocation of VM
                    movem.l   (sp)+,a1/d0

NoEmptyPageCollection
                    cmpi.w    #$ffff,IDNestCnt(a6)          ; compares TD and IDNestCnt
                                                            ; simulataneously
                    bne       LetPrePagerFreeIt

                    cmpi.l    #2*PAGESIZE,d0

                    bcs       FreeDirectly

                    * This region is more than 2 pages long. Inform the
                    * pagehandler to free the corresponding frames or
                    * swap pages. Then do the standard free procedure

                    PRINT_MEM "FreeMem: More than 2 pages"

                    movem.l   d2-d3/a2-a3,-(sp)
                    move.l    d0,d2
                    move.l    a1,a3
                    move.l    #(MEMF_PUBLIC+MEMF_CLEAR),d1
                    move.l    #VMM_SIZE,d0
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)
                    tst.l     d0
                    beq       DontFreePages
                    move.l    d0,a2
                    move.l    ThisTask(a6),VMM_VMSender(a2)
                    GET_SIGNAL
                    ext.w     d0
                    move.w    d0,VMM_ReplySignal(a2)
                    move.w    d0,d3

                    move.w    #VMCMD_FreePages,VMM_VMCommand(a2)
                    move.l    a3,VMM_FreeAddress(a2)
                    move.l    d2,VMM_FreeSize(a2)
                    move.l    a2,a1
                    move.l    _PageHandlerPort,a0
                    jsr       _LVOPutMsg(a6)

                    * Have to wait for the page-handler to complete its
                    * operation, otherwise the memory list might be corrupted.
                    moveq.l   #1,d0
                    lsl.l     d3,d0
                    jsr       _LVOWait(a6)

                    move.w    d3,d0
                    RELEASE_SIGNAL
                    move.l    _OrigFreeMem,a0
                    move.l    a2,a1
                    moveq.l   #VMM_SIZE,d0
                    jsr       (a0)

DontFreePages       move.l    d2,d0
                    move.l    a3,a1
                    movem.l   (sp)+,d2-d3/a2-a3
FreeDirectly
                    MEM_FORBID

                    tst.w     _MemTracking
                    beq       DontTrack

                    * Check if this was a tracked allocation
                    move.l    a1,a0
                    cmpi.l    #TRACK_MAGIC,-(a0)
                    bne       DontTrack
                    clr.l     (a0)           ; delete magic
                    movem.l   a1/d0,-(sp)
                    move.l    -(a0),-(sp)
                    jsr       _FreeTrackInfo
                    add.w     #4,sp
                    movem.l   (sp)+,a1/d0
                    suba.w    #8,a1          ; free header also
                    addq.l    #8,d0
DontTrack
                    move.l    _OrigFreeMem,a0

                    jsr       (a0)

                    MEM_PERMIT
                    rts

                    ALIGN_LONG

NoVirtMemFree
                    PRINT_MEM "FreeMem: size %ld",d0
                    PRINT_MEM "FreeMem: location %lx",a1

                    move.l    _OrigFreeMem,a0
                    jmp       (a0)

LetPrePagerFreeIt:  * enqueue this chunk in the VMToBeFreed list
                    * no need to enclose this in Forbid/Permit because
                    * we're already in forbidden state

                    PRINT_MEM "FreeMem via PrePager"
                    move.l    _VMFreeRecycling,a0
                    tst.l     a0
                    beq       AllocNew
                    move.l    FF_NextFree(a0),_VMFreeRecycling
                    bra       GotFFS              * ForbiddenFreeStruct
AllocNew            movem.l   a1/d0,-(sp)
                    moveq     #FF_SIZE,d0
                    move.l    #MEMF_PUBLIC,d1
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)
                    tst.l     d0
                    beq       FreeError
                    move.l    d0,a0
                    movem.l   (sp)+,a1/d0

GotFFS              move.l    a1,FF_address(a0)
                    move.l    d0,FF_size(a0)

                    lea       _VMToBeFreed,a1
                    move.l    (a1),FF_NextFree(a0)
                    move.l    a0,(a1)

                    move.l    _PrePagerTask,a1
                    move.w    _FreeVMSignal,d1
                    moveq     #1,d0
                    lsl.l     d1,d0
                    jmp       _LVOSignal(a6)

FreeError           * there's no memory left for the FFS
                    * Bite the bullet and free it directly
                    * despite this might cause failure

                    PRINT_MEM "No mem left for ffs"
                    PRINT_MEM "Freeing directly"
                    movem.l   (sp)+,a1/d0
                    bra       FreeDirectly

****************************************************************

_AllocVM            add.w     #1,_AllocMemUsers
                    move.l    d7,-(sp)
                    moveq     #0,d7
                    bclr.l    #MEMB_PUBLIC,d1
                    bra       DoAlloc

****************************************************************

                    ALIGN_LONG

_AllocMemPatch      * Handling this is quite complicated, so I first wrote it
                    * down in C for clarity. Following is the C code.
                    * It is not very up to date with the real assembler code
                    *
                    * void *C_AllocMem (ULONG size, ULONG flags)
                    *
                    * {
                    * void *buf;
                    * ULONG orig_flags;
                    *
                    * if (VMD_NestCnt != NULL)
                    *   flags |= MEMF_PUBLIC;
                    * else if (!(flags & MEMF_CHIP))
                    *   flags = CheckHashTable (size, flags);
                    *
                    * orig_flags = flags;
                    * if (!(flags & MEMF_CHIP))
                    *   flags |= MEMF_FAST;
                    *
                    * for (;;)
                    *   {
                    *   if (flags & MEMF_PUBLIC)
                    *     buf = CallOrigAllocMem (size, flags);
                    *   else
                    *     {
                    *     MEM_FORBID;
                    *     buf = CallOrigAllocMem (size, flags);
                    *     MEM_PERMIT;
                    *     }
                    *
                    *   if (buf != NULL)
                    *     return (buf);
                    *   if (NumPageFrames * PAGESIZE <= MinMem || FreeFrames (flags) == NULL)
                    *     {
                    *     if (flags != orig_flags)
                    *       flags = orig_flags;
                    *     else
                    *       return (NULL);
                    *     }
                    *   }
                    * }
                    *

                    * Called with:
                    * d0 : size of allocation
                    * d1 : flags
                    * a6 : SysBase

                    add.w     #1,_AllocMemUsers
                    move.l    d7,-(sp)
                    moveq     #0,d7                    ; used as a marker whether LowMem has been set

                    PRINT_MEM "AllocMemPatch: size %ld",d0
                    PRINT_MEM "              flags %lx",d1

                    IFD       DEBUG
                    IFD       TRACE_MEM
                    movem.l   d0-d1,-(sp)
                    moveq     #0,d0
                    move.l    d0,d1
                    jsr       _LVOSetSR(a6)
                    btst.l    #13,d0
                    movem.l   (sp)+,d0-d1
                    beq       NoSVMode
                    PRINT_MEM "AllocMem from SV: size %ld",d0
                    PRINT_MEM "AllocMem from SV: flags %lx",d1
NoSVMode
                    ENDC
                    ENDC

                    cmp.l     _MinVMAlloc,d0
                    bcs       ForcePublic

                    tst.w     _VMD_NestCnt
                    beq       TestChip

ForcePublic         bset.l    #MEMB_PUBLIC,d1
                    bra       DoAlloc

                    ALIGN_LONG

TestChip            btst.l    #MEMB_CHIP,d1
                    beq       ConsultHashTable
                    bra       ForcePublic

ConsultHashTable    bsr       CheckHashTable
                    PRINT_MEM "CheckHashTable returned %lx",d1

DoAlloc
                    movem.l   d0-d1,-(sp)         ; save size and flags for
                                                  ; repeated usage
                    btst.l    #MEMB_CHIP,d1
                    bne       DontSetFast
                    bset.l    #MEMB_FAST,d1

DontSetFast
RetryAlloc          btst.l    #MEMB_PUBLIC,d1
                    beq       TryVM

***************** Allocation of public mem ****************

                    PRINT_MEM "AllocMem: Trying PUBLIC"
                    move.l    d1,-(sp)
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)

                    PRINT_MEM "AllocMem PUBLIC: location %lx",d0

                    tst.l     d0
                    bne       AllocSuccess
                    bra       AllocFailed

***************** Allocation of VM ************************

TryVM:              PRINT_MEM "AllocMem: Trying VM"

                    tst.w     _MemTracking
                    beq       NoTracking

                    addq.l    #8,d0               ; for the header
                    
                    MEM_FORBID
                    move.l    d1,-(sp)
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)

                    PRINT_MEM "AllocMem VM: location %lx",d0

                    tst.l     d0
                    beq       AllocFailedTracked

                    ; check if the allocated buffer is in public mem
                    IN_VM     d0,BufIsPublic

                    move.l    4(sp),-(sp)          ; size of original request
                    move.l    d0,-(sp)
                    jsr       _CreateTrackInfo
                    add.w     #8,sp
                    MEM_PERMIT
                    
                    bra       AllocSuccess

AllocFailedTracked  MEM_PERMIT
                    bra       AllocFailed

BufIsPublic:        ; The allocated buffer is not in virtual memory,
                    ; i.e. it should not be tracked.
                    ; Free the trailing 8 bytes

                    MEM_PERMIT
                    move.l    d0,-(sp)
                    move.l    8(sp),d1             ; size of original request
                    addq.l    #7,d1
                    and.b     #~7,d1               ; align to 8 byte boundary
                    add.l     d0,d1                ; add start address

                    move.l    d1,a1
                    moveq     #8,d0                    
                    move.l    _OrigFreeMem,a0
                    jsr       (a0)

                    ; restore buffer pointer
                    move.l    (sp)+,d0
                    bra       AllocSuccess

NoTracking
                    MEM_FORBID
                    move.l    d1,-(sp)
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)
                    MEM_PERMIT

                    PRINT_MEM "AllocMem VM: location %lx",d0

                    tst.l     d0
                    beq       AllocFailed

AllocSuccess        add.w     #12,sp
                    tst.l     d7
                    beq       NoLowMem1
                    sub.w     #1,_LowMem
NoLowMem1           move.l    (sp)+,d7
                    sub.w     #1,_AllocMemUsers
                    tst.l     d0                  ; just to make bad programs work
                    rts

AllocFailed         move.l    (sp)+,d1            ; restore flags
                    btst.l    #MEMB_NO_EXPUNGE,d1
                    bne       NoSuccess
                    move.l    _NumPageFrames,d0
                    asl.l     #8,d0
                    asl.l     #(PAGESIZESHIFT-8),d0
                    lea       _CurrentConfig,a0
                    cmp.l     MinMem(a0),d0
                    ble       CheckFlags
                    move.l    (sp),d0             ; saved size
                    bsr       FreeFrame
                    tst.l     d0
                    beq       CheckFlags
                    move.l    (sp),d0             ; saved size
                    bra       RetryAlloc

CheckFlags:         cmp.l     4(sp),d1            ; saved flags
                    beq       NoSuccess
                    move.l    4(sp),d1
                    PRINT_MEM "Retrying with orig flags"
                    move.l    (sp),d0             ; saved size
                    bra       RetryAlloc

NoSuccess:          PRINT_MEM "Alloc failed"
                    add.w     #8,sp
                    moveq     #0,d0
                    tst.l     d7
                    beq       NoLowMem2
                    sub.w     #1,_LowMem           
NoLowMem2           move.l    (sp)+,d7
                    sub.w     #1,_AllocMemUsers
                    tst.l     d0                  ; just to make bad programs work
                    rts


                    *****************************************

                    ALIGN_LONG

FreeFrame:          * Function: Tries to make the pagehandler free one frame
                    * Inputs: A6 = SysBase (not to be changed)
                    *         D0: Size of request (may be destroyed)
                    *         D1: Flags from AllocMem (not to be modified)
                    * Outputs: d0 <> 0 indicates success

                    PRINT_MEM "Trying to free a frame"
                    tst.l     d7
                    bne       LowMemAlreadySet
                    add.w     #1,_LowMem
LowMemAlreadySet
                    moveq     #1,d7
                    movem.l   d1-d3/a2,-(sp)
                    move.l    d0,d3
                    moveq     #0,d2               * Success indicator:
                                                  * No success
                    lea       _Free,a0
                    FORBID
                    REMHEAD
                    PERMIT
                    tst.l     d0
                    beq       NothingToBeDone
                    move.l    d0,a2
                    move.l    ThisTask(a6),TS_FaultTask(a2)
                    clr.l     TS_FaultAddress(a2)
                    move.l    d3,TS_RemFrameSize(a2)
                    move.l    d1,TS_RemFrameFlags(a2)
                    clr.w     TS_FramesRemoved(a2)

                    GET_SIGNAL
                    ext.w     d0
                    move.w    d0,TS_WakeupSignal(a2)      ; -1 for no signal

                    lea       _PageReq,a0
                    move.l    a2,a1
                    FORBID
                    ADDTAIL
                    PERMIT

                    move.w    _PageFaultSignal,d1
                    move.l    _PageHandlerTask,a1
                    moveq     #1,d0
                    lsl.l     d1,d0
                    jsr       _LVOSignal(a6)

                    moveq     #1,d0
                    move.w    TS_WakeupSignal(a2),d1
                    lsl.l     d1,d0

                    jsr       _LVOWait(a6)

                    PRINT_MEM "Received reply"

                    move.w    TS_WakeupSignal(a2),d0
                    RELEASE_SIGNAL

                    move.w    TS_FramesRemoved(a2),d2
                    PRINT_DEB "Frames removed: %lx",d2

                    lea       _Free,a0
                    move.l    a2,a1
                    FORBID
                    ADDTAIL
                    PERMIT

NothingToBeDone     move.l    d2,d0
                    movem.l   (sp)+,d1-d3/a2
                    rts

****************************************************************

                    ALIGN_LONG

AskVMManager:       * Function: Call the VMManager to enter the current
                    *           task into the hash table.
                    * Inputs: A6 = SysBase (not to be changed)

                    PRINT_MEM "AllocMem: ExtCheckMem called"

                    move.l    a2,-(sp)
                    move.l    #(MEMF_PUBLIC+MEMF_CLEAR),d1
                    move.l    #VMM_SIZE,d0
                    move.l    _OrigAllocMem,a0
                    jsr       (a0)
                    tst.l     d0
                    bne       GotMsg
                    move.l    (sp)+,a2
                    rts

GotMsg              move.l    d0,a2          ; Msg for VM_Manager
                    GET_SIGNAL
                    move.l    ThisTask(a6),VMM_VMSender(a2)
                    ext.w     d0
                    move.w    d0,VMM_ReplySignal(a2)
                    move.w    #VMCMD_AskAllocMem,VMM_VMCommand(a2)
                    PRINT_DEB "Asking VM_Manager"
                    move.l    _VMPort,a0
                    move.l    a2,a1

                    jsr       _LVOPutMsg(a6)

                    move.w    VMM_ReplySignal(a2),d1
                    moveq     #1,d0
                    asl.l     d1,d0
                    jsr       _LVOWait(a6)

FreeAll             move.w    VMM_ReplySignal(a2),d0
                    RELEASE_SIGNAL
                    move.l    a2,a1
                    move.l    #VMM_SIZE,d0
                    move.l    _OrigFreeMem,a0
                    jsr       (a0)
                    move.l    (sp)+,a2
                    rts

****************************************************************

                    ALIGN_LONG

CheckHashTable:     * Function: Checks, if the current task may use
                    *           VM.
                    * Inputs:  D0: size of desired allocation
                    *          D1: flags of desired allocation
                    *          A6 = SysBase (not to be changed)
                    * Outputs: D0: size of desired allocation
                    *          D1: modified flags for allocation

                    movem.l   d0-d1,-(sp)
                    FORBID
                    bsr       FindHashEntry       ; returns pointer to 
                    movem.l   (sp)+,d0-d1         ; hash entry in a0

                    btst.l    #MEMB_PUBLIC,d1
                    st        HE_Referenced(a0)   ; does not affect CCR
                    beq       CheckNonPublic
                    cmp.l     HE_MinPublic(a0),d0
                    bcs       ThatsIt             ; unsigned blt
                    bclr.l    #MEMB_PUBLIC,d1
ThatsIt             PERMIT
                    rts

                    ALIGN_LONG

CheckNonPublic:     cmp.l     HE_MinNonPublic(a0),d0
                    bcc       ThatsIt             ; unsigned bge
                    bset.l    #MEMB_PUBLIC,d1
                    PERMIT
                    rts

****************************************************************

FindHashEntry:      * Function: Find a hash entry in the table
                    *           If the corresponding entry could not be 
                    *           found the VM_Manager is asked.
                    *           Has to be called in forbidden state.
                    * Inputs:  a6: SysBase
                    * Outputs: a0: Pointer to hash entry

* Check if this is a CLI or another type of task

                    move.l    ThisTask(a6),a1
                    cmp.b     #NT_TASK,LN_TYPE(a1)
                    move.l    LN_NAME(a1),a0      ; does not affect CCR
                    beq       IsNotCLI
                    movea.l   pr_CLI(a1),a1
                    tst.l     a1
                    beq       IsNotCLI
                    adda.l    a1,a1          ; BPTR --> PTR
                    adda.l    a1,a1
                    move.l    cli_CommandName(a1),a1
                    tst.l     a1
                    beq       IsNotCLI
                    adda.l    a1,a1          ; BPTR --> PTR
                    adda.l    a1,a1
                    tst.b     1(a1)
                    beq       IsNotCLI
                    bra       IsCLI

                    ALIGN_LONG

IsNotCLI            move.l    a0,a1

                    * A1 contains pointer to name here

IsCLI               movem.l   a2-a3,-(sp)
                    lea       _HashTab,a0
                    move.l    a1,d0
                    HASH_VAL  d0
                    move.l    (a0,d0.w*4),a0
loop                tst.l     a0
                    bne       ValidEntry

NotFound:           movem.l   (sp)+,a2-a3
                    bsr       AskVMManager
                    bra       FindHashEntry

                    ALIGN_LONG

ValidEntry          move.l    a1,a3               ; pointer to name
                    move.l    HE_Name(a0),a2
                    move.w    HE_NumLongsM1(a0),d0

cmp_loop            cmp.l     (a2)+,(a3)+
                    dbne      d0,cmp_loop
                    beq       FoundIt

NextInChain:        move.l    HE_NextEntry(a0),a0
                    PRINT_MEM "AllocMem: Next in chain used"
                    bra       loop

                    ALIGN_LONG

FoundIt             movem.l   (sp)+,a2-a3
                    rts


****************************************************************

                    ALIGN_LONG

_AvailMemPatch:     * inputs:
                    * d1 : attributes
                    * a6 : SysBase
                    * outputs:
                    * d0 : size

                    tst.w     _VMD_NestCnt
                    beq       TestChip2

                    bset.l    #MEMB_PUBLIC,d1
                    bra       CallOrig

TestChip2           btst.l    #MEMB_CHIP,d1
                    bne       CallOrig

* New: Check if this task is permitted to use virtual mem.
*      If not return only the amount PUBLIC free mem.
                    move.l     #$7fffffff,d0
                    bsr       CheckHashTable

                    move.l    _OrigAvailMem,a1
                    MEM_FORBID
                    jsr       (a1)
                    MEM_PERMIT
                    rts

CallOrig:           move.l    _OrigAvailMem,a1

                    jmp       (a1)

****************************************************************

_DoOrigAllocMem     * Function to call the original AllocMem function
                    * without going through the patch from C. It is safe
                    * to call even when the patch has not yet been
                    * installed.
                    * C prototype is
                    * void *DoOrigAllocMem (ULONG size, ULONG flags)

                    move.l    8(sp),d1
                    move.l    4(sp),d0
                    move.l    a6,-(sp)
                    move.l    _OrigAllocMem,a0
                    move.l    4,a6
                    tst.l     a0
                    beq       UseLVO
                    jsr       (a0)
                    move.l    (sp)+,a6
                    rts

UseLVO              jsr       _LVOAllocMem(a6)
                    move.l    (sp)+,a6
                    rts

****************************************************************

_DoOrigAvailMem     * Function to call the original AvailMem function
                    * without going through the patch from C. It is safe
                    * to call even when the patch has not yet been
                    * installed.
                    * C prototype is
                    * void *DoOrigAvailMem (ULONG flags)

                    move.l    4(sp),d1
                    move.l    a6,-(sp)
                    move.l    _OrigAvailMem,a0
                    move.l    4,a6
                    tst.l     a0
                    beq       UseLVO2
                    jsr       (a0)
                    move.l    (sp)+,a6
                    rts

UseLVO2             jsr       _LVOAvailMem(a6)
                    move.l    (sp)+,a6
                    rts

****************************************************************

_FreeMemAfterExit:  * a1 : location
                    * d0 : size
                    * a6 : SysBase

_PatchLoc1          cmp.l     #$ffffffff,a1   * Dummy for end, patched later
                    bhi       NoVirtMemFree2
_PatchLoc2          cmp.l     #$ffffffff,a1   * Dummy for start, patched later
                    bcs       NoVirtMemFree2
                    * Count down rest of allocated memory
                    addq.l    #7,d0
                    and.w     #$fff8,d0
                    lea       _RemainingBytes(pc),a0
                    sub.l     d0,(a0)
                    bne       FreeReady

                    * There's no virtual memory allocated anymore.
                    * Remove this routine.

_PatchLoc3          lea.l     $12345678,a0    * Dummy for first address in
                                              * root table, patched later
                    moveq     #NUM_PTR_TABLES-1,d0
                    lea       _RootTableContents(pc),a1
RestoreRootTab      move.l    (a1)+,(a0)+
                    dbra      d0,RestoreRootTab


                    move.l    a6,a1
                    move.w    #-$d2,a0
                    move.l    _ExecFreeMem(pc),d0
                    FORBID
                    jsr       _LVOSetFunction(a6)
                    lea       _FreeMemAfterExit(pc),a1
                    cmp.l     a1,d0
                    beq       FunctionKilled
                    move.l    a1,d0
                    move.w    #-$d2,a0
                    move.l    a6,a1
                    jsr       _LVOSetFunction(a6)

FunctionKilled      PERMIT
FreeReady           rts

NoVirtMemFree2:     move.l    _ExecFreeMem(pc),a0
                    jmp       (a0)

_ExecFreeMem        dc.l      0
_RemainingBytes     dc.l      0
_RootTableContents  ds.l      NUM_PTR_TABLES

_FreeMemAfterExitEnd:

                    dc.l      0              ; dummy

GarbageCollName     GARBAGE_COLL_NAME

                    end
