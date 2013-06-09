
#define DEBUG 1
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <libraries/configvars.h>
#include <hardware/cpu/memory.h>

#include "exec_intern.h"
#include "early.h"
#undef KernelBase

#define FASTREMAP 1

#define _STR(A) #A
#define STR(A) _STR(A)

#define NAME "mmu"
#define VERSION 41
#define REVISION 1

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT name_string[] = NAME;
static const TEXT version_string[] =
   NAME " " STR(VERSION) "." STR(REVISION) "\n";

extern void mmu_end(void);

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)&mmu_end,
   RTF_COLDSTART,
   VERSION,
   NT_UNKNOWN,
   100,
   (STRPTR)name_string,
   (STRPTR)version_string,
   (APTR)Init
};

void enable_mmu(void *kb);
void debug_mmu(void *kb);
extern BOOL init_mmu(void *kb);

extern BYTE _rom_start;
extern BYTE _rom_end;
extern BYTE _ext_start;
extern BYTE _ext_end;

static void mmuprotect(void *KernelBase, ULONG addr, ULONG size)
{
	KrnSetProtection((void*)addr, size, MAP_Readable | MAP_Executable);
}
static void mmuram(void *KernelBase, ULONG addr, ULONG size)
{
	KrnSetProtection((void*)addr, size, MAP_Readable | MAP_Writable | MAP_Executable);
}
static void mmuchipram(void *KernelBase, ULONG addr, ULONG size)
{
	KrnSetProtection((void*)addr, size, MAP_Readable | MAP_Writable | MAP_Executable | MAP_CacheInhibit);
}
static void mmuio(void *KernelBase, ULONG addr, ULONG size)
{
	KrnSetProtection((void*)addr, size, MAP_Readable | MAP_Writable | MAP_CacheInhibit);
}

static APTR AllocPagesAligned(ULONG pages)
{
    APTR ret;
    ret = AllocMem((pages + 1) * PAGE_SIZE, MEMF_CLEAR | MEMF_FAST | MEMF_REVERSE);
    if (ret == NULL)
    	return NULL;
    Forbid();
    FreeMem(ret, (pages + 1) * PAGE_SIZE);
    ret = AllocAbs((pages * PAGE_SIZE + PAGE_SIZE - 1) & PAGE_MASK, (APTR)((((ULONG)ret) + PAGE_SIZE - 1) & PAGE_MASK));
    Permit();
    return ret;
}

static void swapvbr(APTR vbr)
{
	asm volatile (
	".chip 68010\n"
	"move.l	%0,%%d0\n"
	"move.l 4.w,%%a6\n"
	"lea	newvbr(%%pc),%%a5\n"
	"jsr	-0x1e(%%a6)\n"
	"bra.s	0f\n"
	"newvbr:\n"
	"movec	%%d0,%%vbr\n"
	"rte\n"
	"0:\n"
	: : "m" (vbr) : "d0", "d1", "a5", "a6");
}

static BOOL ISA3000(void)
{
    if (!(SysBase->AttnFlags & AFF_68030))
    	return FALSE;
    if (SysBase->AttnFlags & AFF_68040)
    	return FALSE;
    if (ReadGayle())
    	return FALSE;
    /* We should check for RAMSEY.. Later.. */
    /* 0x07000000 - 0x07ffffff is A3000-only RAM region */
    return TypeOfMem((APTR)0x07ff0000) != 0;
}

