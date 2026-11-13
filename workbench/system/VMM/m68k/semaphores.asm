                    SECTION   CODE

                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "exec/semaphores.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"

                    XDEF      VMMObtainSemaphore
                    XDEF      VMMReleaseSemaphore
                    XDEF      VMMInitSemaphore
                    XDEF      _VMMObtainSemaphore
                    XDEF      _VMMReleaseSemaphore
                    XDEF      _VMMInitSemaphore

******************************************************************************
* This file implements the ObtainSemaphore and ReleaseSemaphore system calls.
* They offer the same functionality as the system calls but they don't use the 
* SIGB_SINGLE bit since this causes problems in conjunction with ramlib. 
* Additionally it doesn't support nesting of semaphore calls.
* The calls with the leading underscore take their argument on the stack
* and the ones without take them in the usual register.
******************************************************************************

 STRUCTURE  MY_SSR,SSR_SIZE
    ULONG   MY_SSR_SIGNAL
    LABEL   MY_SSR_SIZE

VMMObtainSemaphore:
                    * Called with:
                    * a0: SignalSemaphore
                    * a6: SysBase
                    * This function does not modify any registers

                    FORBID
                    addq.w    #1,SS_QUEUECOUNT(a0)
                    bne       SemaInUse
                    move.l    ThisTask(a6),SS_OWNER(a0)
                    jmp       _LVOPermit(a6)

SemaInUse:          movem.l   d0-d1/a0-a1,-(sp)
                    lea       -MY_SSR_SIZE(sp),sp
                    move.l    ThisTask(a6),SSR_WAITER(sp)
                    GET_SIGNAL
                    moveq     #0,d1
                    move.b    d0,d1
                    move.l    d1,MY_SSR_SIGNAL(sp)
                    lea       SS_WAITQUEUE(a0),a0
                    move.l    sp,a1
                    ADDTAIL
                    moveq     #1,d0
                    lsl.l     d1,d0
                    jsr       _LVOWait(a6)
                    move.l    MY_SSR_SIGNAL(sp),d0
                    RELEASE_SIGNAL
                    lea       MY_SSR_SIZE(sp),sp
                    movem.l   (sp)+,d0-d1/a0-a1
                    jmp       _LVOPermit(a6)

******************************************************************************

_VMMObtainSemaphore:
                    move.l    4(sp),a0
                    move.l    a6,-(sp)
                    move.l    4,a6
                    bsr       VMMObtainSemaphore
                    move.l    (sp)+,a6
                    rts

******************************************************************************

VMMReleaseSemaphore:
                    * Called with:
                    * a0: SignalSemaphore
                    * a6: SysBase
                    * This function does not modify any registers

                    FORBID
                    clr.l     SS_OWNER(a0)
                    subq.w    #1,SS_QUEUECOUNT(a0)
                    bge       ReviveWaiter
                    jmp       _LVOPermit(a6)

ReviveWaiter:       movem.l   d0-d1/a0-a3,-(sp)
                    move.l    a0,a2                         ; save semaphore
                    lea       SS_WAITQUEUE(a0),a0
                    REMHEADQ  a0,a1,a3                      ; head node in a1 now
                    move.l    MY_SSR_SIGNAL(a1),d1
                    moveq     #1,d0
                    lsl.l     d1,d0
                    move.l    SSR_WAITER(a1),a1
                    move.l    a1,SS_OWNER(a2)
                    jsr       _LVOSignal(a6)
                    movem.l   (sp)+,d0-d1/a0-a3
                    jmp       _LVOPermit(a6)

******************************************************************************

_VMMReleaseSemaphore:
                    move.l    4(sp),a0
                    move.l    a6,-(sp)
                    move.l    4,a6
                    bsr       VMMReleaseSemaphore
                    move.l    (sp)+,a6
                    rts

******************************************************************************

VMMInitSemaphore    * Called with:
                    * a0: SignalSemaphore

                    clr.l     SS_OWNER(a0)
                    move.w    #-1,SS_QUEUECOUNT(a0)
                    move.b    #NT_SIGNALSEM,LN_TYPE(a0)
                    lea       SS_WAITQUEUE(a0),a1
                    NEWLIST   a1
                    rts

******************************************************************************

_VMMInitSemaphore:  move.l    4(sp),a0
                    bra       VMMInitSemaphore

                    END
