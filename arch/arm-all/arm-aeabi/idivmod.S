			.text
			.align 2
			.globl __aeabi_idiv
			.globl __aeabi_idivmod
__aeabi_idiv:
			subs	r2, r1, #0
			bxeq	lr		/* div/0 !*/

			eor	ip, r1, r0

			rsbmi	r2, r2, #0

			movs	r1, r0

			rsbmi	r1, r1, #0

			cmp	r2, r0
			movhi	r1, r0
			movhi	r0, #0
			bxhi	lr

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

			cmp	ip, #0
			rsbmi	r0, r0, #0

			bx lr

__aeabi_idivmod:	stmfd	sp!,{r0, r1, ip, lr}
			bl	__aeabi_idiv
			ldmfd	sp!,{r1, r2, ip, lr}
			mul	r3, r0, r2
			sub	r1, r1, r3
			bx	lr

			.global __aeabi_uldivmod
__aeabi_uldivmod: bx lr