static void mmuprotectregion(void *KernelBase, const UBYTE *name, APTR addr, ULONG size, ULONG flags)
{
    ULONG allocsize = (size +  PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    KrnSetProtection(addr, allocsize, 0);
    if (FASTREMAP && TypeOfMem(addr) & MEMF_CHIP) {
    	APTR newmem = AllocPagesAligned(allocsize / PAGE_SIZE);
    	if (newmem) {
    	    CopyMem(addr, newmem, size);
    	    D(bug("Remapped %d byte Chip region to Fast, %p - %p -> %p - %p (%s), flags %08x\n",
                size, addr, addr + size - 1, newmem, newmem + size - 1, name, flags));
	    KrnMapGlobal(addr, newmem, allocsize, flags);
    	    return;
    	}
    }
    D(bug("Protected %d byte region %p - %p (%s) using flags %08x\n", size, addr, addr + size - 1, name, flags));
    KrnSetProtection(addr, allocsize, flags);
}

/* MMU protect ArosBootStrap loaded ROM modules */
static void mmuprotectextrom(void *KernelBase)
{
    UWORD i;
    struct MemList *ml;
    struct BootStruct *bs = GetBootStruct(SysBase);

    if (bs == NULL)
        return;

    ForeachNode(bs->mlist, ml) {
        if (ml->ml_Node.ln_Type == NT_KICKMEM) {
            for(i = 0; i < ml->ml_NumEntries; i++) {
                mmuprotectregion(KernelBase, "ROM", ml->ml_ME[i].me_Addr, ml->ml_ME[i].me_Length, MAP_Readable | MAP_Executable);
            }
        }
    }
}

#define MAX_HEADERS 100

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
	AROS_USERFUNC_INIT

	void *KernelBase;
	void *ExpansionBase;
	struct	ConfigDev *cd;
	ULONG addr, size;
	ULONG *memheaders;
	struct MemHeader *mh;
	UWORD i, cnt;
	char *args;
	BOOL ZeroPageInvalid = FALSE, ZeroPageProtect = FALSE;
	BOOL usemmu = FALSE;
	UBYTE *vbrpage;
	ULONG *zero = (ULONG*)0;

	if (!(SysBase->AttnFlags & AFF_68020))
		return NULL;

	KernelBase = OpenResource("kernel.resource");
	if (!KernelBase)
		return NULL;

	/* Parse some arguments from command line */
	args = (char *)LibGetTagData(KRN_CmdLine, 0, KrnGetBootInfo());
	if (args) {
		if (strstr(args, "nommu")) {
			return NULL;
		} else if (strstr(args, "debugmmu")) {
			usemmu = TRUE;
			ZeroPageInvalid = TRUE;
		} else if (strstr(args, "pmmu")) {
			usemmu = TRUE;
			ZeroPageProtect = TRUE;
		} else if (strstr(args, "mmu")) {
			usemmu = TRUE;
		}
	}

	/* if 68030 A3000 = use MMU, we are guaranteed to have full 68030 */
	if (!usemmu && ISA3000())
		usemmu = TRUE;

	/* 68030/68851: Only enable if mmu commandline detected. */
	if (!(SysBase->AttnFlags & AFF_68040) && !usemmu)
		return FALSE;

	if (!init_mmu(KernelBase)) {
		D(bug("MMU initialization failed\n"));
		return NULL;
	}

	D(bug("Initializing MMU setup\n"));

	Disable();

	vbrpage = AllocPagesAligned(1);
	if (vbrpage) {
		/* Move VBR to Fast RAM */
		CopyMem(zero, vbrpage, PAGE_SIZE);
		swapvbr(vbrpage);
		D(bug("VBR %p\n", vbrpage));
		if (ZeroPageInvalid || ZeroPageProtect) {
			/* Corrupt original zero page vectors, makes bad programs crash faster if we don't
	 	 	* want MMU special zero page handling */
			for (i = 0; i < 64; i++) {
				if (i != 1)
					zero[i] = 0xdeadf00d;
			}
		}
	}

	/* RAM */
	memheaders = AllocVec(sizeof(ULONG) * 2 * MAX_HEADERS, MEMF_PUBLIC);
	if (!memheaders)
		return NULL;
	Forbid();
	cnt = 0;
	mh = (struct MemHeader*)SysBase->MemList.lh_Head;
	while (mh->mh_Node.ln_Succ && cnt < MAX_HEADERS) {
		memheaders[cnt * 2 + 0] = (ULONG)mh->mh_Lower;
		memheaders[cnt * 2 + 1] = (ULONG)mh->mh_Upper;
		cnt++;
		mh = (struct MemHeader*)(mh->mh_Node.ln_Succ);
	}
	Permit();
	for (i = 0; i < cnt; i++) {
		ULONG tm;
		addr = memheaders[i * 2 + 0];
		addr &= PAGE_MASK;
		size = memheaders[i * 2 + 1] - addr;
		size += PAGE_SIZE - 1;
		size &= PAGE_MASK;
		tm = TypeOfMem((void*)(addr + 2 * PAGE_SIZE));
		if (tm & MEMF_CHIP)
			mmuchipram(KernelBase, addr, size);
		else if (tm & MEMF_FAST)
			mmuram(KernelBase, addr, size);
	}
	FreeVec(memheaders);
	if (ReadGayle()) {
		/* PCMCIA regions */
		mmuram(KernelBase, 0x00600000, 0x00400000);
		mmuio(KernelBase, 0x00a00000, 0x00050000);
	}

	if (ZeroPageInvalid) {
		/* Mark "zero page" invalid, MMU support handles ExecBase fetches transparently.
	 	* Special bus error handler checks if access was LONG READ from address 4.
	 	*/
		KrnSetProtection(0, PAGE_SIZE, 0);
	} else if (ZeroPageProtect) {
		/* Remap zero page to Fast RAM, faster SysBase access */
		mmuprotectregion(KernelBase, "ZeroPage", 0, PAGE_SIZE, MAP_Readable);
	} else {
		/* No special protection, cacheable */
		KrnSetProtection(0, PAGE_SIZE, MAP_Readable | MAP_Writable);
	}
	/* Protect Supervisor stack if MMU debugging mode */
	mmuprotectregion(KernelBase, "SS_Stack", SysBase->SysStkLower, SysBase->SysStkUpper - SysBase->SysStkLower,
		MAP_Readable | MAP_Writable | ((ZeroPageInvalid || ZeroPageProtect) ? MAP_Supervisor : 0));

	/* Expansion IO devices */
	ExpansionBase = TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
	cd = NULL;
	while ((cd = FindConfigDev(cd, -1, -1))) {
		if (cd->cd_Rom.er_Type & ERTF_MEMLIST)
			continue;
		/* Mark all non-RAM (IO) regions as noncacheabled */
		mmuio(KernelBase, (ULONG)cd->cd_BoardAddr, cd->cd_BoardSize);
	}
	CloseLibrary(ExpansionBase);
	
	/* Some boards may use this as an IO.. */
	mmuio(KernelBase, 0x00f00000, 0x00080000);
	/* ROM areas */
	mmuprotect(KernelBase, 0x00e00000, 0x00080000);
	mmuprotect(KernelBase, 0x00f80000, 0x00080000);

	mmuprotectextrom(KernelBase);

	/* Custom chipset & Clock & Mainboard IO */
	addr = (ULONG)SysBase->MaxExtMem;
	if (addr < 0x00d80000)
		addr = 0x00d80000;
	mmuio(KernelBase, addr, 0x00e00000 - addr);
	/* CIA */
	mmuio(KernelBase, 0x00bfd000, 0x00001000);
	mmuio(KernelBase, 0x00bfe000, 0x00001000);
	/* CD32 Akiko */
	mmuio(KernelBase, 0x00b80000, 0x00001000);

	//debug_mmu(KernelBase);

   	CacheClearU();
	enable_mmu(KernelBase);
	
	Enable();

	AROS_USERFUNC_EXIT

	return NULL;
}

void mmu_end(void) { };
