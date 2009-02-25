#define DEBUG 1
#include <aros/debug.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <stdlib.h>

#include "kernel_intern.h"
#include "../bootstrap/multiboot.h"
#include LC_LIBDEFS_FILE

#define CONFIG_LAPICS

extern const unsigned long start64;
extern const void * _binary_smpbootstrap_start;
extern const unsigned long _binary_smpbootstrap_size;

/* Pre-exec init */

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start64\n\t"
    ".type start64,@function\n"
    "start64: movq tmp_stack_end(%rip),%rsp\n\t"
    "movq %rdi,%rbx\n\t"
    "call __clear_bss\n\t"
    "movq %rbx,%rdi\n\t"
    "movq stack_end(%rip), %rsp\n\t"
    "movq target_address(%rip), %rsi\n\t"
    "jmp *%rsi\n\t"
    ".string \"Native/CORE v3 (" __DATE__ ")\""
    "\n\t.text\n\t"
);

static struct   KernBootPrivate *__KernBootPrivate = NULL;

void __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss;
    bss = krnGetTagData(KRN_KernelBss, 0, msg);

    if (bss)
    {
        while (bss->addr)
        {
            bzero(bss->addr, bss->len);
            bss++;
        }
    }
}

/* Post exec init */

static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    TLS_SET(KernelBase, LIBBASE);
    struct ExecBase *SysBase = TLS_GET(SysBase);

    LIBBASE->kb_XTPIC_Mask = 0xfffb;

    for (i=0; i < 256; i++)
    {
        NEWLIST(&LIBBASE->kb_Intr[i]);
        switch(i)
        {
            case 0x20 ... 0x2f:
                LIBBASE->kb_Intr[i].lh_Type = KBL_XTPIC;
                break;
            case 0xfe:
                LIBBASE->kb_Intr[i].lh_Type = KBL_APIC;
                break;
            default:
                LIBBASE->kb_Intr[i].lh_Type = KBL_INTERNAL;
                break;
        }
    }

    D(bug("[Kernel] Kernel_Init: Post-exec init. KernelBase @ %p\n", LIBBASE));
    D(bug("[Kernel] Kernel_Init: Interupt List initialised\n"));

    LIBBASE->kb_APIC_Count = 1;

    LIBBASE->kb_APIC_DriverID = __KernBootPrivate->kbp_APIC_DriverID;
    LIBBASE->kb_APIC_Drivers = __KernBootPrivate->kbp_APIC_Drivers;

    LIBBASE->kb_APIC_IDMap = AllocVec(sizeof(UWORD), MEMF_CLEAR);
    LIBBASE->kb_APIC_BaseMap = AllocVec(sizeof(IPTR), MEMF_CLEAR);

    D(bug("[Kernel] Kernel_Init: APIC IDMap @ %p, BaseMap @ %p\n", LIBBASE->kb_APIC_IDMap, LIBBASE->kb_APIC_BaseMap));

    D(bug("[Kernel] Kernel_Init: APIC Drivers @ %p, Using No %d\n", LIBBASE->kb_APIC_Drivers, LIBBASE->kb_APIC_DriverID));

    LIBBASE->kb_APIC_IDMap[0] = __KernBootPrivate->kbp_APIC_BSPID;
    LIBBASE->kb_APIC_BaseMap[0] = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)(LIBBASE->kb_APIC_Drivers[LIBBASE->kb_APIC_DriverID])->getbase));

    LIBBASE->kb_APIC_TrampolineBase = __KernBootPrivate->kbp_APIC_TrampolineBase;

    D(bug("[Kernel] Kernel_Init: BSP APIC ID %d, Base @ %p\n", LIBBASE->kb_APIC_IDMap[0], LIBBASE->kb_APIC_BaseMap[0]));

    IPTR retval = AROS_UFC1(IPTR, ((struct GenericAPIC *)LIBBASE->kb_APIC_Drivers[LIBBASE->kb_APIC_DriverID])->init,
            AROS_UFCA(IPTR, LIBBASE->kb_APIC_BaseMap[0], A0));

    if (__KernBootPrivate->kbp_ACPIRSDP)
    {
        LIBBASE->kb_ACPIRSDP = __KernBootPrivate->kbp_ACPIRSDP;

        core_ACPIInitialise(LIBBASE);
    }

    LIBBASE->kb_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096);
    D(bug("[Kernel] Kernel_Init: MemPool @ %012p\n", LIBBASE->kb_MemPool));
}

