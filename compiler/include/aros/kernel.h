#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: TagItems for the kernel.resource
    Lang: english
*/

#include <aros/macros.h>
#include <utility/tagitem.h>

/* Type of scheduler. See KrnGetScheduler()/KrnSetScheduler() functions. */
typedef enum
{
    SCHED_RR = 1	/* Old good round robin scheduler */
} KRN_SchedType;

/* Flags for KrnMapGlobal */
typedef enum
{
	MAP_CacheInhibit 	= 0x0001,
	MAP_WriteThrough	= 0x0002,
	MAP_Guarded 		= 0x0004,

	MAP_Readable		= 0x0100,
	MAP_Writable		= 0x0200,
	MAP_Executable		= 0x0400,
	MAP_Supervisor		= 0x0800
} KRN_MapAttr;

/* Tags for kernel boot message */
#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)	 /* Kickstart base address (start of code section)	*/
#define KRN_KernelLowest        (KRN_Dummy + 2)	 /* Lowest kickstart address 				*/
#define KRN_KernelHighest       (KRN_Dummy + 3)	 /* Highest kickstart address			  	*/
#define KRN_KernelBss           (KRN_Dummy + 4)	 /* (struct KernelBSS *) - BSS sections list	  	*/
#define KRN_GDT                 (KRN_Dummy + 5)	 /* Boot-time GDT address (x86-specific)	  	*/
#define KRN_IDT                 (KRN_Dummy + 6)	 /* Boot-time IDT address (x86-specific)	  	*/
#define KRN_PL4                 (KRN_Dummy + 7)	 /* Base address of MMU control data		  	*/
#define KRN_VBEModeInfo         (KRN_Dummy + 8)	 /* VBE mode information structure			*/
#define KRN_VBEControllerInfo   (KRN_Dummy + 9)  /* VBE controller information structure		*/
#define KRN_MMAPAddress         (KRN_Dummy + 10) /* Address of memory map in multiboot format		*/
#define KRN_MMAPLength          (KRN_Dummy + 11) /* Lenght of memory map				*/
#define KRN_CmdLine             (KRN_Dummy + 12) /* Address of command line arguments			*/
#define KRN_ProtAreaStart       (KRN_Dummy + 13) /* Start address of preallocated area			*/
#define KRN_ProtAreaEnd         (KRN_Dummy + 14) /* End address of preallocated area			*/
#define KRN_VBEMode             (KRN_Dummy + 15) /* Current VBE mode number				*/
#define KRN_VBEPaletteWidth     (KRN_Dummy + 16) /* Current VBE palette width				*/
#define KRN_MEMLower         	(KRN_Dummy + 17) /* Amount of low memory in bytes (PC-specific)		*/
#define KRN_MEMUpper          	(KRN_Dummy + 18) /* Amount of upper memory in bytes (PC-specific)	*/
#define KRN_OpenFirmwareTree	(KRN_Dummy + 19) /* Pointer to OpenFirmware device tree structure	*/
#define KRN_HostInterface	(KRN_Dummy + 20) /* Pointer to host OS interface structure (hosted)	*/
#define KRN_DebugInfo		(KRN_Dummy + 21) /* Kicksrart debug information, see debug.library	*/
#define KRN_BootLoader          (KRN_Dummy + 22) /* Pointer to bootloader name string			*/
#define KRN_EFISystemTable	(KRN_Dummy + 23) /* Pointer to EFI system table				*/
#define KRN_KernelStackBase     (KRN_Dummy + 26) /* Kickstart Boot Task stack base address */
#define KRN_KernelStackSize     (KRN_Dummy + 27) /* Kickstart Boot Task stack size */
#define KRN_VMEMLower           (KRN_Dummy + 28) /* Lowest address of framebuffer, e.g. on embedded systems */
#define KRN_VMEMUpper           (KRN_Dummy + 29) /* Highest address of framebuffer, e.g. on embedded systems */
#define KRN_KernelPhysLowest    (KRN_Dummy + 30) /* Lowest *PHYSICAL* address occupied by Kernel */
#define KRN_Platform            (KRN_Dummy + 31) /* Arch specifc platform ID provided */
#define KRN_FlattenedDeviceTree (KRN_Dummy + 32) /* Flattened device tree as used e.g. by linux kernels */

/* Magic value passed by the bootstrap as second parameter */
#define AROS_BOOT_MAGIC AROS_MAKE_ID('A', 'R', 'O', 'S')

/* BSS segments descriptor */
struct KernelBSS
{
    void *addr;
    IPTR len;
};

/* Exception and IRQ handler types */
typedef int (*exhandler_t)(void *ctx, void *data, void *data2);
typedef void (*irqhandler_t)(void *data, void *data2);

/* System attributes */
#define KATTR_Architecture	(TAG_USER + 0x03F00000) /* [.G] (char *)  - Name of architecture, like "i386-pc"                        */
#define KATTR_PeripheralBase	(TAG_USER + 0x03F00001) /* [.G] (IPTR)    - SoC Peripheral IO base address (on relevant hardware)       */
#define KATTR_AffinityMask	(TAG_USER + 0x03F00002) /* [.G] (IPTR)    - */
#define KATTR_SystemLoad        (TAG_USER + 0x03F00003)
#define KATTR_CPULoad           (TAG_USER + 0x03F00004)
#define KATTR_CPULoad_END       (KATTR_CPULoad + 32)

/* Tag IDs for KrnStatMemory() */
#define KMS_Free		(TAG_USER + 0x04000000)
#define KMS_Total		(TAG_USER + 0x04000001)
#define KMS_LargestAlloc	(TAG_USER + 0x04000002)
#define KMS_SmallestAlloc	(TAG_USER + 0x04000003)
#define KMS_LargestFree		(TAG_USER + 0x04000004)
#define KMS_SmallestFree	(TAG_USER + 0x04000005)
#define KMS_NumAlloc		(TAG_USER + 0x04000006)
#define KMS_NumFree		(TAG_USER + 0x04000007)
#define KMS_PageSize		(TAG_USER + 0x04000008)

#endif /* AROS_KERNEL_H */
