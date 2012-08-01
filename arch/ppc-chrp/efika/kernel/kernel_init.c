/*
 * kernel_init.c
 *
 *  Created on: Aug 19, 2008
 *      Author: misc
 */

#define DEBUG 1

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <asm/mpc5200b.h>
#include <proto/kernel.h>
#include <strings.h>

#include <proto/exec.h>

#include "exec_intern.h"
#include "etask.h"

#include "kernel_intern.h"
#include LC_LIBDEFS_FILE
#include "syscall.h"
#include "memory.h"

static void __attribute__((used)) kernel_cstart(struct TagItem *msg);
struct OFWNode *krnCopyOFWTree(struct OFWNode *orig);
int exec_main(struct TagItem *msg, void *entry);

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,@function\n"
    "start:\n\t"

    "mr %r29,%r3\n\t"	/* Keep it for later */

	"li %r0,0x2000\n\t"
	"mtmsr %r0\n\t"		/* Disable address translation */
	"sync; isync\n\t"

/* Set up BAT registers with a very simple map. */
/* IBAT0 virtual 0xff800000 into physical 0x07800000, size 8MB, read only, super only, cached */
	"lis %r0, 0xff80; "
	"ori %r0, %r0, 0x2 | 0xfc; "
	"mtspr 528,%r0; "		/* Clear all BAT entries */
	"lis %r0, 0x0780;"
	"ori %r0,%r0,0x01 | 0x10; "
	"mtspr 529, %r0;"

/* IBAT1 virtual 0x07800000 into physical 0x07800000, size 8MB, read only, super only, cached */
	"lis %r0, 0x0780; "
	"ori %r0, %r0, 0x2 | 0xfc; "
	"mtspr 530,%r0; "		/* Clear all BAT entries */
	"lis %r0, 0x0780;"
	"ori %r0,%r0,0x01 | 0x10; "
	"mtspr 531, %r0;"

/* Clear IBAT1 - IBAT7 */
	"lis %r0, 0; "
	"mtspr 532, %r0;"
	"mtspr 533, %r0;"
	"mtspr 534, %r0;"
	"mtspr 535, %r0;"

	"mtspr 560, %r0;"
	"mtspr 561, %r0;"
	"mtspr 562, %r0;"
	"mtspr 563, %r0;"
	"mtspr 564, %r0;"
	"mtspr 565, %r0;"
	"mtspr 566, %r0;"
	"mtspr 567, %r0;"

/* DBAT0 virtual 0xff000000, physical 0x07000000, size 8MB, read write, super only, cached */
	"lis %r0, 0xff00; "
	"ori %r0, %r0, 0x2 | 0xfc; "
	"mtspr 536,%r0; "		/* Clear all BAT entries */
	"lis %r0, 0x0700;"
	"ori %r0,%r0,0x02 | 0x10; "
	"mtspr 537, %r0;"
/* DBAT1 virtual 0xff800000, physical 0x07800000, size 8MB, read only, super only, cached */
	"lis %r0, 0xff80; "
	"ori %r0, %r0, 0x2 | 0xfc; "
	"mtspr 538,%r0; "		/* Clear all BAT entries */
	"lis %r0, 0x0780;"
	"ori %r0,%r0,0x01 | 0x10; "
	"mtspr 539, %r0;"
/* DBAT2 virtual 0x00000000, physical 0x00000000, 128MB, read write, super only, cached */
	"lis %r0, 0x0000; "
	"ori %r0, %r0, 0x2 | 0x1ffc; "
	"mtspr 540,%r0; "		/* Clear all BAT entries */
	"lis %r0, 0x0000;"
	"ori %r0,%r0,0x02 | 0x10; "
	"mtspr 541, %r0;"
/* DBAT3 virtual 0xf0000000, physical 0xf0000000, 128KB, read write, super only, not cached */
	"lis %r0, 0xf000; "
	"ori %r0, %r0, 0x3; "
	"mtspr 542,%r0; "
	"lis %r0, 0xf000;"
	"ori %r0,%r0,0x0a | 0x20; "
	"mtspr 543, %r0;"

