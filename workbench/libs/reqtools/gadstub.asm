
	INCLUDE "exec/types.i"
	INCLUDE "intuition/intuition.i"
	INCLUDE "utility/tagitem.i"
	INCLUDE "libraries/gadtools.i"

	SECTION "text",CODE

	XDEF		_myGT_SetGadgetAttrs
	XDEF		_myCreateGadget

	XREF		_GadToolsBase

_LVOGT_SetGadgetAttrs	equ		-$2a
_LVOCreateGadget			equ		-$1e

_myGT_SetGadgetAttrs:
	movem.l	d2/d3/a2/a3/a6,-(a7)
	movem.l	4+4*5(a7),a0/a1/a2
	lea		16+4*5(a7),a3
	move.l	_GadToolsBase(a4),a6
	jsr		_LVOGT_SetGadgetAttrs(a6)
	bra.b		endstub

_myCreateGadget:
	movem.l	d2/d3/a2/a3/a6,-(a7)
	movem.l	4+4*5(a7),d0/a0/a1
	lea		16+4*5(a7),a2
	move.l	_GadToolsBase(a4),a6
	jsr		_LVOCreateGadget(a6)
endstub:
	movem.l	(a7)+,d2/d3/a2/a3/a6
	rts

	END
