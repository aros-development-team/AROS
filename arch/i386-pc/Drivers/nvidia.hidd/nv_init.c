/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: nVidia Hidd initializer
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include <hidd/pcibus.h>
#include <hidd/graphics.h>

#include <string.h>


#include "nv.h"
#include "riva_hw.h"

//#include "vgaclass.h"

#undef SysBase

#warning: prototypes which should actually be in some public header
HIDDT_PCI_Device **HIDD_PCI_FindDevice(OOP_Object *obj, struct TagItem *tags);
VOID HIDD_PCI_FreeQuery(OOP_Object *obj, HIDDT_PCI_Device **devices);

void new_setupAccel(struct nv_staticdata *nsd);
OOP_Class *init_nvclass (struct nv_staticdata *nsd);
OOP_Class *nv_init_onbmclass(struct nv_staticdata *nsd);
OOP_Class *nv_init_offbmclass(struct nv_staticdata *nsd);

#define SetBitField(value,from,to) SetBF(to,GetBF(value,from))
#define SetBit(n)		(1<<(n))
#define Set8Bits(value)		((value)&0xff)

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		nvHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		8
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB

#define NOEXPUNGE

struct nvbase
{
    struct Library   library;
    struct ExecBase *sysbase;
    BPTR             seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef SysBase
#undef OOPBase

#define OOPBase (nsd->oopbase)
#define SysBase (nsd->sysbase)

#define MAX_CURS		32

#define MAKE_RGB15(c) (((c & 0xf80000) >> 9 ) | ((c & 0xf800) >> 6 ) | ((c & 0xf8) >> 3 ) | 0x8000)

#define WW  ( MAKE_RGB15(0xffffff) )
#define ii  ( MAKE_RGB15(0xff2020) )
#define bb	( MAKE_RGB15(0x080808) )

UWORD default_cursor[] = {
    ii,WW,00,00,00,00,00,00,00,00,00,
    bb,ii,WW,WW,00,00,00,00,00,00,00,
    00,bb,ii,ii,WW,WW,00,00,00,00,00,
    00,bb,ii,ii,ii,ii,WW,WW,00,00,00,
    00,00,bb,ii,ii,ii,ii,ii,WW,WW,00,
    00,00,bb,ii,ii,ii,ii,ii,ii,ii,00,
    00,00,00,bb,ii,ii,ii,WW,00,00,00,
    00,00,00,bb,ii,ii,bb,ii,WW,00,00,
    00,00,00,00,bb,ii,00,bb,ii,WW,00,
    00,00,00,00,bb,ii,00,00,bb,ii,WW,
    00,00,00,00,00,00,00,00,00,bb,ii
};

#undef WW
#undef ii
#undef bb

#define writel(b,addr) (*(volatile unsigned int *) (addr) = (b))

void load_cursor(struct nv_staticdata *nsd, UWORD *tmp)
{
//    int show = nsd->riva.ShowHideCursor(&nsd->riva, 0);
    int         *image, i, numInts;
    numInts = (MAX_CURS*MAX_CURS*2) / sizeof(int);

    nsd->cursor = tmp;

    image   = (int *)tmp;
    for (i = 0; i < numInts; i++)
	nsd->riva.CURSOR[i] = image[i];

    nsd->riva.ShowHideCursor(&nsd->riva, nsd->cvisible);			//show);
//	nsd->riva.ShowHideCursor(&nsd->riva, show);
}

void convert_cursor(UWORD *src, int width, int height, UWORD *dst)
{
	int x,y;
	
	for (y=0; y<height; y++)
	{
		for (x=0; x < width; x++)
		{
			dst[x + y*MAX_CURS] = *(src++);
		}
	
		for (x=width; x < MAX_CURS; x++)
		{
			dst[x + y*MAX_CURS] = 0;
		}
	}
	for (y=height; y < MAX_CURS; y++)
	{
		for (x=0; x < MAX_CURS; x++)
		{
			dst[x + y*MAX_CURS] = 0;
		}
	}
}

static const struct riva_regs reg_template = {
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,	/* ATTR */
	 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	 0x41, 0x01, 0x0F, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* CRT  */
	 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE3,	/* 0x10 */
	 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x20 */
	 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x30 */
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00,							/* 0x40 */
	 },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,	/* GRA  */
	 0xFF},
	{0x03, 0x01, 0x0F, 0x00, 0x0E},				/* SEQ  */
	0xEB							/* MISC */
};

