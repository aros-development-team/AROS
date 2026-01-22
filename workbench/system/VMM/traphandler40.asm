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


* $Id: traphandler40.asm,v 3.6 95/12/16 18:36:30 Martin_Apel Exp $

                    IFD       DYN_MMU_SETUP
                    XDEF      _DynMMUTrap40
                    ELSE
                    XDEF      _TrapHandler40
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

                    MACHINE   MC68040

* On the 68040, VMM and Enforcer work together, because they use the
* same pagesize (4K). Then address 4 (pointer to ExecBase) is mapped
* as invalid. Access faults to this address should be as quick as 
* possible therefore.
* On the 68030, VMM and Enforcer don't work together since they use
* different pagesizes, thus making the speed argument for address 4
* invalid.


FRAMESIZE_B         EQU       60
FRAMESIZE_L         EQU       FRAMESIZE_B/4

SAVED_REGS          REG       d0-d1/a0-a2/a6
NUM_REGS            EQU       6

                    * Offsets into the stackframe
OFFS_SR             EQU       $0
OFFS_PC             EQU       $2
OFFS_FRAME_ID       EQU       $6
OFFS_SSW            EQU       $c
OFFS_WB3S           EQU       $e
OFFS_WB2S           EQU       $10
OFFS_WB1S           EQU       $12
OFFS_FA             EQU       $14
OFFS_WB3A           EQU       $18
OFFS_WB3D           EQU       $1c
OFFS_WB2A           EQU       $20
OFFS_WB2D           EQU       $24
OFFS_WB1A           EQU       $28
OFFS_WB1D           EQU       $2c
OFFS_PD0            EQu       $2c
OFFS_PD1            EQu       $30
OFFS_PD2            EQu       $34
OFFS_PD3            EQu       $38

                    * Bit definitions
SR_SV               EQU       5
SSW_ATC             EQU       2
SSW_MA              EQU       11
WBS_VALID           EQU       7
                    SECTION   CODE

                    IFD       DYN_MMU_SETUP
_DynMMUTrap40:
                    ELSE
_TrapHandler40:
                    ENDC

                    move.l    d0,-(sp)
                    move.w    OFFS_FRAME_ID+4(sp),d0
                    cmp.w     #$7008,d0
                    beq       right_format
                    bra       wrong_format

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

right_format        btst.b    #SSW_ATC,OFFS_SSW+4(sp)
                    beq       wrong_format        ; ATC in SSW unset

                    move.l    OFFS_FA+4(sp),d0    ; FaultAddress

                    IFND      DYN_MMU_SETUP
                    IN_VM     d0,NoVirtMem
                    ENDC

ValidAddr
*                    PRINT_DEB "TRAP: Fault for task %lx",ThisTask(a6)

                    move.l    (sp)+,d0
                    btst.b    #SR_SV,OFFS_SR(sp)
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

*                    PRINT_DEB "TRAP: SV mode 1"

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


*                    PRINT_DEB "TRAP: User mode 1"

                    move.l    4,a6
                    move.l    NUM_REGS*4+OFFS_FA(sp),d0
                    btst.b    #SSW_MA-8,OFFS_SSW+NUM_REGS*4(sp)
                    beq       NotMisaligned

                    PRINT_DEB "Misaligned fault. Address = %lx",d0
                    add.l     #(PAGESIZE-1),d0
                    and.w     #~(PAGESIZE-1),d0        ; ALIGN_UP

                    IFD       DEBUG
                    move.w    OFFS_SSW+NUM_REGS*4(sp),a1
                    PRINT_DEB "SSW = %lx",a1
                    ENDC

NotMisaligned       move.l    d0,TS_FaultAddress(a2)
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

                    * Complete possible write-backs here

                    move.l    TS_TopOfStackFrame(a2),a0

                    IFD       DEBUG
                    move.w    OFFS_SSW(a0),d0
                    and.w     #$1f,d0
                    bne       NoCachePush

                    PRINT_DEB "TRAP: Doing pushback"

                    move.l    OFFS_FA(a0),a1
                    move.l    OFFS_PD0(a0),(a1)+
                    move.l    OFFS_PD1(a0),(a1)+
                    move.l    OFFS_PD2(a0),(a1)+
                    move.l    OFFS_PD3(a0),(a1)+
NoCachePush
                    ENDC

                    move.b    OFFS_WB2S+1(a0),d1
                    move.l    OFFS_WB2D(a0),d0
                    move.l    OFFS_WB2A(a0),a1
                    bsr       DoWriteBack

                    move.b    OFFS_WB3S+1(a0),d1
                    move.l    OFFS_WB3D(a0),d0
                    move.l    OFFS_WB3A(a0),a1
                    bsr       DoWriteBack

                    move.l    a5,-(sp)                 ; UserStack
                    lea       PageIsBack(pc),a5
                    jmp       _LVOSupervisor(a6)


                    ALIGN_LONG
PageIsBack:        * PRINT_DEB "TRAP: SV mode 2"

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

**********************************************************************

                    ALIGN_LONG
DoWriteBack:        * called with:
                    * d0   : WBData
                    * a1   : WBAddress
                    * d1.b : WBStatus

                    btst.l    #WBS_VALID,d1
                    beq       NoWriteBack

                    and.b     #$60,d1
                    tst.b     d1
                    bne       NoLongWB
                    move.l    d0,(a1)
NoWriteBack         rts

                    ALIGN_LONG
NoLongWB            cmpi.b    #$40,d1
                    bne       NoWordWB
                    move.w    d0,(a1)
                    rts

                    ALIGN_LONG
NoWordWB            move.b    d0,(a1)
                    rts

********************************************************************

SVAlarm:            PRINT_DEB "TRAP: SV alarm"

                    move.l    4,a6
                    move.l    #NoTrapStructsAlertNum,d7
                    jmp       _LVOAlert(a6)

********************************************************************

               	end
