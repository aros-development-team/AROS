;* ============================================================================
;*	SetupGadgetIBox: figures the real hit box for a gadget
;*
;*			SetupGadgetIBox(gadget,domain,result)
;*					   a0    a1     a2
;*
;*	This function computes the hit box for a gadget, taking the "gadget
;*	relativity" flags, such as GRELWIDTH, into account.
;* ============================================================================

			include		"exec/types.i"
			include		"intuition/intuition.i"

			SECTION		gadgetbox.asm,CODE

			xdef		_SetupGadgetIBox,SetupGadgetIBox,@SetupGadgetIBox

; bit definitions for intuition gadget flags

GBOTTOM		equ			3
GRIGHT		equ			4
GWIDTH		equ			5
GHEIGHT		equ			6
			
_SetupGadgetIBox:
			move.l		a2,-(sp)
			movem.l		8(sp),a0-a2
			bsr			SetupGadgetIBox
			move.l		(sp)+,a2
			rts

@SetupGadgetIBox:
SetupGadgetIBox:
; two long moves faster than four word moves!
; we can do this because the structure data is adjacent word data.
;			move.w		gg_LeftEdge(a0),ibox_Left(a2)
;			move.w		gg_TopEdge(a0),ibox_Top(a2)
;			move.w		gg_Width(a0),ibox_Width(a2)
;			move.w		gg_Height(a0),ibox_Height(a2)

			move.l		gg_LeftEdge(a0),ibox_Left(a2)
			move.l		gg_Width(a0),ibox_Width(a2)

			move.w		gg_Flags(a0),d0

			btst		#GRIGHT,d0
			beq.s		1$
			move.w		ibox_Width(a1),d1
			add.w		d1,ibox_Left(a2)

1$			btst		#GBOTTOM,d0
			beq.s		2$
			move.w		ibox_Height(a1),d1
			add.w		d1,ibox_Top(a2)

2$			btst		#GWIDTH,d0
			beq.s		3$
			move.w		ibox_Width(a1),d1
			add.w		d1,ibox_Width(a2)

3$			btst		#GHEIGHT,d0
			beq.s		4$
			move.w		ibox_Height(a1),d1
			add.w		d1,ibox_Height(a2)

4$			rts

			end