void riva_wclut(RIVA_HW_INST *chip, UBYTE regnum, UBYTE red,
		       UBYTE green, UBYTE blue)
{
	VGA_WR08(chip->PDIO, 0x3c8, regnum);
	VGA_WR08(chip->PDIO, 0x3c9, red);
	VGA_WR08(chip->PDIO, 0x3c9, green);
	VGA_WR08(chip->PDIO, 0x3c9, blue);
}

void load_state(struct nv_staticdata *nsd, struct riva_regs *regs)
{
	int i;
	RIVA_HW_STATE *state = &regs->ext;

	CRTCout(nsd, 0x11, 0x00);

	nsd->riva.LockUnlock(&nsd->riva, 0);

	nsd->riva.LoadStateExt(&nsd->riva, state);

	MISCout(nsd, regs->misc_output);

	for (i = 0; i < NUM_CRT_REGS; i++) {
		switch (i) {
		case 0x19:
		case 0x20 ... 0x40:
			break;
		default:
			CRTCout(nsd, i, regs->crtc[i]);
		}
	}

	for (i = 0; i < NUM_ATC_REGS; i++) {
		ATTRout(nsd, i, regs->attr[i]);
	}

	for (i = 0; i < NUM_GRC_REGS; i++) {
		GRAout(nsd, i, regs->gra[i]);
	}

	for (i = 0; i < NUM_SEQ_REGS; i++) {
		SEQout(nsd, i, regs->seq[i]);
	}
}

void save_state(struct nv_staticdata *nsd, struct riva_regs *regs)
{
	int i;

	nsd->riva.LockUnlock(&nsd->riva, 0);

	nsd->riva.UnloadStateExt(&nsd->riva, &regs->ext);

	regs->misc_output = MISCin(nsd);

	for (i = 0; i < NUM_CRT_REGS; i++) {
		regs->crtc[i] = CRTCin(nsd, i);
	}

	for (i = 0; i < NUM_ATC_REGS; i++) {
		regs->attr[i] = ATTRin(nsd, i);
	}

	for (i = 0; i < NUM_GRC_REGS; i++) {
		regs->gra[i] = GRAin(nsd, i);
	}

	for (i = 0; i < NUM_SEQ_REGS; i++) {
		regs->seq[i] = SEQin(nsd, i);
	}
}

#define is(did,dev) (did==DEVICE_##dev)

