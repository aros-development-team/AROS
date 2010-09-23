#include <aros/arossupportbase.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <inttypes.h>

/*
 * Private exec.library include, needed for MEMHEADER_TOTAL.
 * TODO: may be bring it out to public includes ?
 */
#include "memory.h"

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_romtags.h"
#include "kernel_tagitems.h"
#include "kernel_mingw32.h"

/*
 * External early init function from exec.library
 * TODO: find some way to discover it dynamically
 */
extern struct ExecBase *PrepareExecBase(struct MemHeader *, char *, struct HostInterface *);

/* Some globals we can't live without */
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;

static char *kernel_functions[] = {
    "core_init",
    "core_raise",
    "core_protect",
    "core_putc",
    "core_getc",
    "TrapVector",
    "IRQVector",
    "Ints_Enabled",
    "Supervisor",
    "Sleep_Mode",
    "LastErrorPtr", 
    NULL
};

/* rom startup */
int __startup startup(struct TagItem *msg)
{
    struct ExecBase *SysBase;
    void *hostlib;
    char *errstr;
    unsigned long badsyms;
    struct MemHeader *mh;
    void *memory;
    IPTR memlen;
    struct TagItem *tag;
    struct TagItem *tstate = msg;
    struct HostInterface *hif = NULL;
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
	    hif = (struct HostInterface *)tag->ti_Data;
	    break;

	case KRN_CmdLine:
	    args = (char *)tag->ti_Data;
	    break;
	}
    }

    /* Set globals only AFTER __clear_bss() */
    BootMsg = msg;
    HostIFace = hif;

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

    badsyms = HostIFace->HostLib_GetInterface(hostlib, kernel_functions, &KernelIFace);
    if (badsyms) {
	mykprintf("[Kernel] Failed to find %u symbols in host-side module\n", badsyms);
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
    SysBase = PrepareExecBase(mh, args, HostIFace);
    D(mykprintf("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);)

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    if ((mh = (struct MemHeader *)AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC)))
    {
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

    /* In order for these functions to work before KernelBase and ExecBase are set up */
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;

    ranges[0] = klo;
    ranges[1] = khi;
    SysBase->ResModules = krnRomTagScanner(SysBase, ranges);

    D(mykprintf("[Kernel] initializing host-side kernel module\n"));
    *KernelIFace.TrapVector = core_TrapHandler;
    *KernelIFace.IRQVector  = core_IRQHandler;
    if (!KernelIFace.core_init(SysBase->VBlankFrequency)) {
	mykprintf("[Kernel] Failed to start up virtual machine!\n");
	return -1;
    }

    mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);

    mykprintf("[Kernel] leaving startup!\n");
    HostIFace->HostLib_Close(hostlib, NULL);
    return 1;
}
