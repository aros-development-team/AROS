#   Copyright © 2011, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Stack-realigning exit stub for x86-64
#   Lang: english

	.text
	.align	16
	.globl	TaskExitStub
	.type	TaskExitStub, @function

TaskExitStub:
	add	$16, %rsp
	ret
	.size	TaskExitStub, .-TaskExitStub
