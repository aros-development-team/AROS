#define DEBUG 1

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <inttypes.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include "memory.h"
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <strings.h>

#include <proto/exec.h>

#include LC_LIBDEFS_FILE
#include <kernel_romtags.h>
#include "kernel_intern.h"

/* forward declarations */
static void __attribute__((used)) kernel_cstart(struct TagItem *msg);

extern void exec_main(struct TagItem *msg, void *entry);

/* A very very very.....
 * ... very ugly code.
 * 
 * The AROS kernel gets executed at this place. The stack is unknown here, might be
 * set properly up, might be totally broken aswell and thus one cannot trust the contents
 * of %r1 register. Even worse, the kernel has been relocated most likely to some virtual 
 * address and the MMU mapping might not be ready now.
 * 
 * The strategy is to create one MMU entry first, mapping first 16MB of ram into last 16MB 
 * of address space in one turn and then making proper MMU map once the bss sections are cleared
 * and the startup routine in C is executed. This "trick" assumes two evil things:
 * - the kernel will be loaded (and will fit completely) within first 16MB of RAM, and
 * - the kernel will be mapped into top (last 16MB) of memory.
 * 
 * Yes, I'm evil ;) 
 */ 
 
asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,@function\n"
    "start:\n\t"
    "mr %r29,%r3\n\t"           /* Don't forget the message */
    "lis %r5,0x4152\n\t"        /* Load AROS magic value    */
    "ori %r5,%r5,0x4f53\n\t"
    "cmpw %r5,%r4\n\t"          /* Check if it was passed as 2nd parameter */
    "bnelr\n\t"                 /* No, then return to caller */
    "lis %r9,0xff00\n\t"        /* Virtual address 0xff000000 */
    "li %r10,0\n\t"             /* Physical address 0x00000000 */
    "ori %r9,%r9,0x0270\n\t"    /* 16MB page. Valid one */
    "li %r11,0x043f\n\t"        /* Write through cache. RWX enabled :) */
    "li %r0,0\n\t"              /* TLB entry number 0 */
    "tlbwe %r9,%r0,0\n\t"
    "tlbwe %r10,%r0,1\n\t"
    "tlbwe %r11,%r0,2\n\t"
    "isync\n\t"                         /* Invalidate shadow TLB's */
    "li %r9,0; mttbl %r9; mttbu %r9; mttbl %r9\n\t"
    "lis %r9,tmp_stack_end@ha\n\t"      /* Use temporary stack while clearing BSS */
    "lwz %r1,tmp_stack_end@l(%r9)\n\t"
    "bl __clear_bss_tagged\n\t"                /* Clear 'em ALL!!! */
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

static void __attribute__((used)) __clear_bss_tagged(struct TagItem *msg) 
{
    struct KernelBSS *bss = (struct KernelBSS *) krnGetTagData(KRN_KernelBss, 0, msg);

    __clear_bss(bss);
}

static union {
    struct TagItem bootup_tags[64];
    uint32_t       tmp_stack  [128];
} tmp_struct                                   __attribute__((used, section(".data"), aligned(16)));
static const uint32_t *tmp_stack_end           __attribute__((used, section(".text"))) = &tmp_struct.tmp_stack[124];
static uint32_t        stack[STACK_SIZE]       __attribute__((used, aligned(16)));
static const uint32_t *stack_end               __attribute__((used, section(".text"))) = &stack[STACK_SIZE-4];
static const void     *target_address          __attribute__((used, section(".text"))) = (void*)kernel_cstart;
static char            CmdLine[200]            __attribute__((used));

struct MinList *modlist;
uintptr_t	memlo;
struct ExecBase *SysBase;

