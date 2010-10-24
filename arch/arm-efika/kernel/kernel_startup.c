/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,%function\n"
	"start: ldr	r12, 1f\n\t"
	"		ldr	pc, [r12]\n"
	"1:		.word startup"
);

void startup()
{
	while(1);
}
