	.text
	.balign 16
	.globl	get_ssp

get_ssp:
	move.l	%ssp,%d0
	rts


	/*
 	 * Leave this!
	 */
	.text
	.balign 16
	.globl end_marker

end_marker:
	rts
