#    Copyright © 1995-96, The AROS Development Team. All rights reserved.
#    $Id$
#    $Log$
#    Revision 1.2  1998/10/20 16:40:29  hkiel
#    Amiga Research OS
#
#    Revision 1.1  1997/03/27 01:11:01  ldp
#    libaros.a -> libarossupport.a
#
#    Revision 1.2  1996/08/31 12:58:10  aros
#    Merged in/modified for FreeBSD.
#
#    Revision 1.1  1996/08/23 17:26:44  digulla
#    Files with functions for RT and Purify
#
#    Revision 1.3  1996/08/13 14:03:18	digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:25	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	RT_Leave
#
#   FUNCTION
#	Tell RT that you have left a function.
#
#   INPUTS
#	none
#
#   RESULT
#	none
#
#   NOTES
#	Preserves all registers.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************
	.text
	.balign	16
	.globl	RT_Leave
	.type	RT_Leave,@function
RT_Leave:
	# Push all registers
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

	# Call RT_IntLeave
	call	RT_IntLeave

	# Restore registers and return
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret

