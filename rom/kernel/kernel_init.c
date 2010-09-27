#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_tagitems.h>

#define D(x)

/* Some globals we can't live without */
struct TagItem *BootMsg = NULL;
struct KernelBase *KernelBase = NULL;
#if AROS_MODULES_DEBUG
static struct MinList *Debug_ModList = NULL;
#endif

void __clear_bss(struct KernelBSS *bss)
{
    while (bss->addr) {
	bzero((void*)bss->addr, bss->len);
        bss++;
    }
}

static int Kernel_Init(struct KernelBase *kBase)
{
    int i;
    char *args;

    KernelBase = kBase;
    D(bug("[KRN] Kernel_Init(0x%p)\n", KernelBase));

    for (i=0; i < EXCEPTIONS_COUNT; i++)
	NEWLIST(&KernelBase->kb_Exceptions[i]);

    for (i=0; i < IRQ_COUNT; i++)
        NEWLIST(&KernelBase->kb_Interrupts[i]);

    NEWLIST(&KernelBase->kb_Modules);
#if AROS_MODULES_DEBUG
    Debug_ModList = &KernelBase->kb_Modules;
#endif
    InitSemaphore(&KernelBase->kb_ModSem);

    KernelBase->kb_KernelModules = (dbg_seg_t *)krnGetTagData(KRN_DebugInfo, 0, BootMsg);
    KernelBase->kb_VBlankEnable = 1; /* VBlank is enabled by default	   */
    KernelBase->kb_VBlankTicks  = 1; /* 1 timer tick per VBlank by default */

    /* Parse startup time arguments */
    args = (char *)krnGetTagData(KRN_CmdLine, 0, BootMsg);
    if (args)
    {
	char *s;

	D(bug("[KRN] Found arguments: %s\n", args));
	s = strstr(args, "tickrate=");
	if (s)
	{
	    unsigned int v = atoi(&s[9]);

	    D(bug("[KRN] Argument: %s, Value: %u\n", s, v));
	    /*
	     * Value is given in Hz, so we divide it by VBlank frequency to get the
	     * actual multiplier. We also ensure that multiplier is integer and greater
	     * than zero.
	     */
	    v = v / SysBase->VBlankFrequency;
	    if (v)
		KernelBase->kb_VBlankTicks = v;
	}
    }

    /* Calculate fixed up value of timer frequency */
    KernelBase->kb_TimerFrequency = SysBase->VBlankFrequency * KernelBase->kb_VBlankTicks;
    /* Specify timer frequency as EClock frequency. timer.device may override it when it comes up */
    SysBase->ex_EClockFrequency = KernelBase->kb_TimerFrequency;

    D(bug("[KRN] Kernel_Init() done\n"));
    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
