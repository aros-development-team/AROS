
#include "aros/m68k/asm.h"

#include "060sp/copyright.S"

jmp_exception:
	/* exception stack frame is ready, jump to correct exception vector */
	clr.l	%sp@-
	movem.l	%d0-%d1,%sp@-
	movec	%vbr,%d1
	move.w	%sp@(8+6),%d0 /* fetch exception vector number */
	and.w	#0xfff,%d0
	lsl.w	#2,%d0
	add.w	%d0,%d1
	move.l	%d1,%sp@(8)
	movem.l	%sp@+,%d0-%d1
	rts

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
# M68000 Hi-Performance Microprocessor Division
# M68060 Software Package Production Release 
# 
# M68060 Software Package Copyright (C) 1993, 1994, 1995, 1996 Motorola Inc.
# All rights reserved.
# 
# THE SOFTWARE is provided on an "AS IS" basis and without warranty.
# To the maximum extent permitted by applicable law,
# MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
# INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS
# FOR A PARTICULAR PURPOSE and any warranty against infringement with
# regard to the SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
# and any accompanying written materials. 
# 
# To the maximum extent permitted by applicable law,
# IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
# (INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
# BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS)
# ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
# 
# Motorola assumes no responsibility for the maintenance and support
# of the SOFTWARE.  
# 
# You are hereby granted a copyright license to use, modify, and distribute the
# SOFTWARE so long as this entire notice is retained without alteration
# in any modified and/or redistributed versions, and that such modified
# versions are clearly identified as such.
# No licenses are granted by implication, estoppel or otherwise under any
# patents or trademarks of Motorola, Inc.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

/*
#################################
# EXAMPLE CALL-OUTS 		#
# 				#
# _060_dmem_write()		#
# _060_dmem_read()		#
# _060_imem_read()		#
# _060_dmem_read_byte()		#
# _060_dmem_read_word()		#
# _060_dmem_read_long()		#
# _060_imem_read_word()		#
# _060_imem_read_long()		#
# _060_dmem_write_byte()	#
# _060_dmem_write_word()	#
# _060_dmem_write_long()	#
#				#
# _060_real_trace()		#
# _060_real_access()		#
#################################
*/

/*
#
# _060_dmem_write():
#
# Writes to data memory while in supervisor mode.
#
# INPUTS:
#	a0 - supervisor source address	
#	a1 - user destination address
#	d0 - number of bytes to write	
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d1 - 0 = success, !0 = failure
#
*/

_060_dmem_write:
	moveb	%a0@+,%a1@+	|# copy 1 byte
	subql	#0x1,%d0	|# decr byte counter
	bnes	_060_dmem_write	|# quit if ctr = 0
	moveq	#0,%d1		|# return success
	rts

/*
#
# _060_imem_read(), _060_dmem_read():
#
# Reads from data/instruction memory while in supervisor mode.
#
# INPUTS:
#	a0 - user source address
#	a1 - supervisor destination address
#	d0 - number of bytes to read
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d1 - 0 = success, !0 = failure
#
*/

_060_imem_read:
_060_dmem_read:
	moveb	%a0@+,%a1@+	|# copy 1 byte
	subql	#0x1,%d0	|# decr byte counter
	bnes	_060_dmem_read	|# quit if ctr = 0
	moveq	#0,%d1		|# return success
	rts

/*
#
# _060_dmem_read_byte():
# 
# Read a data byte from user memory.
#
# INPUTS:
#	a0 - user source address
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d0 - data byte in d0
#	d1 - 0 = success, !0 = failure
#
*/

_060_dmem_read_byte:
	moveq	#0,%d1			|# return success
	moveq	#0,%d0			|# clear whole longword
	moveb	%a0@,%d0		|# fetch byte
	rts

/*
#
# _060_imem_read_word():
# Read an instruction word from user memory.
#
# _060_dmem_read_word():
# Read a data word from user memory.
#
# INPUTS:
#	a0 - user source address
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d0 - data word in d0
#	d1 - 0 = success, !0 = failure
#
*/

_060_imem_read_word:
_060_dmem_read_word:
	moveq	#0,%d1			|# return success
	moveq	#0,%d0			|# clear whole longword
	movew	%a0@,%d0		|# fetch word
	rts

/*
# 
# _060_imem_read_long():
# Read an instruction longword from user memory.
#
# _060_dmem_read_long():
# Read an data longword from user memory.
# 
#
# INPUTS:
#	a0 - user source address
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d0 - data longword in d0
#	d1 - 0 = success, !0 = failure
#
*/

_060_imem_read_long:
_060_dmem_read_long:
	moveq	#0,%d1			|# return success
	movel	%a0@,%d0		|# fetch longword
	rts

/*
#
# _060_dmem_write_byte():
#
# Write a data byte to user memory.
#
# INPUTS:
#	a0 - user destination address
# 	d0 - data byte in d0
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d1 - 0 = success, !0 = failure
#
*/

_060_dmem_write_byte:
	moveq	#0,%d1			|# return success
	moveb	%d0,%a0@		|# store byte
	rts

/*
#
# _060_dmem_write_word():
#
# Write a data word to user memory.
#
# INPUTS:
#	a0 - user destination address
# 	d0 - data word in d0
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d1 - 0 = success, !0 = failure
#
*/

_060_dmem_write_word:
	moveq	#0,%d1			|# return success
	movew	%d0,%a0@		|# store word
	rts

/*
#
# _060_dmem_write_long():
#
# Write a data longword to user memory.
#
# INPUTS:
#	a0 - user destination address
# 	d0 - data longword in d0
# 	a6@(0x4),bit5 - 1 = supervisor mode, 0 = user mode
# OUTPUTS:
#	d1 - 0 = success, !0 = failure
#
*/

_060_dmem_write_long:
	moveq	#0,%d1			|# return success
	movel	%d0,%a0@		|# store longword
	rts

/*
#
# _060_real_trace():
#
# This is the exit point for the 060FPSP when an instruction is being traced
# and there are no other higher priority exceptions pending for this instruction
# or they have already been processed.
#
# The sample code below simply executes an "rte".
#
*/

_060_real_trace:
	bra		jmp_exception

/*
#
# _060_real_access():
#
# This is the exit point for the 060FPSP when an access error exception
# is encountered. The routine below should point to the operating system
# handler for access error exceptions. The exception stack frame is an
# 8-word access error frame.
#
# We jump directly to the 68060 buserr handler.
# If we had a sane ld, we could use use that entry point directly...
#
*/

_060_real_access:
	bra		jmp_exception

#include "m68k_060int.S"
#include "m68k_060fpu.S"