static uint32_t exec_SelectMbs440(uint32_t bcr)
{
    uint32_t val;

    wrdcr(SDRAM0_CFGADDR, bcr);
    val = rddcr(SDRAM0_CFGDATA);

    switch (val & SDRAM_SDSZ_MASK)
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

static uint32_t exec_SelectMbs460(uint32_t val)
{
    D(bug("[%s] DCR 0x%08x\n", __func__, val));
    switch (val & MQ0_BASSZ_MASK)
    {
        case MQ0_BASSZ_4096MB: return 4096;
        case MQ0_BASSZ_2048MB: return 2048;
        case MQ0_BASSZ_1024MB: return 1024;
        case  MQ0_BASSZ_512MB: return  512;
        case  MQ0_BASSZ_256MB: return  256;
        case  MQ0_BASSZ_128MB: return  128;
        case   MQ0_BASSZ_64MB: return   64;
        case   MQ0_BASSZ_32MB: return   32;
        case   MQ0_BASSZ_16MB: return   16;
        case    MQ0_BASSZ_8MB: return    8;
    }

    return 0;
}

/* Detect and report amount of available memory in mega bytes via device control register bus */
static uint32_t exec_GetMemory()
{
    uint32_t mem = 0;
    uint32_t pvr = rdspr(PVR);

    if (krnIsPPC440(pvr)) {
        mem += exec_SelectMbs440(SDRAM0_B0CR);
        mem += exec_SelectMbs440(SDRAM0_B1CR);
        mem += exec_SelectMbs440(SDRAM0_B2CR);
        mem += exec_SelectMbs440(SDRAM0_B3CR);
    } else if (krnIsPPC460(pvr)) {
        /* Hmm. Probably a 460EX */
        mem += exec_SelectMbs460(rddcr(MQ0_B0BAS));
        mem += exec_SelectMbs460(rddcr(MQ0_B1BAS));
        mem += exec_SelectMbs460(rddcr(MQ0_B2BAS));
        mem += exec_SelectMbs460(rddcr(MQ0_B3BAS));
    } else {
        bug("Memory: Unrecognized PVR model 0x%08x\n", pvr);
        bug("Memory: Assuming you have 64M of RAM\n");
        mem = 64;
    }

    return mem;
}

void exec_main(struct TagItem *msg, void *entry)
{
    UWORD *memrange[3];
    uint32_t mem, krnLowest, krnHighest, memLowest;
    struct MemHeader *mh;

    D(bug("[exec] AROS for Sam440 - The AROS Research OS\n"));

    /* Prepare the exec base */
    D(bug("[exec] Preparing the ExecBase...\n"));

    /* Get the kernel memory locations */
    krnLowest = krnGetTagData(KRN_KernelLowest, 0, msg);
    krnHighest = krnGetTagData(KRN_KernelHighest, 0, msg);

    memLowest = (krnHighest + 0xffff) & 0xffff0000;

    D(bug("[exec] Kernel and romtags:  @%p - %p\n", krnLowest, krnHighest));
    D(bug("[exec] Create memory header @%p - %p\n", memLowest, 0x01000000-1));
    krnCreateMemHeader("RAM", -10, (APTR)(memLowest), 0x01000000 - (IPTR)memLowest,
               MEMF_CHIP | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL | MEMF_24BITDMA);

    memrange[0] = (UWORD *)(krnLowest + 0xff000000);
    memrange[1] = (UWORD *)(krnHighest + 0xff000000);
    memrange[2] = (UWORD *)-1;

    mh = (struct MemHeader *)memLowest;

    D(bug("[exec] Prepare exec base in %p\n", mh));
    krnPrepareExecBase(memrange, mh, msg);
    D(bug("[exec] ExecBase at %08x\n", SysBase));

    /* Set up %r2 to point to SysBase
     * For now, this is only used by the mmu_handler
     * to patch up the damage that Parthenope does
     * when it loads modules, and assumes that
     * &SysBase == (void **)4;
     */
    asm volatile ("mr %%r2, %0\n" :: "r"(SysBase));

    mem = exec_GetMemory();
    D(bug("[exec] Adding memory (%uM)\n", mem));

    AddMemList((mem - 16) * 1024*1024,
               MEMF_FAST | MEMF_PUBLIC | MEMF_KICK | MEMF_LOCAL,
               0,
               (APTR)0x01000000,
               "Fast RAM");

    D(bug("[exec] InitCode(RTF_SINGLETASK)\n"));
    InitCode(RTF_SINGLETASK, 0);

    D(bug("[exec] InitCode(RTF_COLDSTART)\n"));
    InitCode(RTF_COLDSTART, 0);

    D(bug("[exec] I should never get here...\n"));
}


static void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
    struct TagItem *tmp = tmp_struct.bootup_tags;
    int modlength;

    /* Lowest usable kernel memory */
    memlo = 0xff000000;

    /* Disable interrupts and let FPU work */
    wrmsr((rdmsr() & ~(MSR_CE | MSR_EE | MSR_ME)) | MSR_FP);

    /* Enable FPU */
    wrspr(CCR0, rdspr(CCR0) & ~0x00100000);
    wrspr(CCR1, rdspr(CCR1) | (0x80000000 >> 24));

    /* First message after FPU is enabled, otherwise illegal instruction */
    D(bug("[KRN] Sam440 Kernel built on %s\n", __DATE__));

    wrspr(SPRG0, 0);
    wrspr(SPRG1, 0);
    wrspr(SPRG2, 0);
    wrspr(SPRG3, 0);
    wrspr(SPRG4, 0);    /* Clear KernelBase */
    wrspr(SPRG5, 0);    /* Clear SysBase */

    D(bug("[KRN] Kernel resource pre-exec init\n"));
    D(bug("[KRN] MSR=%08x CRR0=%08x CRR1=%08x\n", rdmsr(), rdspr(CCR0), rdspr(CCR1)));
    D(bug("[KRN] USB config %08x\n", rddcr(SDR0_USB0)));

    D(bug("[KRN] msg @ %p\n", msg));
    D(bug("[KRN] Copying msg data\n"));
    while(msg->ti_Tag != TAG_DONE)
    {
        *tmp = *msg;

        if (tmp->ti_Tag == KRN_CmdLine)
        {
            strcpy(CmdLine, (char*) msg->ti_Data);
            tmp->ti_Data = (STACKIPTR) CmdLine;
            D(bug("[KRN] CmdLine: %s\n", tmp->ti_Data));
        }
        else if (tmp->ti_Tag == KRN_BootLoader)
        {
            tmp->ti_Data = (STACKIPTR) memlo;
            strcpy((char*)tmp->ti_Data, (const char*) msg->ti_Data);
            memlo += (strlen((char *)memlo) + 4) & ~3;
            D(bug("[KRN] BootLoader: %s\n", tmp->ti_Data));
        }
        else if (tmp->ti_Tag == KRN_DebugInfo)
        {
            int i;
            struct MinList *mlist = (struct MinList *)tmp->ti_Data;
            module_t *mod;

            D(bug("[KRN] DebugInfo at %08x\n", mlist));

            modlist = (struct MinList *)memlo;
            memlo += sizeof(*modlist);
            NEWLIST(modlist);

            mod = (module_t *)memlo;

            ListLength(mlist, modlength);
            memlo = (uintptr_t)&mod[modlength];

            D(bug("[KRN] Bootstrap loaded debug info for %d modules\n", modlength));
            /* Copy the module entries */
            for (i=0; i < modlength; i++)
            {
                module_t *m = (module_t *)REMHEAD(mlist);
                symbol_t *sym;
                ULONG str_l = ~0, str_h = 0;

                D(bug("[KRN] Module %s\n", m->m_name));

                /* Discover size of the string table */
                ForeachNode(&m->m_symbols, sym) {
                    ULONG end = (ULONG)sym->s_name + strlen(sym->s_name) + 1 + 1;
                    if ((ULONG)sym->s_name < str_l)
                        str_l = (ULONG)sym->s_name;
                    if (end > str_h)
                        str_h = end;
                }

                if (str_l > str_h)
                    str_h = str_l = 0;

                mod[i].m_lowest = m->m_lowest;
                mod[i].m_highest = m->m_highest;
                mod[i].m_str = (char *)memlo;
                memcpy(mod[i].m_str, (char *)str_l, str_h - str_l);
                memlo += ((str_h - str_l) + 3) & ~3;
                mod[i].m_name = (char *)memlo;
                memlo += (strlen(m->m_name) + 4) & ~3;
                strcpy(mod[i].m_name, m->m_name);

                NEWLIST(&mod[i].m_symbols);

                ForeachNode(&m->m_symbols, sym)
                {
                    symbol_t *newsym = (symbol_t *)memlo;
                    memlo += sizeof(symbol_t);

                    newsym->s_name = (IPTR)sym->s_name - str_l + mod[i].m_str;
                    newsym->s_lowest = sym->s_lowest;
                    newsym->s_highest = sym->s_highest;

                    ADDTAIL(&mod[i].m_symbols, newsym);
                }

                ADDTAIL(modlist, &mod[i]);
            }

            D(bug("[KRN] Debug info uses %d KB of memory\n", ((intptr_t)memlo - (intptr_t)modlist) >> 10));
            D(bug("[KRN] Debug info relocated from %p to %p\n", tmp->ti_Data, modlist));

            /* Ugh. Parthenope doesn't use the ELF_ModuleInfo format.
             * Prevent debug.library from becoming confused.
             */
            tmp->ti_Data = 0;
        }

        ++tmp;
        ++msg;
    }

    memlo = (memlo + 4095) & ~4095;

    BootMsg = tmp_struct.bootup_tags;
    D(bug("[KRN] BootMsg @ %p\n", BootMsg));

    /* Do a slightly more sophisticated MMU map */
    mmu_init(BootMsg);
    intr_init();

    /* Initialize exec.library */
    exec_main(BootMsg, NULL);

    bug("[KRN] Uhm? Nothing to do?\n[KRN] STOPPED\n");

    /* 
     * Do never ever try to return. This code would attempt to go back to the physical address
     * of asm trampoline, not the virtual one!
     */
    for (;;);
}

