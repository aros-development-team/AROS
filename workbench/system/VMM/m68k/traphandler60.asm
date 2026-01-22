                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/lists.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"
                    IFD       DEBUG
                    INCLUDE   "exec/semaphores.i"
                    ENDC


* $Id: traphandler60.asm,v 1.3 95/12/16 18:36:31 Martin_Apel Exp $

                    IFD       DYN_MMU_SETUP
                    XDEF      _DynMMUTrap60
                    ELSE
                    XDEF      _TrapHandler60
                    ENDC

                    XREF      _PageHandlerTask
                    XREF      _VM_ManagerProcess
                    XREF      _PageFaultSignal
                    XREF      _Free
                    XREF      _PageReq
                    XREF      _VirtAddrStart
                    XREF      _VirtAddrEnd
                    XREF      _OrigWait

                    IFD       DYN_MMU_SETUP
                    XREF      _InstallMapping
                    XREF      _OrigDynMMUTrap
                    ELSE
                    XREF      _OrigTrapHandler
                    ENDC

                    IFD       DEBUG
                    XREF      _EnforcerHits
                    XREF      _InstructionFaults
                    XREF      _FindHunk
                    ENDC

                    MACHINE   MC68060

* On the 68040 and probably on the 68060 too, VMM and Enforcer work 
* together, because they use the same pagesize (4K). Then address 4 
* (pointer to ExecBase) is mapped as invalid. Access faults to this 
* address should be as quick as possible therefore.
* On the 68030, VMM and Enforcer don't work together since they use
* different pagesizes, thus making the speed argument for address 4
* invalid.


FRAMESIZE_B         EQU       16
FRAMESIZE_L         EQU       FRAMESIZE_B/4

SAVED_REGS          REG       d0-d1/a0-a2/a6
NUM_REGS            EQU       6

                    * Offsets into the stackframe
OFFS_SR             EQU       $0
OFFS_PC             EQU       $2
OFFS_FRAME_ID       EQU       $6
OFFS_FA             EQU       $8
OFFS_FSLW           EQU       $C

                    * Bit definitions

                    BITDEF    SR,SV,5
                    BITDEF    FSLW,MA,27
                    BITDEF    FSLW,LK,25
                    BITDEF    FSLW,IO,15
                    BITDEF    FSLW,PBE,14
                    BITDEF    FSLW,SBE,13
                    BITDEF    FSLW,PTA,12
                    BITDEF    FSLW,PTB,11
                    BITDEF    FSLW,IL,10
                    BITDEF    FSLW,PF,9
                    BITDEF    FSLW,SP,8
                    BITDEF    FSLW,WP,7
                    BITDEF    FSLW,TWE,6
                    BITDEF    FSLW,RE,5
                    BITDEF    FSLW,WE,4
                    BITDEF    FSLW,TTR,3
                    BITDEF    FSLW,BPE,2
                    BITDEF    FSLW,SEE,0

                    BITDEF    CACR,CABC,22

UNWANTED_EVENTS     EQU       (FSLWF_LK|FSLWF_PBE|FSLWF_SBE|FSLWF_IL|FSLWF_SP|FSLWF_WP|FSLWF_TWE|FSLWF_RE|FSLWF_WE|FSLWF_TTR|FSLWF_SEE)

                    SECTION   CODE

                    IFD       DYN_MMU_SETUP
_DynMMUTrap60:
                    ELSE
_TrapHandler60:
                    ENDC

                    move.l    d0,-(sp)
                    move.w    OFFS_FRAME_ID+4(sp),d0
                    cmp.w     #$4008,d0
                    beq       right_format

NoVirtMem:
wrong_format:       ; restore original stack setting
                    IFD       DEBUG
                    move.l    OFFS_FA+4(sp),d0    ; FaultAddress
                    cmpi.l    #4,d0
                    beq       SysBaseAccess
                    addq.l    #1,_EnforcerHits
                    PRINT_DEB "EnforcerHit occurred for address %lx",d0
                    move.l    OFFS_PC+4(sp),-(sp)
                    jsr       _FindHunk
                    addq.w    #4,sp
SysBaseAccess
                    ENDC
                    move.l     (sp)+,d0

                    ; give it to the original handler
                    IFD       DYN_MMU_SETUP
                    move.l    _OrigDynMMUTrap,-(sp)
                    ELSE
                    move.l    _OrigTrapHandler,-(sp)
                    ENDC
                    rts

right_format        move.l    OFFS_FSLW+4(sp),d0
                    PRINT_DEB "Trap60: FSLW = %08lx",d0
                    and.l     #UNWANTED_EVENTS,d0
                    bne       wrong_format

                    move.l    OFFS_FA+4(sp),d0    ; FaultAddress

                    IFND      DYN_MMU_SETUP
                    IN_VM     d0,NoVirtMem
                    ENDC

ValidAddr
                    PRINT_DEB "TRAP: Fault for task %lx",ThisTask(a6)

                    move.l    (sp)+,d0
                    btst.b    #SRB_SV,OFFS_SR(sp)
                    beq       TrapFromUserMode

TrapFromSVMode:     PRINT_DEB "*** TRAP: Called from SV mode"

                    IFD       DEBUG
                    PRINT_DEB "***: ThisTask = %lx",ThisTask(a6)
                    moveq     #0,d0
                    move.w    OFFS_SR(sp),d0
                    PRINT_DEB "***: SR = %lx",d0
                    move.l    OFFS_PC(sp),d0
                    PRINT_DEB "***: PC = %lx",d0
                    move.l    d0,-(sp)
                    jsr       _FindHunk
                    add.w     #4,sp

                    move.l    OFFS_FA(sp),d0
                    PRINT_DEB "***: FA = %lx",d0
                    move.l    usp,a0
                    PRINT_DEB "***: USP = %lx",a0
                    move.l    4,a0
                    PRINT_DEB "***: SysBase = %lx",a0
                    PRINT_DEB "***: SSP = %lx",sp
                    ENDC

                    bra       SVAlarm

                    ALIGN_LONG

TrapFromUserMode    movem.l   SAVED_REGS,-(sp)

                    PRINT_DEB "TRAP: SV mode 1"

                    move.l    OFFS_FSLW+NUM_REGS*4(sp),d0
                    btst.l    #FSLWB_BPE,d0
                    beq       NoBranchPredError
                    PRINT_DEB "Trap60: Clearing branch prediction cache"
                    movec     CACR,d0
                    bset.l    #CACRB_CABC,d0
                    movec     d0,cacr
NoBranchPredError

                    IFD       DEBUG

                    move.l    4,a6
                    cmpi.b    #-1,IDNestCnt(a6)
                    beq       DisableNotBroken
                    PRINT_DEB "Disable broken"
DisableNotBroken    cmpi.b    #-1,TDNestCnt(a6)
                    beq       ForbidNotBroken

                    lea       _VirtMemSema,a0
                    move.l    SS_OWNER(a0),d0
                    cmp.l     ThisTask(a6),d0
                    beq       ForbidNotBroken
ForbidBroken        PRINT_DEB "Forbid broken"
ForbidNotBroken     
                    move.l    OFFS_FSLW+NUM_REGS*4(sp),d0
                    btst.l    #FSLWB_IO,d0
                    beq       NoInstructionFault
                    move.l    OFFS_FA+NUM_REGS*4(sp),a0
                    PRINT_DEB "TRAP: Instruction fault at %lx",a0
                    move.l    a0,-(sp)
                    jsr       _FindHunk
                    add.w     #4,sp
                    addq.l    #1,_InstructionFaults
NoInstructionFault
                    ENDC

                    lea       _Free,a0
                    REMHEAD
                    tst.l     d0
                    move.l    d0,a2               ; does not affect CCR
                    beq       SVAlarm

                    lea       TS_TmpStack+TMP_STACKSIZE(a2),a6
                    move.l    usp,a0                        ; Store orig usp
                    move.l    a0,-(a6)
                    lea       FRAMESIZE_B+NUM_REGS*4(sp),a1
                    moveq     #FRAMESIZE_L-1,d0
