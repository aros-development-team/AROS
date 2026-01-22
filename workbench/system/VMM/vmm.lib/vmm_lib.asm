                    SECTION   CODE


                    INCLUDE   "exec/types.i"
                    INCLUDE   "exec/resident.i"
                    INCLUDE   "exec/memory.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/libraries.i"
                    INCLUDE   "exec/alerts.i"
                    INCLUDE   "exec/initializers.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "dos/dostags.i"
                    INCLUDE   "dos/dos_lib.i"
                    INCLUDE   "/shared_defs.i"
                    INCLUDE   "vmm_lib.i"

                    MACHINE   68030

VERSION             EQU       3
REVISION            EQU       0
MYPRI               EQU       0

                    * These two are missing in dos/dos_lib.i

_LVONewLoadSeg      EQU       -$300
_LVOCreateNewProc   EQU       -$1f2

                    * A library cannot be called directly

Start:              moveq     #-1,d0
                    rts

RomTag:             dc.w      RTC_MATCHWORD
                    dc.l      RomTag
                    dc.l      EndCode
                    dc.b      RTF_AUTOINIT
                    dc.b      VERSION
                    dc.b      NT_LIBRARY
                    dc.b      MYPRI
                    dc.l      VM_Name
                    dc.l      VM_Id
                    dc.l      Init

VM_Name:            LIBNAME
VM_Id:              dc.b      "$VER: vmm.library 3.0 (4.3.95)",0

DOSName:            dc.b      "dos.library",0

VMPortName          VMPORTNAME
StarterPortName     STARTER_PORT_LIB
ManagerName         VM_MANAGER_NAME
VMMName             PROGPATH
                    ds.l      0

VM_ManagerTags      dc.l      NP_Seglist,0
                    dc.l      NP_FreeSeglist,1
                    dc.l      NP_Name,ManagerName
                    dc.l      NP_StackSize,4000
                    dc.l      TAG_DONE,0


Init:               dc.l      VM_LIB_SIZE
                    dc.l      FuncTable
                    dc.l      DataTable
                    dc.l      InitRoutine

FuncTable:          dc.l      Open
                    dc.l      Close
                    dc.l      Expunge
                    dc.l      Null

                    dc.l      AllocVMem
                    dc.l      FreeVMem
                    dc.l      AvailVMem
                    dc.l      AllocVVec
                    dc.l      FreeVVec
                    dc.l      -1

DataTable:          INITBYTE  LN_TYPE,NT_LIBRARY
                    INITBYTE  LN_PRI,MYPRI
                    INITLONG  LN_NAME,VM_Name
                    INITBYTE  LIB_FLAGS,LIBF_SUMUSED+LIBF_CHANGED
                    INITWORD  LIB_VERSION,VERSION
                    INITWORD  LIB_REVISION,REVISION
                    INITLONG  LIB_IDSTRING,VM_Id
                    dc.l      0


* The init routine checks, if VMM is running. If it is, it simply requests
* the address of the memory header and the semaphore. If VMM has not been
* started yet, its code is loaded and the VM_Manager created. As soon as the
* VM_Port exists, the required data (as before) is requested.                    

InitRoutine:        * Called with:
                    *    Seglist in A0
                    *    Lib ptr in D0

                    * Store the seglist and open dos.library
                    move.l    a5,-(sp)
                    move.l    d0,a5
                    move.l    a0,VM_LIB_SEGLIST(a5)
                    moveq     #37,d0
                    lea       DOSName(pc),a1
                    jsr       _LVOOpenLibrary(a6)
                    move.l    d0,VM_DOSLIB(a5)
                    beq       InitFailure

                    * install the public starter port
                    jsr       _LVOCreateMsgPort(a6)
                    move.l    d0,VM_STARTERPORT(a5)
                    beq       InitFailure
                    move.l    d0,a1
                    lea       StarterPortName(pc),a0
                    move.l    a0,LN_NAME(a1)
                    clr.b     LN_PRI(a1)
                    jsr       _LVOAddPort(a6)

                    * Check if VMM is already running
                    lea       VMPortName(pc),a1
                    jsr       _LVOFindPort(a6)
                    move.l    d0,VM_PORT(a5)
                    bne       FoundPort

