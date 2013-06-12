
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

static BOOL map_region2(struct KernelBase *kb, void *addr, void *physaddr, ULONG size, BOOL invalid, BOOL writeprotect, BOOL supervisor, UBYTE cachemode);


static void map_pagetable(struct KernelBase *kb, void *addr, ULONG size)
{
	/* 68040+ MMU tables should be serialized */
	map_region2(kb, addr, NULL, size, FALSE, FALSE, FALSE, CM_SERIALIZED);
}

/* Allocate MMU descriptor page, it needs to be (1 << bits) * sizeof(ULONG) aligned */
static ULONG alloc_descriptor(struct KernelBase *kb, UBYTE mmutype, UBYTE bits, UBYTE level)
{
	struct PlatformData *pd = kb->kb_PlatformData;
	ULONG *desc, dout;
	ULONG size = sizeof(ULONG) * (1 << bits);
	ULONG ps = 1 << PAGE_SIZE;
	UWORD i;

	while (pd->page_free >= size && (((ULONG)pd->page_ptr) & (size - 1))) {
		pd->page_ptr += 0x100;
		pd->page_free -= 0x100;
	}
	while (pd->page_free < size) {
		/* allocate in aligned blocks of PAGE_SIZE */
		UBYTE *mem, *newmem, *pagemem;

		mem = AllocMem(2 * ps, MEMF_PUBLIC);
		if (!mem)
			return 0;
		Forbid();
		FreeMem(mem, 2 * ps);
		newmem = (UBYTE*)((((ULONG)mem) + ps - 1) & ~(ps - 1));
		pagemem = AllocAbs(ps, newmem);
		Permit();
		if (!pagemem)
			return 0;
		pd->page_ptr = pagemem;
		pd->page_free = ps;
		// bug("New chunk %p-%p\n", pagemem, pagemem + ps - 1);
		if (level > 0 && mmutype >= MMU040)
			map_pagetable(kb, pagemem, ps);
	}
	desc = (ULONG*)pd->page_ptr;
	for (i = 0; i < (1 << bits); i++)
		desc[i] = INVALID_DESCRIPTOR;
	dout = (ULONG)desc;
	if (mmutype == MMU030)
		dout |= 2; /* Valid 4 byte descriptor */
	else
		dout |= 3; /* Resident descriptor */
	// bug("Level%c %p-%p: %08x\n", level + 'A', pd->page_ptr, pd->page_ptr + size - 1, dout);
	pd->page_ptr += size;
	pd->page_free -= size;
	return dout;
}	

BOOL init_mmu(struct KernelBase *kb)
{
	UBYTE mmutype = kb->kb_PlatformData->mmu_type;
	
	if (!mmutype)
		return FALSE;
	kb->kb_PlatformData->MMU_Level_A = (ULONG*)(alloc_descriptor(kb, mmutype, LEVELA_SIZE, 0) & ~3);
	if (!kb->kb_PlatformData->MMU_Level_A) {
		kb->kb_PlatformData->mmu_type = 0;
		return FALSE;
	}
	if (mmutype >= MMU040)
		map_pagetable(kb, kb->kb_PlatformData->MMU_Level_A, 1 << PAGE_SIZE);
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
	"subq.l	#8,%%a7\n"
	/* Disable MMU, setup root pointers,
	 * uses 68040 MMU descriptor levels (7/7/6, 4K page size) */
	"move.l	#0x00c07760,%%d1\n"
	"move.l	%%d1,%%a7@\n"
	"pmove	%%a7@,%%tc\n"
	/* Set bus error exception vector */
	"movec	%%vbr,%%a5\n"
	"move.l	#addrerror030,%%a5@(12)\n"
	"move.l	#buserror030,%%a5@(8)\n"
	/* Configure CRP. Valid 4 byte descriptor, other features disabled. */
	"move.l	#0x80000002,%%a7@\n"
	/* First level descriptor pointer */
	"move.l	%%d0,%%a7@(4)\n"
	/* Set CRP */
	"pmove	%%a7@,%%crp\n"
	/* Set MMU enabled bit */
	"bset	#31,%%d1\n"
	"move.l	%%d1,%%a7@\n"
	/* MMU on! */
	"pmove	%%a7@,%%tc\n"
	/* Clear transparent translation */
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
static void enable_mmu040(ULONG *levela, UBYTE cpu060, UBYTE *zeropagedescriptor)
{
    asm volatile (
    	".chip 68060\n"
	"move.l	%0,%%d0\n"
	"move.b	%1,%%d1\n"
	"move.l %2,%%a1\n"
	"move.l 4.w,%%a6\n"
	"lea	.esuper040(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	".esuper040:\n"
	/* Do not interrupt us */
	"or	#0x0700,%%sr\n"
	"movec	%%vbr,%%a5\n"
	"move.l %%a1,253*4(%%a5)\n"
	"lea	buserror040,%%a6\n"
	"lea	addrerror040,%%a0\n"
	"tst.b	%%d1\n"
	"beq.s	.cpu040\n"
	"lea	buserror060,%%a6\n"
	"lea	addrerror060,%%a0\n"
	".cpu040:\n"
	"move.l	%%a6,%%a5@(8)\n"
	"move.l %%a0,%%a5@(12)\n"
	"moveq	#0,%%d1\n"
	/* Disable MMU, setup root pointers */
	"movec	%%d1,%%tc\n"
	"movec	%%d0,%%urp\n"
	"movec	%%d0,%%srp\n"
	/* Flush data caches and ATC */
	"cpusha	%%dc\n"
	"cinva	%%dc\n"
	"pflusha\n"
	/* Enable MMU, 4K page size */
	"move.l	#0x00008000,%%d0\n"
	"movec	%%d0,%%tc\n"
	/* Disable transparent translation */
	"movec	%%d1,%%itt0\n"
	"movec	%%d1,%%itt1\n"
	"movec	%%d1,%%dtt0\n"
	"movec	%%d1,%%dtt1\n"
    	"rte\n"
    	"0:\n"
	: : "m" (levela), "m" (cpu060), "m" (zeropagedescriptor) : "d0", "d1", "a1", "a5", "a6");
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
		enable_mmu040(kb->kb_PlatformData->MMU_Level_A, kb->kb_PlatformData->mmu_type == MMU060, kb->kb_PlatformData->zeropagedescriptor);
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
			UBYTE cm, sp;
			if (mmutype == MMU030) {
				cm = (odesc >> 6) & 1;
				sp = 0;
			} else {
				cm = (odesc >> 5) & 3;
				sp = (odesc >> 7) & 1;
			}
			bug("%p - %p: %p WP=%d S=%d CM=%d (%08x)\n",
				startaddr, addr - 1, odesc & ~((1 << PAGE_SIZE) - 1),
				(odesc & 4) ? 1 : 0, sp, cm, odesc);
			startaddr = addr;
			odesc = desc;
		}
	}
	bug("MMU dump end\n");
}			

