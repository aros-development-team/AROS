#include <asm/amcc440.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/arossupportbase.h>
#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdarg.h>
#include <strings.h>
#include <inttypes.h>

#include "etask.h"
#include "exec_util.h"

#include "exec_intern.h"
#include "memory.h"

#undef KernelBase
#include "../kernel/kernel_intern.h"

D(extern void debugmem(void));

void exec_main(struct TagItem *msg, void *entry);
extern CONST_APTR Exec_FuncTable[];
extern ULONG Exec_MakeFunctions(APTR, CONST_APTR, CONST_APTR, struct ExecBase *);
void exec_DefaultTaskExit();
IPTR **exec_RomTagScanner(struct TagItem *msg);
extern struct Library * PrepareAROSSupportBase (void);

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

const char exec_name[] = "exec.library";
const char exec_idstring[] = "$VER: exec 41.11 (16.12.2000)\r\n";
const char exec_chipname[] = "Chip Memory";
const char exec_fastname[] = "Fast Memory";

const short exec_Version = 41;
const short exec_Revision = 11;

const struct __attribute__((section(".text"))) Resident Exec_resident =
{
        RTC_MATCHWORD,          /* Magic value used to find resident */
        &Exec_resident,         /* Points to Resident itself */
        (APTR)&Exec_resident+1, /* Where could we find next Resident? */
        0,                      /* There are no flags!! */
        41,                     /* Version */
        NT_LIBRARY,             /* Type */
        126,                    /* Very high startup priority. */
        (STRPTR)exec_name,      /* Pointer to name string */
        (STRPTR)exec_idstring,  /* Ditto */
        exec_main               /* Library initializer (for exec this value is irrelevant since we've jumped there at the begining to bring the system up */
};

static uint32_t exec_SelectMbs(uint32_t bcr)
{
    switch (bcr & SDRAM_SDSZ_MASK)
    {
        case SDRAM_SDSZ_256MB: return 256;
        case SDRAM_SDSZ_128MB: return 128;
        case  SDRAM_SDSZ_64MB: return  64;
        case  SDRAM_SDSZ_32MB: return  32;
        case  SDRAM_SDSZ_16MB: return  16;
        case   SDRAM_SDSZ_8MB: return   8;
    }

    return 0;
}

/* Detect and report amount of available memory in mega bytes via device control register bus */
static uint32_t exec_GetMemory()
{
    uint32_t mem;
    wrdcr(SDRAM0_CFGADDR, SDRAM0_B0CR);
    mem = exec_SelectMbs(rddcr(SDRAM0_CFGDATA));
    //D(bug("[exec] B0CR %08x %uM\n", rddcr(SDRAM0_CFGDATA), mem));

    wrdcr(SDRAM0_CFGADDR, SDRAM0_B1CR);
    mem += exec_SelectMbs(rddcr(SDRAM0_CFGDATA));
    //D(bug("[exec] B1CR %08x %uM\n", rddcr(SDRAM0_CFGDATA), mem));

    wrdcr(SDRAM0_CFGADDR, SDRAM0_B2CR);
    mem += exec_SelectMbs(rddcr(SDRAM0_CFGDATA));
    //D(bug("[exec] B2CR %08x %uM\n", rddcr(SDRAM0_CFGDATA), mem));

    wrdcr(SDRAM0_CFGADDR, SDRAM0_B3CR);
    mem += exec_SelectMbs(rddcr(SDRAM0_CFGDATA));
    //D(bug("[exec] B3CR %08x %uM\n", rddcr(SDRAM0_CFGDATA), mem));

    return mem;
}

