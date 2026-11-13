/*

Copyright (C) 2011 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


/****i* prism2.device/UpdateMIC ********************************************
*
*   NAME
*	UpdateMIC -- Include new data into a Michael MIC value.
*
*   SYNOPSIS
*	UpdateMIC(left, right, data, count)
*
*	VOID UpdateMIC(ULONG *, ULONG *, ULONG *, ULONG);
*
*   INTERNALS
*	Register assignment relative to C implementation:
*
*	d2: l
*	d3: r
*	d4: count
*	a2: left
*	a3: right
*	a4: data
*
****************************************************************************
*
*/

	.globl	_UpdateMIC

_UpdateMIC:

	/* Anchor stack arguments */

	link	a5,#0

	/* Push registers that should be preserved on to stack */

	movem.l	d2-d7/a2-a4,-(sp)

	/* Apply transformation */

	movea.l	(8,a5),a2
	movea.l	(12,a5),a3
	movea.l	(16,a5),a4
	move.l	(20,a5),d4
	subq.w	#1,d4

	move.l	(a2),d2
	move.l	(a3),d3

mic_loop$:

	move.l	(a4)+,d0
	rol.w	#8,d0
	swap.w	d0
	rol.w	#8,d0
	eor.l	d0,d2

	move.l	d2,d0
	swap.w	d0
	rol.l	#1,d0
	eor.l	d0,d3

	add.l	d3,d2

	move.l	d2,d0
	rol.w	#8,d0
	swap.w	d0
	rol.w	#8,d0
	swap.w	d0
	eor.l	d0,d3

	add.l	d3,d2

	move.l	d2,d0
	rol.l	#3,d0
	eor.l	d0,d3

	add.l	d3,d2

	move.l	d2,d0
	ror.l	#2,d0
	eor.l	d0,d3

	add.l	d3,d2

	dbra	d4,mic_loop$

	/* Store new left and right values */

	move.l	d2,(a2)
	move.l	d3,(a3)

	/* Pop preserved registers off stack and return with original SP */

	movem.l	(sp)+,d2-d7/a2-a4
	unlk	a5
	rts




/****i* prism2.device/TKIPKeyMix2 ******************************************
*
*   NAME
*	TKIPKeyMix2 -- Apply phase 2 of the TKIP key-mixing function.
*
*   SYNOPSIS
*	TKIPKeyMix2(rc4_seed, ttak, tk, iv16)
*
*	VOID TKIPKeyMix2(UBYTE *, UWORD *, UWORD *, UWORD);
*
*   INTERNALS
*	Register assignment relative to C implementation:
*
*	d4: iv16
*	a0: temp_key
*	a1: sbox
*	a2: rc4_seed
*	a3: ttak
*	a4: tk
*
****************************************************************************
*
*/

	.globl	_TKIPKeyMix2

_TKIPKeyMix2:

	/* Anchor stack arguments */

	link	a5,#0

	/* Push registers that should be preserved on to stack */

	movem.l	d2-d7/a2-a4,-(sp)

	/* --- */

	movea.l	(12,a5),a3

	move.l	(a3)+,d5
	move.l	(a3)+,d6
	move.w	(a3)+,d7

	move.w	d7,d1
	move.w	(22,a5),d4
	add.w	d4,d1
	swap.w	d7
	move.w	d1,d7

	movea.l	(16,a5),a4
	lea	_sbox,a1

	/* --- */

	move.w	(a4)+,d0
	eor.w	d0,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d5
	add.w	d1,d5

	move.w	(a4)+,d1
	eor.w	d5,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d5
	add.w	d1,d5

	move.w	(a4)+,d1
	eor.w	d5,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d6
	add.w	d1,d6

	move.w	(a4)+,d1
	eor.w	d6,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d6
	add.w	d1,d6

	move.w	(a4)+,d1
	eor.w	d6,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d7
	add.w	d1,d7

	move.w	(a4)+,d1
	eor.w	d7,d1
	moveq	#0,d2
	move.b	d1,d2
	lsr.w	#8,d1
	lsl.w	#1,d2
	move.w	(a1,d2:w),d0
	lsl.w	#1,d1
	move.w	(a1,d1:w),d1
	rol.w	#8,d1
	eor.w	d0,d1
	swap.w	d7
	add.w	d1,d7

	/* --- */

	move.w	(a4)+,d0
	eor.w	d7,d0
	ror.w	#1,d0
	swap.w	d5
	add.w	d0,d5

	move.w	(a4)+,d0
	eor.w	d5,d0
	ror.w	#1,d0
	swap.w	d5
	add.w	d5,d0
	move.w	d0,d5

	ror.w	#1,d0
	swap.w	d6
	add.w	d6,d0
	move.w	d0,d6

	ror.w	#1,d0
	swap.w	d6
	add.w	d6,d0
	move.w	d0,d6

	ror.w	#1,d0
	swap.w	d7
	add.w	d7,d0
	move.w	d0,d7

	ror.w	#1,d0
	swap.w	d7
	add.w	d7,d0
	move.w	d0,d7

	/* Write RC4 seed */

	movea.l	(8,a5),a2

	move.w	d4,d1
	rol.w	#8,d1
	move.b	d1,(a2)+
	ori.b	#0x20,d1
	andi.b	#0x7f,d1
	move.b	d1,(a2)+
	move.b	d4,(a2)+

	movea.l	(16,a5),a4
	move.w	(a4),d1
	eor.w	d1,d0
	lsr.w	#1,d0
	move.b	d0,(a2)+

	moveq	#5,d3

	rol.w	#8,d5
	swap.w	d5
	rol.w	#8,d5
	swap.w	d5
	move.l	d5,(a2)+

	rol.w	#8,d6
	swap.w	d6
	rol.w	#8,d6
	swap.w	d6
	move.l	d6,(a2)+

	rol.w	#8,d7
	swap.w	d7
	rol.w	#8,d7
	swap.w	d7
	move.l	d7,(a2)+

	/* Pop preserved registers off stack and return with original SP */

	movem.l	(sp)+,d2-d7/a2-a4
	unlk	a5
	rts



