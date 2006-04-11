/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
 *  Copyright (C) 2003  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
/*
 * This code was stolen from the files enter.sh, leave.sh, lzo1x_d.sh,
 * lzo1x_f.s and lzo_asm.h in LZO version 1.08, and was heavily modified
 * to adapt it to GRUB's requirement.
 *
 * See <http://www.oberhumer.com/opensource/lzo/>, for more information
 * about LZO.
 */

#define INP	4+16(%esp)
#define INS	8+16(%esp)
#define OUTP	12+16(%esp)
#define NN	3
#define N_3	%ebp
#define N_255	$255
#define LODSB	movb (%esi), %al ;  incl %esi
#define NOTL_3(r)	xorl N_3, r
#define MOVSL(r1,r2,x)	movl (r1), x ; addl $4, r1 ; movl x, (r2) ; addl $4, r2
#define COPYL_C(r1,r2,x,rc)	9: MOVSL(r1,r2,x) ; decl rc ; jnz 9b
#define COPYL(r1,r2,x)	COPYL_C(r1,r2,x,%ecx)
	
lzo1x_decompress:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	cld

	movl	INP, %esi
	movl	OUTP, %edi
	movl	$3, %ebp

	
	xorl	%eax, %eax
	xorl	%ebx, %ebx	/* high bits 9-32 stay 0 */
	lodsb
	cmpb	$17, %al
	jbe	.L01
	subb	$17-NN, %al
	jmp	.LFLR


/***********************************************************************
// literal run
************************************************************************/

0:	addl	N_255, %eax
1:	movb	(%esi), %bl
	incl	%esi
	orb	%bl, %bl
	jz	0b
	leal	18+NN(%eax,%ebx), %eax
	jmp	3f


	.align	8
.L00:
	LODSB
.L01:
	cmpb	$16, %al
	jae	.LMATCH

	/* a literal run */
	orb	%al, %al
	jz	1b
	addl	$3+NN, %eax
3:
.LFLR:
	movl	%eax, %ecx
	NOTL_3(%eax)
	shrl	$2, %ecx
	andl	N_3, %eax
	COPYL(%esi,%edi,%edx)
	subl	%eax, %esi
	subl	%eax, %edi

	LODSB
	cmpb	$16, %al
	jae	.LMATCH


/***********************************************************************
// R1
************************************************************************/

	shrl	$2, %eax
	movb	(%esi), %bl
	leal	-0x801(%edi), %edx
	leal	(%eax,%ebx,4), %eax
	incl	%esi
	subl	%eax, %edx
	movl	(%edx), %ecx
	movl	%ecx, (%edi)
	addl	N_3, %edi
	jmp	.LMDONE


/***********************************************************************
// M2
************************************************************************/

	.align	8
.LMATCH:
	cmpb	$64, %al
	jb	.LM3MATCH

	/* a M2 match */
	movl	%eax, %ecx
	shrl	$2, %eax
	leal	-1(%edi), %edx
	andl	$7, %eax
	movb	(%esi), %bl
	shrl	$5, %ecx
	leal	(%eax,%ebx,8), %eax
	incl	%esi
	subl	%eax, %edx

	addl	$1+3, %ecx

	cmpl	N_3, %eax
	jae	.LCOPYLONG
	jmp	.LCOPYBYTE


/***********************************************************************
// M3
************************************************************************/

0:	addl	N_255, %eax
1:	movb	(%esi), %bl
	incl	%esi
	orb	%bl, %bl
	jz	0b
	leal	33+NN(%eax,%ebx), %ecx
	xorl	%eax, %eax
	jmp	3f


	.align	8
.LM3MATCH:
	cmpb	$32, %al
	jb	.LM4MATCH

	/* a M3 match */
	andl	$31, %eax
	jz	1b
	lea	2+NN(%eax), %ecx
3:
	movw	(%esi), %ax
	leal	-1(%edi), %edx
	shrl	$2, %eax
	addl	$2, %esi
	subl	%eax, %edx

	cmpl	N_3, %eax
	jb	.LCOPYBYTE


/***********************************************************************
// copy match
************************************************************************/

	.align	2
.LCOPYLONG:			 /* copy match using longwords */
	leal	-3(%edi,%ecx), %eax
	shrl	$2, %ecx
	COPYL(%edx,%edi,%ebx)
	movl	%eax, %edi
	xorl	%ebx, %ebx

.LMDONE:
	movb	-2(%esi), %al
	andl	N_3, %eax
	jz	.L00
.LFLR3:
	movl	(%esi), %edx
	addl	%eax, %esi
	movl	%edx, (%edi)
	addl	%eax, %edi

	LODSB
	jmp	.LMATCH


	.align	8
.LCOPYBYTE:			 /* copy match using bytes */
	xchgl	%edx,%esi
	subl	N_3,%ecx

	rep
	movsb
	movl	%edx, %esi
	jmp	.LMDONE


/***********************************************************************
// M4
************************************************************************/

0:	addl	N_255, %ecx
1:	movb	(%esi), %bl
	incl	%esi
	orb	%bl, %bl
	jz	0b
	leal	9+NN(%ebx,%ecx), %ecx
	jmp	3f


	.align	8
.LM4MATCH:
	cmpb	$16, %al
	jb	.LM1MATCH

	/* a M4 match */
	movl	%eax, %ecx
	andl	$8, %eax
	shll	$13, %eax	/* save in bit 16 */
	andl	$7, %ecx
	jz	1b
	addl	$2+NN, %ecx
3:
	movw	(%esi), %ax
	addl	$2, %esi
	leal	-0x4000(%edi), %edx
	shrl	$2, %eax
	jz	.LEOF
	subl	%eax, %edx
	jmp	.LCOPYLONG


/***********************************************************************
// M1
************************************************************************/

	.align	8
.LM1MATCH:
	/* a M1 match */
	shrl	$2, %eax
	movb	(%esi), %bl
	leal	-1(%edi), %edx
	leal	(%eax,%ebx,4), %eax
	incl	%esi
	subl	%eax, %edx

	movb	(%edx), %al	/* we must use this because edx can be edi-1 */
	movb	%al, (%edi)
	movb	1(%edx), %bl
	movb	%bl, 1(%edi)
	addl	$2, %edi
	jmp	.LMDONE


/***********************************************************************
//
************************************************************************/

.LEOF:
/****	xorl	%eax,%eax	   eax=0 from above */

	cmpl	$3+NN, %ecx	/* ecx must be 3/6 */
	setnz	%al

	/* check compressed size */
	movl	INP, %edx
	addl	INS, %edx
	cmpl	%edx, %esi	 /* check compressed size */
	ja	.L_input_overrun
	jb	.L_input_not_consumed

.L_leave:
	negl	%eax
	jnz	1f
	
	subl	OUTP, %edi	 /* write back the uncompressed size */
	movl	%edi, %eax

1:	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

.L_input_not_consumed:
        movl    $8, %eax         /* LZO_E_INPUT_NOT_CONSUMED */
        jmp     .L_leave

.L_input_overrun:
        movl    $4, %eax         /* LZO_E_INPUT_OVERRUN */
        jmp     .L_leave

#undef INP
#undef INS
#undef OUTP
#undef NN
#undef NN
#undef N_3
#undef N_255
#undef LODSB
#undef NOTL_3
#undef MOVSL
#undef COPYL_C
#undef COPYL