* VMM is not running yet. Load the code and start the VM_Manager
                    moveq     #VMM_SIZE,d0
                    move.l    #MEMF_PUBLIC|MEMF_CLEAR,d1
                    jsr       _LVOAllocMem(a6)
                    move.l    d0,VM_INIT_MSG(a5)
                    beq       InitFailure
                    move.l    VM_DOSLIB(a5),a6
                    lea       VMMName(pc),a0
                    move.l    d2,-(sp)
                    move.l    a0,d1
                    moveq     #0,d2
                    jsr       _LVONewLoadSeg(a6)
                    move.l    (sp)+,d2
                    lea       VM_ManagerTags(pc),a0
                    move.l    d0,ti_Data(a0)
                    move.l    d0,VM_HNDL_SEGLIST(a5)
                    beq       InitFailure
                    move.l    a0,d1
                    jsr       _LVOCreateNewProc(a6)
                    tst.l     d0
                    beq       InitFailure
                    clr.l     VM_HNDL_SEGLIST(a5)

                    * Send startup message to VM_Manager
                    move.l    4,a6
                    move.l    VM_INIT_MSG(a5),a1
                    clr.l     VMM_StartupParams(a1)
                    move.w    #VMCMD_Startup,VMM_VMCommand(a1)
                    clr.w     VMM_ReplySignal(a1)
                    move.l    d0,a0
                    adda.w    #TC_SIZE,a0
                    jsr       _LVOPutMsg(a6)
                    clr.l     VM_INIT_MSG(a5)
                    
                    move.l    VM_STARTERPORT(a5),a0
                    move.b    MP_SIGBIT(a0),d1
                    moveq     #1,d0
                    lsl.l     d1,d0
                    jsr       _LVOWait(a6)

                    move.l    VM_STARTERPORT(a5),a0
                    jsr       _LVOGetMsg(a6)
                    move.l    d0,VM_INIT_MSG(a5)
                    beq       InitFailure
                    move.l    d0,a1
                    cmp.w     #VMCMD_InitReady,VMM_VMCommand(a1)
                    bne       InitFailure
                    bra       GotInitMsg                    

FoundPort:          * Allocate a request msg to send to the VM_Manager
                    moveq     #VMM_SIZE,d0
                    move.l    #MEMF_PUBLIC|MEMF_CLEAR,d1
                    jsr       _LVOAllocMem(a6)
                    move.l    d0,VM_INIT_MSG(a5)
                    beq       InitFailure
                    move.l    d0,a1
                    move.w    #VMCMD_ReqMemHeader,VMM_VMCommand(a1)
                    move.l    ThisTask(a6),d0
                    move.l    d0,VMM_VMSender(a1)
                    move.l    VM_STARTERPORT(a5),a0
                    move.b    MP_SIGBIT(a0),d0
                    ext.w     d0
                    move.w    d0,VMM_ReplySignal(a1)
                    move.l    VM_PORT(a5),a0
                    jsr       _LVOPutMsg(a6)

                    move.l    VM_STARTERPORT(a5),a0
                    move.b    MP_SIGBIT(a0),d1
                    moveq     #1,d0
                    lsl.l     d1,d0
                    jsr       _LVOWait(a6)

GotInitMsg:         move.l    VM_INIT_MSG(a5),a1
                    move.l    VMM_VMSema(a1),VM_SEMA(a5)
                    move.l    VMM_VMHeader(a1),VM_MEMHEADER(a5)

                    bsr       Cleanup
                    move.l    a5,d0
                    move.l    (sp)+,a5
                    rts

InitFailure:        bsr       Cleanup
                    move.l    (sp)+,a5
                    moveq     #0,d0
                    rts

Cleanup:            move.l    a6,-(sp)
                    move.l    4,a6
                    move.l    VM_STARTERPORT(a5),a1
                    tst.l     a1
                    beq       NoPort
                    jsr       _LVORemPort(a6)
                    move.l    VM_STARTERPORT(a5),a0
                    jsr       _LVODeleteMsgPort(a6)

NoPort:             move.l    VM_INIT_MSG(a5),a1
                    tst.l     a1
                    beq       NoMsg
                    moveq     #VMM_SIZE,d0
                    jsr       _LVOFreeMem(a6)

NoMsg:              move.l    VM_HNDL_SEGLIST(a5),d1
                    beq       NoSegList
                    move.l    VM_DOSLIB(a5),a6
                    jsr       _LVOUnLoadSeg(a6)
                    move.l    4,a6

NoSegList:          move.l    VM_DOSLIB(a5),a1
                    jsr       _LVOCloseLibrary(a6)
                    move.l    (sp)+,a6
                    rts

*********************************************************************

Open:
                    add.w     #1,LIB_OPENCNT(a6)
                    bclr.b    #LIBB_DELEXP,LIB_FLAGS(a6)
                    move.l    a6,d0
                    rts                                                 

*********************************************************************

Close:              moveq     #0,d0
                    sub.w     #1,LIB_OPENCNT(a6)
                    bne       CloseReady
                    btst.b    #LIBB_DELEXP,LIB_FLAGS(a6)
                    beq       CloseReady
                    bsr       Expunge
CloseReady          rts

*********************************************************************

                    * Expunge not possible for now

Expunge:            tst.w     LIB_OPENCNT(a6)
                    beq       DoExpunge
                    bset.b    #LIBB_DELEXP,LIB_FLAGS(a6)
                    moveq     #0,d0
                    rts