/* Clear DBAT3 - DBAT7 */
	"lis %r0, 0; "

	"mtspr 568, %r0;"
	"mtspr 569, %r0;"
	"mtspr 570, %r0;"
	"mtspr 571, %r0;"
	"mtspr 572, %r0;"
	"mtspr 573, %r0;"
	"mtspr 574, %r0;"
	"mtspr 575, %r0;"

	"mttbl %r0; mttbu %r0; mttbl %r0;"

	/* Enable address translation again. All accesses will be done through BAT registers */
	"li %r0, 0x2030;"
	"mtmsr %r0;sync;isync;"

	"lis %r9,tmp_stack_end@ha\n\t"      /* Use temporary stack while clearing BSS */
	"lwz %r1,tmp_stack_end@l(%r9)\n\t"
	"mr %r3,%r29\n\t"
	"bl __clear_bss\n\t"                /* Clear BSS */

	"lis %r11,target_address@ha\n\t"    /* Load the address of init code in C */
	"mr %r3,%r29\n\t"                   /* restore the message */
	"lwz %r11,target_address@l(%r11)\n\t"
	"lis %r9,stack_end@ha\n\t"          /* Use brand new stack to do evil things */
	"mtctr %r11\n\t"
	"lwz %r1,stack_end@l(%r9)\n\t"

	"bctr\n\t"                          /* And start the game... */

	".string \"Native/CORE v3 (" __DATE__ ")\""
	"\n\t.text\n\t"
);

static void __attribute__((used)) __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss;

    bss =(struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);

    if (bss)
    {
        while (bss->addr && bss->len)
        {
            memset(bss->addr, 0, bss->len);
            bss++;
        }
    }
}

static __attribute__((used,section(".data"),aligned(16))) union {
    struct TagItem bootup_tags[64];
    uint32_t  tmp_stack[128];
} tmp_struct;
static const uint32_t *tmp_stack_end __attribute__((used, section(".text"))) = &tmp_struct.tmp_stack[124];
static uint32_t stack[STACK_SIZE] __attribute__((used,aligned(16)));
static uint32_t stack_super[STACK_SIZE] __attribute__((used,aligned(16)));
static const uint32_t *stack_end __attribute__((used, section(".text"))) = &stack[STACK_SIZE-4];
static const void *target_address __attribute__((used, section(".text"))) = (void*)kernel_cstart;
static uint8_t *mmu_dir;
static uint32_t *MBAR;

/* Variables which should go to the .data section */
static struct OFWNode *ofw_root_node __attribute__((used,section(".data"))) = NULL;
struct TagItem *BootMsg __attribute__((used,section(".data"))) = NULL;
static uint8_t *memlo __attribute__((used,section(".data"))) = NULL;

module_t *modlist;
uint32_t modlength;