void SetupClocking440(struct PlatformData *pd)
{
    /* PLL divisors */
    wrdcr(CPR0_CFGADDR, CPR0_PLLD0);
    uint32_t reg = rddcr(CPR0_CFGDATA);

    uint32_t fbdv = (reg >> 24) & 0x1f;
    if (fbdv == 0)
        fbdv = 32;
    uint32_t fwdva = (reg >> 16) & 0x1f;
    if (fwdva == 0)
        fwdva = 16;
    uint32_t fwdvb = (reg >> 8) & 7;
    if (fwdvb == 0)
        fwdvb = 8;
    uint32_t lfbdv = reg & 0x3f;
    if (lfbdv == 0)
        lfbdv = 64;

    /* OPB clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_OPBD0);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t opbdv0 = (reg >> 24) & 3;
    if (opbdv0 == 0)
        opbdv0 = 4;

    /* Peripheral clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_PERD0);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t perdv0 = (reg >> 24) & 7;
    if (perdv0 == 0)
        perdv0 = 8;

    /* PCI clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_SPCID);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t spcid0 = (reg >> 24) & 3;
    if (spcid0 == 0)
        spcid0 = 4;

    /* Primary B divisor */
    wrdcr(CPR0_CFGADDR, CPR0_PRIMBD0);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t prbdv0 = (reg >> 24) & 7;
    if (prbdv0 == 0)
        prbdv0 = 8;

    /* All divisors there. Read PLL control register and calculate the m value (see 44ep.book) */
    wrdcr(CPR0_CFGADDR, CPR0_PLLC0);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t m;
    switch ((reg >> 24) & 3) /* Feedback selector */
    {
    case 0:/* PLL output (A or B) */
        if ((reg & 0x20000000)) /* PLLOUTB */
            m = lfbdv * fbdv * fwdvb;
        else
            m = lfbdv * fbdv * fwdva;
        break;
    case 1: /* CPU */
        m = fbdv * fwdva;
    default:
        m = perdv0 * opbdv0 * fwdvb;
    }

    uint32_t vco = (m * 66666666) + m/2;
    pd->pd_CPUFreq = vco / fwdva;
    pd->pd_PLBFreq = vco / fwdvb / perdv0;
    pd->pd_OPBFreq = pd->pd_PLBFreq / opbdv0;
    pd->pd_EPBFreq = pd->pd_PLBFreq / perdv0;
    pd->pd_PCIFreq = pd->pd_PLBFreq / spcid0;

    /*
     * Slow down the decrement interrupt a bit. Rough guess is that UBoot has left us with
     * 1kHz DEC counter. Enable decrementer timer and automatic reload of decrementer value.
     */
    wrspr(DECAR, pd->pd_OPBFreq / 50);
    wrspr(TCR, rdspr(TCR) | TCR_DIE | TCR_ARE);
}


