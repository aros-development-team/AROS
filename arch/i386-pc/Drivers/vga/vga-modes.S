/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <include/machine.i>

			.text
			.globl	SetVGAMode
			_FUNCTION(SetVGAMode)

SetVGAMode:
			pushl	%ebp
			movl	%esp,%ebp
			pushl	%ecx
			pushl	%edx
			pushl	%ebx
			pushl	%edi

			movl	$0x3c3,%edx
			movb	$1,%al
			outb	%al,%dx

			movl	8(%ebp),%edi	/* Get modTable */
			movl	$21,%ecx	/* 21 bytes */
			movb	$0,%bl
			movl	$0x300,%edx
l1:
			movb	$0xda,%dl	/* Magic sequence */
			inb	%dx,%al		/* Reset Flip-flop */
			
			movb	$0xc0,%dl	/* Set register number */
			movb	%bl,%al
			outb	%al,%dx
			nop
			nop
			nop
			nop
			movb	(%edi),%al
			addl	$1,%edi
			outb	%al,%dx

			movb	$0xda,%dl	/* Keep the display on */
			inb	%dx,%al

			movb	$0xc0,%dl
			movb	$0x20,%al
			outb	%al,%dx

			movb	$0xda,%dl
			inb	%dx,%al
			
			addb	$1,%bl
			subl	$1,%ecx
			jne	l1

			movb	(%edi),%al
			addl	$1,%edi
			movb	$0xc2,%dl
			outb	%al,%dx

			movb	$5,%cl
			movb	$0,%bl
			movb	$0xc4,%dl
l2:
			movb	%bl,%al
			outb	%al,%dx
			incb	%dl
			movb	(%edi),%al
			incl	%edi
			outb	%al,%dx
			decb	%dl
			incb	%bl
			decb	%cl
			jne	l2

			movb	(%edi),%al
			incl	%edi
			movb	$0xc6,%dl
			outb	%al,%dx
			movb	(%edi),%al
			incl	%edi
			movb	$0xda,%dl
    /*			outb	%al,%dx      */

			movb	$9,%cl
			movb	$0,%bl
			movb	$0xce,%dl
l3:
			movb	%bl,%al
			outb	%al,%dx
			incb	%dl
			movb	(%edi),%al
			incl	%edi
			outb	%al,%dx
			decb	%dl
			incb	%bl
			decb	%cl
			jne	l3
			
			movb	$25,%cl
			movb	$0,%bl
			movb	$0xd4,%dl
			movb	$0x11,%al
			outb	%al,%dx
			incb	%dl
			movb	$0,%al
			outb	%al,%dx
			decb	%dl

l4:
			movb	%bl,%al
			outb	%al,%dx
			incb	%dl
			movb	(%edi),%al
			incl	%edi
			outb	%al,%dx
			decb	%dl
			incb	%bl
			decb	%cl
			jne	l4

			movl	$0,c_x
			movl	$0,c_y
			
			popl	%edi
			popl	%ebx
			popl	%edx
			popl	%ecx
			
			popl	%ebp
			ret

			.globl	SetVGAPal
			_FUNCTION(SetVGAPal)

SetVGAPal:		pushl	%ebp
			movl	%esp,%ebp
			pushl	%edx
			pushl	%esi
			pushl	%ecx

			movl	8(%ebp),%esi
			movl	12(%ebp),%eax
			movl	16(%ebp),%ecx
			movl	$0x3c8,%edx

			outb	%al,%dx
			incb	%dl
__svp_l:		movb	(%esi),%al
			incl	%esi
			outb	%al,%dx
			movb	(%esi),%al
			incl	%esi
			outb	%al,%dx
			movb	(%esi),%al
			incl	%esi
			outb	%al,%dx
			decl	%ecx
			jne	__svp_l
			
			popl	%ecx
			popl	%esi
			popl	%edx
			popl	%ebp
			ret

			.globl	InitGfxAROS
			_FUNCTION(InitGfxAROS)
			
InitGfxAROS:		pushl	%ebp
			movl	%esp,%ebp
			pushl	%ebx
			pushl	%ecx
			pushl	%edx
			pushl	%esi
			pushl	%edi

			movl	$0,xc_x
			movl	$1,__draw_enable			
			movl	$0,c_x
			movl	$0,c_y
			
			pushl	$2
			call	SetMode
			pushl	$16
			pushl	$0
			pushl	$DefPal
			call	SetVGAPal
			addl	$16,%esp
			
			movl	$0x3ce,%edx
			movl	$0x0700,%eax
			outw	%ax,%dx
			movl	$0x0f01,%eax
			outw	%ax,%dx
			
			movl	$9600,%ecx
			movl	$-1,%eax
			movl	$0x000a0000,%edi
			cld
			rep
			stosl

			movl	$0x3ce,%edx
			movl	$0x0000,%eax
			outw	%ax,%dx
			movl	$0x0001,%eax
			outw	%ax,%dx

			movl	$AROS_planes,%esi
			
			movl	$0x3c4,%edx
			movl	$0x0102,%eax
			outw	%ax,%dx
			movl	$0x000a0000,%edi
			call	__copy_bmp

			movl	$0x0202,%eax
			outw	%ax,%dx
			movl	$0x000a0000,%edi
			call	__copy_bmp

			movl	$0x0402,%eax
			outw	%ax,%dx
			movl	$0x000a0000,%edi
			call	__copy_bmp

			movl	$0x0802,%eax
			outw	%ax,%dx
			movl	$0x000a0000,%edi
			call	__copy_bmp

			movl	$0x3c4,%edx
			movl	$0x0f02,%eax
			outw	%ax,%dx
			
			movl	$0x3ce,%edx
			movl	$0x0f01,%eax
			outw	%ax,%dx
			movl	$0x000a0076,%edi

			movl	$16,%ecx
			movl	$0x0000,%eax