int findCard(struct nv_staticdata *nsd)
{
	HIDDT_PCI_Device **ptr, **res;

	int arch = 0;

	struct TagItem tags[] = {
		{ tHidd_PCI_VendorID,	VENDOR_NVIDIA },
		{ tHidd_PCI_Class,                  3 },
		{ tHidd_PCI_SubClass,		    0 },
		{ tHidd_PCI_Interface,		    0 },

		{TAG_DONE,0}};

	struct TagItem tags_sgs[] = {
		{ tHidd_PCI_VendorID,	VENDOR_NVIDIA_SGS },
		{ tHidd_PCI_DeviceID,	   DEVICE_RIVA128 },
		{ tHidd_PCI_Class,			3 },
		{ tHidd_PCI_SubClass,			0 },
		{ tHidd_PCI_Interface,			0 },

		{TAG_DONE,0}};

	D(bug("find nvidia card...\n"));


	nsd->card = NULL;

	res = ptr = HIDD_PCI_FindDevice(nsd->pcihidd, tags);
	while (*ptr)
	{
		UWORD d = (*ptr)->DeviceID;
		
		if (is (d, TNT)
                 || is (d, TNT2)
                 || is (d, UTNT2)
                 || is (d, VTNT2)
                 || is (d, UVTNT2)
                 || is (d, ITNT2)
                 || is (d, GEFORCE_SDR)
                 || is (d, GEFORCE_DDR)
                 || is (d, QUADRO)
                 || is (d, GEFORCE2_MX)
                 || is (d, GEFORCE2_MX2)
                 || is (d, QUADRO2_MXR)
                 || is (d, GEFORCE2_GTS)
                 || is (d, GEFORCE2_GTS2)
                 || is (d, GEFORCE2_ULTRA)
                 || is (d, QUADRO2_PRO)
                 || is (d, GEFORCE4_MX460)
                 || is (d, GEFORCE4_MX440)
                 || is (d, GEFORCE4_MX420)
                 || is (d, GEFORCE4_440GO)
                 || is (d, GEFORCE4_420GO)
                 || is (d, GEFORCE4_420GO32)
                 || is (d, QUADRO4_500)
                 || is (d, GEFORCE4_440GO64)
                 || is (d, QUADRO4_200)
                 || is (d, QUADRO4_550)
                 || is (d, QUADRO4_500GOGL)
                 || is (d, GEFORCE2_IGPU)
                 || is (d, GEFORCE3)
                 || is (d, GEFORCE3_200)
                 || is (d, GEFORCE3_500)
                 || is (d, QUADRO_DCC)
                 || is (d, GEFORCE4_4600)
                 || is (d, GEFORCE4_4400)
                 || is (d, GEFORCE4_4200)
                 || is (d, QUADRO4_900)
                 || is (d, QUADRO4_750)
                 || is (d, QUADRO4_700))
		{
			D(bug("found in VENDOR_NVIDIA!\n"));
			
			nsd->card = *ptr;
			arch = d;
			
			break;
		}
	
		ptr++;
	}
	HIDD_PCI_FreeQuery(nsd->pcihidd, res);

	if (!nsd->card)
	{
		res = ptr = HIDD_PCI_FindDevice(nsd->pcihidd, tags_sgs);
		while (*ptr)
		{
			if ((*ptr)->DeviceID == DEVICE_RIVA128)
			{
				D(bug("found in VENDOR_NVIDIA_SGS!\n"));

				nsd->card = *ptr;
				arch = (*ptr)->DeviceID;
				
				break;
			}
			ptr++;
		}
		HIDD_PCI_FreeQuery(nsd->pcihidd, res);
	}
	
	switch (arch & 0x0ff0)
	{
		case 0x0010:
			arch = NV_ARCH_03;
			break;
		case 0x0020:
		case 0x00a0:
			arch = NV_ARCH_04;
			break;
		case 0x0100:
		case 0x0110:
		case 0x0150:
		case 0x0170:
		case 0x01a0:
			arch = NV_ARCH_10;
			break;
		case 0x0200:
		case 0x0250:
//			arch = NV_ARCH_20;
			break;
	}
	
	if (nsd->card)
	{
		ULONG mmio = (ULONG)nsd->card->BaseAddress[0] & 0xffffc000;
		ULONG base = (ULONG)nsd->card->BaseAddress[1] & 0xff800000;
		
		nsd->riva.Architecture = arch;
		nsd->riva.EnableIRQ = 0;
		nsd->riva.PRAMDAC   = (unsigned *)(mmio + 0x00680000);
		nsd->riva.PFB       = (unsigned *)(mmio + 0x00100000);
		nsd->riva.PFIFO     = (unsigned *)(mmio + 0x00002000);
		nsd->riva.PGRAPH    = (unsigned *)(mmio + 0x00400000);
		nsd->riva.PEXTDEV   = (unsigned *)(mmio + 0x00101000);
		nsd->riva.PTIMER    = (unsigned *)(mmio + 0x00009000);
		nsd->riva.PMC       = (unsigned *)(mmio + 0x00000000);
		nsd->riva.FIFO      = (unsigned *)(mmio + 0x00800000);

		nsd->riva.PCIO = (U008 *)(mmio + 0x00601000);
		nsd->riva.PDIO = (U008 *)(mmio + 0x00681000);
		nsd->riva.PVIO = (U008 *)(mmio + 0x000C0000);

		nsd->riva.IO = 0x3d0;	//(MISCin(nsd) & 0x01) ? 0x3D0 : 0x3B0;

		switch (nsd->riva.Architecture)
		{
			case NV_ARCH_03:
				nsd->riva.PRAMIN = (unsigned *)(base + 0x00C00000);

				nsd->base0 = (ULONG *)&(nsd->riva.PGRAPH[0x00000630/4]);
				nsd->base1 = (ULONG *)&(nsd->riva.PGRAPH[0x00000634/4]);
				nsd->base2 = (ULONG *)&(nsd->riva.PGRAPH[0x00000638/4]);
				nsd->base3 = (ULONG *)&(nsd->riva.PGRAPH[0x0000063C/4]);
				
				nsd->pitch0 = (ULONG *)&(nsd->riva.PGRAPH[0x00000650/4]);
				nsd->pitch1 = (ULONG *)&(nsd->riva.PGRAPH[0x00000654/4]);
				nsd->pitch2 = (ULONG *)&(nsd->riva.PGRAPH[0x00000658/4]);
				nsd->pitch3 = (ULONG *)&(nsd->riva.PGRAPH[0x0000065C/4]);

				break;

			case NV_ARCH_04:
			case NV_ARCH_10:
				nsd->riva.PCRTC = (unsigned *)(mmio + 0x00600000);
				nsd->riva.PRAMIN = (unsigned *)(mmio + 0x00710000);

				nsd->base0 = (ULONG *)&(nsd->riva.PGRAPH[0x00000640/4]);
				nsd->base1 = (ULONG *)&(nsd->riva.PGRAPH[0x00000644/4]);
				nsd->base2 = (ULONG *)&(nsd->riva.PGRAPH[0x00000648/4]);
				nsd->base3 = (ULONG *)&(nsd->riva.PGRAPH[0x0000064C/4]);

				nsd->pitch0 = (ULONG *)&(nsd->riva.PGRAPH[0x00000670/4]);
				nsd->pitch1 = (ULONG *)&(nsd->riva.PGRAPH[0x00000674/4]);
				nsd->pitch2 = (ULONG *)&(nsd->riva.PGRAPH[0x00000678/4]);
				nsd->pitch3 = (ULONG *)&(nsd->riva.PGRAPH[0x0000067C/4]);

				break;
		}

		RivaGetConfig(&nsd->riva);

		CRTCout(nsd, 0x11, 0xFF);
		nsd->riva.LockUnlock(&nsd->riva, 0);

//		save_state(nsd, &nsd->init_state);
		
	#if USE_ALLOCATE
	    	{
		    struct MemChunk *mc = (struct MemChunk *)base;
	    	    
		    nsd->memheader.mh_Node.ln_Type = NT_MEMORY;
		    nsd->memheader.mh_Node.ln_Name = "NVIDIA video mem";
		    nsd->memheader.mh_First = mc;
		    nsd->memheader.mh_Lower = (APTR)mc;
		    nsd->memheader.mh_Free  = nsd->riva.RamAmountKBytes * 1024;
		    nsd->memheader.mh_Upper = (APTR)(nsd->memheader.mh_Free + (IPTR)base);
		    
		    mc->mc_Next = NULL;
		    mc->mc_Bytes = nsd->memheader.mh_Free;
		    
		    nsd->memory = (APTR)base;
		}
	#else
	
		nsd->memory = (APTR)base;
		nsd->memsize = nsd->riva.RamAmountKBytes * 1024;
		nsd->memfree = nsd->memsize;
		nsd->first = (struct vMemChunk *)base;
		nsd->first->next = NULL;
		nsd->first->size = nsd->memsize;
    	#endif
	
		D(bug("Video memory: %dKB\n", nsd->riva.RamAmountKBytes));
	}
	
	return (nsd->card) ? TRUE : FALSE;
}