ADD2INITLIB(Kernel_Init, 0)

static struct TagItem *BootMsg;
static struct vbe_controller vbectrl;
static struct vbe_mode vbemd;
static intptr_t addr;
static intptr_t len;

int kernel_cstart(struct TagItem *msg, void *entry)
{
    IPTR lowpages = (krnGetTagData(KRN_MEMLower, 0, msg) * 1024);
    struct TagItem *tag;
    IPTR _APICBase;
    UBYTE _APICID;
    UBYTE _EnableDebug = 0;

    /* Enable fxsave/fxrstor */ 
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    if ((__KernBootPrivate == NULL) && (lowpages > 0x2000) && ((lowpages - 0x2000) > PAGE_SIZE))
    {
        __KernBootPrivate = (struct KernBootPrivate *)((lowpages - PAGE_SIZE) & ~PAGE_MASK);
        lowpages = (IPTR)(__KernBootPrivate - 1)/1024;

        krnSetTagData(KRN_MEMLower, lowpages, msg);
        __KernBootPrivate->kbp_InitFlags = NULL;
        __KernBootPrivate->kbp_PrivateNext = __KernBootPrivate;
        __KernBootPrivate->kbp_PrivateNext += sizeof(struct KernBootPrivate);
    }
    else if (__KernBootPrivate == NULL)
    {
        /* Couldnt allocate private storage - so halt the cpu! */
        asm("hlt");
    }

    /* Initialise the serial hardware ASAP so all debug is output correctly!
        rkprintf (screen output) _may_ also be directed to the serial output */
    if (!(__KernBootPrivate->kbp_InitFlags & KERNBOOTFLAG_SERDEBUGCONFIGURED))
    {
        BOOL            _DoDebug = FALSE;

        tag = krnFindTagItem(KRN_CmdLine, msg);
        if (tag)
        {
            STRPTR cmd;
            ULONG temp;
            cmd = stpblk(tag->ti_Data);
            while(cmd[0])
            {
                /* Split the command line */
                temp = strcspn(cmd," ");
                if (strncmp(cmd, "DEBUG", 5)==0)
                {
                    _EnableDebug = 1;
                    if (cmd[5] == '=')
                    {
                        /* Check if our debug is requested .. */
#warning "TODO: Check for kernel.resource in the debug line and set _DoDebug if found"
                    }
                    else
                    {
                        /* All debug is enabled ... so dump ours also */
                        _DoDebug = TRUE;
                    }

                    if (_DoDebug)
                    {
                        __KernBootPrivate->kbp_InitFlags |= KERNBOOTFLAG_DEBUG;
                        __serial_rawio_debug = 1;
                    }
                }
                else if (strncmp(cmd, "SERIAL=", 7)==0)
                {
                    ULONG sertemp;
                    /*
                        Parse the serial debug port options
                        Format: SERIAL=<port>,<speed>,<databits>,<parity>,stopbits>
                    */
                    if (cmd[7] == '1')
                        __serial_rawio_port = 0x3F8;
                    else if (cmd[7] == '2')
                        __serial_rawio_port = 0x2F8;

                    sertemp = strcspn(cmd + 9,",") + 9;
                    cmd[sertemp] = '\0';
                    __serial_rawio_speed = atoi(&cmd[9]);
                    cmd[sertemp] = ',';

                    if (cmd[sertemp + 1] == '5')
                        __serial_rawio_databits = 0x00;
                    else if (cmd[sertemp + 1] == '6')
                        __serial_rawio_databits = 0x01;
                    else if (cmd[sertemp + 1] == '7')
                        __serial_rawio_databits = 0x02;
                    else if (cmd[sertemp + 1] == '8')
                        __serial_rawio_databits = 0x03;

                    if (cmd[sertemp + 3] == 'n')
                        __serial_rawio_parity = 0x00;
                    else if (cmd[sertemp + 3] == 'o')
                        __serial_rawio_parity = 0x08;
                    else if (cmd[sertemp + 3] == 'e')
                        __serial_rawio_parity = 0x18;

                    if (cmd[sertemp + 5] == '1')
                        __serial_rawio_stopbits = 0x00;
                    else if (cmd[sertemp + 5] == '2')
                        __serial_rawio_stopbits = 0x04;
                }
                cmd = stpblk(cmd+temp);
            }
        }
        if (_EnableDebug == 1)
        {
            struct ExecBase *SysBase = NULL;
            Exec_SerialRawIOInit();
        }
        __KernBootPrivate->kbp_InitFlags |= KERNBOOTFLAG_SERDEBUGCONFIGURED;
    }

    rkprintf("[Kernel] kernel_cstart: Jumped into kernel.resource @ %p [asm stub @ %p].\n", kernel_cstart, &start64);

    if (!(__KernBootPrivate->kbp_InitFlags & KERNBOOTFLAG_BOOTCPUSET))
    {
        if ((_EnableDebug == 1) && (__KernBootPrivate->kbp_InitFlags & KERNBOOTFLAG_SERDEBUGCONFIGURED))
        {
            rkprintf("[Kernel] kernel_cstart: Serial Debug initialised [port 0x%x, speed=%d, flags=0x%x].\n", __serial_rawio_port, __serial_rawio_speed, (__serial_rawio_databits | __serial_rawio_parity | __serial_rawio_stopbits));
        }

        core_APICProbe(__KernBootPrivate);

        _APICBase = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase);
        _APICID = (UBYTE)AROS_UFC1(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));
        rkprintf("[Kernel] kernel_cstart[%d]: launching on BSP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase);
        rkprintf("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p [%d bytes], Next @ %p\n", _APICID, __KernBootPrivate, sizeof(struct KernBootPrivate), __KernBootPrivate->kbp_PrivateNext);

        __KernBootPrivate->kbp_APIC_BSPID = _APICID;
        __KernBootPrivate->kbp_InitFlags |= KERNBOOTFLAG_BOOTCPUSET;

         tag = krnFindTagItem(KRN_CmdLine, msg);
        if (tag)
        {
            if (tag->ti_Data != (IPTR)__KernBootPrivate->kbp_BOOTCmdLine) {
                strncpy(__KernBootPrivate->kbp_BOOTCmdLine, tag->ti_Data, 200);
                tag->ti_Data = (IPTR)__KernBootPrivate->kbp_BOOTCmdLine;
            }
        }

        tag = krnFindTagItem(KRN_VBEModeInfo, msg);
        if (tag)
        {
            if (tag->ti_Data != (IPTR)&vbemd) {
                bcopy(tag->ti_Data, &vbemd, sizeof(vbemd));
                tag->ti_Data = (IPTR)&vbemd;
            }
        }

        tag = krnFindTagItem(KRN_VBEControllerInfo, msg);
        if (tag)
        {
            if (tag->ti_Data != (IPTR)&vbectrl) {
                bcopy(tag->ti_Data, &vbectrl, sizeof(vbectrl));
                tag->ti_Data = (IPTR)&vbectrl;
            }
        }

        BootMsg = msg;

        /* Initialize the ACPI boot-time table parser. */
        __KernBootPrivate->kbp_ACPIRSDP = core_ACPIProbe(msg, __KernBootPrivate);
        rkprintf("[Kernel] kernel_cstart[%d]: core_ACPIProbe() returned %p\n", _APICID, __KernBootPrivate->kbp_ACPIRSDP);

        lowpages = (krnGetTagData(KRN_MEMLower, 0, msg) * 1024);
        rkprintf("[Kernel] kernel_cstart[%d]: lowpages = %p\n", _APICID, lowpages);
        if ((lowpages > 0x2000) && ((lowpages - 0x2000) > PAGE_SIZE))
        {
            __KernBootPrivate->kbp_APIC_TrampolineBase = (lowpages - PAGE_SIZE) & ~PAGE_MASK;
            lowpages = (__KernBootPrivate->kbp_APIC_TrampolineBase - 1)/1024;

            krnSetTagData(KRN_MEMLower, lowpages, msg);
            rkprintf("[Kernel] kernel_cstart[%d]: Allocated %d bytes for APIC Trampoline @ %p\n", _APICID, PAGE_SIZE, __KernBootPrivate->kbp_APIC_TrampolineBase);

#if defined(CONFIG_LAPICS)
            memcpy(__KernBootPrivate->kbp_APIC_TrampolineBase, &_binary_smpbootstrap_start,
                        &_binary_smpbootstrap_size);

            rkprintf("[Kernel] kernel_cstart[%d]: Copied APIC bootstrap code to Trampoline from %p, %d bytes\n", _APICID, &_binary_smpbootstrap_start,
                        &_binary_smpbootstrap_size);
#endif
        }
    }
    else
    {
        _APICBase = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase);
        _APICID = (UBYTE)AROS_UFC1(IPTR,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));

        rkprintf("[Kernel] kernel_cstart[%d]: launching on AP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase);
        rkprintf("[Kernel] kernel_cstart[%d]: KernelBootPrivate @ %p\n", _APICID, __KernBootPrivate);
    }
    /* Prepair GDT */
    core_SetupGDT(__KernBootPrivate);

    /* Set TSS, GDT, LDT and MMU up */
    core_CPUSetup(_APICBase);
    core_SetupIDT(__KernBootPrivate);
    core_SetupMMU(__KernBootPrivate);

    (rkprintf("[Kernel] kernel_cstart[%d]: APIC_BASE_MSR=%016p\n", _APICID, _APICBase + 0x900));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
        addr = krnGetTagData(KRN_KernelBase, 0, msg);
        len = krnGetTagData(KRN_KernelHighest, 0, msg) - addr;
        core_ProtKernelArea(addr, len, 1, 0, 1);

        /* Lock page 0! */
        core_ProtKernelArea(0, PAGE_SIZE, 0, 0, 0);

        /* Setup the 8259 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0x20)); /* Initialization sequence for 8259A-1 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x11),"i"(0xa0)); /* Initialization sequence for 8259A-2 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x20),"i"(0x21)); /* IRQs at 0x20 - 0x27 */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x28),"i"(0xa1)); /* IRQs at 0x28 - 0x2f */
        asm("outb   %b0,%b1\n\tcall delay": :"a"((char)0x04),"i"(0x21)); /* 8259A-1 is master */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x02),"i"(0xa1)); /* 8259A-2 is slave */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0x21)); /* 8086 mode for both */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0x01),"i"(0xa1));
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0x21)); /* Enable cascade int */
        asm("outb   %b0,%b1\n\tcall delay"::"a"((char)0xff),"i"(0xa1)); /* Mask all interrupts */

        rkprintf("[Kernel] kernel_cstart: Interrupts redirected. We will go back in a minute ;)\n");
        rkprintf("[Kernel] kernel_cstart: Booting exec.library\n\n");

        return exec_main(msg, entry);
    }

