/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
 */

#include <exec/types.h>
#include <aros/macros.h>
#include <inttypes.h>
#include <asm/cpu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <aros/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/videocore.h>

#include "boot.h"
#include "serialdebug.h"
#include "mmu.h"
#include "atags.h"
#include "vc_mb.h"
#include "vc_fb.h"
#include "elf.h"
#include "devicetree.h"
#include "io.h"

#define DBOOT(x) x

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)

uint32_t __arm_periiobase = 0;

extern void mem_init(void);
extern unsigned int uartclock;
extern unsigned int uartdivint;
extern unsigned int uartdivfrac;
extern unsigned int uartbaud;

asm("   .section .aros.startup      \n"
"       .globl bootstrap            \n"
"       .type bootstrap,%function   \n"
"bootstrap:                         \n"
"       mrs     r4, cpsr_all        \n" /* Check if in hypervisor mode */
"       and     r4, r4, #0x1f       \n"
"       mov     r8, #0x1a           \n"
"       cmp     r4, r8              \n"
"       beq     leave_hyper         \n"
"continue_boot:                     \n"
"       cps     #0x13               \n" /* Should be in SVC (supervisor) mode already, but just incase.. */
"       setend  be                  \n" /* Switch to big endian mode */
"       ldr     sp, tmp_stack_ptr   \n"
"       b       boot                \n"
"leave_hyper:                       \n"
"       setend  be                  \n"
"       ldr     r4, =continue_boot  \n"
"       msr     ELR_hyp, r4         \n"
"       mrs     r4, cpsr_all        \n"
"       and     r4, r4, #0x1f       \n"
"       orr     r4, r4, #0x13       \n"
"       msr     SPSR_hyp, r4        \n"
"       eret                        \n"
"       .section .text              \n"
".byte 0                            \n"
".string \"$VER: arosraspi-be.img v40.46 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup"))) = (void *)(0x1000 - 16);
static struct TagItem *boottag;
static unsigned long *mem_upper;
static void *pkg_image = NULL;
static uint32_t pkg_size = 0;

struct tag;

static const char bootstrapName[] = "Bootstrap/ARM v7-a BigEndian";

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

    kprintf("[BOOT] Base = %08x, Size = %08x\n", AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[6]));

    boottag->ti_Tag = KRN_VMEMLower;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]);
    boottag++;

    boottag->ti_Tag = KRN_VMEMUpper;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]) + AROS_LE2LONG(vc_msg[6]);
    boottag++;

    mmu_map_section(AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[6]), 1, 0, 3, 0);
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
            uint32_t *addr = p->op_value;
            uint32_t lower = AROS_BE2LONG(*addr++);
            uint32_t upper = AROS_BE2LONG(*addr++);

            kprintf("[BOOT] System memory range: %08x-%08x\n", lower, upper-1);

            boottag->ti_Tag = KRN_MEMLower;
            if ((boottag->ti_Data = lower) < sizeof(struct bcm2708bootmem))
                boottag->ti_Data = sizeof(struct bcm2708bootmem); // Skip the *reserved* space for the cpu vectors/boot tmp stack/kernel private data.

            boottag++;
            boottag->ti_Tag = KRN_MEMUpper;
            boottag->ti_Data = upper;

            mem_upper = &boottag->ti_Data;

            boottag++;

            mmu_map_section(lower, lower, upper - lower, 1, 1, 3, 1);
        }
    }
}

