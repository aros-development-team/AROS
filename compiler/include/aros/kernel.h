#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: TagItems for the kernel.resource
    Lang: english
*/

#include <dos/elf.h>
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
} KRN_MapAttr;

/* Tags for kernel boot message */
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
#define KRN_DebugInfo		(KRN_Dummy + 21) /* (dbg_seg_t *) */
#define KRN_BootLoader          (KRN_Dummy + 22)

/* BSS segments descriptor */
struct KernelBSS
{
    void *addr;
    IPTR len;
};

/* Kernel debug info structures. Must be placed in kernel memory! */
typedef struct
{
    char *s_name;	/* Symbol name			*/
    void *s_lowest;	/* Start address	     	*/
    void *s_highest;	/* End address		     	*/
} dbg_sym_t;

typedef struct
{
    dbg_sym_t     *m_symbols;	/* Array of associated symbols    */
    unsigned long  m_symcnt;	/* Number of symbols in the array */
    char	   m_name[1];	/* Module name, variable length	  */
} dbg_mod_t;

typedef struct debug_seginfo
{
    struct debug_seginfo *s_next;	/* Next segment			*/
    void 	 	 *s_lowest;	/* Start address	     	*/
    void 		 *s_highest;	/* End address		     	*/
    dbg_mod_t		 *s_module;	/* To what module it belongs	*/
    char		 *s_name;	/* Segment name			*/
    unsigned int	  s_num;	/* Segment number		*/
} dbg_seg_t;

/* Known debug info types */
#define DEBUG_ELF 1

/* ELF debug info */
struct ELF_DebugInfo
{
    struct elfheader *eh;
    struct sheader *sh;
};

/* Tags for KrnDecodeLocation() */
#define KDL_Dummy		(TAG_USER + 0x03e00000)
#define KDL_ModuleName		(KDL_Dummy + 1)
#define KDL_SegmentName		(KDL_Dummy + 2)
#define KDL_SegmentPointer	(KDL_Dummy + 3)
#define KDL_SegmentNumber	(KDL_Dummy + 4)
#define KDL_SegmentStart	(KDL_Dummy + 5)
#define KDL_SegmentEnd		(KDL_Dummy + 6)
#define KDL_SymbolName		(KDL_Dummy + 7)
#define KDL_SymbolStart		(KDL_Dummy + 8)
#define KDL_SymbolEnd		(KDL_Dummy + 9)

/* Exception and IRQ handler types */
typedef int (*exhandler_t)(void *ctx, void *data, void *data2);
typedef void (*irqhandler_t)(void *data, void *data2);

/* System attributes */
#define KATTR_Architecture	(TAG_USER + 0x03F00000) /* [.G] (char *)   - Name of architecture, like "i386-pc"	 */
#define KATTR_VBlankEnable	(TAG_USER + 0x03F00001) /* [SG] (BOOL)     - Enable or disable exec VBlank emulation	 */
#define KATTR_TimerIRQ		(TAG_USER + 0x03F00002) /* [.G] (uint8_t)  - Number of periodic timer IRQ		 */
#define KATTR_PageSize		(TAG_USER + 0x03F00003) /* [.G] (uint32_t) - Memory page size				 */

#endif /* AROS_KERNEL_H */