static void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
	D(bug("[KRN] EFika5200B Kernel build on %s\n", __DATE__));
	struct TagItem *tmp = tmp_struct.bootup_tags;
	struct TagItem *src = msg;
	struct OFWNode *node;

	wrspr(SPRG0, (uint32_t)&stack_super[STACK_SIZE-4]);
	wrspr(SPRG4, 0);
	wrspr(SPRG5, 0);
	wrmsr(rdmsr() | MSR_FP);

	/* MMU directory - 1MB is recommended size for systems with 128MB ram
	 * USE PHYSICAL ADDRESS HERE! */
	mmu_dir = (uint8_t *)0x07000000;

	D(bug("[KRN] BootMsg @ %p\n", msg));

	if (msg == BootMsg)
	{
		D(bug("[KRN] Reboot detected\n"));
	}
	else
	{
		/* Set the memlo pointer right after MMU dir */
		memlo = 0xff100000;

		D(bug("[KRN] Copying TagList and data\n"));

		while(msg->ti_Tag != TAG_DONE)
		{
			*tmp = *msg;

			if (tmp->ti_Tag == KRN_CmdLine)
			{
				tmp->ti_Data = (intptr_t)memlo;
				memlo += (strlen(msg->ti_Data) + 4) & ~3;
				memcpy(tmp->ti_Data, msg->ti_Data, strlen(msg->ti_Data)+1);
				D(bug("[KRN] CmdLine: '%s'\n", tmp->ti_Data));
			}
			else if (tmp->ti_Tag == KRN_KernelBss)
			{
				struct KernelBSS *bss_in, *bss_out;

				bss_out = (struct KernelBSS *)memlo;
				bss_in = msg->ti_Data;

				tmp->ti_Data = (intptr_t)bss_out;

				while(bss_in->addr && bss_in->len)
				{
					*bss_out++ = *bss_in++;
				}
				*bss_out++ = *bss_in++;

				memlo = (uint8_t *)bss_out;
			}
			else if (tmp->ti_Tag == KRN_OpenFirmwareTree)
			{
				ofw_root_node = krnCopyOFWTree((struct OFWNode *)msg->ti_Data);
				tmp->ti_Data = (intptr_t)ofw_root_node;
			}
			else if (tmp->ti_Tag == KRN_DebugInfo)
			{
				int i;
				struct MinList *mlist = tmp->ti_Data;

				D(bug("[KRN] DebugInfo at %08x\n", modlist));

				module_t *mod = (module_t *)memlo;

				ListLength(mlist, modlength);
				modlist = mod;

				memlo = &mod[modlength];

				D(bug("[KRN] Bootstrap loaded debug info for %d modules\n", modlength));

				/* Copy the module entries */
				for (i=0; i < modlength; i++)
				{
					module_t *m = REMHEAD(mlist);
					symbol_t *sym;

					mod[i].m_lowest = m->m_lowest;
					mod[i].m_highest = m->m_highest;
					mod[i].m_str = NULL;
					NEWLIST(&mod[i].m_symbols);
					mod[i].m_name = (char *)memlo;
					memlo += (strlen(m->m_name) + 4) & ~3;
					strcpy(mod[i].m_name, m->m_name);

					ForeachNode(&m->m_symbols, sym)
					{
						symbol_t *newsym = memlo;
						memlo += sizeof(symbol_t);

						newsym->s_name = memlo;
						memlo += (strlen(sym->s_name)+4)&~3;
						strcpy(newsym->s_name, sym->s_name);

						newsym->s_lowest = sym->s_lowest;
						newsym->s_highest = sym->s_highest;

						ADDTAIL(&mod[i].m_symbols, newsym);
					}
				}

				D(bug("[KRN] Debug info uses %d KB of memory\n", ((intptr_t)memlo - (intptr_t)mod) >> 10));
			}

			tmp++;
			msg++;
		}
		*tmp = *msg;

		memlo = (char *)(((intptr_t)memlo + 4095) & ~4095);

		BootMsg = tmp_struct.bootup_tags;
	}

	intr_init();
	mmu_init(mmu_dir, 0x100000);

	/*
	 * Find the "/builtin" node in ofw tree and read it's reg property. It is
	 * the only way to obtain the value of MBAR register (that is, location of
	 * the CPU in-chip peripherials
	 */
	ForeachNode(&ofw_root_node->on_children, node)
	{
		if (!strcmp(node->on_name, "builtin"))
			break;
	}

	if (node)
	{
		struct OFWProperty *prop = NULL;

		ForeachNode(&node->on_properties, prop)
		{
			if (!strcmp(prop->op_name,"reg"))
				break;
		}

		if (prop)
		{
			MBAR = *(uint32_t**)prop->op_value;

			D(bug("[KRN] MBAR at %08x\n", MBAR));
		}
	}

	/* Enable dynamic power management */
	wrspr(1008, 0x0010c000);
	D(bug("[KRN] HID0=%08x HID1=%08x\n", rdspr(1008), rdspr(1009)));

	ictl_init(MBAR);

	D(bug("[KRN] Mapping 00003000-06ffffff range for public use\n"));
	mmu_map_area(0x00003000, 0x00003000, 0x06ffd000, (2 << 3) | 2); // PP = 10, WIMG = 0010

	D(bug("[KRN] Mapping %08x-%08x range for MBAR\n", MBAR, MBAR + 0xc000 - 1));
	mmu_map_area((uint64_t)MBAR & 0xffffffff, (uint64_t)MBAR & 0xffffffff, 0x0000c000, (5 << 3) | 2); // PP = 10, WIMG = 0101

	D(bug("[KRN] Mapping %08x-%08x range for public read-only access\n", 0xff100000, memlo - 1));
	mmu_map_area(0xff100000, 0x07100000, (intptr_t)memlo - 0xff100000, (2 << 3) | 1); // PP = 01, WIMG = 0010

	uintptr_t krn_base = krnGetTagData(KRN_KernelBase, 0, BootMsg);
	uintptr_t krn_lowest = krnGetTagData(KRN_KernelLowest, 0, BootMsg) & ~4095;
	uintptr_t krn_highest = (krnGetTagData(KRN_KernelHighest, 0, BootMsg) + 4095) & ~4095;

	D(bug("[KRN] Mapping %08x-%08x range for supervisor\n", memlo, (krn_lowest ^ 0xf8000000)-1));
	mmu_map_area((uint32_t)memlo, (uint32_t)memlo ^ 0xf8000000, (krn_lowest ^ 0xf8000000) - (intptr_t)memlo, (2 << 3) | 0); // PP = 00, WIMG = 0010

	D(bug("[KRN] Mapping %08x-%08x range for read-write\n", krn_lowest^0xf8000000, (krn_base ^ 0xf8000000)-1));
	mmu_map_area((uint32_t)krn_lowest^0xf8000000, (uint32_t)krn_lowest, (krn_base - krn_lowest), (2 << 3) | 2); // PP = 10, WIMG = 0010

	D(bug("[KRN] Mapping %08x-%08x range for read-only\n", krn_base^0xf8000000, (krn_highest ^ 0xf8000000)-1));
	mmu_map_area((uint32_t)krn_base^0xf8000000, (uint32_t)krn_base, (krn_highest - krn_base), (2 << 3) | 3); // PP = 11, WIMG = 0010

	D(bug("[KRN] Mapping %08x-%08x range for supervisor\n", krn_highest ^ 0xf8000000, 0xffffffff));
	mmu_map_area((uint32_t)krn_highest^0xf8000000, (uint32_t)krn_highest, -(krn_highest^0xf8000000), (2 << 3) | 0); // PP = 00, WIMG = 0010

    /* Prepare the MemHeader structure for this region */
	struct MemHeader *mh = (struct MemHeader *)memlo;
	mh->mh_First = (struct MemChunk *)((uint8_t *)mh + MEMHEADER_TOTAL);
	mh->mh_Free = ((krn_lowest^0xf8000000) - MEMHEADER_TOTAL - (intptr_t)memlo) & ~(MEMCHUNK_TOTAL-1);
	mh->mh_First->mc_Next = NULL;
	mh->mh_First->mc_Bytes = mh->mh_Free;

	D(bug("[KRN] Supervisor mem: %dKB free\n", mh->mh_Free >> 10));

    exec_main(BootMsg, NULL);

	goSuper();
	D(bug("[KRN] Uhm? Nothing to do?\n[KRN] STOPPED\n"));
	/* Dead end */
	while(1) {
		wrmsr(rdmsr() | MSR_EE);
		wrmsr(rdmsr() | MSR_POW);
	}
}


