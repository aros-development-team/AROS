/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: S-mode entry point for the opensbi-riscv64 target.

    OpenSBI (fw_jump/fw_payload, or QEMU -kernel) enters here in S-mode
    with:
        a0 = boot hart id
        a1 = physical address of the device tree blob

    Both are passed through untouched to kernel_cstart().
*/

	.section .aros.init,"ax"
	.globl	_start
	.type	_start,%function

/*
 * RISC-V Linux boot image header, overlaid with a PE/COFF header so the
 * same file serves three loaders:
 *
 *   - OpenSBI / qemu -kernel  : entered at _start (offset 0)
 *   - U-Boot "booti"          : reads the RISC-V header, entered at 0
 *   - UEFI                     : loads it as an EFI application and
 *                               enters at efi_stub_entry
 *
 * The first halfword doubles as the "MZ" DOS magic UEFI looks for:
 * "c.li s4, -13" encodes as 0x5a4d. Anything entering at offset 0 falls
 * through to the branch over the header, exactly as before.
 */
#define IMAGE_FILE_MACHINE_RISCV64  0x5064
#define IMAGE_SUBSYSTEM_EFI_APP     10
#define PE_OPT_MAGIC_PE32PLUS       0x020b
/* CODE | READ | EXECUTE */
#define SCN_TEXT_FLAGS              0x60000020
/* INITIALIZED_DATA | READ | WRITE */
#define SCN_DATA_FLAGS              0xC0000040

_start:
	c.li	s4, -13				/* "MZ" (code0)              */
	j	_start_kernel			/* branch over the header    */
	.balign	8
	.dword	0x200000			/* text_offset (2MiB)        */
	.dword	__kernel_end - _start		/* image_size                */
	.dword	0				/* flags: little endian      */
	.word	0x00000002			/* version 0.2               */
	.word	0				/* res1                      */
	.dword	0				/* res2                      */
	.dword	0x5643534952			/* magic "RISCV"             */
	.word	0x05435352			/* magic2 "RSC\x05"          */
	.word	pe_header - _start		/* res4: PE header offset    */

pe_header:
	.ascii	"PE\0\0"
coff_header:
	.short	IMAGE_FILE_MACHINE_RISCV64	/* Machine                   */
	.short	2				/* NumberOfSections          */
	.long	0				/* TimeDateStamp             */
	.long	0				/* PointerToSymbolTable      */
	.long	0				/* NumberOfSymbols           */
	.short	section_table - optional_header	/* SizeOfOptionalHeader      */
	.short	0x0206				/* EXECUTABLE|LINE_NUMS_STRIPPED|DEBUG_STRIPPED */
optional_header:
	.short	PE_OPT_MAGIC_PE32PLUS		/* PE32+                     */
	.byte	0x02, 0x14			/* Major/MinorLinkerVersion  */
	.long	__rodata_end - efi_header_end	/* SizeOfCode                */
	.long	__bss_start - __rodata_end	/* SizeOfInitializedData     */
	.long	0				/* SizeOfUninitializedData   */
	.long	efi_stub_entry - _start		/* AddressOfEntryPoint       */
	.long	efi_header_end - _start		/* BaseOfCode                */
	.quad	0				/* ImageBase                 */
	.long	0x1000				/* SectionAlignment          */
	.long	0x1000				/* FileAlignment             */
	.short	0, 0				/* Major/MinorOSVersion      */
	.short	0, 0				/* Major/MinorImageVersion   */
	.short	0, 0				/* Major/MinorSubsysVersion  */
	.long	0				/* Win32VersionValue         */
	.long	__kernel_end - _start		/* SizeOfImage               */
	.long	efi_header_end - _start		/* SizeOfHeaders             */
	.long	0				/* CheckSum                  */
	.short	IMAGE_SUBSYSTEM_EFI_APP		/* Subsystem                 */
	.short	0				/* DllCharacteristics        */
	.quad	0				/* SizeOfStackReserve        */
	.quad	0				/* SizeOfStackCommit         */
	.quad	0				/* SizeOfHeapReserve         */
	.quad	0				/* SizeOfHeapCommit          */
	.long	0				/* LoaderFlags               */
	.long	(section_table - .) / 8		/* NumberOfRvaAndSizes       */

	.quad	0				/* ExportTable               */
	.quad	0				/* ImportTable               */
	.quad	0				/* ResourceTable             */
	.quad	0				/* ExceptionTable            */
	.quad	0				/* CertificationTable        */
	.quad	0				/* BaseRelocationTable       */

	/*
	 * Two sections: the firmware applies W^X to loaded images, so the
	 * writable part (data and bss) must not live in the code section.
	 */
section_table:
	.ascii	".text\0\0\0"
	.long	__rodata_end - efi_header_end	/* VirtualSize               */
	.long	efi_header_end - _start		/* VirtualAddress            */
	.long	__rodata_end - efi_header_end	/* SizeOfRawData             */
	.long	efi_header_end - _start		/* PointerToRawData          */
	.long	0				/* PointerToRelocations      */
	.long	0				/* PointerToLineNumbers      */
	.short	0				/* NumberOfRelocations       */
	.short	0				/* NumberOfLineNumbers       */
	.long	SCN_TEXT_FLAGS			/* Characteristics           */

	.ascii	".data\0\0\0"
	.long	__kernel_end - __rodata_end	/* VirtualSize (incl. bss)   */
	.long	__rodata_end - _start		/* VirtualAddress            */
	.long	__bss_start - __rodata_end	/* SizeOfRawData             */
	.long	__rodata_end - _start		/* PointerToRawData          */
	.long	0				/* PointerToRelocations      */
	.long	0				/* PointerToLineNumbers      */
	.short	0				/* NumberOfRelocations       */
	.short	0				/* NumberOfLineNumbers       */
	.long	SCN_DATA_FLAGS			/* Characteristics           */

	/* The image proper starts on the first section-aligned boundary */
	.balign	4096
efi_header_end:

	.globl	_start_kernel
	.type	_start_kernel,%function
_start_kernel:
	/* Mask S-mode interrupts until the interrupt dispatcher exists */
	csrw	sie, zero

	/* Set up the global pointer (linker relaxation must be off while
	   gp itself is being loaded) */
.option push
.option norelax
	la	gp, __global_pointer$
.option pop

	/* Boot stack */
	la	sp, __boot_stack_end

	/* Install the trap handler (needs a valid sp/gp) */
	la	t0, __trap_entry
	csrw	stvec, t0

	/* Enable FPU state - LP64D kernel code may use FP registers at
	   any time. Vector state stays off; the scheduler enables and
	   tracks it per-task (see kernel_cpu.h) */
	li	t0, 0x2000		/* SSTATUS_FS_INITIAL */
	csrs	sstatus, t0

	/* Clear .bss (the ldscript 8-aligns __bss_start/__bss_end) */
	la	t0, __bss_start
	la	t1, __bss_end
1:	bgeu	t0, t1, 2f
	sd	zero, 0(t0)
	addi	t0, t0, 8
	j	1b
2:
	/* kernel_cstart(hartid, fdt) - a0/a1 still hold the SBI values */
	call	kernel_cstart

	/* Should never return; fall through to the spinner */
	.balign	4
__trap_hang:
3:	wfi
	j	3b

	.section .bss
	.balign	16
__boot_stack:
	.space	65536
__boot_stack_end:
