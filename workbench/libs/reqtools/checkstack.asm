
	SECTION 'text',CODE

	INCLUDE "exec/funcdef.i"
	INCLUDE "exec/exec_lib.i"
	INCLUDE "exec/tasks.i"
	INCLUDE "dos/dosextens.i"
	INCLUDE	"libraries/reqtools.i"
	INCLUDE	"lvo/reqtools_lib.i"

* This function will make sure there are at least 4096 bytes free on
* the stack.  If this is not the case it will allocate a new stack and
* use this for your function.

* Usage of CheckStackCallFunc():
*   Push your real functions' address on the stack.
*   Call CheckStackCallFunc()
*   (note that you CANNOT pass arguments on the stack to your function
*    because it is possible your function will run with a newly allocated
*    _clear_ stack!)

ThisTask			equ	$114

	XDEF _CheckStackCallFunc

_CheckStackCallFunc:
	movem.l	d0-d7/a0-a6,-(a7)		; push all regs
	move.l	$4.w,a6
	move.l	ThisTask(a6),a3
	move.l	#4096,d7
;	move.l	pr_CLI(a3),d0
;	beq.b	fromwb
;	lsl.l	#2,d0
;	move.l	d0,a0
;	move.l	cli_DefaultStack(a0),d1
;	lsl.l	#2,d1
;	bra.b	dostack
;fromwb:

	cmp.l	TC_SPUPPER(a3),a7
	bcc.b	swapstack

	cmp.l	TC_SPLOWER(a3),a7
	bcs.b	swapstack

	move.l	a7,d1
	sub.l	TC_SPLOWER(a3),d1
dostack:
	cmp.l	d7,d1
	bcc.b	okstack

swapstack:
	moveq	#StackSwapStruct_SIZEOF,d0
	add.l	d7,d0				; room for stackswap struct
	move.l	#$10000,d1
	jsr	_LVOAllocVec(a6)
	tst.l	d0
	beq.b	nostack

	move.l	d0,a4
	add.l	d7,a4

	move.l	d0,stk_Lower(a4)		; lower bound
	move.l	a4,stk_Upper(a4)		; upper bound

	move.l	a4,a2

	move.l	d0,-(a2)			; push address of new stack on new stack
	lea	returnaddr2(PC),a0
	move.l	a0,-(a2)			; push returnaddr2 on new stack

	move.l	60+4(a7),d0
	move.l	d0,-(a2)			; push function address on new stack

	lea	60(a7),a1			; copy over regs from old to new stack
	moveq	#14,d0
.copyregs:
	move.l	-(a1),-(a2)
	dbra	d0,.copyregs

	move.l	a2,stk_Pointer(a4)		; current stack pointer
	move.l	a4,a0
	jsr	_LVOStackSwap(a6)

	movem.l	(a7)+,d0-d7/a0-a6
	rts					; call function, return to returnaddr2

returnaddr2:
	move.l	(a7)+,a4			; get address of new stack
	move.l	d0,d7
	move.l	a4,a0
	add.l	#4096,a0
	move.l	$4.w,a6
	jsr	_LVOStackSwap(a6)		; swap back old stack
	move.l	a4,a1
	jsr	_LVOFreeVec(a6)
	move.l	d7,d0
	addq.l	#4,a7				; purge old d0
	movem.l	(a7)+,d1-d7/a0-a6
	bra.b	endcheckstack

okstack:
	movem.l	(a7)+,d0-d7/a0-a6
	pea	endcheckstack(PC)
	move.l	8(a7),-(a7)			; push function's address
	rts					; call function

nostack:
	movem.l	(a7)+,d0-d7/a0-a6
	moveq	#0,d0
endcheckstack:
	move.l	(a7),4(a7)
	addq.l	#4,a7
	rts

	END
