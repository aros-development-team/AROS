# jmplvo macro
.macro jmplvo
	lwz	r2,_LVO\1+4(r2)
	mtlr	r2
	blrl
.endm

# very long jump macro
.macro ljmp
	li	r2,\1
	mtlr	r2
	blrl
.endm

# remap PPC registers to meaningful names
.set r1,sp
.set r2,base
.set r3,ret
.set r3,p1
.set r4,p2
.set r5,p3
.set r6,p4
.set r7,p5
.set r8,p6
.set r9,p7
.set.r10,p8


# stack macros
.macro pop
	lwz		\1,0(sp)
	addi		sp,sp,4
.endm

.macro push
	stwu		\1,-4(sp)
.endm

# subroutine macros
.macro subr
    push    r31
    mflr    r31
    push    r31
.endm

.macro rts
    pop    r31
    mtlr    r31
    pop    r31
    blr
.endm

# btst.x macros
.macro btstb # (n,rA)
	push		r31
	lbz		r31,0(\2)
	rlwinm.	r0,r2,31-\1,0,0
	pop		r31
.endm

.macro btstw
	push		r31
	lhz		r31,0(\2)
	rlwinm.	r0,r2,31-\1,0,0
	pop		r31
.endm

.macro btstl
	push		r31
	lwz		r31,(\2)
	rlwinm.	r0,r2,31-\1,0,0
	pop		r31
.endm
