#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <sys/mman.h>
#include <inttypes.h>

/*
 * Private exec.library include, needed for MEMHEADER_TOTAL.
 * TODO: may be bring it out to public includes ?
 */
#include "memory.h"

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"
#include "kernel_tagitems.h"

/*
 * External early init function from exec.library
 * TODO: find some way to discover it dynamically
 */
extern struct ExecBase *PrepareExecBase(struct MemHeader *, char *, struct HostInterface *);

/* Some globals we can't live without */
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;

/* libc functions that we use */
static const char *kernel_functions[] = {
    "raise",
    "sigprocmask",
    "sigemptyset",
    "sigfillset",
    "sigdelset",
    "sigsuspend",
    "sigaction",
    "setitimer",
    "mprotect",
    "write",
#ifdef HOST_OS_linux
    "__errno_location",
#else
    "__error",
#endif
    NULL
};

/*
 * Kickstart entry point. Note that our code area is already made read-only by the bootstrap.
 */
int __startup startup(struct TagItem *msg)
{
    void* _stack = AROS_GET_SP;
    struct ExecBase *SysBase;
    void *hostlib;
    char *errstr;
    unsigned int i;
    struct MemHeader *mh;
    void *memory;
    IPTR memlen;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
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

    /* If there's no HostIFace, we can't even say anything */
    if (!HostIFace)
	return -1;

    D(bug("[Kernel] Starting up...\n"));

    if ((!klo) || (!khi) || (!mmap)) {
	bug("[Kernel] Not enough parameters from bootstrap!\n");
	return -1;
    }

    if (strcmp(HostIFace->System, AROS_ARCHITECTURE))
    {
	bug("[Kernel] This kernel is built for %s architecture\n", AROS_ARCHITECTURE);
	bug("[Kernel] Your bootstrap is running on %s which is incompatible\n", HostIFace->System);
	return -1;
    }

    if (HostIFace->Version < HOSTINTERFACE_VERSION)
    {
	bug("[Kernel] Obsolete bootstrap interface (found v%u, need v%u)\n",
		  HostIFace->Version, HOSTINTERFACE_VERSION);
	return -1;
    }

    hostlib = HostIFace->HostLib_Open(LIBC_NAME, &errstr);
    AROS_HOST_BARRIER
    if (!hostlib) {
	bug("[Kernel] Failed to load %s: %s\n", LIBC_NAME, errstr);
	return -1;
    }

    for (i = 0; kernel_functions[i]; i++)
    {
	void *func = HostIFace->HostLib_GetPointer(hostlib, kernel_functions[i], &errstr);

	AROS_HOST_BARRIER
        if (!func) {
	    bug("[Kernel] Failed to find symbol %s in host-side module: %s\n", kernel_functions[i], errstr);
	    return -1;
	}
	((void **)&KernelIFace)[i] = func;
    }

    bug("[Kernel] preparing first mem header\n");
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

    D(bug("[Kernel] calling PrepareExecBase(), mh_First = 0x%p, args = %s\n", mh->mh_First, args));
    /*
     * FIXME: This routine is part of exec.library, however it doesn't have an LVO
     * (it can't have one because exec.library is not initialized yet) and is called
     * only from here. Probably the code should be reorganized and this routine needs
     * to be moved to kernel.resource
     */
    SysBase = PrepareExecBase(mh, args, HostIFace);
    D(bug("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);)

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

    /* Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    if ((mh = (struct MemHeader *)AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC)))
    {
        mh->mh_Node.ln_Type = NT_MEMORY;
        mh->mh_Node.ln_Name = "stack memory";
        mh->mh_Node.ln_Pri = -128;
        mh->mh_Attributes = MEMF_KICK;
        mh->mh_First = NULL;
        mh->mh_Lower = _stack - 3072;
        mh->mh_Upper = _stack;
        mh->mh_Free = 0;                        /* Never allocate from this chunk! */
        Enqueue(&SysBase->MemList, &mh->mh_Node);
    }

    ranges[0] = klo;
    ranges[1] = khi;
    SysBase->ResModules = krnRomTagScanner(SysBase, ranges);

    bug("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);

    bug("[Kernel] leaving startup!\n");
    HostIFace->HostLib_Close(hostlib, NULL);
    AROS_HOST_BARRIER

    return 1;
}
