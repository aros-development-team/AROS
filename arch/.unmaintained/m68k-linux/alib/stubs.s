/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

.text
	.align	2
.globl BltBitMap
	.type	BltBitMap,@function
BltBitMap:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%d5,-(%sp)
	move.l	%d6,-(%sp)
	move.l	%d7,-(%sp)
	move.l	%a2,-(%sp)
	move.l	36(%sp),%a0
	move.l	40(%sp),%d0
	move.l	44(%sp),%d1
	move.l	48(%sp),%a1
	move.l	52(%sp),%d2
	move.l	56(%sp),%d3
	move.l	60(%sp),%d4
	move.l	64(%sp),%d5
	move.l	68(%sp),%d6
	move.l	72(%sp),%d7
	move.l	76(%sp),%a2
	move.l	80(%sp),%a6
	jsr	-30(%a6)
	move.l	(%sp)+,%a2
	move.l	(%sp)+,%d7
	move.l	(%sp)+,%d6
	move.l	(%sp)+,%d5
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	BltBitMap,.-BltBitMap
.text
	.align	2
.globl BltBitMapRastPort
	.type	BltBitMapRastPort,@function
BltBitMapRastPort:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%d5,-(%sp)
	move.l	%d6,-(%sp)
	move.l	28(%sp),%a0
	move.l	32(%sp),%d0
	move.l	36(%sp),%d1
	move.l	40(%sp),%a1
	move.l	44(%sp),%d2
	move.l	48(%sp),%d3
	move.l	52(%sp),%d4
	move.l	56(%sp),%d5
	move.l	60(%sp),%d6
	move.l	64(%sp),%a6
	jsr	-606(%a6)
	move.l	(%sp)+,%d6
	move.l	(%sp)+,%d5
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	BltBitMapRastPort,.-BltBitMapRastPort
.text
	.align	2
.globl BltMaskBitMapRastPort
	.type	BltMaskBitMapRastPort,@function
BltMaskBitMapRastPort:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%d5,-(%sp)
	move.l	%d6,-(%sp)
	move.l	%a2,-(%sp)
	move.l	32(%sp),%a0
	move.l	36(%sp),%d0
	move.l	40(%sp),%d1
	move.l	44(%sp),%a1
	move.l	48(%sp),%d2
	move.l	52(%sp),%d3
	move.l	56(%sp),%d4
	move.l	60(%sp),%d5
	move.l	64(%sp),%d6
	move.l	68(%sp),%a2
	move.l	72(%sp),%a6
	jsr	-636(%a6)
	move.l	(%sp)+,%a2
	move.l	(%sp)+,%d6
	move.l	(%sp)+,%d5
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	BltMaskBitMapRastPort,.-BltMaskBitMapRastPort
.text
	.align	2
.globl ClipBlit
	.type	ClipBlit,@function
ClipBlit:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%d5,-(%sp)
	move.l	%d6,-(%sp)
	move.l	28(%sp),%a0
	move.l	32(%sp),%d0
	move.l	36(%sp),%d1
	move.l	40(%sp),%a1
	move.l	44(%sp),%d2
	move.l	48(%sp),%d3
	move.l	52(%sp),%d4
	move.l	56(%sp),%d5
	move.l	60(%sp),%d6
	move.l	64(%sp),%a6
	jsr	-552(%a6)
	move.l	(%sp)+,%d6
	move.l	(%sp)+,%d5
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	ClipBlit,.-ClipBlit
.text
	.align	2
.globl NewModifyProp
	.type	NewModifyProp,@function
NewModifyProp:
	move.l	%a6,-(%sp)
	move.l	%a2,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%d5,-(%sp)
	move.l	28(%sp),%a0
	move.l	32(%sp),%a1
	move.l	36(%sp),%a2
	move.l	40(%sp),%d0
	move.l	44(%sp),%d1
	move.l	48(%sp),%d2
	move.l	52(%sp),%d3
	move.l	56(%sp),%d4
	move.l	60(%sp),%d5
	move.l	64(%sp),%a6
	jsr	-468(%a6)
	move.l	(%sp)+,%d5
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a2
	move.l	(%sp)+,%a6
	rts
	.size	NewModifyProp,.-NewModifyProp
.text
	.align	2
.globl CreateBehindHookLayer
	.type	CreateBehindHookLayer,@function
CreateBehindHookLayer:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%a3,-(%sp)
	move.l	%a2,-(%sp)
	move.l	28(%sp),%a0
	move.l	32(%sp),%a1
	move.l	36(%sp),%d0
	move.l	40(%sp),%d1
	move.l	44(%sp),%d2
	move.l	48(%sp),%d3
	move.l	52(%sp),%d4
	move.l	56(%sp),%a3
	move.l	60(%sp),%a2
	move.l	64(%sp),%a6
	jsr	-192(%a6)
	move.l	(%sp)+,%a2
	move.l	(%sp)+,%a3
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	CreateBehindHookLayer,.-CreateBehindHookLayer
.text
	.align	2
.globl CreateUpfrontHookLayer
	.type	CreateUpfrontHookLayer,@function
CreateUpfrontHookLayer:
	move.l	%a6,-(%sp)
	move.l	%d2,-(%sp)
	move.l	%d3,-(%sp)
	move.l	%d4,-(%sp)
	move.l	%a3,-(%sp)
	move.l	%a2,-(%sp)
	move.l	28(%sp),%a0
	move.l	32(%sp),%a1
	move.l	36(%sp),%d0
	move.l	40(%sp),%d1
	move.l	44(%sp),%d2
	move.l	48(%sp),%d3
	move.l	52(%sp),%d4
	move.l	56(%sp),%a3
	move.l	60(%sp),%a2
	move.l	64(%sp),%a6
	jsr	-186(%a6)
	move.l	(%sp)+,%a2
	move.l	(%sp)+,%a3
	move.l	(%sp)+,%d4
	move.l	(%sp)+,%d3
	move.l	(%sp)+,%d2
	move.l	(%sp)+,%a6
	rts
	.size	CreateUpfrontHookLayer,.-CreateUpfrontHookLayer