AROS_LH0I(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void *, KrnCreateContext,
         struct KernelBase *, KernelBase, 18, Kernel)
{
    AROS_LIBFUNC_INIT

    context_t *ctx;

    uint32_t oldmsr = goSuper();

    ctx = Allocate(KernelBase->kb_SupervisorMem, sizeof(context_t));
    memset(ctx, 0, sizeof(context_t));

    wrmsr(oldmsr);

    if (!ctx)
    	ctx = AllocMem(sizeof(context_t), MEMF_PUBLIC|MEMF_CLEAR);

    return ctx;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnDeleteContext,
		AROS_LHA(void *, context, A0),
         struct KernelBase *, KernelBase, 19, Kernel)
{
    AROS_LIBFUNC_INIT

    /* Was context in supervisor space? Deallocate it there :) */
    if ((intptr_t)context & 0xf0000000)
    {
        uint32_t oldmsr = goSuper();

        Deallocate(KernelBase->kb_SupervisorMem, context, sizeof(context_t));

        wrmsr(oldmsr);
    }
    else
    	FreeMem(context, sizeof(context_t));

    /* Was this context owning a FPU? Make FPU totally free then */
    if (KernelBase->kb_FPUOwner == context)
    	KernelBase->kb_FPUOwner = NULL;

    AROS_LIBFUNC_EXIT
}


struct OFWNode *krnCopyOFWTree(struct OFWNode *orig)
{
	uint32_t node_length = sizeof(struct OFWNode) + strlen(orig->on_name) + 1;
	struct OFWNode *new_node = (struct OFWNode *)memlo;
	struct OFWNode *child;
	struct OFWProperty *prop, *new_prop;
	memlo += (node_length + 3) & ~3;

	new_node->on_name = new_node->on_storage;
	memcpy(new_node->on_name, orig->on_name, strlen(orig->on_name) + 1);
	NEWLIST(&new_node->on_children);
	NEWLIST(&new_node->on_properties);

