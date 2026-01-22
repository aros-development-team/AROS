                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/lists.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/ables.i"
                    IFD       DEBUG
                    INCLUDE   "exec/semaphores.i"
                    ENDC
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"

* $Id: traphandler30.asm,v 3.6 95/12/16 18:36:09 Martin_Apel Exp $

                    IFD       DYN_MMU_SETUP
                    XDEF      _DynMMUTrap30
                    ELSE
                    XDEF      _TrapHandler30
                    ENDC

                    XREF      _PageHandlerTask
                    XREF      _VM_ManagerProcess
                    XREF      _PageFaultSignal
                    XREF      _OrigTrapHandler
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
                    XREF      _InstructionFaults
                    XREF      _EnforcerHits
                    ENDC

                    MACHINE   MC68030

* On the 68040, VMM and Enforcer work together, because they use the
* same pagesize (4K). Then address 4 (pointer to ExecBase) is mapped
* as invalid. Access faults to this address should be as quick as 
* possible therefore.
* On the 68030, VMM and Enforcer don't work together since they use
* different pagesizes, thus making the speed argument for address 4
* invalid.


FRAME_A_SIZE_B      EQU       32                   ; Frame $A for page fault
FRAME_A_SIZE_L      EQU       FRAME_A_SIZE_B/4     ; on instruction boundary
FRAME_B_SIZE_B      EQU       92                   ; Frame $B for page fault
FRAME_B_SIZE_L      EQU       FRAME_B_SIZE_B/4     ; during instruction

SAVED_REGS          REG       d0-d2/a0-a2/a6
NUM_REGS            EQU       7

                    * Offsets into the stackframe
OFFS_SR             EQU       $0
OFFS_PC             EQU       $2
OFFS_FRAME_ID       EQU       $6
OFFS_SSW            EQU       $a
OFFS_FA             EQU       $10

                    * Bit definitions
SR_SV               EQU       5

                    SECTION   CODE

                    IFD       DYN_MMU_SETUP
_DynMMUTrap30:
                    ELSE
_TrapHandler30:
                    ENDC

                    movem.l   SAVED_REGS,-(sp)
                    move.w    OFFS_FRAME_ID+NUM_REGS*4(sp),d0
                    and.w     #$f000,d0
                    cmp.w     #$A000,d0
                    bne       NotShortFrame
                    moveq     #FRAME_A_SIZE_L,d2
                    bra       right_format
NotShortFrame:      cmp.w     #$B000,d0
                    bne       wrong_format
                    moveq     #FRAME_B_SIZE_L,d2
                    bra       right_format

NoVirtMem:
wrong_format:       ; restore original stack setting
                    IFD       DEBUG
                    addq.l    #1,_EnforcerHits
                    move.l    OFFS_FA+NUM_REGS*4(sp),d0
                    PRINT_DEB "EnforcerHit occurred for address %lx",d0
                    ENDC
                    movem.l   (sp)+,SAVED_REGS

                    ; give it to the original handler
                    IFD       DYN_MMU_SETUP
                    move.l    _OrigDynMMUTrap,-(sp)
                    ELSE
                    move.l    _OrigTrapHandler,-(sp)
                    ENDC
                    rts


                    * d2 contains the size of the used stack frame in 
                    * longwords throughout the traphandler

right_format        move.l    OFFS_FA+NUM_REGS*4(sp),d0    ; FaultAddress
                    IFND      DYN_MMU_SETUP
                    IN_VM     d0,NoVirtMem
                    ENDC

ValidAddr
*                    PRINT_DEB "TRAP: Fault for task %lx",ThisTask(a6)

                    btst.b    #SR_SV,OFFS_SR+NUM_REGS*4(sp)
                    beq       TrapFromUserMode

TrapFromSVMode:     PRINT_DEB "*** TRAP: Called from SV mode"

                    IFD       DEBUG
                    PRINT_DEB "***: ThisTask = %lx",ThisTask(a6)
                    moveq     #0,d0
                    move.w    OFFS_SR+NUM_REGS*4(sp),d0
                    PRINT_DEB "***: SR = %lx",d0
                    move.l    OFFS_PC+NUM_REGS*4(sp),d0
                    PRINT_DEB "***: PC = %lx",d0
                    move.l    usp,a0
                    PRINT_DEB "***: USP = %lx",a0
                    move.l    4,a0
                    PRINT_DEB "***: SysBase = %lx",a0
                    PRINT_DEB "***: SSP = %lx",sp
                    move.l    4,a6
                    jmp       _LVOColdReboot(a6)
                    ELSE
                    bra       SVAlarm
                    ENDC

                    ALIGN_LONG

TrapFromUserMode    * PRINT_DEB "TRAP: SV mode 1"

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
                    move.w    OFFS_SSW+NUM_REGS*4(sp),d0
                    and.w     #$7,d0
                    move.l    OFFS_FA+NUM_REGS*4(sp),a0
                    cmpi.b    #2,d0
                    bne       NoInstructionFault
*                    PRINT_DEB "TRAP: Instruction fault at %lx",a0
                    move.l    a0,-(sp)
*                    jsr       _FindHunk
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
                    move.l    d2,d0
                    lea       NUM_REGS*4(sp,d2.w*4),a1
                    subq.w    #1,d0
copy_frame:         move.l    -(a1),-(a6)
                    dbra      d0,copy_frame

                    lea       NUM_REGS*4(sp),a1
                    moveq     #NUM_REGS-1,d0
copy_reglist        move.l    -(a1),-(a6)
                    dbra      d0,copy_reglist

                    move.l    a6,usp

                    lea       NUM_REGS*4(sp,d2.w*4),sp ; destroy frame

                    andi.w    #$dfff,sr                     ; switch to
                                                            ; user mode
                    * Tmp stack format is now
                    * high:  | orig_usp
                    *        | stack frame
                    * low:   | SAVED_REGS


*                    PRINT_DEB "TRAP: User mode 1"

                    move.l    4,a6
                    move.l    NUM_REGS*4+OFFS_FA(sp),TS_FaultAddress(a2)
*                    PRINT_DEB "FaultAddress = %lx",TS_FaultAddress(a2)
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

*                    PRINT_DEB "TRAP: User mode 2"

                    move.l    _OrigWait,a0
                    jsr       (a0)            ; no need to go through patch

                    * Page is back

*                    PRINT_DEB "TRAP: User mode 3"

                    move.w    TS_WakeupSignal(a2),d0
                    RELEASE_SIGNAL

                    ELSE      ; DYN_MMU_SETUP

                    move.l    TS_FaultAddress(a2),-(sp)
                    jsr       _InstallMapping
                    add.w     #4,sp

                    ENDC

                    move.l    TS_TopOfStackFrame(a2),a0

                    move.l    a5,-(sp)                 ; UserStack
                    lea       PageIsBack(pc),a5
                    jmp       _LVOSupervisor(a6)


                    ALIGN_LONG
PageIsBack:     *    PRINT_DEB "TRAP: SV mode 2"

                    add.w     #8,sp                    ; take Supervisor
                    move.l    usp,a6                   ; stackframe from stack
                    move.l    (a6)+,a5

                    ; copy stack frame from user stack
                    move.l    d2,d0
                    lea       NUM_REGS*4(a6,d2.w*4),a1
                    subq.w    #1,d0
copy_frame2:        move.l    -(a1),-(sp)
                    dbra      d0,copy_frame2

                    move.l    NUM_REGS*4(a6,d2.w*4),a0  ; orig USP
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
