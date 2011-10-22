
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
extern BYTE _bss;
extern BYTE _bss_end;

static void mmuprotect(void *KernelBase, ULONG addr, ULONG size)
{
	KrnSetProtection((void*)addr, size, MAP_Readable | MAP_Executable);
}
static void mmuprotectremap(void *KernelBase, ULONG addr, ULONG size)
{
	/* Set invalid first so that possible old mapping will be overridden */
	KrnSetProtection((void*)addr, size, 0);
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
    ret = AllocMem((pages + 1) * PAGE_SIZE, MEMF_CLEAR | MEMF_PUBLIC);
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

/* MMU protect ArosBootStrap loaded ROM modules */
static void mmuprotectextrom(void *KernelBase)
{
    IPTR *list = SysBase->ResModules;
    if (list)
    {
	while (*list)
	{
	    struct Resident *res;
	    if (*list & RESLIST_NEXT)
	    {
	    	list = (IPTR *)(*list & ~RESLIST_NEXT); 
            	continue;
            }
	    res = (struct Resident *)*list++;
	    if (res->rt_Flags & (1 << 5)) {
	    	ULONG start, end;
	    	LONG size;
		start = ((ULONG)res) & ~(PAGE_SIZE - 1);
		if (!((start >= 0x00e00000 && start < 0x00e7ffff) || ((start >= 0x00f80000 && start < 0x00ffffff)))) {
		    end = (((ULONG)res->rt_EndSkip) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
		    size = end - start;
		    if (size > 0) {
		    	//bug("start=%p end=%p size=%d %s", res, res->rt_EndSkip, (UBYTE*)res->rt_EndSkip - (UBYTE*)res, res->rt_IdString);
			mmuprotectremap(KernelBase, start, size);
		    }
		}
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
	BOOL mmucommandline = FALSE;
	UBYTE *pages;
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
			mmucommandline = TRUE;
			ZeroPageInvalid = TRUE;
		} else if (strstr(args, "pmmu")) {
			mmucommandline = TRUE;
			ZeroPageProtect = TRUE;
		} else if (strstr(args, "mmu")) {
			mmucommandline = TRUE;
		}
	}

	/* 68030/68851: Only enable if mmu commandline detected. */
	if (!(SysBase->AttnFlags & AFF_68040) && !mmucommandline)
		return FALSE;

	if (!init_mmu(KernelBase)) {
		D(bug("MMU initialization failed\n"));
		return NULL;
	}

	D(bug("Initializing MMU setup\n"));

	pages = AllocPagesAligned(3);
	if (!pages)
		return NULL;

	/* Move VBR to Fast RAM */
	CopyMem(zero, pages, PAGE_SIZE);
	swapvbr(pages);
	D(bug("VBR %p\n", pages));
	if (ZeroPageInvalid || ZeroPageProtect) {
		/* Corrupt original zero page vectors, makes bad programs crash faster if we don't
	 	 * want MMU special zero page handling */
		for (i = 0; i < 64; i++) {
			if (i != 1)
				zero[i] = 0xdeadf00d;
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
		KrnMapGlobal(0, pages + PAGE_SIZE, PAGE_SIZE, MAP_Readable);
	} else {
		/* No special protection, cacheable */
		KrnSetProtection(0, PAGE_SIZE, MAP_Readable | MAP_Writable);
	}
	/* Protect Supervisor stack if MMU debugging mode */
	if (ZeroPageInvalid || ZeroPageProtect) {
		KrnSetProtection(SysBase->SysStkLower, SysBase->SysStkUpper - SysBase->SysStkLower, 0);
		KrnSetProtection(SysBase->SysStkLower, SysBase->SysStkUpper - SysBase->SysStkLower, MAP_Readable | MAP_Writable | MAP_Supervisor); 
	}
#if 0
	/* Remap BSS to Fast RAM, faster performance than Chip RAM */
	KrnMapGlobal((void*)(0 + PAGE_SIZE), pages + 2 * PAGE_SIZE, PAGE_SIZE, MAP_Readable | MAP_Writable);
#endif

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

	//debug_mmu(KernelBase);

	CopyMem(pages, pages + PAGE_SIZE, PAGE_SIZE);
	CopyMem((APTR)((ULONG)_bss & PAGE_MASK), pages + 2 * PAGE_SIZE, PAGE_SIZE);
	enable_mmu(KernelBase);

	AROS_USERFUNC_EXIT

	return NULL;
}

void mmu_end(void) { };
