/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: boot.c - AArch64 bootstrap for Raspberry Pi 3
*/

#include <exec/types.h>
#include <aros/macros.h>
#include <inttypes.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <aros/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "boot.h"
#include "bcm2708_boot.h"
#include "serialdebug.h"
#include "mmu.h"
#include "vc_mb.h"
#include "vc_fb.h"
#include "elf.h"
#include "devicetree.h"
#include "io.h"

/* Set to 'x' to enable verbose boot tracing */
#define DBOOT(x)

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)

uintptr_t __arm_periiobase = 0;

extern void mem_init(void);
extern unsigned int uartclock;
extern unsigned int uartdivint;
extern unsigned int uartdivfrac;
extern unsigned int uartbaud;

/*
 * AArch64 bootstrap entry point.
 *
 * On Raspberry Pi 3 in 64-bit mode, the firmware loads kernel8.img at 0x80000
 * and starts execution in EL2 (or EL1 depending on config.txt).
 *
 * Register state on entry:
 *   x0 = 32-bit pointer to device tree blob (DTB)
 *   x1 = 0 (reserved)
 *   x2 = 0 (reserved)
 *   x3 = 0 (reserved)
 *
 * We need to:
 *   1. Determine current Exception Level (EL3/EL2/EL1)
 *   2. Drop to EL1 if in higher EL
 *   3. Enable FP/NEON
 *   4. Set up stack
 *   5. Jump to C boot() function
 */
