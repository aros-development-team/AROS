#include <inttypes.h>
#include <utility/tagitem.h>
#include <exec/resident.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/kernel.h>

#include <asm/cpu.h>
#include <asm/segments.h>

#include <hardware/intbits.h>
#include <hardware/custom.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <string.h>
#include <stdio.h>

#include "../bootstrap/multiboot.h"
#include "../kernel/kernel_intern.h"

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

#undef AROS_MUNGWALL_DEBUG
#define P(x) ((void *)(x))

#define __text      __attribute__((section(".text")))
#define __no_ret    __attribute__((noreturn))
#define __packed    __attribute__((packed))

extern CONST_APTR Exec_FuncTable[];
void exec_DefaultTaskExit();/*??todo*/
extern ULONG Exec_MakeFunctions(APTR, APTR, APTR, APTR);
IPTR **exec_RomTagScanner(struct TagItem *msg);
UBYTE core_APICGetTotal();
UBYTE core_APICGetNumber();

AROS_UFP5(void, SoftIntDispatch,
          AROS_UFPA(ULONG, intReady, D1),
          AROS_UFPA(struct Custom *, custom, A0),
          AROS_UFPA(IPTR, intData, A1),
          AROS_UFPA(IPTR, intCode, A5),
          AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_UFP5S(void, IntServer,
    AROS_UFPA(ULONG, intMask, D0),
    AROS_UFPA(struct Custom *, custom, A0),
    AROS_UFPA(struct List *, intList, A1),
    AROS_UFPA(APTR, intCode, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));


struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
void krnSetTagData(Tag tagValue, intptr_t newtagValue, const struct TagItem *tagList);

/*
 * First, we will define exec.library (global) to make it usable outside this
 * file.
 */
const char exec_name[] = "exec.library";

/* Now ID string as it will be used in a minute in resident structure. */
const char exec_idstring[] = "$VER: exec 41.11 (16.12.2000)\r\n";

/* We would need also version and revision fields placed somewhere here. */
const short exec_Version = 41;
const short exec_Revision = 11;

const struct __text Resident Exec_resident =
{
    RTC_MATCHWORD,		/* Magic value used to find resident */
    &Exec_resident,		/* Points to Resident itself */
    P(&Exec_resident + 1),	/* Where could we find next Resident? */
    0,				/* There are no flags!! */
    41,				/* Version */
    NT_LIBRARY,			/* Type */
    126,			/* Very high startup priority. */
    (STRPTR)exec_name,		/* Pointer to name string */
    (STRPTR)exec_idstring,	/* Ditto */
    NULL			/* Library initializer (irrelevant for exec) */
};

extern UBYTE core_APICGetTotal();
extern UBYTE core_APICGetNumber();

/** Screen/Serial Debug **/
extern void Exec_SerialRawIOInit();
extern void Exec_SerialRawPutChar(UBYTE chr);

//void scr_RawPutChars(char *, int);
extern void vesa_init(int width, int height, int depth, void *base);

static ULONG   negsize = LIB_VECTSIZE;             /* size of vector table */
static UBYTE   apicready = 0;
/****/
    
const char exec_chipname[] = "Chip Memory";
const char exec_fastname[] = "Fast Memory";
const char exec_sysbasename[] = "SysBase";
const char exec_kernalname[] = "Kernel Memory";

/*
    The MMU pages and directories. They are stored at fixed location and may be either reused in the 
    64-bit kernel, or replaced by it. Four PDE directories (PDE2M structures) are enough to map whole
    4GB address space.
*/
static struct PML4E PML4[512] __attribute__((used,aligned(4096)));
static struct PDPE PDP[512] __attribute__((used,aligned(4096)));
static struct PDE2M PDE[4][512] __attribute__((used,aligned(4096)));

#define MCHIP (MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL | MEMF_24BITDMA)
#define MFAST (MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL)
#define exec_InsertMemory(a, b, c) InsertMemory(kernLow, kernTop, b, c, SysBase)
static void InsertMemory(
	uintptr_t kernLow,
	uintptr_t kernTop,
	uintptr_t lower,
	uintptr_t upper,
	struct ExecBase *SysBase)
{
    size_t size = upper - lower + 1;

    if (size < 0x1000)
	return;

    if (lower < kernLow && upper > kernTop)
    {
	exec_InsertMemory(msg, lower, kernLow - 1);
	exec_InsertMemory(msg, kernTop, upper);
    }
    else /* other cases are tested by caller */
    {
	if (lower < 0x01000000)
	{
	    size_t s2 = (upper < 0x01000000 ? ++upper : 0x01000000) - lower;

	    rkprintf("[exec] C %p - %p\n", P(lower), P(lower + s2 - 1));
	    AddMemList(s2, MCHIP, -10, P(lower), (STRPTR)exec_chipname);
	    lower += s2;
	    size -= s2;
	}

	if (size >= 0x1000)
	{
	    rkprintf("[exec] F %p - %p\n", P(lower), P(lower + size - 1));
	    AddMemList(size, MFAST, 0, P(lower), (STRPTR)exec_fastname);
	}
    }
}

#undef KernelBase

struct ExecBase *SysBase = 0; /* global SysBase */

struct BootMem {
    struct MemHeader header;
    union {
	struct MemChunk chunk;
	char reserved[0x2000]; /* space for ExecBase + AROSSupportBase */
    } __attribute__ ((aligned (MEMCHUNK_TOTAL))) u;
};

int exec_main(struct TagItem *msg, void *entry)
{
    struct BootMem *bm;
    struct MemHeader *mh;

    struct vbe_mode *mode;
    uintptr_t addr_lower;

    struct mb_mmap *mmap;
    uintptr_t len;
    char *cmd;
    int i;

    if (msg == (void *)-1)
    {
        /* Launched on AP */
        UBYTE _APICNO = core_APICGetNumber();
        apicready += 1;
        rkprintf("[exec] APIC No. %d Going IDLE (Halting)...\n", _APICNO);
        while (1) asm volatile("hlt");
    }

    /* Launched on BSP */
    addr_lower = krnGetTagData(KRN_MEMLower, 0, msg) * 1024;
    cmd = (char *)krnGetTagData(KRN_CmdLine, 0, msg);

    if ((mode = (struct vbe_mode *)krnGetTagData(KRN_VBEModeInfo, 0, msg)))
    {
	vesa_init(mode->x_resolution, mode->y_resolution, 
	    mode->bits_per_pixel, (void *)(uintptr_t)mode->phys_base);
    }

    clr();
    rkprintf("[exec] AROS64 - The AROS Research OS, ");
    rkprintf("64-bit version\n[exec] Compiled %s\n", __DATE__);
    CONST_APTR *fp = Exec_FuncTable;

    /* Calculate the size of the vector table */
    while (*fp++ != (APTR) -1)
	negsize += LIB_VECTSIZE;

    if (addr_lower != 0)
	bm = (struct BootMem *)addr_lower;
    else
	/* Warn that theres no lowmem pages to load execbase into? */
	bm = (struct BootMem *)0x1000;

#define BALIGN(p, x) ((void *)((uintptr_t)((char *)(p) + (x) - 1) & ~((x) - 1)))
#define BMATTR MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA | MEMF_KICK
#define BCSIZE sizeof(bm->u.reserved)
    bm = BALIGN(bm, MEMCHUNK_TOTAL);
    mh = &bm->header;

    mh->mh_Node.ln_Succ		= NULL;
    mh->mh_Node.ln_Pred		= NULL;
    mh->mh_Node.ln_Type		= NT_MEMORY;
    mh->mh_Node.ln_Name		= (char *)exec_chipname;
    mh->mh_Node.ln_Pri		= -5;
    mh->mh_Attributes		= BMATTR;
    mh->mh_First		= &bm->u.chunk;
    mh->mh_First->mc_Next	= NULL;
    mh->mh_First->mc_Bytes	= BCSIZE;
    mh->mh_Lower		= mh->mh_First;
    mh->mh_Upper		= (APTR)((char *)mh->mh_Lower + BCSIZE - 1);
    mh->mh_Free			= BCSIZE;

    /* reserve this memory space */
    addr_lower = (uintptr_t)(bm + 1);

    rkprintf("[exec] chunk	    = %p\n", &bm->u.chunk);
    rkprintf("[exec] addr_lower	    = %p\n", P(addr_lower));
    rkprintf("[exec] mh->mh_Free    = %u\n", mh->mh_Free);

    rkprintf("[exec] Preparing ExecBase...\n");
    SysBase = PrepareExecBase(mh, cmd, NULL);
    rkprintf("[exec] SysBase 0x%p\n", SysBase);
    rkprintf("[exec] mh->mh_Free    = %u\n", mh->mh_Free);

    /* Store sysbase in TLS */
    TLS_SET(SysBase, SysBase);

    uintptr_t kernLow = krnGetTagData(KRN_KernelLowest, 0, msg);
    uintptr_t kernTop = krnGetTagData(KRN_KernelHighest, 0, msg);

    rkprintf("[exec] Adding memory ..\n");

    if ((len = krnGetTagData(KRN_MMAPLength, 0, msg)))
    {
	rkprintf("[exec] Registering MMAP regions (len = %lu)\n", len);
	rkprintf("[exec] K %p - %p\n", P(kernLow), P(kernTop - 1));
	mmap = (struct mb_mmap *)(krnGetTagData(KRN_MMAPAddress, 0, msg));

	while (len >= sizeof(struct mb_mmap))
	{
	    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
	    {
		uintptr_t lower = mmap->addr;
		uintptr_t upper = lower + mmap->len - 1;

		if (addr_lower > lower)
		    lower = addr_lower;

		if (lower >= kernLow && lower < kernTop)
		    lower = kernTop;

		if (upper >= kernLow && upper < kernTop)
		    upper = kernLow;

		exec_InsertMemory(msg, lower, upper);
	    }

	    len -= mmap->size + 4;
	    mmap = (struct mb_mmap *)(mmap->size + (IPTR)mmap + 4);
	}
    }
    else
    {
	uintptr_t addr_upper = krnGetTagData(KRN_MEMUpper, 0, msg);

	rkprintf("[exec] Registering mem_lower/mem_upper Memory Region\n");

	if (addr_lower > 0)
	{
	    uintptr_t chip_start = 0X2000;
	    uintptr_t chip_end = (addr_lower * 1024) - 1;

	    if (chip_start < chip_end)
	    {
		rkprintf("[exec]   Registering Lower Mem Range (%p - %p)\n",
			P(chip_start), P(chip_end));
		exec_InsertMemory(msg, chip_start, chip_end);
	    }
	}

	if (addr_upper > 0)
	{
	    uintptr_t fast_start = 0x0000100000;
	    uintptr_t fast_end = (addr_upper * 1024) + (fast_start - 1);

	    if (fast_start < fast_end)
	    {
		if (fast_end > 0x01000000)
		{
		    rkprintf("[exec]   Registering Upper Chip Mem Range "
			    "(%p - %p)\n", P(fast_start), P(0x00ffffff));
		    exec_InsertMemory(msg, fast_start, 0x00ffffff);
		    rkprintf("[exec]   Registering Upper Fast Mem Range "
			    "(%p - %p)\n", P(0x01000000), P(fast_end));
		    exec_InsertMemory(msg, 0x01000000, fast_end);
		}
		else
		{
		    rkprintf("[exec]   Registering Upper Mem Range "
			    "(%p - %p)\n", P(fast_start), P(fast_end));
		    exec_InsertMemory(msg, fast_start, fast_end);
		}
	    }
	}
    }

    SumLibrary((struct Library *)SysBase);
    SysBase->LibNode.lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
    Enqueue(&SysBase->LibList,&SysBase->LibNode.lib_Node);
    rkprintf("[exec] SysBase Enqueued in Exec Liblist\n");

    for (i = 0; i < 16; ++i)
    {
	if( (1 << i) & (INTF_PORTS|INTF_COPER|INTF_VERTB|INTF_EXTER|INTF_SETCLR))
	{
	    struct Interrupt *is;
	    struct SoftIntList *sil;
	    
	    is = AllocMem
	    (
		sizeof(struct Interrupt) + sizeof(struct SoftIntList),
		MEMF_CLEAR | MEMF_PUBLIC
	    );

	    if ( is == NULL )
	    {
		rkprintf("[exec] ERROR: Cannot install Interrupt Servers!\n");
	    }
	    
	    sil = (struct SoftIntList *)(is + 1);
	    is->is_Code = &IntServer;
	    is->is_Data = sil;
	    NEWLIST((struct List *)sil);
	    SetIntVector(i,is);
	}
	else
	{
	    struct Interrupt *is;

	    switch (i)
	    {
		case INTB_SOFTINT :
		    is = AllocMem
		    (
			sizeof(struct Interrupt),
			MEMF_CLEAR | MEMF_PUBLIC
		    );
		    if (is == NULL)
		    {
			rkprintf("[exec] Error: Cannot install Interrupt Servers!\n");
			// Alert(AT_DeadEnd | AN_IntrMem);
		    }
		    is->is_Node.ln_Type = NT_SOFTINT;   //INTERRUPT;
		    is->is_Node.ln_Pri = 0;
		    is->is_Node.ln_Name = "SW Interrupt Dispatcher";
		    is->is_Data = NULL;
		    is->is_Code = (void *)SoftIntDispatch;
		    SetIntVector(i,is);
		    break;
	    }
	}
    }

    /* Enable interrupts and set int disable level to -1 */
    asm("sti");
    SysBase->TDNestCnt = -1;
    SysBase->IDNestCnt = -1;

    /* Now it's time to calculate exec checksum. It will be used
     * in future to distinguish whether we'd had proper execBase
     * before restart */
    {
	UWORD sum=0, *ptr = &SysBase->SoftVer;
	int i=((IPTR)&SysBase->IntVects[0] - (IPTR)&SysBase->SoftVer) / 2,
	    j;

	/* Calculate sum for every static part from SoftVer to ChkSum */
	for (j=0;j < i;j++)
	{
	    sum+=*(ptr++);
	}

	SysBase->ChkSum = ~sum;
    }

    rkprintf("[exec] Registering Special Regions in MemList\n");
    if ((mh = AllocMem(sizeof(*mh) + sizeof(struct MemChunk), MEMF_CLEAR)))
    {
	mh->mh_Node.ln_Type=NT_MEMORY;
	mh->mh_Node.ln_Pri = -10;
	mh->mh_Node.ln_Name = (char *)exec_sysbasename;
	mh->mh_Attributes = (MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL | MEMF_24BITDMA);
	mh->mh_First = (struct MemChunk *)(mh + sizeof(struct MemHeader));/*??bug*/
	mh->mh_First->mc_Next = NULL;
	mh->mh_First->mc_Bytes =  (negsize + sizeof(struct IntExecBase));
	mh->mh_Lower = (struct MemChunk *)(SysBase - negsize);
	mh->mh_Upper = (APTR)((UBYTE *)mh->mh_Lower + mh->mh_First->mc_Bytes);
	mh->mh_Free = 0; /* All used! */

	Forbid();
	Enqueue(&SysBase->MemList, &mh->mh_Node);
	Permit();
    }

    if ((mh = AllocMem(sizeof(struct MemHeader) + sizeof(struct MemChunk), MEMF_CLEAR)) != NULL)
    {
	uintptr_t kernLow = krnGetTagData(KRN_KernelLowest, 0, msg);
	uintptr_t kernHigh = krnGetTagData(KRN_KernelHighest, 0, msg);

	mh->mh_Node.ln_Type=NT_MEMORY;
	mh->mh_Node.ln_Pri = -10;
	mh->mh_Node.ln_Name = (char *)exec_kernalname;
	mh->mh_Attributes = (MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL);
	mh->mh_First = (struct MemChunk *)(mh + 1);
	mh->mh_First->mc_Next = NULL;
	mh->mh_First->mc_Bytes =  kernHigh - kernLow + 1;
	mh->mh_Lower = (APTR)kernLow;
	mh->mh_Upper = (APTR)kernHigh;
	mh->mh_Free = 0; /* All used! */

	Forbid();
	Enqueue(&SysBase->MemList, &mh->mh_Node);
	Permit();
    }

    rkprintf("[exec] Creating the very first task...\n");

    /* Create boot task.  Sigh, we actually create a Process sized Task,
	since DOS needs to call things which think it has a Process and
	we don't want to overwrite memory with something strange do we?

	We do this until at least we can boot dos more cleanly.
    */
    {
	struct Task    *t;
	struct MemList *ml;

	ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);
	t  = (struct Task *)   AllocMem(sizeof(struct Process), MEMF_PUBLIC|MEMF_CLEAR);

	if( !ml || !t )
	{
	    rkprintf("[exec] ERROR: Cannot create Boot Task!\n");
	}
	ml->ml_NumEntries = 1;
	ml->ml_ME[0].me_Addr = t;
	ml->ml_ME[0].me_Length = sizeof(struct Process);

	NEWLIST(&t->tc_MemEntry);
	NEWLIST(&((struct Process *)t)->pr_MsgPort.mp_MsgList);

	/* It's the boot process that RunCommand()s the boot shell, so we
	   must have this list initialized */
	NEWLIST((struct List *)&((struct Process *)t)->pr_LocalVars);

	AddHead(&t->tc_MemEntry, &ml->ml_Node);

	t->tc_Node.ln_Name = (char *)exec_name;
	t->tc_Node.ln_Pri = 0;
	t->tc_Node.ln_Type = NT_TASK;
	t->tc_State = TS_RUN;
	t->tc_SigAlloc = 0xFFFF;
	t->tc_SPLower = 0;          /* This is the system's stack */
	t->tc_SPUpper = (APTR)~0UL;
	t->tc_Flags |= TF_ETASK;

	if (t->tc_Flags & TF_ETASK)
	{
	    t->tc_UnionETask.tc_ETask = AllocVec
	    (
		sizeof(struct IntETask), 
		MEMF_ANY|MEMF_CLEAR
	    );

	    if (!t->tc_UnionETask.tc_ETask)
	    {
		rkprintf("[exec] Not enough memory for first task\n");
	    }

	    /* Initialise the ETask data. */
	    InitETask(t, t->tc_UnionETask.tc_ETask);

	    GetIntETask(t)->iet_Context = AllocTaskMem(t
		, SIZEOF_ALL_REGISTERS
		, MEMF_PUBLIC|MEMF_CLEAR
	    );

	    if (GetIntETask(t)->iet_Context == NULL)
		rkprintf("[exec] Not enough memory for first task\n");
	}

	SysBase->ThisTask = t;
    }

    rkprintf("[exec] Done. SysBase->ThisTask = 0x%p\n", SysBase->ThisTask);

    rkprintf("[exec] Leaving supervisor mode\n");
    asm volatile (
	    "mov %[user_ds],%%ds\n\t"    // Load DS and ES
	    "mov %[user_ds],%%es\n\t"
	    "mov %%rsp,%%r12\n\t"
	    "pushq %[ds]\n\t"      // SS
	    "pushq %%r12\n\t"            // rSP        
	    "pushq $0x3002\n\t"         // rFLANGS
	    "pushq %[cs]\n\t"      // CS
	    "pushq $1f\n\t iretq\n 1:"
	    ::[user_ds]"r"(USER_DS),[ds]"i"(USER_DS),[cs]"i"(USER_CS):"r12");
    rkprintf("[exec] Done?! Still here?\n");

    SysBase->TDNestCnt++;

    Permit();

    struct TagItem *tag = krnFindTagItem(KRN_CmdLine, msg);
    BOOL _debug = FALSE;

    if (tag)
    {
	STRPTR cmd = stpblk((char *)tag->ti_Data);
	ULONG temp;

	while (cmd[0])
	{
	    /* Split the command line */
	    temp = strcspn(cmd," ");

	    if (strncmp(cmd, "DEBUG", 5)==0)
	    {
		_debug = TRUE;
		SetFunction(&SysBase->LibNode, -84 * LIB_VECTSIZE,
			AROS_SLIB_ENTRY(SerialRawIOInit, Exec));
		SetFunction(&SysBase->LibNode, -86 * LIB_VECTSIZE,
			AROS_SLIB_ENTRY(SerialRawPutChar, Exec));
		break;
	    }

	    cmd = stpblk(cmd + temp);
	}
    }

    /* Scan for valid RomTags */
    SysBase->ResModules = exec_RomTagScanner(msg);
    /*??sum*/
    apicready = 1;

    rkprintf("[exec] InitCode(RTF_SINGLETASK)\n");
    if (!_debug)
	rkprintf("\3");

    InitCode(RTF_SINGLETASK, 0);
    PrivExecBase(SysBase)->KernelBase = TLS_GET(KernelBase);
    PrivExecBase(SysBase)->PageSize = MEMCHUNK_TOTAL;

    UBYTE apictotal;

    if ((apictotal = core_APICGetTotal()) > 1)
    {
	rkprintf("[exec] Waiting for %d APICs to initialise ..\n", apictotal-1);
	while (apicready < apictotal)
	{
	    rkprintf("[exec] %d of %d APICs Ready ..\n", apicready, apictotal);
	}
    }

    rkprintf("[exec] InitCode(RTF_COLDSTART)\n");
    InitCode(RTF_COLDSTART, 0);

    rkprintf("[exec] ERROR: System Boot Failed? Halting ...\n");
    while (1) asm volatile("hlt");

    return 0;
}


