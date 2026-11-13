
	.text
	.align 2
	.global __aeabi_uidiv
	.type __aeabi_uidiv,%function
	.global __aeabi_uidivmod
	.type __aeabi_uidivmod,%function

__aeabi_uidiv:
__aeabi_uidivmod:
			subs	r2, r1, #0
			bxeq	lr		/* div/0 !*/

			cmp	r2, r0
			movhi	r1, r0
			movhi	r0, #0
			bxhi	lr

			mov	r1, r0

			clz	r3, r1
			clz	r0, r2
			rsb	r0, r3, r0
			mov	r3, #1
			mov	r2, r2, asl r0
			mov	r3, r3, asl r0
			mov	r0, #0
1:
			cmp	r1, r2
			rsbcs	r1, r2, r1
			orrcs	r0, r0, r3
			movs	r3, r3, lsr #1
			mov	r2, r2, lsr #1
			bne	1b
			bx lr
