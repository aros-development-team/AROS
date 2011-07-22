
#include <aros/debug.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_intern.h"

/* 68030 (68851), 68040 and 68060 supported, 68030 (68851) is configured like a 68040,
 * no 68030 special features used, not worth the extra complexity */

#define LEVELA_SIZE 7
#define LEVELB_SIZE 7
#define LEVELC_SIZE 6
#define PAGE_SIZE 12 // = 1 << 12 = 4096

/* Macros that hopefully make MMU magic a bit easier to understand.. */

#define LEVELA_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE                            ))) & ((1 << LEVELA_SIZE) - 1))
#define LEVELB_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE              ))) & ((1 << LEVELB_SIZE) - 1))
#define LEVELC_VAL(x) ((((ULONG)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE + LEVELC_SIZE))) & ((1 << LEVELC_SIZE) - 1))

#define LEVELA(root, x) (root[LEVELA_VAL(x)])
#define LEVELB(a, x) (((ULONG*)(((ULONG)a) & ~((1 << (LEVELB_SIZE + 2)) - 1)))[LEVELB_VAL(x)])
#define LEVELC(b, x) (((ULONG*)(((ULONG)b) & ~((1 << (LEVELC_SIZE + 2)) - 1)))[LEVELC_VAL(x)])

#define INVALID_DESCRIPTOR 0xDEAD0000
#define ISINVALID(x) ((((ULONG)x) & 3) == 0)


/* Allocate MMU descriptor page, it needs to be (1 << bits) * sizeof(ULONG) aligned */
static ULONG alloc_descriptor(UBYTE mmutype, UBYTE bits)
{
	ULONG *desc, dout;
	ULONG size = sizeof(ULONG) * (1 << bits);
	UWORD i;

	desc = AllocMem(3 * size, MEMF_PUBLIC);
	if (!desc)
		return 0;
	Forbid();
	FreeMem(desc, 3 * size);
	desc = AllocAbs(size, (APTR)((((ULONG)desc) + size - 1) & ~(size - 1)));
	Permit();
	for (i = 0; i < (1 << bits); i++)
		desc[i] = INVALID_DESCRIPTOR;
	dout = (ULONG)desc;
	/* Descriptor pointer is resident */
	if (mmutype == MMU030)
		dout |= 2;
	else
		dout |= 3;
	return dout;
}	

BOOL init_mmu(struct KernelBase *kb)
{
	UBYTE mmutype = kb->kb_PlatformData->mmu_type;
	
	if (!mmutype)
		return FALSE;
	kb->kb_PlatformData->MMU_Level_A = (ULONG*)(alloc_descriptor(mmutype, LEVELA_SIZE) & ~3);
	if (!kb->kb_PlatformData->MMU_Level_A) {
		kb->kb_PlatformData->mmu_type = 0;
		return FALSE;
	}
	return TRUE;
}

static void enable_mmu030(ULONG *levela)
{
    asm volatile (
    	".chip 68030\n"
	"move.l	%0,%%d0\n"
	"move.l 4.w,%%a6\n"
	"lea	.esuper030(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	".esuper030:\n"
	/* Do not interrupt us */
	"or	#0x0700,%%sr\n"
	/* Disable MMU, setup root pointers,
	 * uses 68040 MMU descriptor levels (7/7/6, 4K page size) */
	"subq.l	#8,%%a7\n"
	"move.l	#0x00a07760,%%d1\n"
	"move.l	%%d1,%%a7@\n"
	"pmove	%%a7@,%%tc\n"
	"move.l	#0x80000002,%%a7@\n"
	"move.l	%%d0,%%a7@(4)\n"
	"pmove	%%a7@,%%crp\n"
	"bset	#31,%%d1\n"
	"move.l	%%d1,%%a7@\n"
	"pmove	%%a7@,%%tc\n"
	"clr.l	%%a7@\n"
	"pmove	%%a7@,%%tt0\n"
	"pmove	%%a7@,%%tt1\n"
	"addq.l	#8,%%a7\n"
	"rte\n"
	"0:\n"
	: : "m" (levela) : "d0", "d1", "a5", "a6");
}
static void disable_mmu030(void)
{
    asm volatile (
    	".chip 68030\n"
	"move.l 4.w,%%a6\n"
	"lea	.dsuper030(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	".dsuper030:\n"
	/* Do not interrupt us */
	"or	#0x0700,%%sr\n"
	/* Disable MMU */
	"subq.l	#4,%%a7\n"
	"clr.l	%%a7@\n"
	"pmove	%%a7@,%%tc\n"
	"addq.l	#4,%%a7\n"
	"rte\n"
	"0:\n"
	: : : "d0", "d1", "a5", "a6");
}
static void enable_mmu040(ULONG *levela)
{
    asm volatile (
    	".chip 68060\n"
	"move.l	%0,%%d0\n"
	"move.l 4.w,%%a6\n"
	"lea	.esuper040(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	".esuper040:\n"
	/* Do not interrupt us */
	"or	#0x0700,%%sr\n"
	"moveq	#0,%%d1\n"
	/* Disable MMU, setup root pointers */
	"movec	%%d1,%%tc\n"
	"movec	%%d0,%%urp\n"
	"movec	%%d0,%%srp\n"
	/* flush data caches and ATC */
	"cpusha	%%dc\n"
	"cinva	%%dc\n"
	"pflusha\n"
	/* Enable MMU */
	"move.w	#0x8000,%%d0\n"
	"movec	%%d0,%%tc\n"
	/* Disable transparent translation */
	"movec	%%d1,%%itt0\n"
	"movec	%%d1,%%itt1\n"
	"movec	%%d1,%%dtt0\n"
	"movec	%%d1,%%dtt1\n"
    	"rte\n"
    	"0:\n"
	: : "m" (levela) : "d0", "d1", "a5", "a6");
}

