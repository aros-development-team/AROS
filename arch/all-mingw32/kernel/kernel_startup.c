#include <aros/arossupportbase.h>
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
#include "exception.h"

#define D(x)

/* External functions from exec.library */
extern struct ExecBase *PrepareExecBase(struct MemHeader *);
extern ULONG **Exec_RomTagScanner(struct ExecBase*, UWORD**);

/* External functions from kernel_debug.c */
extern int mykprintf(const UBYTE * fmt, ...);
extern int myvkprintf(const UBYTE *fmt, va_list args);
extern int myrkprintf(const STRPTR foo, const STRPTR bar, int baz, const UBYTE * fmt, ...);

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
    "core_exception",
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
    struct mb_mmap *mmap = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    while ((tag = krnNextTagItem(&tstate))) {
	switch (tag->ti_Tag) {
	case KRN_KernelLowest:
	    ranges[0] = (UWORD *)tag->ti_Data;
	    break;

	case KRN_KernelHighest:
	    ranges[1] = (UWORD *)tag->ti_Data;
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
	}
    }

    /* Set globals only AFTER __kernel_bss() */
    BootMsg = msg;

    if ((!ranges[0]) || (!ranges[1]) || (!mmap) || (!HostIFace)) {
	mykprintf("[Kernel] Not enough parameters from bootstrap!\n");
	return -1;
    }

    hostlib = HostIFace->HostLib_Open("Libs\\Host\\kernel.dll", &errstr);
    if (!hostlib) {
	mykprintf("[Kernel] Failed to load host-side module: %s\n", errstr);
	HostIFace->HostLib_FreeErrorStr(errstr);
	return -1;
    }

    badsyms = HostIFace->HostLib_GetInterface(hostlib, kernel_functions, &KernelIFace);
    if (badsyms) {
	mykprintf("[Kernel] Failed to find %lu functions in host-side module\n", badsyms);
	HostIFace->HostLib_Close(hostlib, NULL);
	return -1;
    }

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

    D(mykprintf("[Kernel] calling PrepareExecBase@%p mh_First=%p\n",PrepareExecBase,mh->mh_First));
    /*
     * FIXME: This routine is part of exec.library, however it doesn't have an LVO
     * (it can't have one because exec.library is not initialized yet) and is called
     * only from here. Probably the code should be reorganized and this routine needs
     * to be moved to kernel.resource
     */
    SysBase = PrepareExecBase(mh);
    mykprintf("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);

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
	mh->mh_Lower = ranges[0];
	mh->mh_Upper = ranges[1];
	mh->mh_Free = 0;                        /* Never allocate from this chunk! */
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;

    SysBase->ResModules = Exec_RomTagScanner(SysBase,ranges);

    /*
     * BEGIN_EXCEPTION() and END_EXCEPTION() are clever macros which create a SEH
     * frame around the nested code. We use it in order to catch exceptions in
     * AROS thread.
     */
    BEGIN_EXCEPTION(KernelIFace.core_exception);

    mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);

    mykprintf("[Kernel] leaving startup!\n");
    END_EXCEPTION();

    HostIFace->HostLib_Close(hostlib, NULL);
    return 1;
}
