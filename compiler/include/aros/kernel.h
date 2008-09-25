#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: TagItems for the kernel.resource
    Lang: english
*/

#include <utility/tagitem.h>

/* Type of scheduler. See KrnGetScheduler()/KrnSetScheduler() functions. */
typedef enum {
    SCHED_RR = 1	/* Old good round robin scheduler */
} KRN_SchedType;

/* Flags for KrnMapGlobal */
typedef enum {
	MAP_CacheInhibit 	= 0x0001,
	MAP_WriteThrough	= 0x0002,
	MAP_Guarded 		= 0x0004,

	MAP_Readable		= 0x0100,
	MAP_Writable		= 0x0200,
	MAP_Executable		= 0x0400,
} KRN_MapAttr;

#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)
#define KRN_KernelLowest        (KRN_Dummy + 2)
#define KRN_KernelHighest       (KRN_Dummy + 3)
#define KRN_KernelBss           (KRN_Dummy + 4)
#define KRN_GDT                 (KRN_Dummy + 5)
#define KRN_IDT                 (KRN_Dummy + 6)
#define KRN_PL4                 (KRN_Dummy + 7)
#define KRN_VBEModeInfo         (KRN_Dummy + 8)
#define KRN_VBEControllerInfo   (KRN_Dummy + 9)
#define KRN_MMAPAddress         (KRN_Dummy + 10)
#define KRN_MMAPLength          (KRN_Dummy + 11)
#define KRN_CmdLine             (KRN_Dummy + 12)
#define KRN_ProtAreaStart       (KRN_Dummy + 13)
#define KRN_ProtAreaEnd         (KRN_Dummy + 14)
#define KRN_VBEMode             (KRN_Dummy + 15)
#define KRN_VBEPaletteWidth     (KRN_Dummy + 16)
#define KRN_MEMLower         	(KRN_Dummy + 17)
#define KRN_MEMUpper          	(KRN_Dummy + 18)
#define KRN_OpenFirmwareTree	(KRN_Dummy + 19)
#define KRN_HostInterface	(KRN_Dummy + 20)

#endif /* AROS_KERNEL_H */