__ig_1:			outw	%ax,%dx
			movb	$-1,(%edi)
			incl	%edi
			addl	$0x0100,%eax			
			decl	%ecx
			jne	__ig_1	

			popl	%edi
			popl	%esi
			popl	%edx
			popl	%ecx
			popl	%ebx
			popl	%ebp
			ret

__copy_bmp:		movl	$123,%ecx
__iga_1:		movl	$37,%ebx
__iga_2:		movb	(%esi),%al
			movb	%al,(%edi)
			addl	$1,%esi
			addl	$1,%edi
			decl	%ebx
			jne	__iga_2
			addl	$43,%edi
			incl	%esi
			decl	%ecx
			jne	__iga_1
__ret1:			ret

			.globl	putc
			_FUNCTION(putc)

c_x:	.long	0
c_y:	.long	0

			.globl	__draw_enable
__draw_enable:		.long	0

putc:			cmpl	$0,__draw_enable
			je	__ret1
			pushl	%ebp
			movl	%esp,%ebp
			pushl	%esi
			pushl	%edi
			pushl	%ecx
			pushl	%edx
			pushl	%ebx

			movl	8(%ebp),%eax
			
			cmpb	$10,%al
			je	__pc_2

			shll	$3,%eax

			movl	c_y,%edi
			imull	$720,%edi
			addl	c_x,%edi
			addl	$0x000a0000,%edi
			
			addl	$9840,%edi	/* Skip AROS Logo */
			
			movl	$acorndata_8x8,%esi
			addl	%eax,%esi

			movl	$0x3ce,%edx
			movl	$0x0001,%eax
			outw	%ax,%dx
			
			movl	$0x3c4,%edx
			movl	$0x0702,%eax
			outw	%ax,%dx

			movl	$8,%ecx
__pc_lop:		movb	(%esi),%al
			notb	%al
			movb	%al,(%edi)
			incl	%esi
			addl	$80,%edi
			decb	%cl
			jne	__pc_lop

			incl	c_x
			cmpl	$80,c_x
			jb	__pc_1

__pc_2:			movl	$0,c_x
			incl	c_y

			cmpl	$39,c_y
			jb	__pc_1
			
			subl	$3,c_y

			movl	$0x3ce,%edx
			movl	$0x0105,%eax
			outw	%ax,%dx
			movl	$0x000a2670,%edi
			movl	$0x000a2ee0,%esi
			movl	$26400,%ecx
			cld
			rep
			movsb
			movl	$0x0005,%eax
			outw	%ax,%dx
			movl	$0x0700,%eax
			outw	%ax,%dx
			movl	$0x0f01,%eax
			outw	%ax,%dx
			
			movl	$540,%ecx
			movl	$-1,%eax
			cld
			rep
			stosl

			movl	$0x3ce,%edx
			movl	$0x0000,%eax
			outw	%ax,%dx
			movl	$0x0001,%eax
			outw	%ax,%dx
			movl	$0x0005,%eax
			outw	%ax,%dx
			movl	$0x3c4,%edx
			movl	$0x0f02,%eax
			outw	%ax,%dx

__pc_1:			popl	%ebx
			popl	%edx
			popl	%ecx
			popl	%edi
			popl	%esi
			popl	%ebp
			ret

			.globl	xputc
			_FUNCTION(xputc)

xc_x:	.long	0

xputc:			cmpl	$0,__draw_enable
			je	__ret1
			pushl	%ebp
			movl	%esp,%ebp
			pushl	%esi
			pushl	%edi
			pushl	%ecx
			pushl	%edx
			pushl	%ebx

			movl	12(%ebp),%edi

			movl	8(%ebp),%eax
			
			cmpb	$10,%al
			jne	xpc_ok

			movl	$0,xc_x
			jmp	xpc_ex

xpc_ok:			shll	$3,%eax

			imull	$1440,%edi
			addl	xc_x,%edi
			addl	$0x000a0000,%edi
			
			addl	$2038,%edi	/* Skip AROS Logo */
			
			movl	$acorndata_8x8,%esi
			addl	%eax,%esi

			movl	$0x3ce,%edx
			movl	$0x0001,%eax
			outw	%ax,%dx

			movl	$0x3c4,%edx
			movl	$0x0702,%eax
			outw	%ax,%dx

			movl	$8,%ecx
x__pc_lop:		movb	(%esi),%al
			notb	%al
			movb	%al,(%edi)
			movb	%al,80(%edi)
			incl	%esi
			addl	$160,%edi
			decb	%cl
			jne	x__pc_lop

			incl	xc_x

			movl	$0x3ce,%edx
			movl	$0x0000,%eax
			outw	%ax,%dx
			movl	$0x0001,%eax
			outw	%ax,%dx
			movl	$0x0005,%eax
			outw	%ax,%dx
			movl	$0x3c4,%edx
			movl	$0x0f02,%eax
			outw	%ax,%dx

xpc_ex:			popl	%ebx
			popl	%edx
			popl	%ecx
			popl	%edi
			popl	%esi
			popl	%ebp
			ret
