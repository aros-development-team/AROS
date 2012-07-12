
#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>

#include LC_LIBDEFS_FILE
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "kernel_globals.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

extern char * __text_start;
extern char * __text_end;

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))

typedef union _pcicfg
{
    uint32_t    ul;
    uint16_t    uw[2];
    uint8_t             ub[4];
} pcicfg;

static uint32_t _read_config_long(int reg)
{
        uint32_t temp;
    outl_le(CFGADD(0, 0, 0, reg),PCI0_CFGADDR);
    temp=inl_le(PCI0_CFGDATA);
    return temp;
}

static uint16_t _read_config_word(int reg)
{
        pcicfg temp;

    temp.ul = _read_config_long(reg);
    return temp.uw[1 - ((reg&2)>>1)];
}

static void _write_config_long(int reg, uint32_t val)
{
        outl_le(CFGADD(0, 0, 0, reg),PCI0_CFGADDR);
        outl_le(val,PCI0_CFGDATA);
}

static void _write_config_word(int reg, uint16_t val)
{
        pcicfg temp;

        temp.ul = _read_config_long(reg);
        temp.uw[1 - ((reg&2)>>1)] = val;
        _write_config_long(reg, temp.ul);
}

/*
 * Interrupt controller functions. Actually have the following prototypes:
 *
 * void enable_irq(uint8_t num);
 * void disable_irq(uint8_t num);
 */

/* Master copy of UICn_ER */
ULONG uic_er[4];

static inline void enable_irq(int irq)
{
    ULONG mask = (0x80000000 >> (irq & 0x1f));

    uic_er[irq >> 5] |= mask;

    switch (irq >> 5) {
    case 0: wrdcr(UIC0_ER, uic_er[0]); break;
    case 1: wrdcr(UIC1_ER, uic_er[1]); break;
    case 2: wrdcr(UIC2_ER, uic_er[2]); break;
    case 3: wrdcr(UIC3_ER, uic_er[3]); break;
    default: break;
    }
}

static inline void disable_irq(int irq)
{
    ULONG mask = (0x80000000 >> (irq & 0x1f));

    uic_er[irq >> 5] &= ~mask;

    switch (irq >> 5) {
    case 0: wrdcr(UIC0_ER, uic_er[0]); break;
    case 1: wrdcr(UIC1_ER, uic_er[1]); break;
    case 2: wrdcr(UIC2_ER, uic_er[2]); break;
    case 3: wrdcr(UIC3_ER, uic_er[3]); break;
    default: break;
    }
}

void syscall_handler(context_t *ctx, uint8_t exception)
{
    struct KernelBase *KernelBase = getKernelBase();

    D(bug("[KRN] SysCall: SRR0=%p, SRR1=%p, SC=%d\n", ctx->cpu.srr0, ctx->cpu.srr1, ctx->cpu.gpr[3]));
   
    switch (ctx->cpu.gpr[3])
    {
        case SC_CLI:
            /* Disable all external interrupts */
            wrdcr(UIC0_ER, 0);
            break;
        
        case SC_STI:
            /* Enable selected external interrupts */
            wrdcr(UIC0_ER, uic_er[0]);
            break;

        case SC_IRQ_ENABLE:
            enable_irq(ctx->cpu.gpr[4]);
            break;
        
        case SC_IRQ_DISABLE:
            disable_irq(ctx->cpu.gpr[4]);
            break;
        
        case SC_SUPERSTATE:
            ctx->cpu.gpr[3] = ctx->cpu.srr1;
            ctx->cpu.srr1 &= ~MSR_PR;
            break;
        
        case SC_ISSUPERSTATE:
            if (ctx->cpu.srr1 & MSR_PR)
                ctx->cpu.gpr[3] = 0;
            else
                ctx->cpu.gpr[3] = 1;
            break;
        
        case SC_INVALIDATED:
        {
            char *start = (char*)((IPTR)ctx->cpu.gpr[4] & 0xffffffe0);
            char *end = (char*)(((IPTR)ctx->cpu.gpr[4] + ctx->cpu.gpr[5] + 31) & 0xffffffe0);
            char *ptr;
            
            for (ptr = start; ptr < end; ptr +=32)
            {
                asm volatile("dcbi 0,%0"::"r"(ptr));
            }
            asm volatile("sync");
            break;
        }

        case SC_REBOOT:
        {
                /*
                 * Hard case on Sam440. First of all, the CPU has to be found on PCI bus.
                 * The PCI Bus reset signal will be issued there. Further, CPU returns from
                 * exception to address 0xfffffffc.
                 */

                D(bug("[KRN] REBOOT..."));

                D(bug("[KRN] LR=%08x", ctx->cpu.lr));
                D(bug("[KRN] Backtrace:\n"));
                uint32_t *sp = (uint32_t *)ctx->cpu.gpr[1];
                while(*sp) {
                    sp = (uint32_t *)sp[0];
                    D(bug("[KRN]  %08x\n", sp[1]));
                }

                uint64_t newtbu = mftbu() + KernelBase->kb_PlatformData->pd_OPBFreq;
                while(newtbu > mftbu());
                D(bug("3..."));
                newtbu = mftbu() + KernelBase->kb_PlatformData->pd_OPBFreq;
                while(newtbu > mftbu());
                D(bug("2..."));
                newtbu = mftbu() + KernelBase->kb_PlatformData->pd_OPBFreq;
                while(newtbu > mftbu());
                D(bug("1..."));
                newtbu = mftbu() + KernelBase->kb_PlatformData->pd_OPBFreq;
                while(newtbu > mftbu());
                D(bug("\n\n\n"));

                /* PCI Bridge options 2 register */
                uint16_t val = _read_config_word(0x60);
                /* Set the PCI reset signal */
                _write_config_word(0x60, val | (1 << 12));
                int i;
                /* Let the PCI reset last as long as needed */
                for (i=0; i < 100; i++)
                        asm volatile("sync");
                /* De-assert the PCI reset */
                _write_config_word(0x60, val & ~(1 << 12));

                /* Restart CPU */
                asm volatile("mtsrr0 %0; mtsrr1 %1; rfi"::"r"(0xfffffffc), "r"(1 << 6));
                while(1);
        }
        default:
                core_SysCall(ctx->cpu.gpr[3], ctx);
                break;
    }
}
