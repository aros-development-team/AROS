/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <sys/mman.h>

#include <inttypes.h>
#include <string.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_romtags.h"
#include "kernel_unix.h"

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

/* Global HostIFace is needed for krnPutC() to work before KernelBase is set up */
struct HostInterface *HostIFace;

THIS_PROGRAM_HANDLES_SYMBOLSET(STARTUP)
DEFINESET(STARTUP);

#define CHIPMEMSIZE    (16 * 1024 * 1024)

/*
 * Kickstart entry point. Note that our code area is already made read-only by the bootstrap.
 */
int __startup startup(struct TagItem *msg, ULONG magic)
{
    void* _stack = AROS_GET_SP;
    void *hostlib;
    char *errstr;
    char *cmdline = NULL;
    unsigned int mm_PageSize, memsize;
    struct MemHeader *bootmh = NULL, *chipmh = NULL, *fastmh = NULL;
#if (__WORDSIZE == 64)
    struct MemHeader *highmh = NULL;
#endif
    struct TagItem *tag, *tstate = msg;
    struct HostInterface *hif = NULL;
    struct mb_mmap *mmap = NULL;
    unsigned long mmap_len = 0;
    BOOL mem_tlsf = FALSE;

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
                /* we have atleast 1 mmap */
                if (!mmap_len)
                    mmap_len = sizeof(struct mb_mmap);
                break;

            case KRN_MMAPLength:
                mmap_len = tag->ti_Data;
                break;

            case KRN_KernelBss:
                __clear_bss((struct KernelBSS *)tag->ti_Data);
                break;

            case KRN_HostInterface:
                hif = (struct HostInterface *)tag->ti_Data;
                break;

            case KRN_CmdLine:
                cmdline = (char *)tag->ti_Data;
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

    /* Host interface is okay. We have working krnPutC() and can talk now. */
    D(nbug("[Kernel] Starting up...\n"));

    if ((!ranges[0]) || (!ranges[1]) || (!mmap))
    {
	krnPanic(NULL, "Not enough information from the bootstrap\n"
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
    	krnPanic(NULL, "Failed to load %s\n%s", LIBC_NAME, errstr);
	return -1;
    }

    /* Here we can add some variant-specific things. Android and iOS ports use this. */
    if (!set_call_libfuncs(SETNAME(STARTUP), 1, 1, hostlib))
    {
	HostIFace->hostlib_Close(hostlib, NULL);
    	AROS_HOST_BARRIER
	return -1;
    }

    /* Now query memory page size. We need in order to get our memory manager functional. */
    mm_PageSize = krnGetPageSize(hostlib);
    D(nbug("[KRN] Memory page size is %u\n", mm_PageSize));
    if (!(cmdline && strstr(cmdline, "notlsf")))
    {
        /* this should probably be handled directly in krnCreateMemHeader */
        mem_tlsf = TRUE;
        D(nbug("[KRN] Using TLSF Memory Manager\n"));
    }

    if (!mm_PageSize)
    {
    	/* krnGetPageSize() panics itself */
    	HostIFace->hostlib_Close(hostlib, NULL);
    	AROS_HOST_BARRIER
    	return -1;
    }

    /* We know that memory map has only one memory range */
    chipmh = (struct MemHeader *)(IPTR)mmap->addr;
    memsize = mmap->len > CHIPMEMSIZE ? CHIPMEMSIZE : mmap->len;

    /* Prepare the simulated mem headers */
    /* NOTE: two mem headers are created to cover more cases of memory system */
    krnCreateMemHeader("CHIP RAM", -5, chipmh, memsize, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|ARCH_31BIT);
    if (mem_tlsf)
        chipmh = krnConvertMemHeaderToTLSF(chipmh);
    bootmh = chipmh;

    if (mmap->len > memsize)
    {
        fastmh = (struct MemHeader *)(mmap->addr + memsize);
        memsize = mmap->len - memsize;
        krnCreateMemHeader("Fast RAM", 0, fastmh , memsize, MEMF_FAST|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK|ARCH_31BIT);
        if (mem_tlsf)
            fastmh = krnConvertMemHeaderToTLSF(fastmh);
        if (memsize > (1024 * 1024))
            bootmh = fastmh;
    }

#if (__WORDSIZE == 64)
    /* on 64bit platforms, we may be passed an additional mmap */
    if (mmap_len > sizeof(struct mb_mmap))
    {
        struct mb_mmap *mmapnxt = &mmap[1];

        highmh = mmapnxt->addr;

        krnCreateMemHeader("Fast RAM", 0, highmh, mmapnxt->len, MEMF_FAST|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK);
        if (mem_tlsf)
            highmh = krnConvertMemHeaderToTLSF(highmh);
    }
#endif

    /*
     * SysBase pre-validation after a warm restart.
     * This makes sure that it points to a valid accessible memory region.
     */
    if (((IPTR)SysBase < mmap->addr) || ((IPTR)SysBase + sizeof(struct ExecBase) > mmap->addr + mmap->len))
    	SysBase = NULL;

    /* Create SysBase. After this we can use basic exec services, like memory allocation, lists, etc */
    D(nbug("[Kernel] calling krnPrepareExecBase(), mh_First = %p\n", bootmh->mh_First));
    if (!krnPrepareExecBase(ranges, bootmh, msg))
    {
    	/* Hosted krnPanic() returns, allowing us to drop back into bootstrap */
    	HostIFace->hostlib_Close(hostlib, NULL);
    	AROS_HOST_BARRIER
    	return -1;
    }

    D(nbug("[Kernel] SysBase=%p, mh_First=%p\n", SysBase, bootmh->mh_First));

    if (bootmh != chipmh && chipmh)
        Enqueue(&SysBase->MemList, &chipmh->mh_Node);

    if (bootmh != fastmh && fastmh)
        Enqueue(&SysBase->MemList, &fastmh->mh_Node);

#if (__WORDSIZE == 64)
    if (bootmh != highmh && highmh)
        Enqueue(&SysBase->MemList, &highmh->mh_Node);
#endif

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

    /* The following is a typical AROS bootup sequence */
    InitCode(RTF_SINGLETASK, 0);	/* Initialize early modules. This includes hostlib.resource. */
    core_Start(hostlib);		/* Got hostlib.resource. Initialize our interrupt mechanism. */
    InitCode(RTF_COLDSTART, 0);		/* Boot!						     */

    /* If we returned here, something went wrong, and dos.library failed to take over */
    krnPanic(getKernelBase(), "Failed to start up the system");

    HostIFace->hostlib_Close(hostlib, NULL);
    AROS_HOST_BARRIER

    return 1;
}