//    else
    {
        /* A temporary solution - the code for smp is not ready yet... */
#warning "TODO: launch idle task ..."
        rkprintf("[Kernel] kernel_cstart[%d]: Going into endless loop...\n", _APICID);
        while(1) asm volatile("hlt");
    }

    return NULL;    
}


/* Small delay routine used by exec_cinit initializer */
asm("\ndelay:\t.short   0x00eb\n\tretq");

static uint64_t __attribute__((used, section(".data"), aligned(16))) tmp_stack[128];
static const uint64_t *tmp_stack_end __attribute__((used, section(".text"))) = &tmp_stack[120];
static uint64_t stack[STACK_SIZE] __attribute__((used));
static uint64_t stack_panic[STACK_SIZE] __attribute__((used));
static uint64_t stack_super[STACK_SIZE] __attribute__((used));
static uint64_t stack_ring1[STACK_SIZE] __attribute__((used));

static const uint64_t *stack_end __attribute__((used, section(".text"))) = &stack[STACK_SIZE-16];
static const void *target_address __attribute__((section(".text"),used)) = (void*)kernel_cstart;

static struct int_gate_64bit IGATES[256] __attribute__((used,aligned(256)));
static struct tss_64bit TSS[16] __attribute__((used,aligned(128)));
static struct {
    struct segment_desc seg0;      /* seg 0x00 */
    struct segment_desc super_cs;  /* seg 0x08 */
    struct segment_desc super_ds;  /* seg 0x10 */
    struct segment_desc user_cs32; /* seg 0x18 */
    struct segment_desc user_ds;   /* seg 0x20 */
    struct segment_desc user_cs;   /* seg 0x28 */
    struct segment_desc gs;        /* seg 0x30 */
    struct segment_desc ldt;       /* seg 0x38 */
    struct {
        struct segment_desc tss_low;   /* seg 0x40... */
        struct segment_ext  tss_high;
    } tss[16];        
} GDT __attribute__((used,aligned(128)));