/****i* prism2.device/RC4Encrypt *******************************************
*
*   NAME
*	RC4Encrypt -- Encrypt data using the RC4 cipher, and append CRC.
*
*   SYNOPSIS
*	RC4Encrypt(unit, data, size, buffer, seed)
*
*	VOID RC4Encrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*   INTERNALS
*	Register assignment relative to C implementation:
*
*	d2: n, i
*	d3: j
*	d4: "k"
*	d5: crc
*	a0: data
*	a1: buffer
*	a2: s
*	a3: seed, crc32
*
****************************************************************************
*
*/

	.globl	_RC4Encrypt

_RC4Encrypt:

	/* Anchor stack arguments and make space for S table */

	link	a5,#-256

	/* Push registers that should be preserved on to stack */

	movem.l	d2-d7/a2-a4,-(sp)

	/* Initialise RC4 state */

	move.l	#0x00010203,d2
	lea	(-256,a5),a2
	movea.l	a2,a0

preinit_state$:

	move.l	d2,(a0)+
	add.l	#0x04040404,d2
	bcc.b	preinit_state$

	moveq	#0,d2
	moveq	#0,d3
	move.w	#255,d4
	movea.l	(24,a5),a3
	movea.l	a2,a0

init_state$:

	move.b	(a0),d0
	andi.w	#0xf,d2
	add.b	(a3,d2:w),d3
	add.b	d0,d3

	move.b	(a2,d3:w),(a0)+
	move.b	d0,(a2,d3:w)

	addq.b	#1,d2
	dbra	d4,init_state$

	/* Encrypt data and calculate CRC */

	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	move.w	(18,a5),d4
	subq.w	#1,d4
	moveq	#-1,d5
	moveq	#0,d6
	movea.l	(12,a5),a0
	movea.l	(20,a5),a1
	lea	_crc32,a3

encrypt$:

	/* Encrypt a byte */

	addq.b	#1,d2
	move.b	(a2,d2:w),d0
	add.b	d0,d3

	move.b	(a2,d3:w),d1
	move.b	d1,(a2,d2:w)
	move.b	d0,(a2,d3:w)

	add.b	d0,d1
	move.b	(a0)+,d0
	move.b	(a2,d1:w),d1
	eor.b	d0,d1
	move.b	d1,(a1)+

	/* Update CRC */

	eor.l	d5,d0
	andi.l	#0xff,d0
	lsl.l	#2,d0
	move.l	(a3,d0:w),d0
	lsr.l	#8,d5
	eor.l	d0,d5

	dbra	d4,encrypt$

	/* Check if CRC has been encrypted */

	tst.b	d6
	bne.b	crc_done$
	moveq	#1,d6

	/* Copy CRC complement to buffer, then go back and encrypt it */

	not.l	d5
	rol.w	#8,d5
	swap.w	d5
	rol.w	#8,d5
	move.l	d5,(a1)

	movea.l	a1,a0
	moveq	#3,d4
	bra.b	encrypt$

