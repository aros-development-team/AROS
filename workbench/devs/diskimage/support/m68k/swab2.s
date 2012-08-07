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
 
	XDEF _swab2
_swab2:
	move.l	4(a7),a0
	move.l	8(a7),a1
	move.l	12(a7),d0
	moveq	#3,d1
	lsr.l	#1,d0
	beq.s   exit
	and.l	d0,d1
	beq.s	i0
	subq.l	#1,d1
	beq.s	i1
	subq.l	#1,d1
	beq.s	i2
	move.w	(a0)+,d1
	rol.w	#8,d1
	move.w	d1,(a1)+
i2:
	move.w	(a0)+,d1
	rol.w	#8,d1
	move.w	d1,(a1)+
i1:
	move.w	(a0)+,d1
	rol.w	#8,d1
	move.w	d1,(a1)+
i0:
	lsr.l	#2,d0
	beq.s	exit
	movem.w	d2-d4,-(a7)
loop:
	movem.w (a0)+,d1-d4
	rol.w	#8,d1
	rol.w	#8,d2
	rol.w	#8,d3
	rol.w	#8,d4
	movem.w	d1-d4,(a1)
	subq.l	#1,d0
	addq.l	#8,a1
	bne.s	loop
	movem.w	(a7)+,d2-d4
exit:
	rts
