# remap PPC registers to meaningful names
.set r1 ,sp
# the "a6" register
.set r2, base
# int return value register
.set r3, ret
# function arguments
.set r3, arg0
.set r4, arg1
.set r5, arg2
.set r6, arg3
.set r7, arg4
.set r8, arg5
.set r9, arg6
.set r10,arg7
# "scratch" register
.set r30,scr

# jsrlvo macro(offset,base)
.macro jsrlvo
	push	scr
	lwz	scr,_LVO\1+4(\2)
	mtlr	scr
	pop	scr
	blrl
.endm

# very long jump macro
.macro ljmp
	push	scr
	lwz     scr,\1(0)
	mtlr	scr
	pop	scr
	bl
.endm

# stack macros
.macro pop
	lwz	\1,0(sp)
	add	sp,sp,4
.endm

.macro push
	stwu	\1,-4(sp)
.endm

# subroutine macros
.macro subr
    push    scr
    mflr    scr
    push    scr
.endm

.macro rts
    pop    scr
    mtlr   scr
    pop    scr
    blr
.endm

# btst.x macros
.macro btstb
	push	scr
	lbz	scr,0(\2)
	rlwinm.	r0,scr,31-\1,0,0
	pop	scr
.endm

.macro btstw
	push	scr
	lhz	scr,0(\2)
	rlwinm.	r0,scr,31-\1,0,0
	pop	scr
.endm

.macro btstl
	push	scr
	lwz	scr,(\2)
	rlwinm.	r0,scr,31-\1,0,0
	pop	scr
.endm

# context save/restore macros
.macro pusha
	push	r0
	push	r1
	push	r2
	push	r3
	push	r4
	push	r5
	push	r6
	push	r7
	push	r8
	push	r9
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	push	r16
	push	r17
	push	r18
	push	r19
	push	r20
	push	r21
	push	r22
	push	r23
	push	r24
	push	r25
	push	r26
	push	r27
	push	r28
	push	r29
	push	r30
	push	r31

	mflr	r0
	push	r0
	mfcr	r0
	push	r0
.endm

.macro popa
	pop	r0
	mtcr	r0
	pop	r0
	mtlr	r0
	pop	r31
	pop	r30
	pop	r29
	pop	r28
	pop	r27
	pop	r26
	pop	r25
	pop	r24
	pop	r23
	pop	r22
	pop	r21
	pop	r20
	pop	r19
	pop	r18
	pop	r17
	pop	r16
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	r7
	pop	r6
	pop	r5
	pop	r4
	pop	r3
	pop	r2
	pop	r1
	pop	r0
.endm