crc_done$:

	/* Pop preserved registers off stack and return with original SP */

	movem.l	(sp)+,d2-d7/a2-a4
	unlk	a5
	rts



/****i* prism2.device/RC4Decrypt *******************************************
*
*   NAME
*	RC4Decrypt -- Decrypt data using the RC4 cipher, and check CRC.
*
*   SYNOPSIS
*	success = RC4Encrypt(unit, data, size, buffer, seed)
*
*	BOOL RC4Decrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least as big as the input data.
*
*   INTERNALS
*	This implementation skips the CRC/ICV check in order to gain speed.
*
*	Register assignment relative to C implementation:
*
*	d2: n, i
*	d3: j
*	a0: data
*	a1: buffer
*	a2: s
*	a3: seed
*
****************************************************************************
*
*/

	.globl	_RC4Decrypt

_RC4Decrypt:

	/* Anchor stack arguments and make space for S table */

	link	a5,#-256

	/* Push registers that should be preserved on to stack */

	movem.l	d2-d7/a2-a4,-(sp)

	/* Initialise RC4 state */

	move.l	#0x00010203,d2
	lea	(-256,a5),a2
	movea.l	a2,a0

preinit_state2$:

	move.l	d2,(a0)+
	add.l	#0x04040404,d2
	bcc.b	preinit_state2$

	moveq	#0,d2
	moveq	#0,d3
	move.w	#255,d4
	movea.l	(24,a5),a3
	movea.l	a2,a0

init_state2$:

	move.b	(a0),d0
	andi.w	#0xf,d2
	add.b	(a3,d2:w),d3
	add.b	d0,d3

	move.b	(a2,d3:w),(a0)+
	move.b	d0,(a2,d3:w)

	addq.b	#1,d2
	dbra	d4,init_state2$

	/* Decrypt data and calculate CRC */

	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	move.w	(18,a5),d4
	subq.w	#5,d4
	movea.l	(12,a5),a0
	movea.l	(20,a5),a1

decrypt$:

	/* Decrypt a byte */

	addq.b	#1,d2
	move.b	(a2,d2:w),d0
	add.b	d0,d3

	move.b	(a2,d3:w),d1
	move.b	d1,(a2,d2:w)
	move.b	d0,(a2,d3:w)

	add.b	d0,d1
	move.b	(a0)+,d0
	move.b	(a2,d1:w),d1
	eor.b	d1,d0
	move.b	d0,(a1)+

	dbra	d4,decrypt$

	/* Assume a good CRC */

	moveq	#-1,d0


	/* Pop preserved registers off stack and return with original SP */

	movem.l	(sp)+,d2-d7/a2-a4
	unlk	a5
	rts



/****i* prism2.device/AESEncrypt *******************************************
*
*   NAME
*	AESEncrypt -- Encrypt a block using the AES cipher.
*
*   SYNOPSIS
*	AESEncrypt(data, buffer, key)
*
*	VOID AESEncrypt(UBYTE *, UBYTE *, UBYTE *);
*
*   INTERNALS
*	Register assignment relative to C implementation:
*
*	d0: s0
*	d1: s1
*	d2: s2
*	d3: s3
*	d4: i
*	a0: s0, data
*	a1: s1
*	a2: s2
*	a3: s3
*	a4: s3, key, buffer
*	a5: te0
*
****************************************************************************
*
*/

	.globl	_AESEncrypt

_AESEncrypt:

	/* Anchor stack arguments */

	link	a5,#0

	/* Push registers that should be preserved on to stack */

	movem.l	d2-d7/a2-a4,-(sp)

	/* Initialise AES state */

	movea.l	(8,a5),a0
	movea.l	(16,a5),a4
	move.l	a5,d5

	move.l	(a0)+,d0
	move.l	(a4)+,d7
	eor.l	d7,d0
	move.l	(a0)+,d1
	move.l	(a4)+,d7
	eor.l	d7,d1
	move.l	(a0)+,d2
	move.l	(a4)+,d7
	eor.l	d7,d2
	move.l	(a0)+,d3
	move.l	(a4)+,d7
	eor.l	d7,d3

	/* Save s values in address registers */

	movea.l	d0,a0
	movea.l	d1,a1
	movea.l	d2,a2
	movea.l	d3,a3

	/* Apply first 9 rounds */

	lea	_te0,a5
	moveq	#8,d4