copy_frame:         move.l    -(a1),-(a6)
                    dbra      d0,copy_frame

                    lea       NUM_REGS*4(sp),a1
                    moveq     #NUM_REGS-1,d0
copy_reglist        move.l    -(a1),-(a6)
                    dbra      d0,copy_reglist

                    move.l    a6,usp

                    lea       FRAMESIZE_B+NUM_REGS*4(sp),sp ; destroy frame

                    andi.w    #$dfff,sr                     ; switch to
                                                            ; user mode
                    * Tmp stack format is now
                    * high:  | orig_usp
                    *        | stack frame
                    * low:   | SAVED_REGS


                    PRINT_DEB "TRAP: User mode 1"

                    move.l    4,a6
                    move.l    NUM_REGS*4+OFFS_FA(sp),d0
                    move.l    OFFS_FSLW+NUM_REGS*4(sp),d1
                    and.l     #(FSLWF_IO|FSLWF_MA),d1
                    cmp.l     #FSLWF_MA,d1
                    bne       NotMisaligned

                    PRINT_DEB "Misaligned fault. Address = %lx",d0
                    add.l     #(PAGESIZE-1),d0
                    and.w     #~(PAGESIZE-1),d0        ; ALIGN_UP

NotMisaligned       move.l    d0,TS_FaultAddress(a2)
                    PRINT_DEB "FaultAddress = %lx",TS_FaultAddress(a2)
                    move.l    ThisTask(a6),TS_FaultTask(a2)

                    lea       NUM_REGS*4(sp),a1
                    move.l    a1,TS_TopOfStackFrame(a2)

                    IFND      DYN_MMU_SETUP
                    GET_SIGNAL
                    ext.b     d0
                    move.w    d0,TS_WakeupSignal(a2)

                    lea       _PageReq,a0
                    move.l    a2,a1
                    FORBID
                    ADDTAIL
                    PERMIT

                    move.w    _PageFaultSignal,d1
                    moveq     #1,d0
                    move.l    _PageHandlerTask,a1
                    lsl.l     d1,d0
                    jsr       _LVOSignal(a6)

                    moveq     #1,d0
                    move.w    TS_WakeupSignal(a2),d1
                    lsl.l     d1,d0

                    PRINT_DEB "TRAP: User mode 2"

                    move.l    _OrigWait,a0
                    jsr       (a0)            ; no need to go through patch

                    * Page is back

                    PRINT_DEB "TRAP: User mode 3"

                    move.w    TS_WakeupSignal(a2),d0
                    RELEASE_SIGNAL

                    ELSE      ; DYN_MMU_SETUP

                    move.l    TS_FaultAddress(a2),-(sp)
                    jsr       _InstallMapping
                    add.w     #4,sp

                    ENDC

                    move.l    a5,-(sp)                 ; UserStack
                    lea       PageIsBack(pc),a5
                    jmp       _LVOSupervisor(a6)


                    ALIGN_LONG
PageIsBack:         PRINT_DEB "TRAP: SV mode 2"

                    add.w     #8,sp                    ; take Supervisor
                    move.l    usp,a6                   ; stackframe from stack
                    move.l    (a6)+,a5

                    ; copy stack frame from user stack
                    lea       FRAMESIZE_B+NUM_REGS*4(a6),a1
                    moveq     #FRAMESIZE_L-1,d0
copy_frame2:        move.l    -(a1),-(sp)
                    dbra      d0,copy_frame2

                    move.l    FRAMESIZE_B+NUM_REGS*4(a6),a0  ; orig USP
                    move.l    a0,usp

                    lea       _Free,a0
                    move.l    a2,a1
                    ADDTAIL

*                    PRINT_DEB "TRAP: SV mode 3"

                    movem.l   (a6),SAVED_REGS
                    rte

********************************************************************

SVAlarm:            PRINT_DEB "TRAP: SV alarm"

                    move.l    4,a6
                    move.l    #NoTrapStructsAlertNum,d7
                    jmp       _LVOAlert(a6)

********************************************************************

               	end
