/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH3(APTR, CachePreDMA,
 
    SYNOPSIS
 	AROS_LHA(APTR,    address, A0),
 	AROS_LHA(ULONG *, length,  A1),
 	AROS_LHA(ULONG,   flags,  D0),
 
    LOCATION
 	struct ExecBase *, SysBase, 127, Exec)
 
    FUNCTION
 	Do everything necessary to make CPU caches aware that a DMA will happen.
 	Virtual memory systems will make it possible that your memory is not at
 	one block and not at the address you thought. This function gives you
 	all the information you need to split the DMA request up and to convert
 	virtual to physical addresses.
 
    INPUTS
 	address - Virtual address of memory affected by the DMA
 	*length - Number of bytes affected
 	flags	- DMA_Continue	  - This is a call to continue a request that
 				    was broken up.
 		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
 				    to the device. Set this bit in bot calls.
 
    RESULT
 	The physical address in memory.
 	*length contains the number of contiguous bytes in physical memory.
 
    NOTES
 	DMA must follow a call to CachePreDMA() and must be followed
 	by a call to CachePostDMA().
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	CachePostDMA()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

/*
   XDEF AROS_SLIB_ENTRY(CachePreDMA,Exec,127)   	; for 68000/10/20/30
   XDEF AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127)	; for 68040+
*/

	#define CACHEDEBUG 0

	#include "aros/m68k/asm.h"

	#define DMAB_NoModify   2

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePreDMA_00,Exec,127)
	.type	AROS_SLIB_ENTRY(CachePreDMA_00,Exec,127),@function
AROS_SLIB_ENTRY(CachePreDMA_00,Exec,127):
	move.l	%a0,%d0	/* return input address */
	rts

#if CACHEDEBUG
format:
	.string "PreDMA(%p,%x,%x)\n"
#endif

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127)
	.type	AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127),@function
AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127):

#if CACHEDEBUG
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l %d0,-(%sp)
	move.l (%a1),-(%sp)
	move.l %a0,-(%sp)
	pea format
	jsr kprintf
	lea 16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
#endif

	movem.l %a0/%a3/%a5,-(%sp)
	move.l %a0,%d1
	or.l %a1@,%d1
	and.w #0x000f,%d1
	beq.s 0f
	/*
	 * Not cache line aligned.
	 *
	 * Details can be read here:
	 * http://groups.google.com/group/comp.sys.amiga.hardware/browse_thread/thread/6e5caefab6a68a1e/16f93d291b0b1440?hl=en&ie=UTF-8
	 *
	 * We assume MMU is already configured.
	 */

	move.l %a6@(eb_KernelBase),%a3
	move.l %a3@(kb_PlatformData),%a3
	tst.l MMU_Level_A(%a3)
	beq.s 0f
	move.l (%a1),%d0
	/* Non-cacheable page descriptor bits */
	move.b #0x40+0x20,%d1
	lea cacheprepostset,%a5
	jsr Supervisor(%a6)
	/* Store for following CachePostDMA() call */
	move.b %d0,%a3@(cachemodestore)
	bra.s 1f
0:
	move.l #0x0800,%d1
	move.l %a1@,%d0
	jsr -0x282(%a6) /* CacheClearE() */
1:
	movem.l (%sp)+,%a0/%a3/%a5
	move.l %a0,%d0
	rts

.globl cacheprepostset
cacheprepostset:
	movem.l %d2/%d3/%a2,-(%sp)
	move.l %d0,%d2
	add.l %a0,%d2
	move.l %a0,%d0
	and.w #~4095,%d0
	move.l %d0,%a2
	move.b %d1,%d3
	or.w #0x0700,%sr
0:
	nop
	cpushp %dc,(%a2)
	move.l %a2,%a0
	bsr getpagedesc
	/* adjust page descriptor cache flags */
	move.b 3(%a0),%d0
	and.b #~(0x40+0x20),3(%a0)
	or.b %d3,3(%a0)
	cpushl %dc,(%a0)
	lea 4096(%a2),%a2
	cmp.l %a2,%d2
	bhi.s 0b
	pflusha
	and.b #0x40+0x20,%d0
	movem.l (%sp)+,%d2/%d3/%a2
	rte

	/* Fetch page descriptor address. No error checks. */
getpagedesc:
	move.l MMU_Level_A(%a3),%a1
	move.l %a0,%d0
	moveq #32-7,%d1
	lsr.l %d1,%d0
	and.w #127,%d0
	move.l 0(%a1,%d0.w*4),%d0
	clr.b %d0
	move.l %d0,%a1
	move.l %a0,%d0
	moveq #32-7-7,%d1
	lsr.l %d1,%d0
	and.w #127,%d0
	move.l 0(%a1,%d0.w*4),%d0
	clr.b %d0
	move.l %d0,%a1
	move.l %a0,%d0
	moveq #32-7-7-6,%d1
	lsr.l %d1,%d0
	and.w #63,%d0
	lea 0(%a1,%d0.w*4),%a0
	rts