DoExpunge           movem.l   d2/a5,-(sp)
                    move.l    a6,a5
                    move.l    4,a6
                    move.l    VM_LIB_SEGLIST(a5),d2
                    * Remove from library list
                    move.l    a5,a1
                    jsr       _LVORemove(a6)
                    * Free the library's memory
                    moveq     #0,d0
                    move.l    a5,a1
                    move.w    LIB_NEGSIZE(a5),d0
                    suba.w    d0,a1
                    add.w     LIB_POSSIZE(a5),d0
                    jsr       _LVOFreeMem(a6)
                    move.l    d2,d0
                    move.l    a5,a6
                    movem.l   (sp)+,d2/a5
                    rts

*********************************************************************

Null:               moveq     #0,d0
                    rts
      
*********************************************************************

AllocVMem           * inputs:
                    *   d0 : requested size
                    *   d1 : attributes (only MEMF_CLEAR supported)
                    *   a6 : VMMBase
                    * outputs:
                    *   d0 : pointer to allocated memory or NULL

                    movem.l   d0-d1/a2,-(sp)
                    move.l    4,a2
                    exg       a2,a6
                    move.l    VM_SEMA(a2),a0
                    jsr       _LVOObtainSemaphore(a6)  * Does not change registers
                    move.l    VM_MEMHEADER(a2),a0
                    jsr       _LVOAllocate(a6)
                    move.l    VM_SEMA(a2),a0
                    jsr       _LVOReleaseSemaphore(a6)
                    exg       a2,a6

                    move.l    d0,a0
                    movem.l   (sp)+,d0-d1
                    btst.l    #MEMB_CLEAR,d1
                    beq       NoClear

                    addq.l    #7,d0                    * Align to 8-byte boundary                
                    and.w     #$fff8,d0
                    move.l    a0,a2
                    adda.l    d0,a2                    * end pointer
                    move.l    a0,a1
                    moveq     #0,d1
ClearLoop           move.l    d1,(a1)+
                    cmpa.l    a1,a2
                    bne       ClearLoop

NoClear             move.l    (sp)+,a2
                    move.l    a0,d0
                    rts

*********************************************************************

FreeVMem            * inputs:
                    *   a1 : memory to be freed
                    *   d0 : size of block
                    *   a6 : VMMBase
                    move.l    a2,-(sp)
                    move.l    4,a2
                    exg       a2,a6
                    move.l    VM_SEMA(a2),a0
                    jsr       _LVOObtainSemaphore(a6)  * Does not change registers
                    move.l    VM_MEMHEADER(a2),a0
                    jsr       _LVODeallocate(a6)
                    move.l    VM_SEMA(a2),a0
                    jsr       _LVOReleaseSemaphore(a6)
                    exg       a2,a6
                    move.l    (sp)+,a2
                    rts

*********************************************************************

AvailVMem           * inputs:
                    *   d1 : attributes (only MEMF_LARGEST valid)
                    *   a6 : VMMBase
                    * outputs:
                    *   d0: size

                    move.l    a2,-(sp)
                    move.l    4,a2
                    exg       a2,a6
                    move.l    VM_SEMA(a2),a0
                    jsr       _LVOObtainSemaphore(a6)
                    move.l    VM_MEMHEADER(a2),a0
                    btst.l    #MEMB_LARGEST,d1
                    bne       CheckLargest
                    move.l    MH_FREE(a0),d0
                    bra       SizeFound

CheckLargest:       move.l    MH_FIRST(a0),a0
                    moveq     #0,d0

LoopChunks          tst.l     a0
                    beq       SizeFound
                    cmp.l     MC_BYTES(a0),d0
                    bpl       SmallerChunk
                    move.l    MC_BYTES(a0),d0
SmallerChunk        move.l    MC_NEXT(a0),a0
                    bra       LoopChunks

SizeFound:          move.l    VM_SEMA(a2),a0
                    jsr       _LVOReleaseSemaphore(a6)
                    exg       a2,a6
                    move.l    (sp)+,a2
                    rts

*********************************************************************

AllocVVec           * inputs:
                    *   d0 : requested size
                    *   d1 : attributes (only MEMF_CLEAR supported)
                    *   a6 : VMMBase
                    * outputs:
                    *   d0 : pointer to allocated memory or NULL

                    add.l     #4,d0
                    move.l    d0,-(sp)
                    bsr       AllocVMem
                    move.l    d0,a0
                    tst.l     d0
                    beq       NoMem
                    move.l    (sp)+,(a0)
                    addq.l    #4,d0
                    rts

NoMem:              add.w     #4,sp
                    rts

*********************************************************************

FreeVVec            * inputs:
                    *   a1 : memory to be freed
                    *   a6 : VMMBase

                    tst.l     a1
                    beq       NoFree
                    sub.w     #4,a1
                    move.l    (a1),d0
                    bra       FreeVMem

NoFree              rts

EndCode
                    end
