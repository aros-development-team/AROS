#define DEBUG 1

#include <asm/mpc5200b.h>
#include <aros/config.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/arossupportbase.h>
#include <aros/symbolsets.h>
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
#include "exec_intern.h"
#include "exec_util.h"
#include "intservers.h"
#include "memory.h"
#include "taskstorage.h"

D(extern void debugmem(void));

void exec_main(struct TagItem *msg, void *entry);
extern CONST_APTR Exec_FuncTable[];
extern ULONG Exec_15_MakeFunctions(APTR, CONST_APTR, CONST_APTR, struct ExecBase *);
void exec_DefaultTaskExit();
IPTR **exec_RomTagScanner(struct TagItem *msg, struct ExecBase *);
extern struct Library * PrepareAROSSupportBase (struct ExecBase *);
intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);

struct ExecBase *priv_SysBase;

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         APTR, kernelBase, 12, Kernel);
#undef bug

static inline void bug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    /* Our KrnBug() ignores base address */
    AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(format, args, NULL);
    va_end(args);
}

const char exec_name[] = "exec.library";
const char exec_idstring[] = "$VER: exec 41.11 (16.12.2000)\r\n";
const char exec_fastname[] = "System Memory";

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

THIS_PROGRAM_HANDLES_SYMBOLSETS
DEFINESET(INITLIB)

void exec_main(struct TagItem *msg, void *entry)
{
    struct ExecBase *SysBase = NULL;
    struct TaskStorageFreeSlot *tsfs;
    uintptr_t lowmem = 0;
    int i;

    priv_SysBase = NULL;

    D(bug("[exec] AROS for Efika5200B - The AROS Research OS\n"));

    /* Prepare the exec base */

    ULONG   negsize = LIB_VECTSIZE;   /* size of vector table */
    CONST_APTR *fp  = Exec_FuncTable; /* pointer to a function in the table */

    D(bug("[exec] Preparing the ExecBase...\n"));

    /* Calculate the size of the vector table */
    while (*fp++ != (APTR) -1) negsize += LIB_VECTSIZE;

    /* Align the offset for SysBase to the cache line */
    negsize = (negsize + 31) & ~31;

    /* Get the lowest usable memory location */
    lowmem = 0x3000;

    /* And now let's have the SysBase */
    SysBase = (struct ExecBase *)(lowmem + negsize);
    wrspr(SPRG5, SysBase);
    *(struct ExecBase **)4UL = SysBase;
    /* Store the SysBase for local use. It's ugly, it's dirty, it's a hack. */
    priv_SysBase = SysBase;
    lowmem = (lowmem + negsize + sizeof(struct IntExecBase) + 4095) & ~4095;

    D(bug("[exec] ExecBase at %08x\n", SysBase));

    D(bug("[exec] Clearing ExecBase\n"));

    /* How about clearing most of ExecBase structure? */
    bzero(&SysBase->IntVects[0], sizeof(struct IntExecBase) - offsetof(struct ExecBase, IntVects[0]));

    SysBase->KickMemPtr = NULL;
    SysBase->KickTagPtr = NULL;
    SysBase->KickCheckSum = NULL;

    SysBase->DispCount = 0;
    SysBase->IdleCount = 0;

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
    SysBase->Elapsed = 4;
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
        Exec_15_MakeFunctions(SysBase, Exec_FuncTable, NULL, SysBase);

    SumLibrary((struct Library *)SysBase);

    InitSemaphore(&PrivExecBase(SysBase)->MemListSem);
    InitSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    PrivExecBase(SysBase)->TaskStorageSize = TASKSTORAGEPUDDLE;
    NEWLIST(&PrivExecBase(SysBase)->TaskStorageSlots);

    D(bug("[exec] Adding memory\n"));

    AddMemList(0x07000000 - lowmem,
               MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL,
               0,
               (APTR)lowmem,
               (STRPTR)exec_fastname);

    SumLibrary((struct Library *)SysBase);

    Enqueue(&SysBase->LibList,&SysBase->LibNode.lib_Node);

    SysBase->DebugAROSBase = PrepareAROSSupportBase(SysBase);

    /* Scan for valid RomTags */
    SysBase->ResModules = exec_RomTagScanner(msg, SysBase);

    D(bug("[exec] InitCode(RTF_SINGLETASK)\n"));
    InitCode(RTF_SINGLETASK, 0);

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

    tsfs = AllocMem(sizeof(struct TaskStorageFreeSlot), MEMF_PUBLIC|MEMF_CLEAR);
    if (!tsfs)
    {
        D(bug("[exec] ERROR: Cannot create Task Storage!\n"));
    }
    tsfs->FreeSlot = 1;
    AddHead((struct List *)&PrivExecBase(SysBase)->TaskStorageSlots, (struct Node *)tsfs);

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

            GetIntETask(t)->iet_Context = KrnCreateContext();

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

    /* Call init set. This is needed at least to bring up RemTask garbage collector. */
    set_call_libfuncs(SETNAME(INITLIB), 1, 1, SysBase);

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
    struct ExecBase *SysBase = priv_SysBase;
    RemTask(SysBase->ThisTask);
}

IPTR **exec_RomTagScanner(struct TagItem *msg, struct ExecBase *SysBase)
{
    struct List     rtList;             /* List of modules */
    UWORD           *ptr = (UWORD*)(krnGetTagData(KRN_KernelLowest, 0, msg) ^ 0xf8000000);  /* Start looking here */
    UWORD           *maxptr = (UWORD*)(krnGetTagData(KRN_KernelHighest, 0, msg) ^ 0xf8000000);
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

static int __kprintf(const UBYTE *fmt, ...)
{
    va_list ap;
    int result = 0;

    va_start(ap,fmt);
    result = AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(fmt, ap, NULL);
    va_end(ap);

    return result;
}

static int __vkprintf(const UBYTE *fmt, va_list args)
{
    return AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(fmt, args, NULL);
}

static int __rkprintf(const STRPTR mainSystem, const STRPTR subSystem, int level, const UBYTE *fmt, ...)
{
    va_list ap;
    int result = 0;

    va_start(ap,fmt);
    result = AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(fmt, ap, NULL);
    va_end(ap);

    return result;
}

struct Library * PrepareAROSSupportBase(struct ExecBase *SysBase)
{
    struct AROSSupportBase *AROSSupportBase =
        AllocMem(sizeof(struct AROSSupportBase), MEMF_CLEAR);

    AROSSupportBase->kprintf = (void *)__kprintf;
    AROSSupportBase->rkprintf = (void *)__rkprintf;
    AROSSupportBase->vkprintf = (void *)__vkprintf;

    return (struct Library *)AROSSupportBase;
}

void _aros_not_implemented(char *string) {}
