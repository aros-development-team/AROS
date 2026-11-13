/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_intr.h"

#define TRAPDEBUG_HALTONTRAP
#define TRAPDEBUG_UNWIND

int core_GenericTrap(struct ExceptionContext *r,
                     struct KernelBase *KernelBase,
                     UWORD num,
                     UQUAD code);

static void core_DumpAPICInfo(struct KernelBase *KernelBase);

#define DEFINE_TRAP(num) \
static int Trap##num(struct ExceptionContext *r, struct KernelBase *KernelBase, struct ExecBase *SysBase) { \
    UQUAD code = 0; \
    struct PlatformData *pd = KernelBase->kb_PlatformData; \
    if (pd && pd->kb_LastException == num) \
        code = pd->kb_LastExceptionError; \
    if (pd && pd->kb_APIC && pd->kb_APIC->apic_count > 1) \
        core_DumpAPICInfo(KernelBase); \
    return core_GenericTrap(r, KernelBase, num, code); \
}

DEFINE_TRAP(0)  DEFINE_TRAP(1)  DEFINE_TRAP(2)
DEFINE_TRAP(3)  DEFINE_TRAP(4)  DEFINE_TRAP(5)
DEFINE_TRAP(6)  DEFINE_TRAP(7)  DEFINE_TRAP(8)
DEFINE_TRAP(10) DEFINE_TRAP(11) DEFINE_TRAP(12)
DEFINE_TRAP(13) DEFINE_TRAP(14) DEFINE_TRAP(16)
DEFINE_TRAP(17) DEFINE_TRAP(18) DEFINE_TRAP(19)

static void core_DumpAPICInfo(struct KernelBase *KernelBase)
{
    struct PlatformData *pd = KernelBase->kb_PlatformData;
    struct APICData *apic = pd->kb_APIC;
    apicid_t cpu = KrnGetCPUNumber();

    bug("[Kernel] CPU exception occurred on APIC #%u (of %u)\n",
        (unsigned)cpu, (unsigned)apic->apic_count);

    bug("[Kernel]   LAPIC base: 0x%016llx  flags: 0x%04x\n",
        (unsigned long long)apic->lapicBase,
        (unsigned)apic->flags);

    if (apic->msibase && apic->msibase <= apic->msilast)
    {
        ULONG max_start = (APIC_IRQ_BASE - X86_CPU_EXCEPT_COUNT);
        ULONG max_end   = max_start + APIC_IRQ_COUNT - 1;
        ULONG allocated = apic->msilast - apic->msibase + 1;

        bug("[Kernel]   MSI range      : %u–%u\n", max_start, max_end);
        bug("[Kernel]     Allocated    : %u–%u (%u vectors)\n",
            apic->msibase, apic->msilast, allocated);
    }

    if (cpu < apic->apic_count) {
        const struct CPUData *core = &apic->cores[cpu];
        bug("[Kernel]   Core info:\n");
        bug("[Kernel]     LocalAPIC ID: %u  Private ID: %u  ICID: %u\n",
            (unsigned)core->cpu_LocalID,
            (unsigned)core->cpu_PrivateID,
            (unsigned)core->cpu_ICID);
        bug("[Kernel]     GDT=%p  IDT=%p  TLS=%p  MMU=%p\n",
            core->cpu_GDT, core->cpu_IDT, core->cpu_TLS, core->cpu_MMU);
        bug("[Kernel]     TSC=%llu Hz  Timer=%lu Hz  Load=%lu%%\n",
            (unsigned long long)core->cpu_TSCFreq,
            (unsigned long)core->cpu_TimerFreq,
            (unsigned long)core->cpu_Load);
        bug("[Kernel]     LAPICTick=%llu  LastLoadTime=%llu  SleepTime=%llu\n",
            (unsigned long long)core->cpu_LAPICTick,
            (unsigned long long)core->cpu_LastCPULoadTime,
            (unsigned long long)core->cpu_SleepTime);
    }
}

