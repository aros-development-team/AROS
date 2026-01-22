                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "exec/nodes.i"
                    INCLUDE   "exec/lists.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "exec/tasks.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/memory.i"
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"

* $Id: switch_patch.asm,v 3.5 95/12/16 18:36:35 Martin_Apel Exp $

                    XDEF      _SwitchPatch
                    XDEF      _AddTaskPatch
                    XDEF      _WaitPatch
                    IFD       DEBUG
                    XDEF      _RemTaskPatch
                    XREF      _OrigRemTask
                    XDEF      _StackSwapPatch
                    XREF      _OrigStackSwap
                    ENDC

                    XREF      _OrigSwitch
                    XREF      _OrigAddTask
                    XREF      _OrigWait
                    XREF      _Free
                    XREF      _VirtAddrStart
                    XREF      _VirtAddrEnd
                    XREF      _ReportError

                    IFD       _PHXASS_
                    MACHINE   MC68030
                    ENDC

                    SECTION   CODE

* This routine is called in Supervisor mode just before the
* registers of a task are saved onto the user stack by the
* EXEC routine Switch.
* It checks if the stackpointer lies within virtual ram.
* Only then is the USP replaced by a pointer
* to non-virtual ram. This is undone, when the task is next launched


_SwitchPatch        move.l    a0,-(sp)
                    move.l    usp,a0
                    IN_VM     a0,NoVirtMemSwitch

*                    PRINT_DEB "Switch: SP = %lx",a0

                    movem.l   d0/a1/a6,-(sp)
                    lea       _Free,a0
                    move.l    4,a6
                    REMHEAD
                    tst.l     d0
                    beq       SVAlarm

                    move.l    ThisTask(a6),a1
                    move.l    d0,a0
                    move.l    a1,TS_FaultTask(a0)
                    lea       TS_TmpStack+TMP_STACKSIZE(a0),a1
                    move.l    a0,-(a1)            ; Start address of trap struct
                    move.l    usp,a0
                    move.l    a0,-(a1)            ; orig USP
                    move.w    4*4(sp),-(a1)       ; SR
                    move.l    4*4+2(sp),-(a1)     ; PC of task
                    move.l    a1,usp
                    lea       LaunchPatch(pc),a0
                    move.l    a0,4*4+2(sp)
                    movem.l   (sp)+,d0/a1/a6
                    move.l    (sp)+,a0

                    move.l    _OrigSwitch,-(sp)

                    rts                           ; call orig switch routine

                    ALIGN_LONG
NoVirtMemSwitch     move.l    (sp),a0
                    move.l    _OrigSwitch,(sp)
                    rts

SVAlarm:            
                    PRINT_DEB "Switch: SV Alarm"
                    move.l    4,a6
                    move.l    #NoTrapStructsAlertNum,d7
                    jmp       _LVOAlert(a6)

                    ALIGN_LONG
LaunchPatch:        * The trap struct stack now contains the following:
                    * high: | Start address of trap struct
                    *       | orig USP
                    *       | CCR.W 
                    * low : | PC of task

*                    PRINT_DEB "Launch"

                    move.l    a2,([6,sp],-10)     ; put a2 onto user stack
                    move.l    6(sp),a2            ; orig USP
                    sub.w     #10,a2
                    move.l    (sp),6(a2)          ; orig PC
                    move.w    4(sp),4(a2)         ; CCR

                    movem.l   d0/a0-a1/a6,-(a2)
                    move.l    10(sp),a1
                    lea       _Free,a0
                    move.l    4,a6
                    FORBID
                    ADDTAIL
                    PERMIT
                    move.l    a2,sp
                    movem.l   (sp)+,d0/a0-a1/a6
                    move.l    (sp)+,a2

                    rtr

*********************************************************************

                    ALIGN_LONG