static BOOL map_region2(struct KernelBase *kb, void *addr, void *physaddr, ULONG size, BOOL invalid, BOOL writeprotect, BOOL supervisor, UBYTE cachemode)
{
	struct PlatformData *pd = kb->kb_PlatformData;
	ULONG desca, descb, descc, pagedescriptor;
	ULONG page_size = 1 << PAGE_SIZE;
	ULONG page_mask = page_size - 1;
	UBYTE mmutype;
	
	mmutype = pd->mmu_type;
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

	while (size) {
		desca = LEVELA(kb->kb_PlatformData->MMU_Level_A, addr);
		if (ISINVALID(desca))
			desca = LEVELA(kb->kb_PlatformData->MMU_Level_A, addr) = alloc_descriptor(kb, mmutype, LEVELB_SIZE, 1);
		if (ISINVALID(desca))
			return FALSE;
		descb = LEVELB(desca, addr);
		if (ISINVALID(descb))
			descb = LEVELB(desca, addr) = alloc_descriptor(kb, mmutype, LEVELC_SIZE, 2);
		if (ISINVALID(descb))
			return FALSE;
		descc = LEVELC(descb, addr);

		if (addr == 0 && pd->zeropagedescriptor == NULL) {
			/* special case zero page handling */
			pd->zeropagedescriptor = (UBYTE*)(& LEVELC(descb, addr)) + 3;
		}

		if (invalid) {
			pagedescriptor = INVALID_DESCRIPTOR;
			if (addr == 0 && size == page_size) {
				pagedescriptor = ((ULONG)physaddr) & ~page_mask;
				if (mmutype == MMU030) {
					pagedescriptor |= 4;
					pagedescriptor |= 1 << 6;
				} else {
					pagedescriptor |= 4; // write-protected
					pagedescriptor |= CM_SERIALIZED << 5;
				}
			}
		} else {
			BOOL wasinvalid = ISINVALID(descc);
			pagedescriptor = ((ULONG)physaddr) & ~page_mask;
			if (mmutype == MMU030) {
				pagedescriptor |= 1; // page descriptor
				if (writeprotect || (!wasinvalid && (descc & 4)))
					pagedescriptor |= 4; // write-protected
				/* 68030 can only enable or disable caching */
				if (cachemode >= CM_SERIALIZED || (!wasinvalid && (descc & (1 << 6))))
					pagedescriptor |= 1 << 6;
			} else {
				pagedescriptor |= 3; // resident page
				if (writeprotect || (!wasinvalid && (descc & 4)))
					pagedescriptor |= 4; // write-protected
				if (supervisor || (!wasinvalid && (descc & (1 << 7))))
					pagedescriptor |= 1 << 7;
				// do not override non-cached
				if (wasinvalid || cachemode > ((descc >> 5) & 3))
					pagedescriptor |= cachemode << 5;
				else
					pagedescriptor |= ((descc >> 5) & 3) << 5;
				if (addr != 0 || size != page_size)
					pagedescriptor |= 1 << 10; // global if not zero page
			}
		}

		LEVELC(descb, addr) = pagedescriptor;
		size -= page_size;
		addr += page_size;
		physaddr += page_size;
	}

	return TRUE;
}

BOOL map_region(struct KernelBase *kb, void *addr, void *physaddr, ULONG size, BOOL invalid, BOOL writeprotect, BOOL supervisor, UBYTE cachemode)
{
	bug("map_region(%p, %p, %08x, in=%d, wp=%d, s=%d cm=%d\n",
		addr, physaddr, size, invalid ? 1 : 0, writeprotect ? 1 : 0, supervisor ? 1 : 0, cachemode);
	return map_region2(kb, addr, physaddr, size, invalid, writeprotect, supervisor, cachemode);
}

BOOL unmap_region(struct KernelBase *kb, void *addr, ULONG size)
{
	bug("unmap_region(%p, %08x)\n", addr, size);
	return map_region2(kb, addr, NULL, size, TRUE, FALSE, FALSE, 0);
}