asm("   .section .aros.startup      \n"
"       .globl bootstrap            \n"
"       .type bootstrap,%function   \n"
"bootstrap:                         \n"
        /* Only let core 0 boot; secondary cores spin */
"       mrs     x4, MPIDR_EL1      \n"
"       and     x4, x4, #0xFF      \n" /* Aff0 = core ID */
"       cbnz    x4, secondary_spin \n"
"                                   \n"
"       mrs     x4, CurrentEL       \n" /* Get current exception level */
"       lsr     x4, x4, #2          \n" /* Extract EL field (bits [3:2]) */
"       cmp     x4, #3              \n"
"       b.eq    from_el3            \n"
"       cmp     x4, #2              \n"
"       b.eq    from_el2            \n"
"       b       at_el1              \n"
"                                   \n"
"from_el3:                          \n"
"       mrs     x4, SCR_EL3         \n" /* Secure Configuration Register */
"       orr     x4, x4, #(1 << 10) \n" /* RW bit - EL2 is AArch64 */
"       orr     x4, x4, #(1 << 0)  \n" /* NS bit - Non-secure */
"       msr     SCR_EL3, x4         \n"
"       mov     x4, #0x3c9          \n" /* SPSR_EL3: EL2h, DAIF masked */
"       msr     SPSR_EL3, x4        \n"
"       adr     x4, from_el2        \n"
"       msr     ELR_EL3, x4         \n"
"       eret                        \n"
"                                   \n"
"from_el2:                          \n"
"       mrs     x4, HCR_EL2         \n" /* Hypervisor Configuration Register */
"       orr     x4, x4, #(1 << 31) \n" /* RW bit - EL1 is AArch64 */
"       msr     HCR_EL2, x4         \n"
"                                   \n"
        /*
         * SCTLR_EL1 is architecturally UNKNOWN out of reset when entered
         * via EL2. Set a known-safe value (RES1 bits only: MMU/caches off,
         * little-endian, no alignment checking) BEFORE the eret, using
         * movz/movk so no memory access happens with an unknown EE bit.
         */
"       movz    x4, #0x0800         \n"
"       movk    x4, #0x30d0, lsl #16\n" /* SCTLR_EL1 = 0x30d00800 */
"       msr     SCTLR_EL1, x4       \n"
"                                   \n"
"       mov     x4, #0x3c5          \n" /* SPSR_EL2: EL1h, DAIF masked */
"       msr     SPSR_EL2, x4        \n"
"       adr     x4, at_el1          \n"
"       msr     ELR_EL2, x4         \n"
"                                   \n"
        /* Enable CNTP for EL1 */
"       mrs     x4, CNTHCTL_EL2     \n"
"       orr     x4, x4, #3          \n" /* Enable EL1 physical timer access */
"       msr     CNTHCTL_EL2, x4     \n"
"       msr     CNTVOFF_EL2, xzr    \n" /* Virtual timer offset = 0 */
"       eret                        \n"
"                                   \n"
"at_el1:                            \n"
        /* Known-safe SCTLR_EL1 also when entered directly at EL1 */
"       movz    x4, #0x0800         \n"
"       movk    x4, #0x30d0, lsl #16\n"
"       msr     SCTLR_EL1, x4       \n"
"       isb                          \n"
"                                    \n"
        /* Install the bootstrap exception vectors: any fault during boot
         * prints a diagnostic instead of silently hanging at an UNKNOWN
         * VBAR (real HW; QEMU resets VBAR to 0) */
"       ldr     x4, =boot_vectors   \n"
"       msr     VBAR_EL1, x4        \n"
"                                    \n"
        /* Enable FP/NEON */
"       mov     x4, #(3 << 20)      \n" /* CPACR_EL1: FPEN = 0b11 */
"       msr     CPACR_EL1, x4       \n"
"       isb                          \n"
"                                    \n"
        /* Set up stack pointer */
"       ldr     x4, =0x80000        \n" /* Stack just below kernel load address */
"       mov     sp, x4              \n"
"                                    \n"
        /* Save DTB pointer (x0) and jump to C */
"       ldr     x4, =boot          \n"
"       br      x4                  \n"
"                                    \n"
"secondary_spin:                    \n"
"       wfe                         \n"
"       b       secondary_spin      \n"
"                                    \n"
"       .section .text              \n"
".byte 0                            \n"
".string \"$VER: arosraspi-aarch64.img v40.46 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

/*
 * Minimal bootstrap exception vectors: every slot funnels into a C handler
 * that prints the fault syndrome and spins. Without this, VBAR_EL1 is
 * UNKNOWN on real hardware and any bootstrap-time fault (e.g. an unaligned
 * access while all memory is still Device-typed) becomes a silent hang.
 */
asm("   .section .text              \n"
"       .balign 2048                \n"
"boot_vectors:                      \n"
"       .rept 16                    \n"
"       b       boot_exception      \n"
"       .balign 128                 \n"
"       .endr                       \n"
);

void boot_exception(void) __attribute__((used));
void boot_exception(void)
{
    uint64_t esr, elr, far_r;

    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, far_el1" : "=r"(far_r));

    kprintf("[BOOT] EXCEPTION: ESR=%p ELR=%p FAR=%p\n",
            (void *)(uintptr_t)esr, (void *)(uintptr_t)elr, (void *)(uintptr_t)far_r);

    while (1)
        asm volatile("wfe");
}

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup" TARGET_SECTION_COMMENT))) = (void *)(0x4000 - 16);
static struct TagItem *boottag;
static unsigned long *mem_upper;
static void *pkg_image = NULL;
static uint32_t pkg_size = 0;

struct tag;

static const char bootstrapName[] = "Bootstrap/AArch64 ARMv8-A";

void query_vmem()
{
    volatile unsigned int *vc_msg = (unsigned int *) BOOTMEMADDR(bm_mboxmsg);

    kprintf("[BOOT] Query VC memory\n");
    vc_msg[0] = AROS_LONG2LE(8 * 4);
    vc_msg[1] = AROS_LONG2LE(VCTAG_REQ);
    vc_msg[2] = AROS_LONG2LE(VCTAG_GETVCRAM);
    vc_msg[3] = AROS_LONG2LE(8);
    vc_msg[4] = 0;
    vc_msg[5] = 0;
    vc_msg[6] = 0;
    vc_msg[7] = 0;

    vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vc_msg);
    vc_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);

    if (!vc_msg)
    {
        kprintf("[BOOT] GETVCRAM mailbox request failed, skipping VC memory setup\n");
        return;
    }

    kprintf("[BOOT] VC Base = %08x, Size = %08x\n", AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[6]));

    boottag->ti_Tag = KRN_VMEMLower;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]);
    boottag++;

    boottag->ti_Tag = KRN_VMEMUpper;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]) + AROS_LE2LONG(vc_msg[6]);
    boottag++;

    /* Normal Non-Cacheable (normal=1, cacheable=0). The GPU pool and
     * framebuffer live here; NC lets CPU/NEON writes write-combine into
     * bursts instead of one Device access per store. Still uncached, so it
     * stays coherent with the GPU/DMA/HVS uncached-alias reads; the write
     * buffer is drained by the dsb already issued at every GPU submit /
     * DMA kick before the engine reads.
     *
     * Clip at __arm_periiobase: firmware reports the VC region running up to
     * 0x40000000, which would otherwise overwrite the Device-nGnRnE mapping
     * of the peripheral window (0x3F000000-0x3FFFFFFF) with Normal-NC and
     * corrupt MMIO register accesses on real hardware. */
    uint32_t vc_base = AROS_LE2LONG(vc_msg[5]);
    uint32_t vc_len  = AROS_LE2LONG(vc_msg[6]);

    if (vc_base + vc_len > __arm_periiobase)
        vc_len = __arm_periiobase - vc_base;

    mmu_map_section(vc_base, vc_base, vc_len, 1, 0, 3, 0);
}