void exec_DefaultTaskExit()
{
    struct ExecBase *SysBase = TLS_GET(SysBase); //*(struct ExecBase **)4UL;
    RemTask(SysBase->ThisTask);
}


AROS_LH1(struct ExecBase *, open,
         AROS_LHA(ULONG, version, D0),
         struct ExecBase *, SysBase, 1, Exec)
         {
    AROS_LIBFUNC_INIT

    /* I have one more opener. */
    SysBase->LibNode.lib_OpenCnt++;
    return SysBase;

    AROS_LIBFUNC_EXIT
         }

AROS_LH0(BPTR, close,
         struct ExecBase *, SysBase, 2, Exec)
         {
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    SysBase->LibNode.lib_OpenCnt--;
    return 0;
    AROS_LIBFUNC_EXIT
         }

AROS_LH0I(int, null,
          struct ExecBase *, SysBase, 4, Exec)
          {
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
          }

/*
 * RomTag scanner.
 *
 * This function scans kernel for existing Resident modules. If two modules
 * with the same name are found, the one with higher version or priority wins.
 *
 * After building list of kernel modules, the KickTagPtr and KickMemPtr are
 * checksummed. If checksum is proper and all memory pointed in KickMemPtr may
 * be allocated, then all modules from KickTagPtr are added to RT list
 *
 * Afterwards the proper RomTagList is created (see InitCode() for details) and
 * memory after list and nodes is freed.
 */