void exec_main(struct TagItem *msg, void *entry)
{
    struct ExecBase *SysBase = NULL;
    uintptr_t lowmem = 0;
    uint32_t mem;
    int i;

    D(bug("[exec] AROS for Sam440 - The AROS Research OS\n"));

    /* Prepare the exec base */

    ULONG   negsize = LIB_VECTSIZE;   /* size of vector table */
    CONST_APTR *fp  = Exec_FuncTable; /* pointer to a function in the table */

    D(bug("[exec] Preparing the ExecBase...\n"));

    /* Calculate the size of the vector table */
    while (*fp++ != (APTR) -1) negsize += LIB_VECTSIZE;

    /* Align the offset for SysBase to the cache line */
    negsize = (negsize + 31) & ~31;

    /* Get the lowest usable memory location */
    lowmem = (krnGetTagData(KRN_KernelHighest, 0, msg) + 0xffff) & 0xffff0000;

    /* And now let's have the SysBase */
    SysBase = (struct ExecBase *)(lowmem + negsize);
    wrspr(SPRG5, SysBase);
    lowmem = (lowmem + negsize + sizeof(struct IntExecBase) + 4095) & ~4095;

    D(bug("[exec] ExecBase at %08x\n", SysBase));

    D(bug("[exec] Clearing ExecBase\n"));

    /* How about clearing most of ExecBase structure? */
    bzero(&SysBase->IntVects[0], sizeof(struct IntExecBase) - offsetof(struct ExecBase, IntVects[0]));

    SysBase->KickMemPtr = NULL;
    SysBase->KickTagPtr = NULL;
    SysBase->KickCheckSum = NULL;

    /*
     * Now everything is prepared to store ExecBase at the location 4UL and set
     * it complement in ExecBase structure
     */

    D(bug("[exec] Initializing library...\n"));

    SysBase->ChkBase = ~(ULONG)SysBase;

    /* Store memory configuration */
    SysBase->MaxLocMem = (IPTR)0; //locmem;
    SysBase->MaxExtMem = (APTR)0; //extmem;

    /*
     * Initialize exec lists. This is done through information table which consist
     * of offset from begining of ExecBase and type of the list.
     */
    NEWLIST(&SysBase->MemList);
    SysBase->MemList.lh_Type = NT_MEMORY;
    NEWLIST(&SysBase->ResourceList);
    SysBase->ResourceList.lh_Type = NT_RESOURCE;
    NEWLIST(&SysBase->DeviceList);
    SysBase->DeviceList.lh_Type = NT_DEVICE;
    NEWLIST(&SysBase->LibList);
    SysBase->LibList.lh_Type = NT_LIBRARY;
    NEWLIST(&SysBase->PortList);
    SysBase->PortList.lh_Type = NT_MSGPORT;
    NEWLIST(&SysBase->TaskReady);
    SysBase->TaskReady.lh_Type = NT_TASK;
    NEWLIST(&SysBase->TaskWait);
    SysBase->TaskWait.lh_Type = NT_TASK;
    NEWLIST(&SysBase->IntrList);
    SysBase->IntrList.lh_Type = NT_INTERRUPT;
    NEWLIST(&SysBase->SemaphoreList);
    SysBase->SemaphoreList.lh_Type = NT_SIGNALSEM;
    NEWLIST(&SysBase->ex_MemHandlers);

    for (i=0; i<5; i++)
    {
        NEWLIST(&SysBase->SoftInts[i].sh_List);
        SysBase->SoftInts[i].sh_List.lh_Type = NT_SOFTINT;
    }

    /*
     * Exec.library initializer. Prepares exec.library for future use. All
     * lists have to be initialized, some values from ROM are copied.
     */

    SysBase->TaskTrapCode = NULL; //exec_DefaultTrap;
    SysBase->TaskExceptCode = NULL; //exec_DefaultTrap;
    SysBase->TaskExitCode = exec_DefaultTaskExit;
    SysBase->TaskSigAlloc = 0x0000ffff;
    SysBase->TaskTrapAlloc = 0x8000;

    /* Prepare values for execBase (like name, type, pri and other) */

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri = 0;
    SysBase->LibNode.lib_Node.ln_Name = (char *)exec_name;
    SysBase->LibNode.lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
    SysBase->LibNode.lib_PosSize = sizeof(struct IntExecBase);
    SysBase->LibNode.lib_OpenCnt = 1;
    SysBase->LibNode.lib_IdString = (char *)exec_idstring;
    SysBase->LibNode.lib_Version = exec_Version;
    SysBase->LibNode.lib_Revision = exec_Revision;

    SysBase->Quantum = 4;
    SysBase->VBlankFrequency = 50;
    SysBase->PowerSupplyFrequency = 1;

    NEWLIST(&PrivExecBase(SysBase)->ResetHandlers);
    NEWLIST(&PrivExecBase(SysBase)->AllocMemList);

#if AROS_MUNGWALL_DEBUG
    /*
     * TODO: implement command line parsing instead of this awkward hack
     * Or, even better, merge this init code with arch-independent one
     */
    PrivExecBase(SysBase)->IntFlags = EXECF_MungWall;
#endif

    /* Build the jumptable */
    SysBase->LibNode.lib_NegSize =
        Exec_MakeFunctions(SysBase, Exec_FuncTable, NULL, SysBase);

    SumLibrary((struct Library *)SysBase);

    InitSemaphore(&PrivExecBase(SysBase)->MemListSem);
    InitSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    mem = exec_GetMemory();
    D(bug("[exec] Adding memory (%uM)\n", mem));

    AddMemList(0x01000000 - lowmem,
               MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL | MEMF_24BITDMA,
               -10,
               (APTR)lowmem,
               (STRPTR)exec_chipname);
    
    AddMemList((mem - 16) * 1024*1024,
               MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL,
               0,
               (APTR)0x01000000,
               (STRPTR)exec_fastname);
    
    SumLibrary((struct Library *)SysBase);

    Enqueue(&SysBase->LibList,&SysBase->LibNode.lib_Node);

    SysBase->DebugAROSBase = PrepareAROSSupportBase();
    
    /* Scan for valid RomTags */
    SysBase->ResModules = exec_RomTagScanner(msg);

    D(bug("[exec] InitCode(RTF_SINGLETASK)\n"));
    InitCode(RTF_SINGLETASK, 0);

    PrivExecBase(SysBase)->KernelBase = OpenResource("kernel.resource");
    PrivExecBase(SysBase)->PageSize   = MEMCHUNK_TOTAL;

    /* Install the interrupt servers */
    for (i=0; i<16; i++)
    {
        if( (1<<i) & (INTF_PORTS|INTF_COPER|INTF_VERTB|INTF_EXTER|INTF_SETCLR))
        {
            struct Interrupt *is;
            struct SoftIntList *sil;
            is = AllocMem
            (
                sizeof(struct Interrupt) + sizeof(struct SoftIntList),
                MEMF_CLEAR | MEMF_PUBLIC
            );
            if( is == NULL )
            {
                D(bug("[exec] ERROR: Cannot install Interrupt Servers!\n"));
            }
            sil = (struct SoftIntList *)((struct Interrupt *)is + 1);

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
                        D(bug("[exec] Error: Cannot install Interrupt Servers!\n"));
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
            D(bug("[exec] ERROR: Cannot create Boot Task!\n"));
        }
        ml->ml_NumEntries = 1;
        ml->ml_ME[0].me_Addr = t;
        ml->ml_ME[0].me_Length = sizeof(struct Process);

        NEWLIST(&t->tc_MemEntry);
        NEWLIST(&((struct Process *)t)->pr_MsgPort.mp_MsgList);

        /* It's the boot process that RunCommand()s the boot shell, so we
           must have this list initialized */
        NEWLIST((struct List *)&((struct Process *)t)->pr_LocalVars);

        AddHead(&t->tc_MemEntry,&ml->ml_Node);

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
                D(bug("[exec] Not enough memory for first task\n"));
            }

            /* Initialise the ETask data. */
            InitETask(t, t->tc_UnionETask.tc_ETask);

            GetIntETask(t)->iet_Context = AllocMem(SIZEOF_ALL_REGISTERS
                , MEMF_PUBLIC|MEMF_CLEAR
            );

            if (!GetIntETask(t)->iet_Context)
            {
                D(bug("[exec] Not enough memory for first task\n"));
            }
        }

        SysBase->ThisTask = t;
    }

    D(bug("[exec] Done. SysBase->ThisTask = %08p\n", SysBase->ThisTask));

    /* We now start up the interrupts */
    Permit();
    Enable();

    D(debugmem());

    D(bug("[exec] InitCode(RTF_COLDSTART)\n"));
    InitCode(RTF_COLDSTART, 0);

    D(bug("[exec] I should never get here...\n"));
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