#undef is

void load_mode(struct nv_staticdata *nsd,
	int width, int height, int bpp, ULONG pixelc, ULONG base,
	int HDisplay, int VDisplay,
	int HSyncStart, int HSyncEnd, int HTotal,
	int VSyncStart, int VSyncEnd, int VTotal)
{
    struct riva_regs newmode;
    int HDisplaySize = HDisplay;

	D(bug(	"load_mode: width=%d height=%d, bpp=%d, pixelc=%d, base=%08.8lx\n"
			"  hDisp=%d hStart=%d hEnd=%d hTotal=%d\n"
			"  vDisp=%d vStart=%d vEnd=%d vTotal=%d",
			width, height, bpp, pixelc/1000, base,
			HDisplay, HSyncStart, HSyncEnd, HTotal,
			VDisplay, VSyncStart, VSyncEnd, VTotal));


//	memset(&newmode, 0, sizeof(struct riva_regs));
	
	memcpy(&newmode, &reg_template, sizeof(struct riva_regs));

	HDisplay = (HDisplay / 8) - 1;
	HSyncStart 	= (HSyncStart / 8) + 2;
	HSyncEnd 	= (HSyncEnd / 8) - 1;
	HTotal		= (HTotal / 8) - 1;
	
	VDisplay	-= 1;
	VSyncStart	-= 1;
	VSyncEnd	-= 1;
	VTotal		+= 2;

	pixelc /= 1000;

	newmode.crtc[0x0] = Set8Bits (HTotal - 4);
	newmode.crtc[0x1] = Set8Bits (HDisplay);
	newmode.crtc[0x2] = Set8Bits (HDisplay);
	newmode.crtc[0x3] = SetBitField (HTotal, 4: 0, 4:0) | SetBit (7);
	newmode.crtc[0x4] = Set8Bits (HSyncStart);
	newmode.crtc[0x5] = SetBitField (HTotal, 5: 5, 7:7)
		| SetBitField (HSyncEnd, 4: 0, 4:0);
	newmode.crtc[0x6] = SetBitField (VTotal, 7: 0, 7:0);
	newmode.crtc[0x7] = SetBitField (VTotal, 8: 8, 0:0)
		| SetBitField (VDisplay, 8: 8, 1:1)
		| SetBitField (VSyncStart, 8: 8, 2:2)
		| SetBitField (VDisplay, 8: 8, 3:3)
		| SetBit (4)
		| SetBitField (VTotal, 9: 9, 5:5)
		| SetBitField (VDisplay, 9: 9, 6:6)
		| SetBitField (VSyncStart, 9: 9, 7:7);
	newmode.crtc[0x9] = SetBitField (VDisplay, 9: 9, 5:5)
		| SetBit (6);
	newmode.crtc[0x10] = Set8Bits (VSyncStart);
	newmode.crtc[0x11] = SetBitField (VSyncEnd, 3: 0, 3:0)
		| SetBit (5);
	newmode.crtc[0x12] = Set8Bits (VDisplay);
	newmode.crtc[0x13] = ((width / 8) * ((bpp + 1) / 8)) & 0xFF;
	newmode.crtc[0x15] = Set8Bits (VDisplay);
	newmode.crtc[0x16] = Set8Bits (VTotal + 1);

	newmode.ext.bpp = bpp;
	newmode.ext.width = width;
	newmode.ext.height = height;

	nsd->riva.CalcStateExt(&nsd->riva, &newmode.ext, bpp, width,
				  HDisplaySize, HDisplay, HSyncStart, HSyncEnd,
				  HTotal, height, VDisplay, VSyncStart, VSyncEnd,
				  VTotal, pixelc);

//	if (nsd->riva.Architecture >= NV_ARCH_10)
//	{
//		nsd->riva.CURSOR = (U032 *)((U032)nsd->memheader.mh_First + nsd->riva.CursorStart);
//	}

	load_state(nsd, &newmode);

	nsd->riva.LockUnlock(&nsd->riva, 0); /* important for HW cursor */
	load_cursor(nsd, nsd->cursor);
	nsd->riva.SetStartAddress(&nsd->riva, base);	

	new_setupAccel(nsd);

        *(nsd->base0) =
	*(nsd->base1) = 
	*(nsd->base2) = 
	*(nsd->base3) = base;
}

