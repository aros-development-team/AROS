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
	move.b 'A',%d0
	jsr serial_putc
	jmp main_init

	.globl software_reset
	.align 4 

software_reset:
	lea    initial_ssp,%a0
	move.l (%a0),%ssp
//	move.w (_end)+0x1000,%sp
	jmp    main_init

/*
 * Serial port debug output support.
 */

#define MAC_USE_SCC_B

#if !defined(MAC_USE_SCC_A) && !defined(MAC_USE_SCC_B)
#define MAC_USE_SCC_B
#endif
#define mac_scc_cha_b_ctrl_offset	0x0
#define mac_scc_cha_a_ctrl_offset	0x2
#define mac_scc_cha_b_data_offset	0x4
#define mac_scc_cha_a_data_offset	0x6

/*
 * Output character on serial port.
 */

	.globl serial_putc
	.align 4
serial_putc:
	cmpib	#'\n',%d0
	jbne	1f
	/* A little safe recursion is good for the soul */
	move.b	%d0,%d1
	move.b	#'\r',%d0
	jbsr	serial_putc
	move.b	%d1,%d0
1:
#ifdef MAC_USE_SCC_A
	movel	#0x50f0c020,%a1
3:	btst	#2,%a1@(mac_scc_cha_a_ctrl_offset)
	jeq	3b
	moveb	%d0,%a1@(mac_scc_cha_a_data_offset)
#endif	/* MAC_USE_SCC_A */

#ifdef MAC_USE_SCC_B
#ifndef MAC_USE_SCC_A	/* Load mac_sccbase only if needed */
	movel	#0x50f0c020,%a1
#endif	/* MAC_USE_SCC_A */
4:	btst	#2,%a1@(mac_scc_cha_b_ctrl_offset)
	jeq	4b
	moveb	%d0,%a1@(mac_scc_cha_b_data_offset)
#endif	/* MAC_USE_SCC_B */
	rts