void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags, uintptr_t a)
{
    uint32_t tmp, initcr;
    void (*entry)(struct TagItem *);

    boottag = tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE;

    /*
     * Disable MMU, enable caches and branch prediction. Also enabled unaligned memory
     * access. Exceptions are set to run in big-endian mode and this is the mode
     * in which page tables are written.
     */
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(initcr));
    tmp = initcr;
    tmp &= ~1;                                  /* Disable MMU */
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);    /* I and D caches, branch prediction */
    tmp = (tmp & ~2) | (1 << 22);               /* Unaligned access enable */
    tmp |= (1 << 25);                           /* EE bit for exceptions set - big endian */
                                                /* This bit sets also endianess of page tables */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));

    /* Prepare MMU tables but do not load them yet */
    mmu_init();

    /* Initialize simplistic local memory allocator */
    mem_init();

    int dt_mem_usage = mem_avail();
    /* Parse device tree */
    dt_parse(atags);
    dt_mem_usage -= mem_avail();

    /* Prepare mapping for peripherals. Use the data from device tree here */
    of_node_t *e = dt_find_node("/soc");
    if (e)
    {
        of_property_t *p = dt_find_property(e, "ranges");
        uint32_t *ranges = p->op_value;
        int32_t len = p->op_length;

        while(len > 0)
        {
            uint32_t addr_bus, addr_cpu;
            uint32_t addr_len;

            addr_bus = AROS_BE2LONG(*ranges++);
            addr_cpu = AROS_BE2LONG(*ranges++);
            addr_len = AROS_BE2LONG(*ranges++);

            (void)addr_bus;

            /* If periiobase was not set yet, do it now */
            if (__arm_periiobase == 0) {
                __arm_periiobase = (uint32_t)addr_cpu;
            }

            /* Prepare mapping - device type */
            mmu_map_section(addr_cpu, addr_cpu, addr_len, 1, 0, 3, 0);

            len -= 12;
        }
    }
    else
        while(1) asm volatile("wfe");

    serInit();

    kprintf("\n\n[BOOT] Big-Endian AROS %s\n", bootstrapName);
    kprintf("[BOOT] Booted on %s\n", dt_find_property(dt_find_node("/"), "model")->op_value);

    /* first of all, store the arch for the kernel to use .. */
    boottag->ti_Tag = KRN_Platform;
    boottag->ti_Data = (IPTR)arch;
    boottag++;

    /*
        Check if device tree contains /soc/local_intc entry. In this case assume RasPi 2 or 3 with smp setup.
        Neither PiZero nor classic Pi provide this entry.
    */
    e = dt_find_node("/__symbols__");
    if (e)
    {
        of_property_t *p = dt_find_property(e, "local_intc");

        if (p)
        {
            kprintf("[BOOT] local_intc points to %s\n", p->op_value);

            e = dt_find_node(p->op_value);
            p = dt_find_property(e, "reg");
            uint32_t *reg = p->op_value;

            kprintf("[BOOT] Mapping local interrupt area at %p-%p\n", reg[0], reg[0] + reg[1] - 1);

            /* Prepare mapping - device type */
            mmu_map_section(reg[0], reg[0], reg[1] < 0x100000 ? 0x100000 : reg[1], 0, 0, 3, 0);
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
                        tmp &= ~(7 << gpio_soff); // GPIO 47 = 001 - output
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
    }

    DBOOT({
        kprintf("[BOOT] UART clock speed: %d\n", uartclock);
        kprintf("[BOOT] using %d.%d divisor for %d baud\n", uartdivint, uartdivfrac, uartbaud);

        asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
        kprintf("[BOOT] control register init:%08x, now:%08x\n", initcr, tmp);

        asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r"(tmp));
        kprintf("[BOOT] main id register: %08x\n", tmp);
    })

    query_memory();
    query_vmem();

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
        of_property_t *p = dt_find_property(e, "linux,initrd-start");
        if (p)
            pkg_image = (void*)(*((uint32_t *)p->op_value));
        else
            pkg_image = NULL;

        p = dt_find_property(e, "linux,initrd-end");
        if (p)
            pkg_size = *((uint32_t *)p->op_value) - (uint32_t)pkg_image;
        else
            pkg_size = 0;
    }

    kprintf("[BOOT] BSP image: %08x-%08x\n", pkg_image, (int32_t)pkg_image + pkg_size - 1);

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
        getElfSize(&_binary_core_bin_start, &size_rw, &size_ro);

        total_size_ro = size_ro = (size_ro + 4095) & ~4095;
        total_size_rw = size_rw = (size_rw + 4095) & ~4095;

        if (pkg_image && pkg_size)
        {
            uint8_t *base = pkg_image;

            if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
            {
                getElfSize(base, &size_rw, &size_ro);

                total_size_ro += (size_ro + 4095) & ~4095;
                total_size_rw += (size_rw + 4095) & ~4095;
            }
            else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
            {
                uint8_t *file = base+4;
                uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
                const uint8_t *file_end = base+total_length;
                uint32_t len, cnt = 0;

                file = base + 8;

                while(file < file_end)
                {
                    //const char *filename = remove_path(file+4);

                    /* get text length */
                    len = AROS_BE2LONG(*(uint32_t*)file);

                    file += len + 5;

                    len = AROS_BE2LONG(*(uint32_t *)file);
                    file += 4;

                    /* load it */
                    getElfSize(file, &size_rw, &size_ro);

                    total_size_ro += (size_ro + 4095) & ~4095;
                    total_size_rw += (size_rw + 4095) & ~4095;

                    /* go to the next file */
                    file += len;
                    cnt++;
                }
            }
        }

        /* Reserve space for flattened device tree */
        total_size_ro += (dt_total_size() + 31) & ~31;

        /* Reserve space for unpacked device tree */
        total_size_ro += (dt_mem_usage + 31) & ~31;

        /* Align space to 1MB boundary - it will save some space in MMU tables */
        total_size_ro = (total_size_ro + 1024*1024-1) & 0xfff00000;
        total_size_rw = (total_size_rw + 1024*1024-1) & 0xfff00000;

        kernel_phys = *mem_upper - total_size_ro - total_size_rw;
        kernel_virt = KERNEL_VIRT_ADDRESS;

        kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
        kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

        if (dt_total_size() > 0)
        {
            long dt_size = (dt_total_size() + 31) & ~31;
            /* Copy device tree to the end of kernel RO area */
            memcpy((void*)(kernel_phys + total_size_ro - dt_size), atags, dt_size);
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

        /* map kernel memory for user access */
        mmu_map_section(kernel_phys, kernel_virt, total_size_ro, 1, 1, 2, 1);
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

        loadElf(&_binary_core_bin_start);

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
                uint32_t total_length = AROS_BE2LONG(*(uint32_t*)file); /* Total length of the module */
                const uint8_t *file_end = base+total_length;
                uint32_t len, cnt = 0;

                kprintf("[BOOT] Package size: %dKB", total_length >> 10);

                file = base + 8;

                while(file < file_end)
                {
                    const char *filename = remove_path(file+4);

                    /* get text length */
                    len = AROS_BE2LONG(*(uint32_t*)file);
                    /* display the file name */
                    if (cnt % 4 == 0)
                        kprintf("\n[BOOT]    %s", filename);
                    else
                        kprintf(", %s", filename);

                    file += len + 5;

                    len = AROS_BE2LONG(*(uint32_t *)file);
                    file += 4;

                    /* load it */
                    loadElf(file);

                    /* go to the next file */
                    file += len;
                    cnt++;
                }
                kprintf("\n");
            }
        }

        arm_flush_cache(kernel_phys, total_size_ro + total_size_rw);

        boottag->ti_Tag = KRN_KernelBss;
        boottag->ti_Data = (IPTR)tracker;
        boottag++;
    }

    mmu_load();

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
    }

    boottag->ti_Tag = TAG_DONE;
    boottag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)boottag - (intptr_t)(tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE))/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", memory_used);

    kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);

    entry((struct TagItem *)(tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE));

    kprintf("[BOOT] Back? Something wrong happened...\n");

    while(1) asm volatile("wfe");
}