APTR vbuffer_alloc(struct nv_staticdata *nsd, int size)
{
#if USE_ALLOCATE
    	APTR ret;
	
	Forbid();
	ret = Allocate(&nsd->memheader, size);
	Permit();
	
	if (!ret)
	{
	    ret = AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
	}
	else
	{
	    /* Clear memory. */
	    ULONG cnt,*p;

	    p=(ULONG *)ret;
	    cnt=size/sizeof(ULONG);

	    while(cnt--)
		*p++=0;
	}
	
	return ret;
#else
	APTR ret = NULL;
	struct vMemChunk *mc=NULL, *p1, *p2;

	size = (size + 15) & ~16;

	/*
	    The free memory list is only single linked, i.e. to remove
	    elements from the list I need node's predessor. For the
	    first element I can use mh->mh_First instead of a real predessor.
	*/
	
	Forbid();
	
	p1=(struct vMemChunk *)&nsd->first;
	p2=p1->next;

	/* Is there anything in the list? */
	if(p2!=NULL)
	{
	    /* Then follow it */
	    for(;;)
	    {
			/* Check if the current block is large enough */
			if(p2->size>=size)
			{
			    /* It is. */
			    mc=p1;
				break;
			}

			/* Go to next block */
			p1=p2;
			p2=p1->next;

			/* Check if this was the end */
			if(p2==NULL)
			    break;
	    }

	    /* Something found? */
	    if(mc!=NULL)
	    {
			p1=mc;
			p2=p1->next;

			/* Remove the block from the list and return it. */
			if(p2->size==size)
			{
			    /* Fits exactly. Just relink the list. */
			    p1->next=p2->next;
			    mc=p2;
			}
			else
			{
				/* Return the first bytes. */
				p1->next=(struct vMemChunk *)((UBYTE *)p2+size);
				mc=p2;
			    p1=p1->next;
			    p1->next=p2->next;
			    p1->size=p2->size-size;
			}
			nsd->memfree-=size;

			/* No need to forbid dispatching any longer. */
			Permit();

			{
			    /* Clear memory. */
		    	ULONG cnt,*p;

			    p=(ULONG *)mc;
			    cnt=size/sizeof(ULONG);

			    while(cnt--)
					*p++=0;
			}
	
			ret=mc;
			
			return ret;
	    }
	    Permit();
		
		return AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
	}
	Permit();
	return AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
#endif
}