const struct
{
    uint16_t size __attribute__((packed));
    uint64_t base __attribute__((packed));
} 
GDT_sel = {sizeof(GDT)-1, (uint64_t)&GDT};

static tls_t system_tls;

void core_SetupGDT(struct KernBootPrivate *__KernBootPrivate)
{
    IPTR        _APICBase;
    UBYTE       _APICID;
    int i;

    _APICBase = AROS_UFC0(IPTR,
        (*(__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getbase));

    _APICID = (UBYTE)AROS_UFC1(IPTR,
        (*(__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid),
                AROS_UFCA(IPTR, _APICBase, A0));

    D(rkprintf("[Kernel] core_SetupGDT(%d)\n", _APICID));

    if (_APICID == __KernBootPrivate->kbp_APIC_BSPID)
    {
        /* Supervisor segments */
        GDT.super_cs.type=0x1a;     /* code segment */
        GDT.super_cs.dpl=0;         /* supervisor level */
        GDT.super_cs.p=1;           /* present */
        GDT.super_cs.l=1;           /* long (64-bit) one */
        GDT.super_cs.d=0;           /* must be zero */
        GDT.super_cs.limit_low=0xffff;
        GDT.super_cs.limit_high=0xf;
        GDT.super_cs.g=1;

        GDT.super_ds.type=0x12;     /* data segment */
        GDT.super_ds.dpl=0;         /* supervisor level */
        GDT.super_ds.p=1;           /* present */
        GDT.super_ds.limit_low=0xffff;
        GDT.super_ds.limit_high=0xf;
        GDT.super_ds.g=1;
        GDT.super_ds.d=1;

        /* User mode segments */
        GDT.user_cs.type=0x1a;      /* code segment */
        GDT.user_cs.dpl=3;          /* User level */
        GDT.user_cs.p=1;            /* present */
        GDT.user_cs.l=1;            /* long mode */
        GDT.user_cs.d=0;            /* must be zero */
        GDT.user_cs.limit_low=0xffff;
        GDT.user_cs.limit_high=0xf;
        GDT.user_cs.g=1;

        GDT.user_cs32.type=0x1a;    /* code segment for legacy 32-bit code. NOT USED YET! */
        GDT.user_cs32.dpl=3;        /* user elvel */
        GDT.user_cs32.p=1;          /* present */
        GDT.user_cs32.l=0;          /* 32-bit mode */
        GDT.user_cs32.d=1;          /* 32-bit code */
        GDT.user_cs32.limit_low=0xffff;
        GDT.user_cs32.limit_high=0xf;
        GDT.user_cs32.g=1;

        GDT.user_ds.type=0x12;      /* data segment */
        GDT.user_ds.dpl=3;    /* user elvel */
        GDT.user_ds.p=1;            /* present */
        GDT.user_ds.limit_low=0xffff;
        GDT.user_ds.limit_high=0xf;
        GDT.user_ds.g=1;
        GDT.user_ds.d=1;

        for (i=0; i < 16; i++)
        {
            /* Task State Segment */
            GDT.tss[i].tss_low.type=0x09;      /* 64-bit TSS */
            GDT.tss[i].tss_low.limit_low=sizeof(TSS)-1;
            GDT.tss[i].tss_low.base_low=((unsigned int)&TSS[i]) & 0xffff;
            GDT.tss[i].tss_low.base_mid=(((unsigned int)&TSS[i]) >> 16) & 0xff;
            GDT.tss[i].tss_low.dpl=3;          /* User mode task */
            GDT.tss[i].tss_low.p=1;            /* present */
            GDT.tss[i].tss_low.limit_high=((sizeof(TSS)-1) >> 16) & 0x0f;
            GDT.tss[i].tss_low.base_high=(((unsigned int)&TSS[i]) >> 24) & 0xff;
            GDT.tss[i].tss_high.base_ext = 0;  /* is within 4GB :-D */
        }
        intptr_t tls_ptr = (intptr_t)&system_tls;

        GDT.gs.type=0x12;      /* data segment */
        GDT.gs.dpl=3;    /* user elvel */
        GDT.gs.p=1;            /* present */
        GDT.gs.base_low = tls_ptr & 0xffff;
        GDT.gs.base_mid = (tls_ptr >> 16) & 0xff;
        GDT.gs.base_high = (tls_ptr >> 24) & 0xff;   
        GDT.gs.g=1;
        GDT.gs.d=1;
    }
}

void core_CPUSetup(IPTR _APICBase)
{
    UBYTE _APICID = AROS_UFC1(UBYTE,
                ((struct GenericAPIC *)__KernBootPrivate->kbp_APIC_Drivers[__KernBootPrivate->kbp_APIC_DriverID])->getid,
                        AROS_UFCA(IPTR, _APICBase, A0));

    D(rkprintf("[Kernel] core_CPUSetup(%d)\n", _APICID));

//    system_tls.SysBase = (struct ExecBase *)0x12345678;
    
    TSS[_APICID].ist1 = (uint64_t)&stack_panic[STACK_SIZE-2];
    TSS[_APICID].rsp0 = (uint64_t)&stack_super[STACK_SIZE-2];
    TSS[_APICID].rsp1 = (uint64_t)&stack_ring1[STACK_SIZE-2];

    rkprintf("[Kernel] core_CPUSetup[%d]: Reloading the GDT and Task Register\n", _APICID);
    asm volatile ("lgdt %0"::"m"(GDT_sel));
    asm volatile ("ltr %w0"::"r"(TASK_SEG + (_APICID << 4)));
    asm volatile ("mov %0,%%gs"::"a"(SEG_GS));    
}

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
{
    if (!(*tagListPtr)) return 0;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;

        }

        (*tagListPtr)++;
    }
}

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return 0;
}

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && ((ti = krnFindTagItem(tagValue, tagList)) != 0))
        return ti->ti_Data;

    return defaultVal;
}

void krnSetTagData(Tag tagValue, intptr_t newtagValue, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && ((ti = krnFindTagItem(tagValue, tagList)) != 0))
        ti->ti_Data = newtagValue;
}

AROS_LH0I(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;
    
    AROS_LIBFUNC_EXIT
}