_AddTaskPatch:      * Called with the following parameters:
                    * a1 : Pointer to Task
                    * a2 : Initial PC
                    * a3 : Final PC
                    * a6 : SysBase

                    PRINT_DEB "AddTask: PC = %lx. NewTaskName:",a2

                    IFD       DEBUG
                    move.l    a1,-(sp)
                    clr.l     -(sp)
                    move.l    LN_NAME(a1),-(sp)
                    jsr       _PrintDebugMsg
                    add.w     #8,sp
                    move.l    (sp)+,a1
                    ENDC

                    move.l    TC_SPREG(a1),d0
                    IN_VM     d0,NoVirtMemAdd

                    PRINT_DEB "AddTask: Stack in VM. PC = %lx",a2

                    movem.l   a2/a3,-(sp)
                    move.l    a1,-(sp)
                    lea       _Free,a0
                    FORBID
                    REMHEAD                       ; Get a trap struct
                    PERMIT
                    move.l    (sp)+,a1            ; Task
                    tst.l     d0
                    beq       AddTaskAlarm
                    move.l    TC_SPREG(a1),a0
                    move.l    a3,-(a0)            ; FinalPC
                    bne       FinalPCValid
                    move.l    TaskExitCode(a6),(a0)
FinalPCValid        move.l    d0,a3               ; TrapStruct

                    move.l    a5,-(sp)
                    lea       TS_TmpStack+TMP_STACKSIZE(a3),a5
                    move.l    a3,-(a5)            ; TrapStruct
                    move.l    a0,-(a5)            ; orig USP
                    clr.w     -(a5)               ; CCR
                    move.l    a5,TC_SPREG(a1)
                    move.l    (sp)+,a5

                    move.l    a2,a3
                    lea       LaunchPatch(pc),a2
                    move.l    _OrigAddTask,a0
                    jsr       (a0)
                    movem.l   (sp)+,a2-a3
                    rts

                    ALIGN_LONG
NoVirtMemAdd:       move.l    _OrigAddTask,a0
                    jmp       (a0)

AddTaskAlarm:       PRINT_DEB "AddTaskAlarm"
                    move.w    #ERR_CONTINUE,-(sp)
                    pea       AddTaskAlarmText(pc)
                    jsr       _ReportError
                    moveq     #0,d0
                    jmp       _LVOWait(a6)        ; forever

AddTaskAlarmText    dc.b      "No more trapstructs in AddTask."
                    dc.b      10             ; LF
                    dc.b      "Adding task stopped",0
                    ds.l      0

*********************************************************************

                    ALIGN_LONG
_WaitPatch:         * Called with the following parameters:
                    * d0 : Awaited Signals 
                    * a6 : SysBase
                    *
                    * Upon return:
                    * d0 : Received Signals

                    IN_VM     sp,StackNotInVM

                    move.l    sp,a0
*                    PRINT_DEB "WaitPatch: Stack in VM at %lx",a0
                    lea       _Free,a0
                    movem.l   d2/a2,-(sp)
                    move.l    d0,d2               ; save awaited signals,
                                                  ; so they can be accessed 
                                                  ; after StackSwap
                    FORBID
                    REMHEAD
                    PERMIT
                    tst.l     d0
                    beq       NoTrapStruct
                    move.l    d0,a2
                    lea       TS_TmpStackSwap(a2),a0
                    jsr       _LVOStackSwap(a6)
                    move.l    d2,d0               ; restore awaited signals
                    move.l    _OrigWait,a0
*                    PRINT_DEB "WaitPatch: Before Wait"
                    jsr       (a0)
*                    PRINT_DEB "WaitPatch: Returned from Wait. Signals %08lx",d0
                    move.l    d0,d2               ; save received signals
                    lea       TS_TmpStackSwap(a2),a0
                    jsr       _LVOStackSwap(a6)
                    lea       _Free,a0
                    move.l    a2,a1
                    FORBID
                    ADDTAIL
                    PERMIT
                    move.l    d2,d0               ; restore received signals
                    movem.l   (sp)+,d2/a2
                    rts


NoTrapStruct:       move.l    d2,d0
                    movem.l   (sp)+,d2/a2
                    PRINT_DEB "WaitPatch: No TrapStruct"
                    bra       StackNotInVM

                    ALIGN_LONG
StackNotInVM:       move.l    _OrigWait,a0
                    jmp       (a0)

*********************************************************************

                    IFD       DEBUG

_RemTaskPatch       PRINT_DEB "Calling RemTask"
                    move.l    _OrigRemTask,a0
                    jsr       (a0)
                    PRINT_DEB "RemTask returned"
                    rts

*********************************************************************

_StackSwapPatch     move.l    sp,a1
                    PRINT_DEB "Calling StackSwap. CurrentStack: %lx",a1
                    move.l    stk_Pointer(a0),d0
                    PRINT_DEB "New Stack at %lx",d0
                    move.l    _OrigStackSwap,a1
                    jmp       (a1)

                    ENDC

                    end
