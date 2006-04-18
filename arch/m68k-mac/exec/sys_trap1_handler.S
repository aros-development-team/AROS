/*   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: sys_trap1_handler.S 13629 2002-03-15 17:27:41Z stegerg $
*/

	/*
         * Handle trap 1 function calls
	 * Since POSE seems to intercept all other traps except for trap #1
	 * I need to multiplex the trap calls here.
	 */
	.text
	.balign 16
	.globl _sys_trap1_handler

_sys_trap1_handler:
    	move.l  %a0,-(%ssp)
	lea.l	jumptable,%a0     // get the address of the jumptable
	move.l	(%a0,%d7),-(%ssp) // save the address of the function onto the stack
	move.l	4(%ssp),%a0
	move.l	(%ssp)+,(%ssp)
	rts
	
//	move.l	%a0,-8(%ssp)      // save %a0 onto the stack
//	lea.l	jumptable,%a0     // get the address of the jumptable
//	move.l	(%a0,%d7),-(%ssp) // save the address of the function onto the stack
//	move.l  -4(%sp),%a0       // restore %a0
//	rts                       // call the function

	.data
	.balign 4
jumptable:
	dc.l	_sys_dispatch         // d7 = 0
	dc.l	_sys_trap1_SetSR      // d7 = 4
	dc.l	_sys_trap1_Supervisor // d7 = 8
	dc.l	_sys_trap1_SuperState // d7 = 12
