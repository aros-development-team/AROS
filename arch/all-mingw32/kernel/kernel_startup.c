#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_romtags.h"
#include "kernel_mingw32.h"

#define D(x)

/* Some globals we can't live without */
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;
static const char *kernel_functions[];

/* rom startup */
int __startup startup(struct TagItem *msg, ULONG magic)
{
    void *_stack = AROS_GET_SP;
    void *hostlib;
    char *errstr;
    unsigned int i;
    struct MemHeader *mh;
    struct TagItem *tag;
    const struct TagItem *tstate = msg;
    struct HostInterface *hif = NULL;
    struct mb_mmap *mmap = NULL;
    UWORD *ranges[] = {NULL, NULL, (UWORD *)-1};

    /* Fail if we are ocassionally started from within AROS command line */
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

    /* If there's no HostIFace, we can't even say anything */
    if (!hif)
	return -1;

    /* Set globals only AFTER __clear_bss() */
    BootMsg = msg;
    HostIFace = hif;

    /* Validate our HostInterface version */
    if (strcmp(HostIFace->System, "Windows"))
	return -1;
    if (HostIFace->Version != HOSTINTERFACE_VERSION)
	return -1;

    /* Now we have debug output, we can bug() */
    hostlib = HostIFace->hostlib_Open("Libs\\Host\\kernel.dll", &errstr);
    if (!hostlib)
    {
	bug("[Kernel] Failed to load host-side module: %s\n", errstr);
	HostIFace->hostlib_FreeErrorStr(errstr);
	return -1;
    }

    for (i = 0; kernel_functions[i]; i++)
    {
	void *func = HostIFace->hostlib_GetPointer(hostlib, kernel_functions[i], &errstr);

        if (!func)
	{
	    bug("[Kernel] Failed to find symbol %s in host-side module: %s\n", kernel_functions[i], errstr);
	    HostIFace->hostlib_FreeErrorStr(errstr);
	    HostIFace->hostlib_Close(hostlib, NULL);

	    return -1;
	}
	((void **)&KernelIFace)[i] = func;
    }

    /* Now we have core_alert(), krnDisplayAlert() works */
    if ((!ranges[0]) || (!ranges[1]) || (!mmap))
    {
	krnPanic(KernelBase, "Not enough information from the bootstrap\n"
		 "Kickstart start 0x%p, end 0x%p\n"
		 "Memory map address: 0x%p",
		 ranges[0], ranges[1], mmap);
	return -1;
    }

    /*
     * Prepare the first mem header and hand it to PrepareExecBase to take SysBase live
     * We know that memory map has only one RAM element.
     */
    D(bug("[Kernel] preparing first mem header\n"));
    mh = (struct MemHeader *)mmap->addr;
    krnCreateMemHeader("Normal RAM", -5, mh, mmap->len, MEMF_CHIP|MEMF_PUBLIC|MEMF_LOCAL|MEMF_KICK);

    /*
     * TODO: this needs to be replaced by SysBase address validation.
     * This will make KickTags and capture vectors working.
     */
    SysBase = NULL;

    D(bug("[Kernel] calling krnPrepareExecBase(), mh_First = 0x%p, msg = 0x%p\n", mh->mh_First, msg));
    if (!krnPrepareExecBase(ranges, mh, msg))
	return -1;

    D(bug("[Kernel] SysBase=0x%p, mh_First=0x%p\n", SysBase, mh->mh_First);)

    /*
     * ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kickstart.
     */
    krnCreateROMHeader("Kickstart ROM", ranges[0], ranges[1]);

    /*
     * Stack memory header. This special memory header covers a little part of the programs
     * stack so that TypeOfMem() will not return 0 for addresses pointing into the stack
     * during initialization.
     */
    krnCreateROMHeader("Boot stack", _stack - AROS_STACKSIZE, _stack);

    D(bug("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n"));
    InitCode(RTF_SINGLETASK, 0);
    InitCode(RTF_COLDSTART, 0);

    krnPanic(KernelBase, "Failed to start up the system");
    HostIFace->hostlib_Close(hostlib, NULL);
    return 1;
}

/* Functions we want from Libs/Host/kernel.dll */
static const char *kernel_functions[] =
{
    "core_init",
    "core_raise",
    "core_protect",
    "core_putc",
    "core_getc",
    "core_alert",
    "TrapVector",
    "IRQVector",
    "Ints_Enabled",
    "Supervisor",
    "Sleep_Mode",
    "LastErrorPtr",
    NULL
};

static int Platform_Init(struct KernelBase *KernelBase)
{
    char *args;

    /* Our VBlank is software-driven, we can ask the user what frequency to use. Default is 50 Hz. */
    SysBase->VBlankFrequency = 50;

    args = (char *)LibGetTagData(KRN_CmdLine, 0, BootMsg);
    if (args)
    {
	char *s = strstr(args, "vblank=");

	if (s)
	    SysBase->VBlankFrequency = atoi(&s[7]);
    }

    D(bug("[Kernel] initializing host-side kernel module, timer frequency is %u\n", SysBase->VBlankFrequency));

    *KernelIFace.TrapVector = core_TrapHandler;
    *KernelIFace.IRQVector  = core_IRQHandler;

    KernelBase->kb_PageSize = KernelIFace.core_init(SysBase->VBlankFrequency);
    D(bug("[Kernel] System page size: %u\n", KernelBase->kb_PageSize));

    /* core_init() returns 0 on failure */
    return KernelBase->kb_PageSize;
}

ADD2INITLIB(Platform_Init, 10);