const uint8_t fbdv_map[256] = {
      1, 123, 117, 251, 245,  69, 111, 125,
    119,  95, 105, 197, 239, 163,  63, 253,
    247, 187,  57, 223, 233, 207, 157,  71,
    113,  15,  89,  37, 191,  19,  99, 127,
    121, 109,  93,  61, 185, 155,  13,  97,
    107,  11,   9,  81,  31,  49,  83, 199,
    241,  33, 181, 143, 217, 173,  51, 165,
     65,  85, 151, 147, 227,  41, 201, 255,
    249, 243, 195, 237, 221, 231,  35, 189,
     59, 183,  79,  29, 141, 215, 145, 225,
    235, 219,  27, 139, 137, 135, 175, 209,
    159,  53,  45, 177, 211,  23, 167,  73,
    115,  67, 103, 161,  55, 205,  87,  17,
     91, 153,   7,  47, 179, 171, 149,  39,
    193, 229,  77, 213,  25, 133,  43,  21,
    101, 203,   5, 169,  75, 131,   3, 129,
      1, 250, 244, 124, 118, 196, 238, 252,
    246, 222, 232,  70, 112,  36, 190, 126,
    120,  60, 184,  96, 106,  80,  30, 198,
    240, 142, 216, 164,  64, 146, 226, 254,
    248, 236, 220, 188,  58,  28, 140, 224,
    234, 138, 136, 208, 158, 176, 210,  72,
    114, 160,  54,  16,  90,  46, 178,  38,
    192, 212,  24,  20, 100, 168,  74, 128,
    122, 116,  68, 110,  94, 104, 162,  62,
    186,  56, 206, 156,  14,  88,  18,  98,
    108,  92, 154,  12,  10,   8,  48,  82,
     32, 180, 172,  50,  84, 150,  40, 200,
    242, 194, 230,  34, 182,  78, 214, 144,
    218,  26, 134, 174,  52,  44,  22, 166,
     66, 102, 204,  86, 152,   6, 170, 148,
    228,  76, 132,  42, 202,   4, 130,   2,
};

