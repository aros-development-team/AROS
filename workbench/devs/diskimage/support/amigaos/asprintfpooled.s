 ; Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
 ;
 ; Redistribution and use in source and binary forms, with or without
 ; modification, are permitted provided that the following conditions
 ; are met:
 ;
 ; 1. Redistributions of source code must retain the above copyright
 ;    notice, this list of conditions and the following disclaimer.
 ;
 ; 2. Redistributions in binary form must reproduce the above copyright
 ;    notice, this list of conditions and the following disclaimer in the
 ;    documentation and/or other materials provided with the distribution.
 ;
 ; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 ; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 ; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 ; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 ; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 ; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 ; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 ; POSSIBILITY OF SUCH DAMAGE.

	INCLUDE	LVOs.i

	XREF	_SysBase
	XREF	AllocVecPooled
	XREF	CountPutCh
	XREF	SNPrintfPutCh

	XDEF	_ASPrintfPooled
_ASPrintfPooled:
	pea		12(a7)
	move.l	12(a7),-(a7)
	move.l	12(a7),-(a7)
	bsr.s	_VASPrintfPooled
	add.l	#12,a7
	rts
	
	XDEF	_VASPrintfPooled
_VASPrintfPooled:
	movem.l	a2/a3/a6,-(a7)
	move.l	20(a7),a0
	move.l	24(a7),a1
	clr.l	-(a7)
	lea		CountPutCh(pc),a2
	move.l	_SysBase,a6
	move.l	a7,a3
	jsr		_LVORawDoFmt(a6)
	move.l	20(a7),a0
	move.l	(a7),d0
	jsr		AllocVecPooled
	move.l	d0,-(a7)
	bne.s	gotmem
	addq.l	#8,a7
	movem.l (a7)+,a2/a3/a6
	rts
gotmem:
	move.l	28(a7),a0
	move.l	32(a7),a1
	lea		SNPrintfPutCh(pc),a2
	move.l	a7,a3
	move.l	d0,-(a7)
	jsr		_LVORawDoFmt(a6)
	move.l	(a7)+,d0
	addq.l	#8,a7
	movem.l (a7)+,a2/a3/a6
	rts
