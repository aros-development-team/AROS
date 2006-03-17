	.section entrycodes,"aw",@progbits

	// For the following structur have a look at the POSE source
	// code SrcShared/Palm/Platform/Core/System/IncsPrv/MemoryPrv.h

	.globl initial_ssp
initial_ssp:
	.dc.l  0xfffc                 // initial value for SSP
	.dc.l  hardware_reset_init_1  // where the startup code is at.

	.globl hardware_reset_init_1
	.section .text
	.align 8
hardware_reset_init_1:
        jmp hardware_reset_init_2.w(%pc)       // needs to be exactly this
	.dc.b  'b','o','o','t',0x27,0x10,0xff  // way, so xcopilot likes the
                                               // rom

	.globl hardware_reset_init_2
	.align 8
hardware_reset_init_2:
	jmp main_init

	.globl software_reset
	.align 8

software_reset:
	lea    initial_ssp,%a0
	move.l (%a0),%ssp
	jmp    main_init