/* The firmware boots the ARM core in a temporary turbo window (e.g.
 * 1400 MHz on Pi 3B+) and drops it to the 600 MHz setpoint some tens of
 * seconds later; without an OS cpufreq governor it stays there. Request
 * the max rate once — the firmware still enforces its thermal and
 * undervolt limits on top of the setpoint. */
void setup_arm_clock()
{
    volatile unsigned int *vc_msg = (unsigned int *) BOOTMEMADDR(bm_mboxmsg);
    uint32_t arm_cur, arm_max;

    vc_msg[0] = AROS_LONG2LE(8 * 4);
    vc_msg[1] = AROS_LONG2LE(VCTAG_REQ);
    vc_msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    vc_msg[3] = AROS_LONG2LE(8);
    vc_msg[4] = AROS_LONG2LE(4);
    vc_msg[5] = AROS_LONG2LE(3);            /* clock id 3 = ARM */
    vc_msg[6] = 0;
    vc_msg[7] = 0;
    vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vc_msg);
    vc_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);
    if (!vc_msg)
        return;
    arm_cur = AROS_LE2LONG(vc_msg[6]);

    vc_msg[0] = AROS_LONG2LE(8 * 4);
    vc_msg[1] = AROS_LONG2LE(VCTAG_REQ);
    vc_msg[2] = AROS_LONG2LE(VCTAG_GETCLKMAX);
    vc_msg[3] = AROS_LONG2LE(8);
    vc_msg[4] = AROS_LONG2LE(4);
    vc_msg[5] = AROS_LONG2LE(3);
    vc_msg[6] = 0;
    vc_msg[7] = 0;
    vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vc_msg);
    vc_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);
    if (!vc_msg)
        return;
    arm_max = AROS_LE2LONG(vc_msg[6]);

    kprintf("[BOOT] ARM clock: setpoint %u Hz, max %u Hz\n", arm_cur, arm_max);

    if (arm_max && arm_cur < arm_max)
    {
        vc_msg[0] = AROS_LONG2LE(9 * 4);
        vc_msg[1] = AROS_LONG2LE(VCTAG_REQ);
        vc_msg[2] = AROS_LONG2LE(VCTAG_SETCLKRATE);
        vc_msg[3] = AROS_LONG2LE(12);
        vc_msg[4] = AROS_LONG2LE(12);
        vc_msg[5] = AROS_LONG2LE(3);
        vc_msg[6] = AROS_LONG2LE(arm_max);
        vc_msg[7] = 0;                      /* skip_setting_turbo = 0 */
        vc_msg[8] = 0;
        vcmb_write(VCMB_BASE, VCMB_PROPCHAN, (void *)vc_msg);
        vc_msg = vcmb_read(VCMB_BASE, VCMB_PROPCHAN);
        if (vc_msg)
            kprintf("[BOOT] ARM clock set to %u Hz\n", AROS_LE2LONG(vc_msg[6]));
    }
}

void query_memory()
{
    of_node_t *mem = dt_find_node("/memory");

    kprintf("[BOOT] Query system memory\n");
    if (mem)
    {
        of_property_t *p = dt_find_property(mem, "reg");

        if (p != NULL && p->op_length)
        {
            /* The number of 32-bit cells making up an address and a size is
               taken from the root node. The Pi 4 (BCM2711) uses 2 address
               cells (addresses exceed 32 bits), unlike the Pi 2/3. */
            of_node_t *rootn = dt_find_node("/");
            of_property_t *acp = dt_find_property(rootn, "#address-cells");
            of_property_t *scp = dt_find_property(rootn, "#size-cells");
            uint32_t ac = acp ? AROS_BE2LONG(*(uint32_t *)acp->op_value) : 1;
            uint32_t sc = scp ? AROS_BE2LONG(*(uint32_t *)scp->op_value) : 1;
            volatile uint32_t *addr = p->op_value;
            uint32_t cells = p->op_length / 4;
            uint32_t used = 0;
            int ranges = 0;

            /*
             * "reg" holds a list of (address, size) tuples, not just one: the
             * Pi 4 firmware describes anything past the first GB as a separate
             * entry, so a board with more than 1GB loses everything above it if
             * only the first tuple is read. Emit a tag pair per range - the
             * kernel builds one memory header from each.
             */
            while (used + ac + sc <= cells)
            {
                uint64_t base = 0, size = 0;
                uint32_t i;

                for (i = 0; i < ac; i++)
                    base = (base << 32) | AROS_BE2LONG(*addr++);
                for (i = 0; i < sc; i++)
                    size = (size << 32) | AROS_BE2LONG(*addr++);
                used += ac + sc;

                if (size == 0)
                    continue;

                uint64_t lower = base;
                uint64_t upper = base + size;

                kprintf("[BOOT] System memory range %d: %p-%p\n", ranges, lower, upper - 1);

                /* Only the range the kernel is placed in has to be sizeable. */
                if (ranges == 0 && ((upper - lower) >> 20) < 256) {
                    kprintf("[BOOT] MISMATCHED FILES: start.elf and fixup.dat do not fit to each other!\n");
                    while(1) asm volatile("wfi");
                }

                boottag->ti_Tag = KRN_MEMLower;
                if ((boottag->ti_Data = lower) < sizeof(struct bcm2708bootmem))
                    boottag->ti_Data = sizeof(struct bcm2708bootmem);

                boottag++;
                boottag->ti_Tag = KRN_MEMUpper;
                boottag->ti_Data = upper;

                /* The kernel is loaded at the top of the first range, and the
                   loader lowers that tag in place as it does so. */
                if (ranges == 0)
                    mem_upper = &boottag->ti_Data;

                boottag++;
                ranges++;

                mmu_map_section(lower, lower, upper - lower, 1, 1, 3, 1);
            }
        }
    }
}