static const uint8_t fwdv_map[16] = {
    1,   2,  14,   9,   4,  11,  16,  13,
    12,   5,   6,  15,  10,   7,   8,   3,
};

void SetupClocking460(struct PlatformData *pd)
{
    uint32_t reg;

    /* PLL divisors */
    wrdcr(CPR0_CFGADDR, CPR0_PLLD);
    reg = rddcr(CPR0_CFGDATA);

    uint32_t fbdv = fbdv_map[(reg >> 24) & 0xff];
    uint32_t fwdva = fwdv_map[((reg >> 16) & 0xf)];
    uint32_t fwdvb = fwdv_map[(reg >> 8) & 0xf];
    (void)fwdvb; // Unused

    /* Early PLL divisor */
    wrdcr(CPR0_CFGADDR, CPR0_PLBED);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t plbed = (reg >> 24) & 0x7;
    if (plbed == 0)
        plbed = 8;

    /* OPB clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_OPBD);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t opbd = (reg >> 24) & 3;
    if (opbd == 0)
        opbd = 4;

    /* Peripheral clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_PERD);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t perd = (reg >> 24) & 3;
    if (perd == 0)
        perd = 4;

    /* AHB clock divisor */
    wrdcr(CPR0_CFGADDR, CPR0_AHBD);
    reg = rddcr(CPR0_CFGDATA);
    uint32_t ahbd = (reg >> 24) & 1;
    if (ahbd == 0)
        ahbd = 2;

    /* All divisors there.
     * Read PLL control register and calculate the m value
     */
    wrdcr(CPR0_CFGADDR, CPR0_PLLC);
    reg = rddcr(CPR0_CFGDATA);

    uint32_t m;
    if (((reg >> 24) & 3) == 0) {
        /* PLL internal feedback */
        m = fbdv;
    } else {
        /* PLL Per-Clock feedback */
        m = fwdva * plbed * opbd * ahbd;
    }

    D(bug("fbdv %d, fwdva = %d, fwdvb = %d\n", fbdv, fwdva, fwdvb));
    D(bug("plbed %d, opbd = %d, perd = %d, ahbd = %d\n",
                plbed, opbd, perd, ahbd));

    uint64_t vco = m * 55000000;
    pd->pd_CPUFreq = vco / fwdva;
    pd->pd_PLBFreq = vco / fwdva / plbed;
    pd->pd_OPBFreq = pd->pd_PLBFreq / opbd;
    pd->pd_EPBFreq = pd->pd_OPBFreq / perd;
    pd->pd_PCIFreq = pd->pd_PLBFreq / ahbd;

    /*
     * Slow down the decrement interrupt a bit. Rough guess is that UBoot has left us with
     * 1kHz DEC counter. Enable decrementer timer and automatic reload of decrementer value.
     */
    wrspr(DECAR, pd->pd_OPBFreq / 50);
    wrspr(TCR, rdspr(TCR) | TCR_DIE | TCR_ARE);
    wrspr(CCR1, rdspr(CCR1) & ~(0x80000000 >> 24));
}