void exec_DefaultTaskExit()
{
    struct ExecBase *SysBase = getSysBase();
    RemTask(SysBase->ThisTask);
}

IPTR **exec_RomTagScanner(struct TagItem *msg)
{
    struct ExecBase *SysBase = getSysBase();

    struct List     rtList;             /* List of modules */
    UWORD           *ptr = (UWORD*)(krnGetTagData(KRN_KernelLowest, 0, msg) + 0xff000000);  /* Start looking here */
    UWORD           *maxptr = (UWORD*)(krnGetTagData(KRN_KernelHighest, 0, msg) + 0xff000000);
    struct Resident *res;               /* module found */

    int     i;
    IPTR   **RomTag;

    /* Initialize list */
    NEWLIST(&rtList);

    D(bug("[exec] Resident modules (addr: pri version name):\n"));

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
                ptr+=sizeof(struct Resident)/sizeof(UWORD);
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
            D(bug("[exec] + 0x%08lx: %4d %3d \"%s\"\n",
                n->module,
                n->node.ln_Pri,
                n->module->rt_Version,
                n->node.ln_Name));
            RomTag[j] = (IPTR*)n->module;
            FreeMem(n, sizeof(struct rt_node));
        }
        RomTag[i] = 0;
    }

    return RomTag;
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
    We temporarily redefine kprintf() so we use the real version in case
    we have one of these two fn's called before AROSSupportBase is ready.
 */

#undef kprintf
#undef rkprintf
#undef vkprintf
#undef KernelBase

static int __kprintf(const UBYTE *fmt, ...)
{
    va_list ap;
    int result = 0;
    void *KernelBase = getKernelBase();
    
    if (KernelBase)
    {
        va_start(ap,fmt);
        result = KrnBug(fmt, ap);
        va_end(ap);
    }
    return result;
}

static int __vkprintf(const UBYTE *fmt, va_list args)
{
    void *KernelBase = getKernelBase();
    
    if (KernelBase)
        return KrnBug(fmt, args);
    else
        return 0;
}

static int __rkprintf(const STRPTR mainSystem, const STRPTR subSystem, int level, const UBYTE *fmt, ...)
{
    va_list ap;
    int result = 0;
    void *KernelBase = getKernelBase();
    
    if (KernelBase)
    {
        va_start(ap,fmt);
        result = KrnBug(fmt, ap);
        va_end(ap);
    }
    return result;
}

struct Library * PrepareAROSSupportBase(void)
{
    struct ExecBase *SysBase = getSysBase();

    struct AROSSupportBase *AROSSupportBase =
        AllocMem(sizeof(struct AROSSupportBase), MEMF_CLEAR);

    AROSSupportBase->kprintf = (void *)__kprintf;
    AROSSupportBase->rkprintf = (void *)__rkprintf;
    AROSSupportBase->vkprintf = (void *)__vkprintf;

    return (struct Library *)AROSSupportBase;
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

void _aros_not_implemented(char *string) {}
