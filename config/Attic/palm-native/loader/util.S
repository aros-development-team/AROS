	.text
	.balign 16
	.globl	get_ssp

get_ssp:
	move.l	%ssp,%d0
	rts


