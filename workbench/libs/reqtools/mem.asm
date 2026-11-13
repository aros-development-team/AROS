
	SECTION "text",CODE

	INCLUDE "exec/types.i"
	INCLUDE "exec/funcdef.i"
	INCLUDE "exec/memory.i"
	INCLUDE "exec/exec_lib.i"

	XDEF		@AllocVecPooled
	XDEF		@FreeVecPooled

	XREF		_AsmAllocPooled
	XREF		_AsmFreePooled

;
; Function to do AllocVecPooled (pool, memsize)
;                                A0    D0
@AllocVecPooled:
	move.l	a6,-(a7)
	move.l	$4.w,a6
	move.l	a0,d1
	bne.b		.pool
	move.l	#MEMF_PUBLIC!MEMF_CLEAR,d1
	jsr		_LVOAllocVec(a6)
	move.l	(a7)+,a6
	rts
.pool:
	addq.l   #4,d0						; Get space for tracking
	move.l	d0,-(sp)					; Save the size
	jsr		_LVOAllocPooled(a6)	; Call pool...
	move.l	(sp)+,d1					; Get size back...
	tst.l		d0							; Check for error
	beq.s		avp_fail					; If NULL, failed!
	move.l	d0,a0						; Get pointer...
	move.l	d1,(a0)+					; Store size
	move.l	a0,d0						; Get result
avp_fail:
	move.l	(a7)+,a6
	rts

;
; Function to do FreeVecPooled (pool, memory)
;                               A0    A1
@FreeVecPooled:
	move.l	a1,d0
	beq.b		nofree
	move.l	a6,-(a7)
	move.l	$4.w,a6
	move.l	a0,d1
	bne.b		.pool
	jsr		_LVOFreeVec(a6)
	move.l	(a7)+,a6
	rts
.pool:
	move.l	-(a1),d0					; Get size / ajust pointer
	jsr		_LVOFreePooled(a6)
	move.l	(a7)+,a6
nofree:
	rts

	END