struct rt_node
{
    struct Node     node;
    struct Resident *module;
};

IPTR **exec_RomTagScanner(struct TagItem *msg)
{
    struct ExecBase *SysBase = TLS_GET(SysBase); //*(struct ExecBase **)4UL;

    struct List     rtList;             /* List of modules */
    UWORD           *ptr = (UWORD*)krnGetTagData(KRN_KernelLowest, 0, msg);  /* Start looking here */
    UWORD           *maxptr = (UWORD*)krnGetTagData(KRN_KernelHighest, 0, msg);
    struct Resident *res;               /* module found */

    int     i;
    IPTR   **RomTag;

    /* Initialize list */
    NEWLIST(&rtList);

    rkprintf("[exec] Resident modules (addr: pri version name):\n");

    /* Look in whole kernel for resident modules */
    do
    {
        /* Do we have RTC_MATCHWORD? */
        if (*ptr == RTC_MATCHWORD)
        {
            /* Yes, assume we have Resident */
            res = (struct Resident *)ptr;

            /* Does rt_MatchTag point to Resident? */
            if (res == res->rt_MatchTag)
            {
                /* Yes, it is Resident module */
                struct rt_node  *node;

                /* Check if there is module with such name already */
                node = (struct rt_node*)FindName(&rtList, res->rt_Name);
                if (node)
                {
                    /* Yes, there was such module. It it had lower pri then replace it */
                    if (node->node.ln_Pri <= res->rt_Pri)
                    {
                        /* If they have the same Pri but new one has higher Version, replace */
                        if ((node->node.ln_Pri == res->rt_Pri) &&
                            (node->module->rt_Version < res->rt_Version))
                        {
                            node->node.ln_Pri   = res->rt_Pri;
                            node->module        = res;
                        }
                    }
                }
                else
                {
                    /* New module. Allocate some memory for it */
                    node = (struct rt_node *)
                        AllocMem(sizeof(struct rt_node),MEMF_PUBLIC|MEMF_CLEAR);

                    if (node)
                    {
                        node->node.ln_Name  = (char *)res->rt_Name;
                        node->node.ln_Pri   = res->rt_Pri;
                        node->module        = res;

                        Enqueue(&rtList,(struct Node*)node);
                    }
                }

                ptr += sizeof(struct Resident) / sizeof(UWORD);
                continue;
            }
        }

        /* Get next address... */
        ptr++;
    } while (ptr < maxptr);

    /*
     * By now we have valid (and sorted) list of kernel resident modules.
     *
     * Now, we will have to analyze used-defined RomTags (via KickTagPtr and
     * KickMemPtr)
     */
#warning "TODO: Implement external modules!"
    /*
     * Everything is done now. Allocate buffer for normal RomTag and convert
     * list to RomTag
     */

    ListLength(&rtList,i);      /* Get length of the list */

    RomTag = AllocMem((i+1)*sizeof(IPTR),MEMF_PUBLIC | MEMF_CLEAR);

    if (RomTag)
    {
        int             j;
        struct rt_node  *n;

        for (j=0; j<i; j++)
        {
            n = (struct rt_node *)RemHead(&rtList);
            rkprintf("[exec] + 0x%p: %4d %3d \"%s\"\n",
                P(n->module),
                n->node.ln_Pri,
                n->module->rt_Version,
                n->node.ln_Name);
            RomTag[j] = (IPTR*)n->module;
            FreeMem(n, sizeof(struct rt_node));
        }
        RomTag[i] = 0;
    }

    return RomTag;
}

/* IntServer:
    This interrupt handler will send an interrupt to a series of queued
    interrupt servers. Servers should return D0 != 0 (Z clear) if they
    believe the interrupt was for them, and no further interrupts will
    be called. This will only check the value in D0 for non-m68k systems,
    however it SHOULD check the Z-flag on 68k systems.

    Hmm, in that case I would have to separate it from this file in order
    to replace it...
*/
AROS_UFH5S(void, IntServer,
    AROS_UFHA(ULONG, intMask, D0),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(struct List *, intList, A1),
    AROS_UFHA(APTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt * irq;

    ForeachNode(intList, irq)
    {
        if( AROS_UFC4(int, irq->is_Code,
                AROS_UFCA(struct Custom *, custom, A0),
                AROS_UFCA(APTR, irq->is_Data, A1),
                AROS_UFCA(APTR, irq->is_Code, A5),
                AROS_UFCA(struct ExecBase *, SysBase, A6)
        ))
            break;
    }

    AROS_USERFUNC_EXIT
}