void vbuffer_free(struct nv_staticdata *nsd, APTR buff, int size)
{
#if USE_ALLOCATE
	if ( ((ULONG)buff < (ULONG)(nsd->memheader.mh_Lower)) ||
	     ((ULONG)buff >= (ULONG)(nsd->memheader.mh_Upper)) )
	{
		FreeMem(buff, size);
	}
    	else
	{
	    Forbid();
	    Deallocate(&nsd->memheader, buff, size);
	    Permit();
	}
#else
	size = (size + 15) & ~16;

	if ((ULONG)buff < (ULONG)(nsd->memory))
	{
		FreeMem(buff, size);
	}
	else
	{
		struct vMemChunk *p1;
		struct vMemChunk *p2;
		struct vMemChunk *p3;
		UBYTE *p4;
	
		Forbid();
		
		p1 = &nsd->first;
		p2 = nsd->first;
		p3 = (struct vMemChunk *)buff;
		p4 = (UBYTE*)p3 + size;
		if(p2==NULL)
		{
			p3->size=size;
			p3->next=NULL;
			p1->next=p3;
			nsd->memfree+=size;
			Permit();
			return;
	    	}

		do
		{
			/* Found a block with a higher address? */
			if(p2>=p3)
			{
				/* End the loop with p2 non-zero */
				break;
			}

			p1=p2;
			p2=p2->next;

			/* If the loop ends with p2 zero add it at the end. */
	    } while(p2!=NULL);

	    /* If there was a previous block merge with it. */
	    if(p1!=(struct vMemChunk *)&nsd->first)
	    {
			/* Merge if possible */
			if((UBYTE *)p1+p1->size==(UBYTE *)p3)
				p3=p1;
			else
				/* Not possible to merge */
				p1->next=p3;
		}
		else
			p1->next=p3;

		/* Try to merge with next block (if there is one ;-) ). */
		if(p4==(UBYTE *)p2&&p2!=NULL)
	    {
			/*
			    Overlap checking already done. Doing it here after
			    the list potentially changed would be a bad idea.
			*/
			p4+=p2->size;
			p2=p2->next;
	    }

		/* relink the list and return. */
		p3->next=p2;
	    p3->size=p4-(UBYTE *)p3;
	    nsd->memfree+=size;
	    Permit();
	}
#endif
}

/** Class initialization */

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

int initclasses(struct nv_staticdata *nsd)
{
    if (!OOP_ObtainAttrBases(abd))
    	goto failure;

	nsd->nvclass = init_nvclass(nsd);
	if (nsd->nvclass == NULL)
		goto failure;
	
	nsd->onbmclass = nv_init_onbmclass(nsd);
	if (nsd->onbmclass == NULL)
		goto failure;
	
	nsd->offbmclass = nv_init_offbmclass(nsd);
	if (nsd->offbmclass == NULL)
		goto failure;
	
	return TRUE;
	
failure:
	return FALSE;
}

#undef SysBase
#define SysBase      (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
	struct nv_staticdata *nsd;
	nsd = AllocMem( sizeof (struct nv_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
	if (nsd)
	{
		nsd->sysbase = SysBase;
		InitSemaphore(&nsd->HW_acc);
	
		nsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
		if (nsd->oopbase)
		{
			nsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
			if (nsd->utilitybase)
			{
				nsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCIBus, NULL);
				if (nsd->pcihidd)
				{
					if (findCard(nsd))
					{
						D(bug("Card found\n"));
						if (initclasses(nsd))
						{
							D(bug("Everything OK\n"));
							return TRUE;
						}
					}
				}
				CloseLibrary(nsd->utilitybase);
			}
			CloseLibrary(nsd->oopbase);
		}
		FreeMem(nsd, sizeof (struct nv_staticdata));
	}
	return FALSE;
}