static void core_DumpFPUState(struct ExceptionContext *r)
{
    if (!(r->Flags & (ECF_FPFXS | ECF_FPXS))) {
        bug("[Kernel] (No FPU/SSE context present)\n");
        return;
    }

    bug("[Kernel] FXSData=%p  FPUCtxSize=%lu\n",
        r->FXSData, (unsigned long)r->FPUCtxSize);

    struct FPFXSContext *fx = (r->Flags & ECF_FPXS)
        ? &r->XSData->legacy
        : r->FXSData;

    bug("[Kernel] --- FPU/SSE Context ---\n");
    bug("[Kernel] FCW=%04x FSW=%04x FTW=%04x FOP=%04x\n",
        fx->fcw, fx->fsw, fx->ftw, fx->fop);
    bug("[Kernel] IP=%08x:%08x DP=%08x:%08x\n",
        fx->cs, fx->ip, fx->ds, fx->dp);
    bug("[Kernel] MXCSR=%08x\n", fx->mxcsr);

    // Decode FSW bits
    static const char *fswFlags[6] = {
        "IE","DE","ZE","OE","UE","PE"
    };
    for (int i = 0; i < 6; ++i) {
        if (fx->fsw & (1 << i)) {
            bug("[Kernel] FSW: %s set\n", fswFlags[i]);
        }
    }

    if (fx->fsw & (1 << 7)) {
        bug("[Kernel] FSW: Stack fault (#MF)\n");
    }

    bug("[Kernel] FCW masks:");
    for (int i = 0; i < 6; ++i) {
        bug(" %s%s",
            (fx->fcw & (1 << i)) ? fswFlags[i] : "",
            (fx->fcw & (1 << i)) ? "M" : "");
    }
    bug("\n");

    // Decode MXCSR exception flags and masks
    bug("[Kernel] MXCSR masks=%02x exceptions=%02x rounding=%s\n",
        (fx->mxcsr >> 7) & 0x3f,
        fx->mxcsr & 0x3f,
        ((fx->mxcsr >> 13) & 3) == 0 ? "NEAR"
        : ((fx->mxcsr >> 13) & 3) == 1 ? "DOWN"
        : ((fx->mxcsr >> 13) & 3) == 2 ? "UP" : "ZERO");

    // Dump MMX and XMM register contents
    bug("[Kernel] --- FPU/MMX Registers ---\n");
    for (int i = 0; i < 8; ++i) {
        bug("[Kernel] MM%-2d: ", i);
        for (int b = 9; b >= 0; --b) {
            bug("%02x", fx->mm[i].data[b]);
        }
        bug("\n");
    }

    bug("[Kernel] --- SSE/XMM Registers ---\n");
    for (int i = 0; i < 16; ++i) {
        bug("[Kernel] XMM%-2d: ", i);
        for (int b = 15; b >= 0; --b)
            bug("%02x", fx->xmm[i].data[b]);
        bug("\n");
    }
}

unsigned core_BacktraceCurrent(void **out_pcs, unsigned max_depth)
{
    void *rbp;
    __asm__ volatile ("mov %%rbp,%0" : "=r"(rbp));
    return KrnBacktraceFromFrame(rbp, out_pcs, max_depth);
}

