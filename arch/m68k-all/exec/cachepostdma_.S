/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************
 
    NAME
 
 	AROS_LH3(void, CachePostDMA,
 
    SYNOPSIS
 	AROS_LHA(APTR,    address, A0),
 	AROS_LHA(ULONG *, length,  A1),
 	AROS_LHA(ULONG,   flags,  D0),
 
    LOCATION
 	struct ExecBase *, SysBase, 128, Exec)
 
    FUNCTION
 	Do everything necessary to make CPU caches aware that a DMA has
 	happened.
 
    INPUTS
 	address - Virtual address of memory affected by the DMA
 	*length - Number of bytes affected
 	flags	- DMA_NoModify	  - Indicate that the memory did not change.
 		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
 				    to the device. Set this bit in bot calls.
 
    RESULT
 
    NOTES
 	DMA must follow a call to CachePreDMA() and must be followed
 	by a call to CachePostDMA().
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	CachePreDMA()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

/*
   XDEF AROS_SLIB_ENTRY(CachePostDMA_00,Exec,128)  	; for 68000/10/20
   XDEF AROS_SLIB_ENTRY(CachePostDMA_30,Exec,128)	; for 68030+
   XDEF AROS_SLIB_ENTRY(CachePostDMA_40,Exec,128)	; for 68040/68060
*/

	#include "aros/m68k/asm.h"
	#include "cache.h"

	#define DMAB_Continue	    1  /* Continuation flag for CachePreDMA */
	#define DMAB_NoModify	    2  /* Set if DMA does not update memory */
	#define	DMAB_ReadFromRAM    3  /* Set if DMA goes *FROM* RAM to device */

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePostDMA_00,Exec,128)
	.type	AROS_SLIB_ENTRY(CachePostDMA_00,Exec,128),@function
AROS_SLIB_ENTRY(CachePostDMA_00,Exec,128):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	rts

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePostDMA_30,Exec,128)
	.type	AROS_SLIB_ENTRY(CachePostDMA_30,Exec,128),@function
AROS_SLIB_ENTRY(CachePostDMA_30,Exec,128):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	btst.l	#DMAB_NoModify,%d0	/* Has DMA modified data in mem? */
	bne.s	cpd_30_end		/* nope, just exit */
	move.l	%a5,%a1
	lea.l	cachepostdmasup_30,%a5
	jsr     Supervisor(%a6)
cpd_30_end:
	rts

cachepostdmasup_30:
	move.l	%a1,%a5
	/* A DMA device has changed data in main memory. We have to clear
	   the data cache, so we get the chance to see this new data. */
	or.w	#0x0700,%sr	/* Disable interrupts */
	movec	%cacr,%d0
	bset.l	#11,%d0		/* Set CD Clear Data cache bit */
	movec	%d0,%cacr
	rte

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CachePostDMA_40,Exec,128)
	.type	AROS_SLIB_ENTRY(CachePostDMA_40,Exec,128),@function
AROS_SLIB_ENTRY(CachePostDMA_40,Exec,128):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	movem.l %a3/%a5,-(%sp)

	btst	#DMAB_ReadFromRAM,%d0
	bne.s	0f

	move.l %a0,%d1
	or.l %a1@,%d1
	and.w #0x000f,%d1
	beq.s 0f
	/* Not cache line aligned.
	 * Check CachePreDMA for details. */
	move.l %a6@(eb_KernelBase),%a3
	move.l %a3@(kb_PlatformData),%a3
	tst.l MMU_Level_A(%a3)
	beq.s 0f
	move.l %a1@,%d0
	moveq #1,%d1
	lea cacheprepostset,%a5
	jsr Supervisor(%a6)
0:
#if CACHEFULLFLUSH
	jsr -0x27C(%a6) /* CacheClearU() */
#else
	move.l #0x0800,%d1
	move.l %a1@,%d0
	jsr -0x282(%a6) /* CacheClearE() */
#endif
1:
	movem.l (%sp)+,%a3/%a5
	rts


#if CACHEDEBUG
cachedebug:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l %d0,-(%sp)
	move.l (%a1),-(%sp)
	move.l %a0,-(%sp)
	pea format
	jsr kprintf
	lea 16(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
format:
	.string "PostDMA(%p,%x,%x)\n"
#endif
