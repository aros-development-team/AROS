#ifndef HIDD_NV_H
#define HIDD_NV_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nvidia header file.
    Lang: English.
*/

#include "riva_hw.h"

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef HIDD_PCI_H
#	include <hidd/pci.h>
#endif

#define USE_ALLOCATE 1

//#include "bitmap.h"

#define NUM_SEQ_REGS		0x05
#define NUM_CRT_REGS		0x41
#define NUM_GRC_REGS		0x09
#define NUM_ATC_REGS		0x15

/* holds the state of the VGA core and extended Riva hw state from riva_hw.c.
 * From KGI originally. */
struct riva_regs {
	UBYTE attr[NUM_ATC_REGS];
	UBYTE crtc[NUM_CRT_REGS];
	UBYTE gra[NUM_GRC_REGS];
	UBYTE seq[NUM_SEQ_REGS];
	UBYTE misc_output;
	RIVA_HW_STATE ext;
};

/***** nVidia gfx HIDD *******************/

/* IDs */
#define IID_Hidd_NVgfx		"hidd.gfx.nvidia"
#define CLID_Hidd_NVgfx		"hidd.gfx.nvidia"

/* misc */

struct vMemChunk
{
	struct vMemChunk	*next;
	ULONG				size;
};

struct nv_staticdata
{
        struct SignalSemaphore	HW_acc;	/* Exclusive hardware use */

	struct Library		*oopbase;
	struct Library		*utilitybase;
	struct ExecBase		*sysbase;

	HIDDT_PCI_Device	*card;
	OOP_Object			*pcihidd;
	OOP_Object			*nvhidd;

	OOP_Class			*nvclass;
	OOP_Class			*onbmclass;
	OOP_Class			*offbmclass;

	RIVA_HW_INST		riva;
	struct riva_regs	init_state;

	/* Memory manager data */

	APTR				memory;

#if USE_ALLOCATE
    	struct MemHeader    	    	memheader;
#else
	ULONG				memsize;
	ULONG				memfree;
	struct vMemChunk	*first;
#endif
	/* memory Manager end */

	UWORD				*cursor;
	UWORD				cw;
	UWORD				ch;
	UWORD				cx;
	UWORD				cy;
	UBYTE				cvisible;

	struct bitmap_data	*visible;

	ULONG				*base0;
	ULONG				*base1;
	ULONG				*base2;
	ULONG				*base3;
	
	ULONG				*pitch0;
	ULONG				*pitch1;
	ULONG				*pitch2;
	ULONG				*pitch3;

