#include <aros/arossupportbase.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <memory.h>

#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_debug.h"
#include "kernel_init.h"
#include "kernel_tagitems.h"
#include "winapi.h"

/* External functions from exec.library */
extern struct ExecBase *PrepareExecBase(struct MemHeader *, char *);
extern ULONG **Exec_RomTagScanner(struct ExecBase*, UWORD**);

#undef kprintf
#undef vkprintf
#undef rkprintf

/* External functions from kernel_debug.c */
extern int mykprintf(const char * fmt, ...);
extern int myvkprintf(const char *fmt, va_list args);
extern int myrkprintf(const char *foo, const char *bar, int baz, const char *fmt, ...);

/* Some globals we can't live without */
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;

/* auto init */
static int Core_Init(APTR KernelBase)
{
    D(bug("[KRN] initializing host-side kernel module\n"));
    return KernelIFace.core_init(SysBase->VBlankFrequency, SysBase, KernelBase);
}

ADD2INITLIB(Core_Init, 10)

char *kernel_functions[] = {
    "core_init",
    "core_intr_disable",
    "core_intr_enable",
    "core_syscall",
    "core_is_super",
    "core_protect",
    "core_putc",
    "core_getc",
    NULL
};

/* rom startup */
int __startup startup(struct TagItem *msg)
{
    void *hostlib;
    char *errstr;
    unsigned long badsyms;
    struct MemHeader *mh;
    void *memory;
    IPTR memlen;
    struct TagItem *tag;
    struct TagItem *tstate = msg;
    void *klo = NULL;
    void *khi = NULL;
    struct mb_mmap *mmap = NULL;
    char *args = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    while ((tag = krnNextTagItem(&tstate))) {
	switch (tag->ti_Tag) {
	case KRN_KernelLowest:
	    klo = (UWORD *)tag->ti_Data;
	    break;

	case KRN_KernelHighest:
	    khi = (UWORD *)tag->ti_Data;
	    break;

	case KRN_MMAPAddress:
	    mmap = (struct mb_mmap *)tag->ti_Data;
	    break;

	case KRN_KernelBss:
	    __clear_bss((struct KernelBSS *)tag->ti_Data);
	    break;

	case KRN_HostInterface:
	    HostIFace = (struct HostInterface *)tag->ti_Data;
	    break;
	
	case KRN_CmdLine:
	    args = (char *)tag->ti_Data;
	    break;
	}
    }

    /* Set globals only AFTER __kernel_bss() */
    BootMsg = msg;

    if ((!klo) || (!khi) || (!mmap) || (!HostIFace)) {
	mykprintf("[Kernel] Not enough parameters from bootstrap!\n");
	return -1;
    }

    hostlib = HostIFace->HostLib_Open("Libs\\Host\\kernel.dll", &errstr);
    if (!hostlib) {
	mykprintf("[Kernel] Failed to load host-side module: %s\n", errstr);
	HostIFace->HostLib_FreeErrorStr(errstr);
	return -1;
    }

    badsyms = HostIFace->HostLib_GetInterface(hostlib, kernel_functions, (APTR **)&KernelIFace);
    if (badsyms) {
	mykprintf("[Kernel] Failed to find %lu functions in host-side module\n", badsyms);
	HostIFace->HostLib_Close(hostlib, NULL);
	return -1;
    }

    /* Turn kernel space read-only */
    KernelIFace.core_protect(klo, khi - klo + 1, PAGE_EXECUTE_READ);

    mykprintf("[Kernel] preparing first mem header\n");
    /* We know that memory map has only one RAM element */
    memory = (void *)mmap->addr;
    memlen = mmap->len;

    /* Prepare the first mem header and hand it to PrepareExecBase to take SysBase live */
    mh = memory;
    mh->mh_Node.ln_Type  = NT_MEMORY;
    mh->mh_Node.ln_Name = "chip memory";
    mh->mh_Node.ln_Pri = -5;
    mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA | MEMF_KICK;
    mh->mh_First = memory + MEMHEADER_TOTAL;
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = memlen - MEMHEADER_TOTAL;
    mh->mh_Lower = memory;
    mh->mh_Upper = memory + memlen - 1;
    mh->mh_Free = mh->mh_First->mc_Bytes;

    D(mykprintf("[Kernel] calling PrepareExecBase(), mh_First = 0x%p, args = %s\n", mh->mh_First, args));
    /*
     * FIXME: This routine is part of exec.library, however it doesn't have an LVO
     * (it can't have one because exec.library is not initialized yet) and is called
     * only from here. Probably the code should be reorganized and this routine needs
     * to be moved to kernel.resource
     */
    SysBase = PrepareExecBase(mh, args);
    D(mykprintf("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);)

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    if ((mh = (struct MemHeader *)AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC))) {
	mh->mh_Node.ln_Type = NT_MEMORY;
	mh->mh_Node.ln_Name = "rom memory";
	mh->mh_Node.ln_Pri = -128;
	mh->mh_Attributes = MEMF_KICK;
	mh->mh_First = NULL;
	mh->mh_Lower = klo;
	mh->mh_Upper = khi;
	mh->mh_Free = 0;                        /* Never allocate from this chunk! */
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;

    ranges[0] = klo;
    ranges[1] = khi;
    SysBase->ResModules = Exec_RomTagScanner(SysBase,ranges);

    mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);

    mykprintf("[Kernel] leaving startup!\n");

    HostIFace->HostLib_Close(hostlib, NULL);
    return 1;
}
