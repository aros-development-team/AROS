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

	INCLUDE LVOs.i
	
	XREF _SysBase
	
	XDEF _AllocVecPooled
	XDEF AllocVecPooled
_AllocVecPooled:
	move.l	_SysBase,a6
	move.l	4(a7),a0
	move.l	8(a7),d0
AllocVecPooled
	tst.l	d0
	beq.s	avp_fail
	addq.l	#4,d0
	move.l	d0,-(a7)
	jsr		_LVOAllocPooled(a6)
	move.l	(a7)+,d1
	tst.l	d0
	beq.s	avp_fail
	move.l	d0,a0
	move.l	d1,(a0)+
	move.l	a0,d0
avp_fail:
	rts
	
	XDEF _FreeVecPooled
	XDEF FreeVecPooled
_FreeVecPooled:
	move.l	_SysBase,a6
	move.l	4(a7),a0
	move.l	8(a7),a1
FreeVecPooled
	move.l	-(a1),d0
	jsr		_LVOFreePooled(a6)
	rts
