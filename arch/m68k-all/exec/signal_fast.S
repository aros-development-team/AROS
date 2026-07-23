/* Fast common path for Signal() on a uniprocessor 68000. */

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Signal,Exec,54)

        .func AROS_SLIB_ENTRY(Signal,Exec,54)
AROS_SLIB_ENTRY(Signal,Exec,54):
        /* Protect signal fields and the scheduler lists. */
        move.w  #0x4000,0xdff09a
        addq.b  #1,IDNestCnt(%a6)

        /* Exceptions retain the complete generic Signal() semantics. */
        tst.l   tc_SigExcept(%a1)
        bne.s   .slow
        cmp.b   #TS_WAIT,tc_State(%a1)
        bne.s   .slow

        move.l  %d0,%d1
        and.l   tc_SigWait(%a1),%d1
        beq.s   .signal_only

        /* A higher-priority wakeup must perform the generic reschedule. */
        move.l  ThisTask(%a6),%a0
        move.b  ln_Pri(%a1),%d1
        cmp.b   ln_Pri(%a0),%d1
        bgt.s   .slow

        or.l    %d0,tc_SigRecvd(%a1)

        /* Remove the task from TaskWait. */
        move.l  %a2,%sp@-
        move.l  ln_Succ(%a1),%a0
        move.l  ln_Pred(%a1),%a2
        move.l  %a2,ln_Pred(%a0)
        move.l  %a0,ln_Succ(%a2)
        move.b  #TS_READY,tc_State(%a1)

        /* Enqueue by descending signed priority, preserving FIFO order. */
        move.l  TaskReady(%a6),%a0
.find:
        tst.l   ln_Succ(%a0)
        beq.s   .insert
        move.b  ln_Pri(%a1),%d1
        cmp.b   ln_Pri(%a0),%d1
        bgt.s   .insert
        move.l  ln_Succ(%a0),%a0
        bra.s   .find

.insert:
        move.l  ln_Pred(%a0),%a2
        move.l  %a2,ln_Pred(%a1)
        move.l  %a0,ln_Succ(%a1)
        move.l  %a1,ln_Succ(%a2)
        move.l  %a1,ln_Pred(%a0)
        move.l  %sp@+,%a2
        bra.s   .done

.signal_only:
        or.l    %d0,tc_SigRecvd(%a1)

.done:
        subq.b  #1,IDNestCnt(%a6)
        bpl.s   .return
        move.w  #0xc000,0xdff09a
.return:
        rts

.slow:
        /* Undo our Disable() nesting before entering the original C path. */
        subq.b  #1,IDNestCnt(%a6)
        bpl.s   1f
        move.w  #0xc000,0xdff09a
1:
        move.l  %a6,%sp@-
        move.l  %d0,%sp@-
        move.l  %a1,%sp@-
        jsr     Exec_SignalSlow
        lea.l   %sp@(12),%sp
        rts
        .endfunc