static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    struct PlatformData *pd;

    uintptr_t krn_lowest  = krnGetTagData(KRN_KernelLowest,  0, BootMsg);
    uintptr_t krn_highest = krnGetTagData(KRN_KernelHighest, 0, BootMsg);

    D(bug("[KRN] Entered Kernel_Init()\n"));
    /* Get the PLB and CPU speed */

    pd = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pd)
        return FALSE;

    LIBBASE->kb_PlatformData = pd;

    /* Stash the PVR value */
    pd->pd_PVR = rdspr(PVR);
    D(bug("[KRN] PVR: 0x%08x\n", pd->pd_PVR));

    if (krnIsPPC440(pd->pd_PVR)) {
        SetupClocking440(pd);
    } else if (krnIsPPC460(pd->pd_PVR)) {
        SetupClocking460(pd);
    } else {
        bug("kernel.resource: Unknown PVR model 0x%08x\n", pd->pd_PVR);
        for (;;);
    }

    D(bug("[KRN] CPU Speed: %dMz\n", LIBBASE->kb_PlatformData->pd_CPUFreq / 1000000));
    D(bug("[KRN] PLB Speed: %dMz\n", LIBBASE->kb_PlatformData->pd_PLBFreq / 1000000));
    D(bug("[KRN] OPB Speed: %dMz\n", LIBBASE->kb_PlatformData->pd_OPBFreq / 1000000));
    D(bug("[KRN] EPB Speed: %dMz\n", LIBBASE->kb_PlatformData->pd_EPBFreq / 1000000));
    D(bug("[KRN] PCI Speed: %dMz\n", LIBBASE->kb_PlatformData->pd_PCIFreq / 1000000));

    /* 4K granularity for data sections */
    krn_lowest &= 0xfffff000;
    /* 64K granularity for code sections */
    krn_highest = (krn_highest + 0xffff) & 0xffff0000;

    D(bug("[KRN] Allowing userspace to flush caches\n"));
    wrspr(MMUCR, rdspr(MMUCR) & ~0x000c0000);

    for (i=0; i < 16; i++)
        NEWLIST(&LIBBASE->kb_Exceptions[i]);

    for (i=0; i < 64; i++)
        NEWLIST(&LIBBASE->kb_Interrupts[i]);

    LIBBASE->kb_ContextSize = sizeof(context_t);

    D(bug("[KRN] Setting DebugInfo to %p\n", modlist));
    LIBBASE->kb_PlatformData->pd_DebugInfo = (APTR)modlist;

    D(bug("[KRN] Preparing kernel private memory\n"));

    /*
     * Add MemHeader about kernel memory to public MemList to avoid invalid
     * pointer debug messages for pointer that reference correctly into these
     * mem regions.
    */
    krnCreateMemHeader("Kernel Memory", -10, (APTR)memlo, krn_lowest - ((uintptr_t)memlo & 0x00ffffff), MEMF_FAST | MEMF_KICK | MEMF_LOCAL | MEMF_24BITDMA);
    krnCreateROMHeader("Kernel Reserved",
            (APTR)0xff000000, (APTR)(memlo - 1));
    krnCreateROMHeader("Kernel Code + Data Sections",
            (APTR) ((uintptr_t) 0xff000000 + krn_lowest - 1),
            (APTR) ((uintptr_t) 0xff000000 + krn_highest - 1));
    
    /* 
     * kernel.resource is ready to run, leave supervisor mode. External interrupts
     * will be enabled during late exec init.
     */

    wrmsr(rdmsr() | MSR_PR);
    D(bug("[KRN] Entered user mode \n"));

    return TRUE;
}

ADD2INITLIB(Kernel_Init, 0)