void boot(uintptr_t dtb_addr, uintptr_t arch, uintptr_t dummy2, uintptr_t dummy3)
{
    uint64_t tmp;
    void (*entry)(struct TagItem *) = NULL;

    boottag = tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE;

    /*
     * At this point we are running at EL1.
     * Caches and branch prediction are already configured.
     * MMU is off (firmware starts kernel with MMU disabled in aarch64 mode).
     */

    /* Prepare MMU tables but do not load them yet */
    mmu_init();

    /* Initialize simplistic local memory allocator */
    mem_init();

    int dt_mem_usage = mem_avail();
    /* Parse device tree */
    dt_parse((void *)(uintptr_t)dtb_addr);
    dt_mem_usage -= mem_avail();

    /* Prepare mapping for peripherals. Use the data from device tree here */
    of_node_t *e = dt_find_node("/soc");
    if (e)
    {
        of_property_t *p = dt_find_property(e, "ranges");
        if (p == NULL)
            while(1) asm volatile("wfe");

        /* Each ranges entry is <child-address, parent-address, size>. The
           child and size widths come from /soc, the parent width from the
           root. On the Pi 4 the root uses 2 address cells (the peripheral
           window sits above 4 GB in the bus map), so an entry is 4 cells
           rather than the 3 of the Pi 2/3. Reading it with the wrong widths
           mis-maps the MMIO window and serial dies the moment the MMU loads. */
        of_node_t *rootn = dt_find_node("/");
        of_property_t *cacp = dt_find_property(e, "#address-cells");
        of_property_t *cscp = dt_find_property(e, "#size-cells");
        of_property_t *pacp = dt_find_property(rootn, "#address-cells");
        uint32_t child_ac = cacp ? AROS_BE2LONG(*(uint32_t *)cacp->op_value) : 1;
        uint32_t child_sc = cscp ? AROS_BE2LONG(*(uint32_t *)cscp->op_value) : 1;
        uint32_t parent_ac = pacp ? AROS_BE2LONG(*(uint32_t *)pacp->op_value) : 1;
        uint32_t entry_cells = child_ac + parent_ac + child_sc;

        volatile uint32_t *ranges = p->op_value;
        int32_t cells = p->op_length / 4;

        while (cells >= (int32_t)entry_cells)
        {
            uint64_t addr_bus = 0, addr_cpu = 0, addr_len = 0;

            for (uint32_t i = 0; i < child_ac; i++)
                addr_bus = (addr_bus << 32) | AROS_BE2LONG(*ranges++);
            for (uint32_t i = 0; i < parent_ac; i++)
                addr_cpu = (addr_cpu << 32) | AROS_BE2LONG(*ranges++);
            for (uint32_t i = 0; i < child_sc; i++)
                addr_len = (addr_len << 32) | AROS_BE2LONG(*ranges++);

            (void)addr_bus;

            /* If periiobase was not set yet, do it now */
            if (__arm_periiobase == 0) {
                __arm_periiobase = (uintptr_t)addr_cpu;
            }

            /* Prepare mapping - device type (MMIO must be Device-nGnRnE,
             * not Normal-NC, or write-gathering/reordering corrupts register
             * accesses on real hardware) */
            mmu_map_section(addr_cpu, addr_cpu, addr_len, 0, 0, 3, 0);

            cells -= entry_cells;
        }
    }
    else
        while(1) asm volatile("wfe");

    serInit();

    kprintf("\n\n[BOOT] AROS %s\n", bootstrapName);
    {
        of_node_t *root = dt_find_node("/");
        of_property_t *model = root ? dt_find_property(root, "model") : NULL;
        kprintf("[BOOT] Booted on %s\n", model ? (const char *)model->op_value : "unknown");
    }

    /*
     * Store the SoC id for the kernel's platform probe. The peripheral base
     * itself is taken from the device tree (/soc ranges) above, so it is
     * already correct for either SoC; here we only distinguish the interrupt
     * controller / timer family: BCM2836/2837 (Pi 2/3, legacy controller) vs
     * BCM2711 (Pi 4, GIC-400), by scanning the root "compatible" list.
     */
    boottag->ti_Tag = KRN_Platform;
    {
        of_node_t *r = dt_find_node("/");
        of_property_t *compat = r ? dt_find_property(r, "compatible") : NULL;
        IPTR plat = 0xc43; /* default: BCM2836/2837 (Raspberry Pi 2/3) */
        if (compat)
        {
            const char *s = (const char *)compat->op_value;
            int n = (int)compat->op_length, i;
            for (i = 0; i + 7 <= n; i++)
            {
                if (s[i] == 'b' && s[i+1] == 'c' && s[i+2] == 'm' &&
                    s[i+3] == '2' && s[i+4] == '7' && s[i+5] == '1' &&
                    s[i+6] == '1')
                {
                    plat = 0xc44; /* BCM2711 (Raspberry Pi 4) */
                    break;
                }
            }
        }
        boottag->ti_Data = plat;
        kprintf("[BOOT] SoC platform id 0x%x\n", (unsigned)plat);
    }
    boottag++;

    /*
        Check if device tree contains /soc/local_intc entry. In this case assume RasPi 2 or 3 with smp setup.
    */
    e = dt_find_node("/__symbols__");
    if (e)
    {
        of_property_t *p = dt_find_property(e, "local_intc");

        if (p)
        {
            kprintf("[BOOT] local_intc points to %s\n", p->op_value);

            e = dt_find_node(p->op_value);
            if (e)
                p = dt_find_property(e, "reg");
            else
                p = NULL;

            if (p)
            {
                uint32_t *reg = p->op_value;

                kprintf("[BOOT] Mapping local interrupt area at %p-%p\n", AROS_BE2LONG(reg[0]), AROS_BE2LONG(reg[0]) + AROS_BE2LONG(reg[1]) - 1);

                /* Prepare mapping - device type */
                uintptr_t local_base = AROS_BE2LONG(reg[0]);
                uintptr_t local_size = AROS_BE2LONG(reg[1]);
                if (local_size < 0x200000) local_size = 0x200000; /* Align to 2MB minimum */
                mmu_map_section(local_base, local_base, local_size, 0, 0, 3, 0);
            }
        }
    }

    /* Init LED(s) */
    e = dt_find_node("/leds");
    if (e)
    {
        of_node_t *led;
        kprintf("[BOOT] Configuring LEDs\n");
        ForeachNode(&e->on_children, led)
        {
            of_property_t *p = dt_find_property(led, "gpios");
            int32_t gpio = 0;
            if (p && p->op_length >= 8) {
                uint32_t *data = p->op_value;
                uint32_t phandle = AROS_BE2LONG(data[0]);
                gpio = AROS_BE2LONG(data[1]);
                of_node_t *bus = dt_find_node_by_phandle(phandle);

                kprintf("[BOOT] %s: GPIO%d (%08x %08x %08x)\n", led->on_name, gpio,
                    AROS_BE2LONG(data[0]), AROS_BE2LONG(data[1]), AROS_BE2LONG(data[2]));
                if (bus)
                {
                    kprintf("[BOOT] LED attached to %s\n", bus->on_name);

                    if (strncmp(bus->on_name, "gpio", 4) == 0)
                    {
                        int gpio_sel = gpio / 10;
                        int gpio_soff = 3 * (gpio - 10 * gpio_sel);

                        kprintf("[BOOT] GPFSEL=%x, bit=%d\n", gpio_sel * 4, gpio_soff);

                        /* Configure GPIO as output */
                        tmp = rd32le(GPFSEL0 + 4*gpio_sel);
                        tmp &= ~(7 << gpio_soff);
                        tmp |= (1 << gpio_soff);
                        wr32le(GPFSEL0 + 4*gpio_sel, tmp);

                        /* Turn LED off */
                        wr32le(GPCLR0 + 4 * (gpio / 32), 1 << (gpio % 32));
                    }
                }
            }
        }
    }

    kprintf("[BOOT] DT memory usage: %d\n", dt_mem_usage);

    boottag->ti_Tag = KRN_BootLoader;
    boottag->ti_Data = (IPTR)bootstrapName;
    boottag++;

    if (vcfb_init())
    {
        boottag->ti_Tag = KRN_FuncPutC;
        boottag->ti_Data = (IPTR)fb_Putc;
        boottag++;

        /* Hand the linear framebuffer to the kernel/graphics HIDD. The
           surface lives in the VideoCore region, which query_vmem() maps. */
        boottag->ti_Tag = KRN_FBAddr;
        boottag->ti_Data = (IPTR)vcfb_base;
        boottag++;
        boottag->ti_Tag = KRN_FrameBufferWidth;
        boottag->ti_Data = vcfb_width;
        boottag++;
        boottag->ti_Tag = KRN_FrameBufferHeight;
        boottag->ti_Data = vcfb_height;
        boottag++;
        boottag->ti_Tag = KRN_FrameBufferDepth;
        boottag->ti_Data = vcfb_depth;
        boottag++;
        boottag->ti_Tag = KRN_FrameBufferPitch;
        boottag->ti_Data = vcfb_pitch;
        boottag++;
    }

    DBOOT({
        kprintf("[BOOT] UART clock speed: %d\n", uartclock);
        kprintf("[BOOT] using %d.%d divisor for %d baud\n", uartdivint, uartdivfrac, uartbaud);

        asm volatile ("mrs %0, SCTLR_EL1" : "=r"(tmp));
        kprintf("[BOOT] SCTLR_EL1: %016lx\n", tmp);

        asm volatile ("mrs %0, MIDR_EL1" : "=r"(tmp));
        kprintf("[BOOT] MIDR_EL1 (main id register): %08x\n", (uint32_t)tmp);
    })

    query_memory();
    query_vmem();
    setup_arm_clock();

    kprintf("[BOOT] DTB @ %p, size %d\n", (void *)dtb_addr, dt_total_size());
    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);

    boottag->ti_Tag = KRN_ProtAreaStart;
    boottag->ti_Data = (IPTR)&__bootstrap_start;
    boottag++;

    boottag->ti_Tag = KRN_ProtAreaEnd;
    boottag->ti_Data = (IPTR)&__bootstrap_end;
    boottag++;

    kprintf("[BOOT] Topmost address for kernel: %p\n", *mem_upper);

    e = dt_find_node("/chosen");
    if (e)
    {
        /* linux,initrd-start/-end are sized like the root address cells: a
           single 32-bit cell on the Pi 2/3, but 64-bit on the Pi 4. Read the
           width the property actually carries. */
        /* DT property values are only 32-bit aligned. A 64-bit cell must be
           read as two big-endian words: the MMU is still off here, so memory
           is Device-typed and a 64-bit load on a 4-byte boundary faults. The
           volatile pointer stops the compiler folding the pair into one. */
        uintptr_t initrd_start = 0, initrd_end = 0;
        of_property_t *p = dt_find_property(e, "linux,initrd-start");
        if (p)
        {
            volatile uint32_t *v = p->op_value;
            initrd_start = (p->op_length >= 8)
                ? (((uintptr_t)AROS_BE2LONG(v[0]) << 32) | AROS_BE2LONG(v[1]))
                : (uintptr_t)AROS_BE2LONG(v[0]);
        }
        pkg_image = (void *)initrd_start;

        p = dt_find_property(e, "linux,initrd-end");
        if (p)
        {
            volatile uint32_t *v = p->op_value;
            initrd_end = (p->op_length >= 8)
                ? (((uintptr_t)AROS_BE2LONG(v[0]) << 32) | AROS_BE2LONG(v[1]))
                : (uintptr_t)AROS_BE2LONG(v[0]);
        }
        pkg_size = initrd_end - initrd_start;
    }

    kprintf("[BOOT] BSP image: %08x-%08x\n", pkg_image, (uintptr_t)pkg_image + pkg_size - 1);

    kprintf("[BOOT] mem_avail=%d\n", mem_avail());

    unsigned long total_size_ro = 0, total_size_rw = 0;
    void *fdt = NULL;

    if (mem_upper)
    {
        *mem_upper = *mem_upper & ~4095;

        unsigned long kernel_phys = *mem_upper;
        unsigned long kernel_virt = kernel_phys;

        uint32_t size_ro, size_rw;

        /* Calculate total size of kernel and modules */
        kprintf("[BOOT] Calculating core ELF size...\n");
        getElfSize(&_binary_core_bin_start, &size_rw, &size_ro);
        kprintf("[BOOT] Core: ro=%d rw=%d\n", size_ro, size_rw);

        total_size_ro = size_ro = (size_ro + 4095) & ~4095;
        total_size_rw = size_rw = (size_rw + 4095) & ~4095;

        kprintf("[BOOT] Scanning BSP package at %p (size %d)...\n", pkg_image, pkg_size);
        if (pkg_image && pkg_size)
        {
            uint8_t *base = pkg_image;

            kprintf("[BOOT] BSP magic: %02x %02x %02x %02x\n", base[0], base[1], base[2], base[3]);
            if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
            {
                getElfSize(base, &size_rw, &size_ro);

                total_size_ro += (size_ro + 4095) & ~4095;
                total_size_rw += (size_rw + 4095) & ~4095;
            }
            else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
            {
                uint8_t *file = base+4;
                uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file);
                const uint8_t *file_end = base+total_length;
                uint32_t len, cnt = 0;

                kprintf("[BOOT] PKG total_length=%d, base=%p, file_end=%p\n", total_length, base, file_end);

                file = base + 8;

                while(file < file_end)
                {
                    /* get text length */
                    len = AROS_BE2LONG(*(uint32_t*)file);
                    DBOOT(kprintf("[BOOT] PKG[%d] name_len=%d name='%s'\n", cnt, len, file+4));

                    file += len + 5;

                    len = AROS_BE2LONG(*(uint32_t *)file);
                    DBOOT(kprintf("[BOOT] PKG[%d] elf_len=%d elf@%p\n", cnt, len, file+4));
                    file += 4;

                    DBOOT(kprintf("[BOOT] ELF hdr: %02x %02x %02x %02x\n",
                        file[0], file[1], file[2], file[3]));

                    /* load it */
                    getElfSize(file, &size_rw, &size_ro);
                    DBOOT(kprintf("[BOOT] getElfSize returned ro=%d rw=%d\n", size_ro, size_rw));

                    total_size_ro += (size_ro + 4095) & ~4095;
                    total_size_rw += (size_rw + 4095) & ~4095;

                    /* go to the next file */
                    file += len;
                    cnt++;
                    if (cnt <= 3 || cnt % 50 == 0)
                        kprintf("[BOOT] PKG module %d: elf_ro=%d elf_rw=%d file=%p\n", cnt, size_ro, size_rw, file);
                }
                kprintf("[BOOT] PKG scan done: %d modules\n", cnt);
            }
        }

        /* Reserve space for flattened device tree */
        total_size_ro += (dt_total_size() + 31) & ~31;

        /* Reserve space for unpacked device tree */
        total_size_ro += (dt_mem_usage + 31) & ~31;

        /* Align space to 2MB boundary - it will save some space in MMU tables */
        total_size_ro = (total_size_ro + 2*1024*1024-1) & ~(2*1024*1024UL-1);
        total_size_rw = (total_size_rw + 2*1024*1024-1) & ~(2*1024*1024UL-1);

        kprintf("[BOOT] Total sizes: ro=%ld rw=%ld\n", total_size_ro, total_size_rw);

        kernel_phys = *mem_upper - total_size_ro - total_size_rw;
        kernel_virt = KERNEL_VIRT_ADDRESS;

        /*
         * The firmware may have placed the live DTB inside the region we
         * are about to claim and bzero (real Pi firmware puts it near the
         * top of ARM memory; QEMU puts it low). All parsed device-tree
         * data still points into the original blob, and the copy into the
         * kernel area happens after the bzero -- so move the kernel area
         * below the DTB instead of wiping it.
         */
        if (dt_total_size() > 0)
        {
            uintptr_t dtb_start = (uintptr_t)dtb_addr;
            uintptr_t dtb_end = dtb_start + dt_total_size();

            if (dtb_end > kernel_phys && dtb_start < (uintptr_t)*mem_upper)
            {
                kernel_phys = (dtb_start & ~(2*1024*1024UL - 1)) - total_size_ro - total_size_rw;
                kprintf("[BOOT] DTB overlaps kernel area, kernel moved down to %p\n", kernel_phys);
            }
        }

        kprintf("[BOOT] Zeroing kernel memory at %p (%ld bytes)...\n", kernel_phys, total_size_ro + total_size_rw);
        bzero((void*)kernel_phys, total_size_ro + total_size_rw);

        kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
        kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

        if (dt_total_size() > 0)
        {
            long dt_size = (dt_total_size() + 31) & ~31;
            /* Copy device tree to the end of kernel RO area */
            memcpy((void*)(kernel_phys + total_size_ro - dt_size), (void *)(uintptr_t)dtb_addr, dt_size);
            fdt = (void*)(kernel_virt + total_size_ro - dt_size);

            /* Store device tree */
            boottag->ti_Tag = KRN_FlattenedDeviceTree;
            boottag->ti_Data = (IPTR)kernel_virt + total_size_ro - dt_size;
            boottag++;

            kprintf("[BOOT] Device tree (size: %d) moved to %p, phys %p\n", dt_total_size(), boottag[-1].ti_Data, kernel_phys + total_size_ro - dt_size);
        }

        *mem_upper = kernel_phys;

        DBOOT(kprintf("[BOOT] Topmost memory address: %p\n", *mem_upper));

        /* Unmap memory at physical location of kernel. In future this has to be eventually changed */
        mmu_unmap_section(kernel_phys, total_size_ro + total_size_rw);

        /* map kernel memory - RW during boot (kernel will set RO later) */
        mmu_map_section(kernel_phys, kernel_virt, total_size_ro, 1, 1, 3, 1);
        mmu_map_section(kernel_phys + total_size_ro, kernel_virt + total_size_ro, total_size_rw, 1, 1, 3, 1);

        entry = (void (*)(struct TagItem *))kernel_virt;

        initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

        boottag->ti_Tag = KRN_KernelLowest;
        boottag->ti_Data = kernel_virt;
        boottag++;

        boottag->ti_Tag = KRN_KernelHighest;
        boottag->ti_Data = kernel_virt + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
        boottag++;

        boottag->ti_Tag = KRN_KernelPhysLowest;
        boottag->ti_Data = kernel_phys;
        boottag++;

        kprintf("[BOOT] Loading core ELF...\n");
        loadElf(&_binary_core_bin_start);
        kprintf("[BOOT] Core loaded. Loading BSP modules...\n");

        if (pkg_image && pkg_size)
        {
            uint8_t *base = pkg_image;

            if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
            {
                kprintf("[BOOT] Kernel image is ELF file\n");

                loadElf(base);
            }
            else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
            {
                kprintf("[BOOT] Kernel image is a package:\n");

                uint8_t *file = base+4;
                uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file);
                const uint8_t *file_end = base+total_length;
                uint32_t len, cnt = 0;

                kprintf("[BOOT] Package size: %dKB", total_length >> 10);

                file = base + 8;

                while(file < file_end)
                {
                    const char *filename = remove_path(file+4);

                    /* get text length */
                    len = AROS_BE2LONG(*(uint32_t*)file);
                    /* display the file name with its RO load address
                     * (bring-up diagnostic: lets a trap PC be mapped
                     * back to the owning module) */
                    kprintf("\n[BOOT]    %p %s", elf_get_ro_virt(), filename);

                    file += len + 5;

                    len = AROS_BE2LONG(*(uint32_t *)file);
                    file += 4;

                    /* load it */
                    loadElf(file);
                    DBOOT(kprintf("[BOOT] ELF loaded OK (cnt=%d)\n", cnt));

                    /* go to the next file */
                    file += len;
                    cnt++;
                    DBOOT(kprintf("[BOOT] next file=%p file_end=%p\n", file, file_end));
                }
                kprintf("\n[BOOT] All %d BSP modules loaded\n", cnt);
            }
        }

        kprintf("[BOOT] Flushing cache...\n");
        aarch64_flush_cache(kernel_phys, total_size_ro + total_size_rw);
        aarch64_icache_invalidate(kernel_phys, total_size_ro + total_size_rw);
        kprintf("[BOOT] Cache flushed\n");

        boottag->ti_Tag = KRN_KernelBss;
        boottag->ti_Data = (IPTR)tracker;
        boottag++;
    }

    kprintf("[BOOT] Loading MMU tables...\n");
    mmu_load();
    kprintf("[BOOT] MMU loaded\n");

    int memory_used = mem_used();

    /* If device tree is in high kernel memory then parse it again */
    if (dt_total_size() > 0)
    {
        void * dt_location = (void *)(KERNEL_VIRT_ADDRESS + total_size_ro - ((dt_total_size() + 31) & ~31) - ((dt_mem_usage + 31) & ~31));
        kprintf("[BOOT] Creating device tree in kernel area at %p\n", dt_location);

        explicit_mem_init(dt_location, (dt_mem_usage + 31) & ~31);
        dt_parse(fdt);

        boottag->ti_Tag = KRN_OpenFirmwareTree;
        boottag->ti_Data = (IPTR)dt_location;
        boottag++;

        e = dt_find_node("/chosen");
        if (e)
        {
            of_property_t *p = dt_find_property(e, "bootargs");
            if (p)
            {
                boottag->ti_Tag = KRN_CmdLine;
                boottag->ti_Data = (IPTR)p->op_value;
                boottag++;

                kprintf("[BOOT] Kernel parameters @ %p: %s\n", p->op_value, p->op_value);
            }
        }
    }

    boottag->ti_Tag = TAG_DONE;
    boottag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)boottag - (intptr_t)(tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE))/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", memory_used);

    /* entry is only assigned when /memory was found and the kernel was
     * loaded; jumping through an uninitialized pointer turns a bad DTB
     * into a silent wild branch */
    if (!entry)
    {
        kprintf("[BOOT] FATAL: no kernel entry point (missing /memory node or kernel image?)\n");
        while (1)
            asm volatile("wfe");
    }

    kprintf("[BOOT] Heading over to AROS kernel @ %p\n", entry);

    entry((struct TagItem *)(tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE));

    kprintf("[BOOT] Back? Something wrong happened...\n");

    while(1) asm volatile("wfe");
}