static void disable_mmu040(void)
{
    asm volatile (
    	".chip 68060\n"
	"move.l 4.w,%%a6\n"
	"lea	.dsuper040(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	".dsuper040:\n"
	/* Do not interrupt us */
	"or	#0x0700,%%sr\n"
	/* Disable MMU */
	"moveq	#0,%%d0\n"
	"movec	%%d0,%%tc\n"
	"pflusha\n"
	"rte\n"
	"0:\n"
	: : : "d0", "d1", "a5", "a6");
}

void enable_mmu(struct KernelBase *kb)
{
	if (!kb->kb_PlatformData->mmu_type)
		return;
	if (kb->kb_PlatformData->mmu_type == MMU030)
		enable_mmu030(kb->kb_PlatformData->MMU_Level_A);
	else
		enable_mmu040(kb->kb_PlatformData->MMU_Level_A);
}
void disable_mmu(struct KernelBase *kb)
{
	if (!kb->kb_PlatformData->mmu_type)
		return;
	if (kb->kb_PlatformData->mmu_type == MMU030)
		disable_mmu030();
	else
		disable_mmu040();
}

static ULONG getdesc(struct KernelBase *kb, ULONG addr)
{
	ULONG desc;

	desc = LEVELA(kb->kb_PlatformData->MMU_Level_A, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELB(desc, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELC(desc, addr);
	return desc;
}

void debug_mmu(struct KernelBase *kb)
{
	UBYTE mmutype;
	ULONG i;
	ULONG startaddr;
	ULONG odesc;
	ULONG totalpages;
	ULONG pagemask = (1 << PAGE_SIZE) - 1;
	
	mmutype = kb->kb_PlatformData->mmu_type;
	if (!mmutype || kb->kb_PlatformData->MMU_Level_A == NULL)
		return;
	bug("MMU dump start. Root = %p\n", kb->kb_PlatformData->MMU_Level_A);
	totalpages = 1 << (32 - PAGE_SIZE);
	startaddr = 0;
	odesc = getdesc(kb, startaddr);
	for (i = 0; i <= totalpages; i++) {
		ULONG addr = i << PAGE_SIZE;
		ULONG desc = 0;
		if (i < totalpages)
			desc = getdesc(kb, addr);
		if ((desc & pagemask) != (odesc & pagemask) || i == totalpages) {
			UBYTE cm;
			if (mmutype == MMU030)
				cm = (odesc >> 6) & 1;
			else
				cm = (odesc >> 5) & 3;
			bug("%p - %p: %p WP=%d CM=%d (%08x)\n",
				startaddr, addr - 1, odesc & ~((1 << PAGE_SIZE) - 1),
				(odesc & 4) ? 1 : 0, cm, odesc);
			startaddr = addr;
			odesc = desc;
		}
	}
	bug("MMU dump end\n");
}			

BOOL map_region(struct KernelBase *kb, void *addr, void *physaddr, ULONG size, BOOL invalid, BOOL writeprotect, BOOL supervisor, UBYTE cachemode)
{
	ULONG desca, descb, pagedescriptor;
	ULONG page_size = 1 << PAGE_SIZE;
	ULONG page_mask = page_size - 1;
	UBYTE mmutype;
	
	mmutype = kb->kb_PlatformData->mmu_type;
	if (!mmutype)
		return FALSE;
	if (kb->kb_PlatformData->MMU_Level_A == NULL)
		return FALSE;

	if ((size & page_mask) || (((ULONG)addr) & page_mask) || (((ULONG)physaddr) & page_mask)) {
		bug("unaligned MMU page request! %p (%p) %08x\n", addr, physaddr, size);
		return FALSE;
	}
	if (physaddr == NULL)
		physaddr = addr;

	bug("map_region(%p, %p, %08x, in=%d, wp=%d, cm=%d\n",
		addr, physaddr, size, invalid ? 1 : 0, writeprotect ? 1 : 0, cachemode);
	
	while (size) {
		desca = LEVELA(kb->kb_PlatformData->MMU_Level_A, addr);
		if (ISINVALID(desca))
			desca = LEVELA(kb->kb_PlatformData->MMU_Level_A, addr) = alloc_descriptor(mmutype, LEVELB_SIZE);
		if (ISINVALID(desca))
			return FALSE;
		descb = LEVELB(desca, addr);
		if (ISINVALID(descb))
			descb = LEVELB(desca, addr) = alloc_descriptor(mmutype, LEVELC_SIZE);
		if (ISINVALID(descb))
			return FALSE;

		if (invalid) {
			pagedescriptor = INVALID_DESCRIPTOR;
		} else {
			pagedescriptor = ((ULONG)physaddr) & ~page_mask;
			if (mmutype == MMU030) {
				pagedescriptor |= 1; // resident page
				if (writeprotect)
					pagedescriptor |= 4; // write-protected
				/* 68030 can only enable or disable caching */
				if (cachemode >= CM_SERIALIZED)
					pagedescriptor |= 1 << 6;
			} else {
				pagedescriptor |= 3; // resident page
				if (writeprotect)
					pagedescriptor |= 4; // write-protected
				if (supervisor)
					pagedescriptor |= 1 << 7;
				pagedescriptor |= cachemode << 5;
			}
		}

		LEVELC(descb, addr) = pagedescriptor;
		size -= page_size;
		addr += page_size;
		physaddr += page_size;
	}

	return TRUE;
}

BOOL unmap_region(struct KernelBase *kb, void *addr, ULONG size)
{
	return map_region(kb, addr, NULL, size, TRUE, FALSE, FALSE, 0);
}