	VOID				(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR				callbackdata;
};

/* nVidia vendor and idents */

#define VENDOR_NVIDIA			0x10de
#define VENDOR_NVIDIA_SGS		0x12d2

#define DEVICE_TNT				0x0020
#define DEVICE_TNT2				0x0028
#define DEVICE_UTNT2			0x0029
#define DEVICE_VTNT2			0x002C
#define DEVICE_UVTNT2			0x002D
#define DEVICE_ITNT2			0x00A0
#define DEVICE_GEFORCE_SDR		0x0100
#define DEVICE_GEFORCE_DDR		0x0101
#define DEVICE_QUADRO			0x0103
#define DEVICE_GEFORCE2_MX		0x0110
#define DEVICE_GEFORCE2_MX2		0x0111
#define DEVICE_QUADRO2_MXR		0x0113
#define DEVICE_GEFORCE2_GTS		0x0150
#define DEVICE_GEFORCE2_GTS2	0x0151
#define DEVICE_GEFORCE2_ULTRA	0x0152
#define DEVICE_QUADRO2_PRO		0x0153

#define DEVICE_RIVA128			0x0018

/* % */

/* ------------------------------------------------------------------------- *
 *
 * MMIO access macros
 *
 * ------------------------------------------------------------------------- */

static inline void CRTCout(struct nv_staticdata *nsd, unsigned char index,
			   unsigned char val)
{
	VGA_WR08(nsd->riva.PCIO, 0x3d4, index);
	VGA_WR08(nsd->riva.PCIO, 0x3d5, val);
}

static inline unsigned char CRTCin(struct nv_staticdata *nsd,
				   unsigned char index)
{
	VGA_WR08(nsd->riva.PCIO, 0x3d4, index);
	return (VGA_RD08(nsd->riva.PCIO, 0x3d5));
}

static inline void GRAout(struct nv_staticdata *nsd, unsigned char index,
			  unsigned char val)
{
	VGA_WR08(nsd->riva.PVIO, 0x3ce, index);
	VGA_WR08(nsd->riva.PVIO, 0x3cf, val);
}

static inline unsigned char GRAin(struct nv_staticdata *nsd,
				  unsigned char index)
{
	VGA_WR08(nsd->riva.PVIO, 0x3ce, index);
	return (VGA_RD08(nsd->riva.PVIO, 0x3cf));
}

static inline void SEQout(struct nv_staticdata *nsd, unsigned char index,
			  unsigned char val)
{
	VGA_WR08(nsd->riva.PVIO, 0x3c4, index);
	VGA_WR08(nsd->riva.PVIO, 0x3c5, val);
}

static inline unsigned char SEQin(struct nv_staticdata *nsd,
				  unsigned char index)
{
	VGA_WR08(nsd->riva.PVIO, 0x3c4, index);
	return (VGA_RD08(nsd->riva.PVIO, 0x3c5));
}

static inline void ATTRout(struct nv_staticdata *nsd, unsigned char index,
			   unsigned char val)
{
	VGA_WR08(nsd->riva.PCIO, 0x3c0, index);
	VGA_WR08(nsd->riva.PCIO, 0x3c0, val);
}

static inline unsigned char ATTRin(struct nv_staticdata *nsd,
				   unsigned char index)
{
	VGA_WR08(nsd->riva.PCIO, 0x3c0, index);
	return (VGA_RD08(nsd->riva.PCIO, 0x3c1));
}

static inline void MISCout(struct nv_staticdata *nsd, unsigned char val)
{
	VGA_WR08(nsd->riva.PVIO, 0x3c2, val);
}

static inline unsigned char MISCin(struct nv_staticdata *nsd)
{
	return (VGA_RD08(nsd->riva.PVIO, 0x3cc));
}

/* Little macro to construct bitmask for contiguous ranges of bits */
#define BITMASK(t,b) (((unsigned)(1U << (((t)-(b)+1)))-1)  << (b))
#define MASKEXPAND(mask) BITMASK(1?mask,0?mask)

/* Macro to set specific bitfields (mask has to be a macro x:y) ! */
#define SetBF(mask,value) ((value) << (0?mask))
#define GetBF(var,mask) (((unsigned)((var) & MASKEXPAND(mask))) >> (0?mask) )

#define NSD(cl) ((struct nv_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)NSD(cl)->oopbase)
#define UtilityBase	((struct Library *)NSD(cl)->utilitybase)
#define SysBase		(NSD(cl)->sysbase)

/* nVidia prototypes */

extern UWORD default_cursor[];

/**** Accelerated functions */
void acc_SetClippingRectangle(struct nv_staticdata *nsd, int x1, int y1, int x2, int y2);
void acc_DisableClipping(struct nv_staticdata *nsd);
void acc_SetPattern(struct nv_staticdata *nsd, int c1, int c2, int pat1, int pat2);
void acc_SetRop(struct nv_staticdata *nsd, int rop);
void acc_PrepareToFill(struct nv_staticdata *nsd, int color, int rop);
void acc_FillRectangle(struct nv_staticdata *nsd, int x, int y, int w, int h);
void acc_Sync(struct nv_staticdata *nsd);
void acc_SolidLine(struct nv_staticdata *nsd, int color, int x1, int y1, int x2, int y2);

/**** General functions */
void load_cursor(struct nv_staticdata *nsd, UWORD *tmp);
void convert_cursor(UWORD *src, int width, int height, UWORD *dst);
void riva_wclut(RIVA_HW_INST *chip, UBYTE regnum, UBYTE red, UBYTE green, UBYTE blue);
void load_state(struct nv_staticdata *nsd, struct riva_regs *regs);
void save_state(struct nv_staticdata *nsd, struct riva_regs *regs);
int findCard(struct nv_staticdata *nsd);
void load_mode(struct nv_staticdata *nsd, int width, int height, int bpp, ULONG pixelc,
	ULONG base, int HDisplay, int VDisplay,	int HSyncStart, int HSyncEnd, int HTotal,
	int VSyncStart, int VSyncEnd, int VTotal);
APTR vbuffer_alloc(struct nv_staticdata *nsd, int size);
void vbuffer_free(struct nv_staticdata *nsd, APTR buff, int size);
int initclasses(struct nv_staticdata *nsd);









#endif /* HIDD_NV_H */

