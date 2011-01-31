#include <aros/arossupportbase.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/exec.h>

#include <inttypes.h>

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

static const char *kernel_functions[] = {
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
    void* _stack = AROS_GET_SP;
    struct ExecBase *SysBase;
    void *hostlib;
    char *errstr;
    unsigned int i;
    struct MemHeader *mh;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
    struct HostInterface *hif = NULL;
    void *klo = NULL;
    void *khi = NULL;
    struct mb_mmap *mmap = NULL;
    char *args = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    while ((tag = krnNextTagItem(&tstate)))
    {
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

    /* If there's no HostIFace, we can't even say anything */
    if (!hif)
	return -1;

    /* Set globals only AFTER __clear_bss() */
    BootMsg = msg;
    HostIFace = hif;

    if ((!klo) || (!khi) || (!mmap)) {
	mykprintf("[Kernel] Not enough parameters from bootstrap!\n");
	return -1;
    }

    if (strcmp(HostIFace->System, "Windows"))
    {
	bug("[Kernel] This kernel is built for Windows architecture\n");
	bug("[Kernel] Your bootstrap is running on %s which is incompatible\n", HostIFace->System);
	return -1;
    }

    if (HostIFace->Version < HOSTINTERFACE_VERSION)
    {
	mykprintf("[Kernel] Obsolete bootstrap interface (found v%u, need v%u)\n",
		  HostIFace->Version, HOSTINTERFACE_VERSION);
	return -1;
   }

    hostlib = HostIFace->hostlib_Open("Libs\\Host\\kernel.dll", &errstr);
    if (!hostlib) {
	mykprintf("[Kernel] Failed to load host-side module: %s\n", errstr);
	HostIFace->hostlib_FreeErrorStr(errstr);
	return -1;
    }

    for (i = 0; kernel_functions[i]; i++)
    {
	void *func = HostIFace->hostlib_GetPointer(hostlib, kernel_functions[i], &errstr);

        if (!func)
	{
	    mykprintf("[Kernel] Failed to find symbol %s in host-side module: %s\n", kernel_functions[i], errstr);
	    HostIFace->hostlib_FreeErrorStr(errstr);

	    return -1;
	}
	((void **)&KernelIFace)[i] = func;
    }

    /*
     * Prepare the first mem header and hand it to PrepareExecBase to take SysBase live
     * We know that memory map has only one RAM element.
     */
    mykprintf("[Kernel] preparing first mem header\n");
    mh = (struct MemHeader *)mmap->addr;
    krnCreateMemHeader("Normal RAM", -5, mh, mmap->len, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK);

    D(mykprintf("[Kernel] calling PrepareExecBase(), mh_First = 0x%p, args = %s\n", mh->mh_First, args));
    /*
     * FIXME: This routine is part of exec.library, however it doesn't have an LVO
     * (it can't have one because exec.library is not initialized yet) and is called
     * only from here. Probably the code should be reorganized and this routine needs
     * to be moved to kernel.resource
     */
    SysBase = PrepareExecBase(mh, args, HostIFace);
    D(mykprintf("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);)

    ranges[0] = klo;
    ranges[1] = khi;
    SysBase->ResModules = krnRomTagScanner(mh, ranges);

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kickstart.
     */
    krnCreateROMHeader(mh, "Kickstart ROM", klo, khi);

    /*
     * Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    krnCreateROMHeader(mh, "Boot stack", _stack - AROS_STACKSIZE, _stack);

    /* In order for these functions to work before KernelBase and ExecBase are set up */
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf  = mykprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;

    mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
    InitCode(RTF_SINGLETASK, 0);
    InitCode(RTF_COLDSTART, 0);

    mykprintf("[Kernel] leaving startup!\n");
    HostIFace->hostlib_Close(hostlib, NULL);
    return 1;
}

int Platform_Init(struct KernelBase *KernelBase)
{
    D(mykprintf("[Kernel] initializing host-side kernel module, timer frequency is %u\n", SysBase->ex_EClockFrequency));
    *KernelIFace.TrapVector = core_TrapHandler;
    *KernelIFace.IRQVector  = core_IRQHandler;

    KernelBase->kb_PageSize = KernelIFace.core_init(SysBase->ex_EClockFrequency);
    D(mykprintf("[Kernel] System page size: %u\n", KernelBase->kb_PageSize));

    /* core_init() returns 0 on failure */
    return KernelBase->kb_PageSize;
}

ADD2INITLIB(Platform_Init, 10);
