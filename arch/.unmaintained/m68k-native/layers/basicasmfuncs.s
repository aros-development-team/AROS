/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include "machine.i"

	.text
	.balign 4

	.globl	AROS_CDEFNAME(_BltRPtoCR)
	.type	AROS_CDEFNAME(_BltRPtoCR),@function
AROS_CDEFNAME(_BltRPtoCR):
	movem.l	d2-d7/a2-a3/a6,-(sp)
	move.l	a0,a2			/* rp                                    */
	move.l	a1,a3			/* cr                                    */
	move.l	d0,d6			/* mode (minterm)                        */
	movea.l	rp_BitMap(a2),a0	/* rp->BitMap                            */
	move.w	cr_MinX(a3),d0
	ext.l	d0			/* cr->bounds.MinX                       */
	move.w	cr_MinY(a3),d1
	ext.l	d1			/* cr->bounds.MinY                       */
	movea.l	cr_BitMap(a3),a1	/* cr->BitMap                            */
	move.w	cr_MinX(a3),d2
	ext.l	d2
	moveq.l	#0xf,d3
	and.l	d3,d2			/* cr->bounds.MinX & 0xf                 */
	move.w	cr_MinX(a3),d3
	ext.l	d3
	move.w	cr_MaxX(a3),d4
	ext.l	d4
	sub.l	d3,d4
	addq.l	#1,d4			/* cr->bounds.MaxX - cr->bounds.MinX + 1 */
	move.w	cr_MinY(a3),d3
	ext.l	d3
	move.w	cr_MaxX(a3),d5
	ext.l	d5
	sub.l	d3,d5
	addq.l	#1,d5			/* cr->bounds.MaxY - cr->bounds.MinY + 1 */
	moveq.l	#-1,d7			/* mask */
	suba.l	a2,a2			/* tempa */
	movea.l	lb_GfxBase(a6),a6	/* get GfxBase from LayersBase */
	moveq.l	#0,d3			/* dest y */
	jsr	BltBitMap(a6)
	movem.l	(sp)+,d2-d7/a2-a3/a6
	rts

	.globl	AROS_CDEFNAME(_BltCRtoRP)
	.type	AROS_CDEFNAME(_BltCRtoRP),@function
AROS_CDEFNAME(_BltCRtoRP):
	movem.l	d2-d7/a2-a3/a6,-(sp)
	move.l	a0,a2			/* rp                                    */
	move.l	a1,a3			/* cr                                    */
	move.l	d0,d6			/* mode (minterm)                        */
	movea.l	cr_BitMap(a3),a0	/* rp->BitMap                            */
	move.w	cr_MinX(a3),d0
	ext.l	d0
	moveq.l	#0xf,d1
	and.l	d1,d0			/* cr->bounds.MinX & 0xf                 */
	movea.l	rp_BitMap(a2),a1	/* cr->BitMap                            */
	move.w	cr_MinX(a3),d2
	ext.l	d2			/* cr->bounds.MinX                       */
	move.w	cr_MinY(a3),d3
	ext.l	d3			/* cr->bounds.MinY                       */
	move.w	cr_MinX(a3),d1
	ext.l	d1
	move.w	cr_MaxX(a3),d4
	ext.l	d4
	sub.l	d1,d4
	addq.l	#1,d4			/* cr->bounds.MaxX - cr->bounds.MinX + 1 */
	move.w	cr_MinY(a3),d1
	ext.l	d1
	move.w	cr_MaxY(a3),d5
	ext.l	d5
	sub.l	d1,d5
	addq.l	#1,d5			/* cr->bounds.MaxY - cr->bounds.MinY + 1 */
	moveq.l	#-1,d7			/* mask */
	suba.l	a2,a2			/* tempa */
	movea.l	lb_GfxBase(a6),a6	/* get GfxBase from LayersBase */
	moveq.l	#0,d1			/* src y */
	jsr	BltBitMap(a6)
	movem.l	(sp)+,d2-d7/a2-a3/a6
	rts
