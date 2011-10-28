#include <aros/altstack.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <sys/mman.h>

#include <inttypes.h>
#include <string.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"

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

/* Some globals we can't live without */
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;
ULONG mm_PageSize;

/* Here we use INIT set. THIS_PROGRAM_HANDLES_SYMBOLSETS is declared in kernel_init.c. */
DEFINESET(INIT);

/* libc functions that we use */
static const char *kernel_functions[] =
{
    "raise",
    "sigprocmask",
    "sigsuspend",
    "sigaction",
    "mprotect",
    "read",
    "fcntl",
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
    "write",
    "sigwait",
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
    struct HostInterface *hif = NULL;
    ULONG pageSize;
    struct mb_mmap *mmap = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};
    const struct TagItem *tstate = msg;

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
    BootMsg   = msg;
    HostIFace = hif;

    /* If there's no proper HostIFace, we can't even say anything */
    if (!HostIFace)
	return -1;

    if (strcmp(HostIFace->System, AROS_ARCHITECTURE))
	return -1;

    if (HostIFace->Version < HOSTINTERFACE_VERSION)
	return -1;

    /* Host interface is okay. We have working krnPutC() and can talk now */
    D(bug("[Kernel] Starting up...\n"));

    if ((!ranges[0]) || (!ranges[1]) || (!mmap))
    {
	krnPanic("Not enough information from the bootstrap\n"
		 "\n"
		 "Kickstart start 0x%p, end 0x%p\n"
		 "Memory map address: 0x%p",
		 ranges[0], ranges[1], mmap);
	return -1;
    }

    hostlib = HostIFace->hostlib_Open(LIBC_NAME, &errstr);
    AROS_HOST_BARRIER
    if (!hostlib)
    {
    	krnPanic("Failed to load %s\n%s", LIBC_NAME, errstr);
	return -1;
    }

    for (i = 0; kernel_functions[i]; i++)
    {
	void *func = HostIFace->hostlib_GetPointer(hostlib, kernel_functions[i], &errstr);

	AROS_HOST_BARRIER
        if (!func)
        {
            krnPanic("Failed to find symbol %s in host-side libc\n%s", kernel_functions[i], errstr);
	    return -1;
	}
	((void **)&KernelIFace)[i] = func;
    }

    /* Here we can add some variant-specific things. Android and iOS ports use this. */
    if (!set_call_funcs(SETNAME(INIT), 1, 1))
    	return -1;

    /* Now query memory page size. We need in order to get our memory manager functional. */
#ifdef HOST_OS_android
    mm_PageSize = *KernelIFace.__page_size;
#else
    mm_PageSize = KernelIFace.getpagesize();
    AROS_HOST_BARRIER
#endif
    D(bug("[KRN] Memory page size is %u\n", mm_PageSize));

    /* We know that memory map has only one RAM element */
    bootmh = (struct MemHeader *)(IPTR)mmap->addr;

    /* Prepare the first mem header */
    D(bug("[Kernel] preparing first mem header at 0x%p (%u bytes)\n", bootmh, mmap->len));
    krnCreateMemHeader("Normal RAM", 0, bootmh, mmap->len, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|ARCH_31BIT);

    /*
     * SysBase pre-validation after a warm restart.
     * This makes sure that it points to a valid accessible memory region.
     */
    if (((IPTR)SysBase < mmap->addr) || ((IPTR)SysBase + sizeof(struct ExecBase) > mmap->addr + mmap->len))
    	SysBase = NULL;

    /* Create SysBase. After this we can use basic exec services, like memory allocation, lists, etc */
    D(bug("[Kernel] calling krnPrepareExecBase(), mh_First = %p\n", bootmh->mh_First));
    krnPrepareExecBase(ranges, bootmh, msg);

    /*
     * Set up correct stack borders and altstack.
     * Now our boot task can call relbase libraries.
     * In fact on hosted we don't know real stack limits, but
     * we know it's at least of AROS_STACKSIZE bytes long. For existing architectures
     * this seems to be true.
     * TODO: 1. Under UNIX it's possible to call getrlimits() to learn about stack limits.
     *       2. The whole altstack thing can prove unfeasible. At least currently it failed
     *		as a system-wide ABI. Alternative stack is not interrupt-safe, while AROS
     *		libraries may be (and at least several are).
     */
    SysBase->ThisTask->tc_SPLower = _stack - AROS_STACKSIZE;
    SysBase->ThisTask->tc_SPUpper = _stack;
    aros_init_altstack(SysBase->ThisTask);

    D(bug("[Kernel] SysBase=%p, mh_First=%p\n", SysBase, bootmh->mh_First));

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    krnCreateROMHeader("Kickstart ROM", ranges[0], ranges[1]);

    /*
     * Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    krnCreateROMHeader("Boot stack", _stack - AROS_STACKSIZE, _stack);

    /* Start up the system */
    InitCode(RTF_SINGLETASK, 0);
    InitCode(RTF_COLDSTART, 0);

    /* If we returned here, something went wrong, and dos.library failed to take over */
    krnPanic("Failed to start up the system");

    HostIFace->hostlib_Close(hostlib, NULL);
    AROS_HOST_BARRIER

    return 1;
}
