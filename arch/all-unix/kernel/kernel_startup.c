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

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"
#include "kernel_tagitems.h"
#include "memory_intern.h"

/* This macro is defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __const

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
    "sigsuspend",
    "sigaction",
    "setitimer",
    "mprotect",
    "write",
    "mmap",
    "munmap",
#ifdef HOST_OS_linux
    "__errno_location",
#else
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
#endif
#ifdef HOST_OS_android
    "__page_size",
#else
    "getpagesize",
    "sigemptyset",
    "sigfillset",
    "sigaddset",
    "sigdelset",
#endif
    NULL
};

/*
 * Kickstart entry point. Note that our code area is already made read-only by the bootstrap.
 */
int __startup startup(struct TagItem *msg)
{
    void* _stack = AROS_GET_SP;
    struct ExecBase *SysBase = NULL;
    void *hostlib;
    char *errstr;
    unsigned int i;
    struct MemHeader *bootmh;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
    struct HostInterface *hif = NULL;
    UWORD *klo = NULL;
    UWORD *khi = NULL;
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

    hostlib = HostIFace->hostlib_Open(LIBC_NAME, &errstr);
    AROS_HOST_BARRIER
    if (!hostlib) {
	bug("[Kernel] Failed to load %s: %s\n", LIBC_NAME, errstr);
	return -1;
    }

    for (i = 0; kernel_functions[i]; i++)
    {
	void *func = HostIFace->hostlib_GetPointer(hostlib, kernel_functions[i], &errstr);

	AROS_HOST_BARRIER
        if (!func) {
	    bug("[Kernel] Failed to find symbol %s in host-side module: %s\n", kernel_functions[i], errstr);
	    return -1;
	}
	((void **)&KernelIFace)[i] = func;
    }

    bug("[Kernel] preparing first mem header\n");
    /* We know that memory map has only one RAM element */
    bootmh = (struct MemHeader *)mmap->addr;

    /* Prepare the first mem header and hand it to PrepareExecBase to take SysBase live */
    krnCreateMemHeader("Normal RAM", 0, bootmh, mmap->len, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK);

    D(bug("[Kernel] calling PrepareExecBase(), mh_First = %p, args = %s\n", bootmh->mh_First, args));
    /*
     * FIXME: This routine is part of exec.library, however it doesn't have an LVO
     * (it can't have one because exec.library is not initialized yet) and is called
     * only from here. Probably the code should be reorganized and this routine needs
     * to be moved to kernel.resource
     */
    SysBase = PrepareExecBase(bootmh, args, HostIFace);
    D(bug("[Kernel] SysBase=%p, mh_First=%p\n", SysBase, bootmh->mh_First));

    ranges[0] = klo;
    ranges[1] = khi;
    SysBase->ResModules = krnRomTagScanner(bootmh, ranges);

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    krnCreateROMHeader(bootmh, "Kickstart ROM", klo, khi);

    /* Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    krnCreateROMHeader(bootmh, "Boot stack", _stack - 3072, _stack);

    bug("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);
    D(bug("[Kernel] calling InitCode(RTF_COLDSTART,0)\n"));
    InitCode(RTF_COLDSTART, 0);

    bug("[Kernel] leaving startup!\n");
    HostIFace->hostlib_Close(hostlib, NULL);
    AROS_HOST_BARRIER

    return 1;
}
