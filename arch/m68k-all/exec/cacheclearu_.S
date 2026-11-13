/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME
 
 	AROS_LH0(void, CacheClearU,
 
    LOCATION
 	struct ExecBase *, SysBase, 106, Exec)
 
    FUNCTION
 	Flushes the contents of all CPU caches in a simple way.
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
	68000/10: do nothing
	68020/30: clear instruction cache and (030) data cache
	68040/60: push dirty lines to memory and invalidate both caches
 
    HISTORY
 
******************************************************************************/


/*
   XDEF AROS_SLIB_ENTRY(CacheClearU_00,Exec,106)   	; for 68000/68010
   XDEF AROS_SLIB_ENTRY(CacheClearU_20,Exec,106)	; for 68020/68030
   XDEF AROS_SLIB_ENTRY(CacheClearU_40,Exec,106)	; for 68040/68060
   XDEF AROS_SLIB_ENTRY(CacheClearU_60,Exec,106)	; for 68060 (clears not only
						; the i and d caches, but also
						; the branch cache)
*/

	#include "aros/m68k/asm.h"
	#include "cache.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearU_00,Exec,106)
	.type	AROS_SLIB_ENTRY(CacheClearU_00,Exec,106),@function
AROS_SLIB_ENTRY(CacheClearU_00,Exec,106):
	/* Simple 68000s have no chaches */

#if CACHEDEBUG
	bsr		cachedebug
#endif

	rts

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearU_20,Exec,106)
	.type	AROS_SLIB_ENTRY(CacheClearU_20,Exec,106),@function
AROS_SLIB_ENTRY(CacheClearU_20,Exec,106):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1		/* Save a5 */
	lea.l	cacheclearusup_20,%a5
	jmp	Supervisor(%a6)	/* No jsr: this saves an rts */

cacheclearusup_20:
	or.w	#0x0700,%sr	/* Disable interrupts so cacr can not be influenced
				   while we clear the caches */
	movec	%cacr,%d0
	or.w	#0x0808,%d0	/* Set CD and CI bit in cacr */
	movec	%d0,%cacr
	move.l	%a1,%a5		/* Restore a5 */
	rte			/* This rte will restore the SR from the stack */

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearU_40,Exec,106)
	.type	AROS_SLIB_ENTRY(CacheClearU_40,Exec,106),@function
AROS_SLIB_ENTRY(CacheClearU_40,Exec,106):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea.l	cacheclearusup_40,%a5
	jmp	Supervisor(%a6)

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearU_60,Exec,106)
	.type	AROS_SLIB_ENTRY(CacheClearU_60,Exec,106),@function
AROS_SLIB_ENTRY(CacheClearU_60,Exec,106):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea.l	cacheclearusup_60,%a5
	jmp	Supervisor(%a6)

cacheclearusup_40:
	nop /* old 68040 revision bug workaround */
cacheclearusup_60:
	cpusha	%bc	/* Push dirty cache lines to memory and invalidate both caches */
	move.l	%a1,%a5
	rte

#if CACHEDEBUG
cachedebug:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	pea format
	jsr kprintf
	addq.l #4,%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
format:
	.string "CU()\n"
#endif