	ForeachNode(&orig->on_properties, prop)
	{
		uint32_t prop_name_length = strlen(prop->op_name) + 1;
		uint32_t prop_length = sizeof(struct OFWProperty) + prop_name_length + prop->op_length;

		new_prop = (struct OFWProperty *)memlo;
		memlo += (prop_length + 3) & ~3;

		new_prop->op_name = new_prop->op_storage;
		new_prop->op_value = &new_prop->op_storage[prop_name_length];
		new_prop->op_length = prop->op_length;

		memcpy(new_prop->op_name, prop->op_name, strlen(prop->op_name)+1);
		memcpy(new_prop->op_value, prop->op_value, prop->op_length);

		ADDTAIL(&new_node->on_properties, new_prop);
	}

	ForeachNode(&orig->on_children, child)
		ADDTAIL(&new_node->on_children, krnCopyOFWTree(child));

	return new_node;
}

uint64_t tbu1;
uint64_t tbu2;
uint64_t last_calc;
uint64_t idle_time;
uint32_t cpu_usage;
struct Task *idle_task;

/* As soon as IdleTask gets CPU again, it records the current cycle count */
AROS_UFH1(void, idleCount,
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    tbu1 = mftbu();
    SysBase->IdleCount++;

    AROS_USERFUNC_EXIT
}

/*
 * When CPU time gets lost, the cycle count spent in idle task is summed
 * with the total idle time
 */
AROS_UFH1(void, idleOff,
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    tbu2 = mftbu();
    idle_time += tbu2 - tbu1;

    AROS_USERFUNC_EXIT
}

/*
 * Idle task on Efika does not do much. It enters supervisor mode, enables
 * interrupts (so that it survived even in Disable()'d state) and then enters
 * power save mode. Once woken up, a Reschedule is forced.
 */
void idleTask(struct ExecBase *SysBase, struct KernelBase *KernelBase)
{
	D(bug("[KRN] IdleTask entered\n"));

	while(1)
	{
	    if (0)
	    {
		/* Superstate() done quickly */
		int i;
		uint32_t msr = goSuper();
		uint32_t msr_sup = rdmsr() | MSR_EE | MSR_POW;
		/* Enable external/decrementer interrupts */
		asm volatile("sync");
		/* Sleep! */
		wrmsr(msr_sup);
		/* Changing the POW bit needs a context synchronization now. */
		asm volatile("isync");

		/* CPU needs few cycles to enter sleep state */
		for (i=0; i < 10; i++)
		    asm volatile("nop");

		/* Userstate() */
		wrmsr(msr);
		/* And reschedule! */
		KrnSchedule();
	    }
	}
}

struct MemHeader KernelMemory;

void *(*__AllocMem)();

#define ExecAllocMem(bytesize, requirements)        \
AROS_CALL2(void *, __AllocMem,                      \
		AROS_LCA(ULONG, byteSize,     D0),			\
		AROS_LCA(ULONG, requirements, D1),			\
		struct ExecBase *, SysBase)

AROS_LH2(APTR, AllocMem,
	AROS_LHA(ULONG, byteSize,     D0),
	AROS_LHA(ULONG, requirements, D1),
	struct ExecBase *, SysBase, 33, Kernel)
{
	AROS_LIBFUNC_INIT

	return ExecAllocMem(bytesize, requirements & ~MEMF_CHIP);

	AROS_LIBFUNC_EXIT
}