static void core_DumpExceptionState(
    UWORD num, UQUAD code, struct ExceptionContext *r)
{
    UQUAD cr0, cr2, cr3, cr4;
    __asm__ volatile("mov %%cr0,%0" : "=r"(cr0));
    __asm__ volatile("mov %%cr2,%0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3,%0" : "=r"(cr3));
    __asm__ volatile("mov %%cr4,%0" : "=r"(cr4));

    bug("[Kernel] ========= Exception #%u =========\n", num);
    bug("[Kernel] CR0=%016llx CR2=%016llx CR3=%016llx CR4=%016llx\n",
        cr0, cr2, cr3, cr4);

    if (!r) {
        bug("[Kernel] (no ExceptionContext provided)\n");
        return;
    }

    bug("[Kernel] RIP=%016llx CS=%04llx RFLAGS=%016llx\n", r->rip, r->cs, r->rflags);
    bug("[Kernel] RSP=%016llx SS=%04llx RBP=%016llx\n", r->rsp, r->ss, r->rbp);
    bug("[Kernel] RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n",
        r->rax, r->rbx, r->rcx, r->rdx);
    bug("[Kernel] RSI=%016llx RDI=%016llx R8 =%016llx R9 =%016llx\n",
        r->rsi, r->rdi, r->r8, r->r9);
    bug("[Kernel] R10=%016llx R11=%016llx R12=%016llx R13=%016llx\n",
        r->r10, r->r11, r->r12, r->r13);
    bug("[Kernel] R14=%016llx R15=%016llx\n", r->r14, r->r15);
    bug("[Kernel] DS=%04llx ES=%04llx FS=%04llx GS=%04llx\n",
        r->ds, r->es, r->fs, r->gs);
    bug("[Kernel] Error Code=%08lx\n", (unsigned long)code);

    // --- Exception-specific decoding -------------------------------------
    switch (num) {
    case 0: // Divide Error
        bug("[Kernel] Divide-by-zero or overflow in integer division.\n");
        break;

    case 6: // Invalid Opcode
        {
            // Try to show bytes near RIP if accessible
            const UBYTE *rip = (const UBYTE *)(uintptr_t)r->rip;
            bug("[Kernel] Invalid opcode near RIP=%016llx:", r->rip);
            for (int i = -4; i < 4; ++i) {
                UBYTE b;
                if (__builtin_setjmp(/* hypothetical safe fetch? */0)) break;
                b = rip[i];
                bug(" %02x", b);
            }
            bug("\n");
        }
        break;

	case 7:  // Device not available (#NM)
		bug("[Kernel] x87 Device Not Available - FPU not initialized or TS set.\n");
		break;

    case 8: // Double fault
        bug("[Kernel] Double fault: indicates recursive fault during exception handling.\n");
        bug("[Kernel] Error code is always 0 for double fault.\n");
        break;

    case 10: // Invalid TSS
        bug("[Kernel] Invalid TSS: selector = 0x%04lx\n", (unsigned long)code);
        break;

    case 11: // Segment Not Present
        bug("[Kernel] Segment not present: selector = 0x%04lx\n", (unsigned long)code);
        break;

    case 12: // Stack Fault
        bug("[Kernel] Stack segment fault: selector = 0x%04lx\n", (unsigned long)code);
        break;

    case 13: // General Protection
        {
            UWORD sel = (UWORD)code;
            UWORD ti = (sel >> 2) & 1;
            UWORD idx = sel >> 3;
            bug("[Kernel] General Protection Fault: selector = 0x%04x (TI=%u, index=%u)\n",
                sel, ti, idx);
            bug("[Kernel] Possible causes: bad segment, privilege violation, "
                "or descriptor not present.\n");
        }
        break;

    case 14: // Page Fault
        {
            unsigned long ec = (unsigned long)code;
            bug("[Kernel] Page Fault @ linear address 0x%016llx\n", cr2);
            bug("[Kernel] PFEC bits: %s %s %s %s %s\n",
                (ec & 1)  ? "PRESENT" : "NOT-PRESENT",
                (ec & 2)  ? "WRITE"   : "READ",
                (ec & 4)  ? "USER"    : "SUPERVISOR",
                (ec & 8)  ? "RSVD"    : "OK",
                (ec & 16) ? "IFETCH"  : "DATA");
        }
        break;

	case 16: // x87 FPU error (#MF)
		bug("[Kernel] x87 Floating Point Exception (#MF)\n");
		core_DumpFPUState(r);
		break;

    case 17: // Alignment Check
        bug("[Kernel] Alignment Check fault. Address=%016llx, EC bits=%08lx\n", cr2, (unsigned long)code);
        break;

    case 18: // Machine Check
        bug("[Kernel] Machine Check Exception: hardware detected unrecoverable error.\n");
        break;

	case 19: // SIMD exception (#XM)
		bug("[Kernel] SIMD Floating Point Exception (#XM)\n");
		core_DumpFPUState(r);
		break;

    default:
        break;
    }

#if defined(TRAPDEBUG_UNWIND)
    {
        void *pcs[64];
        unsigned n = 0;

        bug("[Kernel] ================================\n");
        /* Prefer unwinding from the trapped frame’s RBP */
        if (r->rbp) {
            n = KrnBacktraceFromFrame((void*)(uintptr_t)r->rbp, pcs, 64);
        } else {
            n = core_BacktraceCurrent(pcs, 64);
        }
        KrnPrintBacktrace("[Kernel] ", pcs, n);
    }
#endif

    // --- tail section ---
    bug("[Kernel] ================================\n");
}

int core_GenericTrap(struct ExceptionContext *r,
                     struct KernelBase *KernelBase,
                     UWORD num,
                     UQUAD code)
{
    core_DumpExceptionState(num, code, r);

#if defined(TRAPDEBUG_HALTONTRAP)
    // Freeze the CPU:
    __asm__ volatile("cli; hlt");
#endif
    return 1;
}

void core_InstallDebugTraps(struct KernelBase *KernelBase)
{
    KrnAddExceptionHandler(0,  Trap0,  KernelBase, NULL);
    KrnAddExceptionHandler(1,  Trap1,  KernelBase, NULL);
    KrnAddExceptionHandler(2,  Trap2,  KernelBase, NULL);
    KrnAddExceptionHandler(3,  Trap3,  KernelBase, NULL);
    KrnAddExceptionHandler(4,  Trap4,  KernelBase, NULL);
    KrnAddExceptionHandler(5,  Trap5,  KernelBase, NULL);
    KrnAddExceptionHandler(6,  Trap6,  KernelBase, NULL);
    KrnAddExceptionHandler(7,  Trap7,  KernelBase, NULL);
    KrnAddExceptionHandler(8,  Trap8,  KernelBase, NULL);
    KrnAddExceptionHandler(10, Trap10, KernelBase, NULL);
    KrnAddExceptionHandler(11, Trap11, KernelBase, NULL);
    KrnAddExceptionHandler(12, Trap12, KernelBase, NULL);
    KrnAddExceptionHandler(13, Trap13, KernelBase, NULL);
    KrnAddExceptionHandler(14, Trap14, KernelBase, NULL);
    KrnAddExceptionHandler(16, Trap16, KernelBase, NULL);
    KrnAddExceptionHandler(17, Trap17, KernelBase, NULL);
    KrnAddExceptionHandler(18, Trap18, KernelBase, NULL);
    KrnAddExceptionHandler(19, Trap19, KernelBase, NULL);
}
