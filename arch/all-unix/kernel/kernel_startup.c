#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <sys/mman.h>

#include <inttypes.h>
#include <string.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"
#include "memory_intern.h"

#define D(x)

/* This macro is defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __const

/*
 * On 64 bit Linux bootloaded gives us RAM region below 2GB.
 * On other 64-bit unices we have no control over it.
 */
#if (__WORDSIZE == 64)
#ifdef HOST_OS_linux
#define ARCH_31BIT MEMF_31BIT
#endif
#endif
#ifndef ARCH_31BIT
#define ARCH_31BIT 0
#endif

/*
 * This prototype is dubbed here because proto/alib.h causes conflict
 * because of struct timeval redefinition.
 */
struct TagItem *LibNextTagItem(const struct TagItem **tstate);

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
int __startup startup(struct TagItem *msg, ULONG magic)
{
    void* _stack = AROS_GET_SP;
    void *hostlib;
    char *errstr;
    unsigned int i;
    struct MemHeader *bootmh;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
    struct HostInterface *hif = NULL;
    struct mb_mmap *mmap = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    /* This bails out if the user started us from within AROS command line, as common executable */
    if (magic != AROS_BOOT_MAGIC)
    	return -1;

    while ((tag = LibNextTagItem(&tstate)))
    {
	switch (tag->ti_Tag)
	{
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
	    hif = (struct HostInterface *)tag->ti_Data;
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

    if ((!ranges[0]) || (!ranges[1]) || (!mmap)) {
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

    /* We know that memory map has only one RAM element */
    bootmh = (struct MemHeader *)(IPTR)mmap->addr;

    /* Prepare the first mem header */
    bug("[Kernel] preparing first mem header at 0x%p (%u bytes)\n", bootmh, mmap->len);
    krnCreateMemHeader("Normal RAM", 0, bootmh, mmap->len, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|ARCH_31BIT);

    SysBase = NULL;	/* TODO: check SysBase validity here when warm reboot is implemented */

    /* Create SysBase. After this we can use basic exec services, like memory allocation, lists, etc */
    D(bug("[Kernel] calling krnPrepareExecBase(), mh_First = %p\n", bootmh->mh_First));
    if (!krnPrepareExecBase(ranges, bootmh, msg)
    {
    	bug("[Kernel] Unable to create ExecBase!\n");
    	return -1;
    }

    D(bug("[Kernel] SysBase=%p, mh_First=%p\n", SysBase, bootmh->mh_First));

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    krnCreateROMHeader(bootmh, "Kickstart ROM", ranges[0], ranges[1]);

    /* Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    krnCreateROMHeader(bootmh, "Boot stack", _stack - AROS_STACKSIZE, _stack);

    bug("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);
    D(bug("[Kernel] calling InitCode(RTF_COLDSTART,0)\n"));
    InitCode(RTF_COLDSTART, 0);

    bug("[Kernel] leaving startup!\n");
    HostIFace->hostlib_Close(hostlib, NULL);
    AROS_HOST_BARRIER

    return 1;
}