static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    struct ExecBase *SysBase = getSysBase();
    int realchip = 0;

	uintptr_t krn_lowest = (krnGetTagData(KRN_KernelLowest, 0, BootMsg) & ~4095) ^ 0xf8000000;
	uintptr_t krn_highest = ((krnGetTagData(KRN_KernelHighest, 0, BootMsg) + 4095) & ~4095) ^ 0xf8000000;
	char *cmd = krnGetTagData(KRN_CmdLine, 0, BootMsg);

    KernelMemory.mh_Node.ln_Name = "Kernel";
    KernelMemory.mh_Node.ln_Type = NT_MEMORY;
    KernelMemory.mh_Node.ln_Pri = -128;
    KernelMemory.mh_Attributes = MEMF_KICK;
    KernelMemory.mh_First = NULL;
    KernelMemory.mh_Free = 0;
    KernelMemory.mh_Lower = krn_lowest;
    KernelMemory.mh_Upper = krn_highest;

    Forbid();
	Enqueue(&SysBase->MemList, &KernelMemory.mh_Node);
    Permit();

    NEWLIST(&LIBBASE->kb_Modules);
    NEWLIST(&LIBBASE->kb_DeadTasks);

    /* Check whether the user passed realchip parameter. */
    if (cmd)
    {
    	while(cmd[0])
    	{
    		uint32_t temp = strcspn(cmd, " ");

    		if (strncmp(cmd, "realchip", 8) == 0)
    		{
    			realchip=1;
    		}

    		cmd = stpblk(cmd+temp);
    	}
    }

    /*
     * If no realchip parameter was given, the AllocMem function will be patched with a stub
     * which *always* clears the MEMF_CHIP flag.
     */
    if (!realchip)
    {
    	__AllocMem = SetFunction(SysBase, -33*LIB_VECTSIZE, AROS_SLIB_ENTRY(AllocMem, Kernel, 33));
    }

    /*
     * Set the KernelBase into SPRG4. At this stage the SPRG5 should be already set by
     * exec.library itself.
     */
    wrspr(SPRG4, LIBBASE);

    LIBBASE->kb_FPUOwner = NULL;

    D(bug("[KRN] Kernel resource post-exec init.\n"));

    for (i=0; i < 21; i++)
        NEWLIST(&LIBBASE->kb_Exceptions[i]);

    for (i=0; i < 64; i++)
        NEWLIST(&LIBBASE->kb_Interrupts[i]);

    /* Prepare private memory block */
    LIBBASE->kb_SupervisorMem = (struct MemHeader *)memlo;

	/*
     * kernel.resource is ready to run. Enable external interrupts and leave
     * supervisor mode
     */
    wrmsr(rdmsr() | (MSR_EE|MSR_FP));
    D(bug("[KRN] Interrupts enabled\n"));

    asm volatile("mtdec %0"::"r"(33000000/1000));

    wrmsr(rdmsr() | (MSR_PR));
    D(bug("[KRN] Entered user mode\n"));

    {
    	/* Add idle task now. */
    	struct Task *t;
    	struct MemList *ml;
    	uint8_t *s;
    	struct TagItem tags[] = {
    			{ TASKTAG_ARG1,   (IPTR)SysBase },
    			{ TASKTAG_ARG2,   (IPTR)LIBBASE },
    			{ TAG_DONE,       0UL }};

    	/* Allocate MemEntry for this task and stack */
    	ml = (struct MemList *)AllocMem(sizeof(struct MemList)+sizeof(struct MemEntry),
    			MEMF_PUBLIC|MEMF_CLEAR);
    	t = (struct Task *)    AllocMem(sizeof(struct Task), MEMF_CLEAR|MEMF_PUBLIC);
    	s = (uint8_t *)          AllocMem(4096,      MEMF_CLEAR|MEMF_PUBLIC);

    	if( !ml || !t || !s )
    	{
    		bug("ERROR: Cannot create Idle Task!\n");
    		Alert( AT_DeadEnd | AG_NoMemory | AN_ExecLib );
    	}

    	ml->ml_NumEntries = 2;
    	ml->ml_ME[0].me_Addr = t;
    	ml->ml_ME[0].me_Length = sizeof(struct Task);
    	ml->ml_ME[1].me_Addr = s;
    	ml->ml_ME[1].me_Length = 4096;

    	NEWLIST(&t->tc_MemEntry);
    	AddHead(&t->tc_MemEntry, &ml->ml_Node);
    	t->tc_SPLower = s;
    	t->tc_SPUpper = s + 4096 - SP_OFFSET;
#if AROS_STACK_GROWS_DOWNWARDS
    	t->tc_SPReg = (char *)t->tc_SPUpper - SP_OFFSET;
#else
    	t->tc_SPReg = (char *)t->tc_SPLower + SP_OFFSET;
#endif

    	t->tc_Node.ln_Name = "Idle Task";
    	t->tc_Node.ln_Pri = -128;
    	t->tc_Launch = &idleCount;
    	t->tc_Switch = &idleOff;
    	t->tc_Flags = TF_LAUNCH | TF_SWITCH;
    	NewAddTask(t, &idleTask, NULL, &tags);
    	idle_task = t;
    }

    return TRUE;
}

ADD2INITLIB(Kernel_Init, 0)
