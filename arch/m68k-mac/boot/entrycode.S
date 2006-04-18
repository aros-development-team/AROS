	.section entrycodes,"aw",@progbits

	// For the following structur have a look at the POSE source
	// code SrcShared/Palm/Platform/Core/System/IncsPrv/MemoryPrv.h

	.globl initial_ssp
initial_ssp:
	.dc.l  0x100000                 // initial value for SSP
//	.dc.l  hardware_reset_init_1  // where the startup code is at.

	.globl hardware_reset_init_1
	.section .text
	.align 4 
hardware_reset_init_1:
	move.l #((_end)+0x10000),%sp
	move.l #0,%a0
	move.l #0,%d0
myloop:
	move.l %d0,%a0
	add.l #0xf9000e00,%a0
	move.l #0x00000000,(%a0)
	add.l #8,%d0
	cmpil #0x500,%d0
	bles myloop
	jmp main_init

	.globl software_reset
	.align 4 

software_reset:
	lea    initial_ssp,%a0
	move.l (%a0),%ssp
//	move.w (_end)+0x1000,%sp
	jmp    main_init