aes_loop$:

	/* Initialise new state from key */

	move.l	(a4)+,d0
	move.l	(a4)+,d1
	move.l	(a4)+,d2
	move.l	(a4)+,d3

	/* Inject s0-based data */

	move.l	a0,d6

	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x300*4:w,a5,d7:w*4),d7
	eor.l	d7,d1

	lsr.w	#8,d6
	move.l	(0x200*4:w,a5,d6:w*4),d7
	eor.l	d7,d2

	swap.w	d6
	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x100*4:w,a5,d7:w*4),d7
	eor.l	d7,d3

	lsr.w	#8,d6
	move.l	(a5,d6:w*4),d7
	eor.l	d7,d0

	/* Inject s1-based data */

	move.l	a1,d6

	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x300*4:w,a5,d7:w*4),d7
	eor.l	d7,d2

	lsr.w	#8,d6
	move.l	(0x200*4:w,a5,d6:w*4),d7
	eor.l	d7,d3

	swap.w	d6
	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x100*4:w,a5,d7:w*4),d7
	eor.l	d7,d0

	lsr.w	#8,d6
	move.l	(a5,d6:w*4),d7
	eor.l	d7,d1

	/* Inject s2-based data */

	move.l	a2,d6

	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x300*4:w,a5,d7:w*4),d7
	eor.l	d7,d3

	lsr.w	#8,d6
	move.l	(0x200*4:w,a5,d6:w*4),d7
	eor.l	d7,d0

	swap.w	d6
	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x100*4:w:w,a5,d7:w*4),d7
	eor.l	d7,d1

	lsr.w	#8,d6
	move.l	(a5,d6:w*4),d7
	eor.l	d7,d2

	/* Inject s3-based data */

	move.l	a3,d6

	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x300*4:w,a5,d7:w*4),d7
	eor.l	d7,d0

	lsr.w	#8,d6
	move.l	(0x200*4:w,a5,d6:w*4),d7
	eor.l	d7,d1

	swap.w	d6
	moveq	#0,d7
	move.b	d6,d7
	move.l	(0x100*4:w,a5,d7:w*4),d7
	eor.l	d7,d2

	lsr.w	#8,d6
	move.l	(a5,d6:w*4),d7
	eor.l	d7,d3

	/* Save state values in address registers */

	movea.l	d0,a0
	movea.l	d1,a1
	movea.l	d2,a2
	movea.l	d3,a3

	dbra	d4,aes_loop$

	/* Apply final round */

	lea	_te4,a5

	/* Initialise new state from key */

	move.l	(a4)+,d0
	move.l	(a4)+,d1
	move.l	(a4)+,d2
	move.l	(a4)+,d3

	/* Inject s0-based data */

	move.l	a0,d6

	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x000000ff,d7
	eor.l	d7,d1

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x0000ff00,d7
	eor.l	d7,d2

	swap.w	d6
	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x00ff0000,d7
	eor.l	d7,d3

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0xff000000,d7
	eor.l	d7,d0

	/* Inject s1-based data */

	move.l	a1,d6

	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x000000ff,d7
	eor.l	d7,d2

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x0000ff00,d7
	eor.l	d7,d3

	swap.w	d6
	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x00ff0000,d7
	eor.l	d7,d0

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0xff000000,d7
	eor.l	d7,d1

	/* Inject s2-based data */

	move.l	a2,d6

	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x000000ff,d7
	eor.l	d7,d3

	lsr.w	#6,d6
	move.l	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x0000ff00,d7
	eor.l	d7,d0

	swap.w	d6
	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x00ff0000,d7
	eor.l	d7,d1

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0xff000000,d7
	eor.l	d7,d2

	/* Inject s3-based data */

	move.l	a3,d6

	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x000000ff,d7
	eor.l	d7,d0

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x0000ff00,d7
	eor.l	d7,d1

	swap.w	d6
	move.w	d6,d7
	andi.w	#0xff,d7
	lsl.w	#2,d7
	move.l	(a5,d7:w),d7
	andi.l	#0x00ff0000,d7
	eor.l	d7,d2

	lsr.w	#6,d6
	move.w	d6,d7
	andi.w	#0x3fc,d7
	move.l	(a5,d7:w),d7
	andi.l	#0xff000000,d7
	eor.l	d7,d3

	/* Write final state to buffer */

	movea.l	d5,a5
	movea.l	(12,a5),a4

	move.l	d0,(a4)+
	move.l	d1,(a4)+
	move.l	d2,(a4)+
	move.l	d3,(a4)+

	/* Pop preserved registers off stack and return with original SP */

	movem.l	(sp)+,d2-d7/a2-a4
	unlk	a5
	rts



