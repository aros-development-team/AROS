
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <libraries/configvars.h>

#define ZeroPageInvalid 1

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

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_COLDSTART,
   VERSION,
   NT_UNKNOWN,
   100,
   (STRPTR)name_string,
   (STRPTR)version_string,
   (APTR)Init
};


#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)

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

static void swapvbr(ULONG *vbr)
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
	ULONG *vbr;
	char *args;

	KernelBase = OpenResource("kernel.resource");
	if (!KernelBase)
		return NULL;
	/* Parse some arguments from command line */
#if 1
	args = (char *)LibGetTagData(KRN_CmdLine, 0, KrnGetBootInfo());
	if (!args)
		return NULL;
	if (!strstr(args, "mmu"))
		return FALSE;
#endif
	if (!init_mmu(KernelBase))
		return NULL;

	bug("Initializing MMU setup\n");

	/* Move VBR to Fast RAM (Preparation for SysBase is the only valid address in "zero page") */
	vbr = AllocMem(256 * sizeof(ULONG), MEMF_PUBLIC);
	if (vbr) {
		ULONG *zero = (ULONG*)0;
		CopyMem(zero, vbr, 256 * sizeof(ULONG));
		swapvbr(vbr);
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
		addr &= ~PAGE_MASK;
		size = memheaders[i * 2 + 1] - addr;
		size += PAGE_MASK;
		size &= ~PAGE_MASK;
		tm = TypeOfMem((void*)(addr + 2 * PAGE_SIZE));
		if (tm & MEMF_CHIP)
			mmuchipram(KernelBase, addr, size);
		else if (tm & MEMF_FAST)
			mmuram(KernelBase, addr, size);
	}
	FreeVec(memheaders);
	/* Mark "zero page" invalid, MMU support handles ExecBase fetches transparently.
	 * Special bus error handler checks if access was LONG READ from address 4.
	 */
	if (ZeroPageInvalid)
		KrnSetProtection(0, PAGE_SIZE, 0);
	else
		KrnSetProtection(0, PAGE_SIZE, MAP_Readable | MAP_CacheInhibit);
	

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
	mmuprotect(KernelBase, (ULONG)&_rom_start, (((ULONG)(&_rom_end - &_rom_start)) + PAGE_MASK) & ~PAGE_MASK);
	mmuprotect(KernelBase, (ULONG)&_ext_start, (((ULONG)(&_ext_end - &_ext_start)) + PAGE_MASK) & ~PAGE_MASK);
	/* Custom chipset & IO */
	addr = (ULONG)SysBase->MaxExtMem;
	if (addr < 0x00d80000)
		addr = 0x00d80000;
	mmuio(KernelBase, addr, 0x00e00000 - addr);
	/* CIA */
	mmuio(KernelBase, 0x00bfd000, 0x00001000);
	mmuio(KernelBase, 0x00bfe000, 0x00001000);

	debug_mmu(KernelBase);
	enable_mmu(KernelBase);
	
	/* We can safely enable data caches now */
	CacheControl(CACRF_EnableD | CACRF_EnableI, CACRF_EnableD | CACRF_EnableI);

	AROS_USERFUNC_EXIT

	return NULL;
}
