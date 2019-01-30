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
#include "elf.h"

#define DBOOT(x) x

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)

uint32_t __arm_periiobase;

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
".string \"$VER: arosraspi.img v40.46 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup"))) = (void *)(0x1000 - 16);
static struct TagItem *boottag;
static unsigned long *mem_upper;
static void *pkg_image;
static uint32_t pkg_size;

struct tag;

static const char bootstrapName[] = "Bootstrap/ARM BCM2708";

static void parse_atags(struct tag *tags)
{
    struct tag *t = NULL;

    kprintf("[BOOT] Parsing ATAGS\n");

    for_each_tag(t, tags)
    {
        kprintf("[BOOT]   %08x: ", AROS_LE2LONG(t->hdr.tag));
        switch (AROS_LE2LONG(t->hdr.tag))
        {
            case 0:
                kprintf("ATAG_NONE - Ignored\n");
                break;

            case ATAG_CORE:
                kprintf("ATAG_CORE - Ignored\n");
                break;

            case ATAG_MEM:
                kprintf("ATAG_MEM (%08x-%08x)\n", AROS_LE2LONG(t->u.mem.start), AROS_LE2LONG(t->u.mem.size) + AROS_LE2LONG(t->u.mem.start) - 1);
                boottag->ti_Tag = KRN_MEMLower;
                if ((boottag->ti_Data = AROS_LE2LONG(t->u.mem.start)) < sizeof(struct bcm2708bootmem))
                    boottag->ti_Data = sizeof(struct bcm2708bootmem); // Skip the *reserved* space for the cpu vectors/boot tmp stack/kernel private data.

                boottag++;
                boottag->ti_Tag = KRN_MEMUpper;
                boottag->ti_Data = AROS_LE2LONG(t->u.mem.start) + AROS_LE2LONG(t->u.mem.size);

                mem_upper = &boottag->ti_Data;

                boottag++;

                mmu_map_section(AROS_LE2LONG(t->u.mem.start), AROS_LE2LONG(t->u.mem.start), AROS_LE2LONG(t->u.mem.size), 1, 1, 3, 1);

                break;

            case ATAG_VIDEOTEXT:
                kprintf("ATAG_VIDEOTEXT - Ignored\n");
                break;

            case ATAG_RAMDISK:
                kprintf("ATAG_RAMDISK - Ignored\n");
                break;

            case ATAG_INITRD2:
                kprintf("ATAG_INITRD2 (%08x-%08x)\n", AROS_LE2LONG(t->u.initrd.start), AROS_LE2LONG(t->u.initrd.size) + AROS_LE2LONG(t->u.initrd.start) - 1);
                pkg_image = (void *)AROS_LE2LONG(t->u.initrd.start);
                pkg_size = AROS_LE2LONG(t->u.initrd.size);
                break;

            case ATAG_SERIAL:
                kprintf("ATAG_SERIAL - Ignored\n");
                break;

            case ATAG_REVISION:
                kprintf("ATAG_REVISION - Ignored\n");
                break;

            case ATAG_VIDEOLFB:
                kprintf("ATAG_VIDEOLFB - Ignored\n");
                break;

            case ATAG_CMDLINE:
            {
                char *cmdline = malloc(strlen(t->u.cmdline.cmdline) + 1);
                strcpy(cmdline, t->u.cmdline.cmdline);
                kprintf("ATAG_CMDLINE \"%s\"\n", cmdline);

                boottag->ti_Tag = KRN_CmdLine;
                boottag->ti_Data = (intptr_t)cmdline;
                boottag++;
            }
            break;

            default:
                kprintf("(UNKNOWN)...\n");
                break;
        }
    }
}

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

    vcmb_write((void *)VCMB_BASE, VCMB_PROPCHAN, (void *)vc_msg);
    vc_msg = vcmb_read((void *)VCMB_BASE, VCMB_PROPCHAN);

    kprintf("[BOOT] Base = %08x, Size = %08x\n", AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[6]));

    boottag->ti_Tag = KRN_VMEMLower;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]);
    boottag++;

    boottag->ti_Tag = KRN_VMEMUpper;
    boottag->ti_Data = AROS_LE2LONG(vc_msg[5]) + AROS_LE2LONG(vc_msg[6]);
    boottag++;

    mmu_map_section(AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[5]), AROS_LE2LONG(vc_msg[6]), 1, 0, 3, 0);
}

void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags, uintptr_t a)
{
    uint32_t tmp, initcr;
    int plus_board = 0;
    void (*entry)(struct TagItem *);

    (void)entry;
    (void)plus_board;

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

    mmu_init();

    /*
        Check processor type - armv6 is old raspberry pi with SOC IO base at 0x20000000.
        armv7 will be raspberry pi 2 with SOC IO base at 0x3f000000
     */
    asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (tmp));

    tmp = (tmp >> 4) & 0xfff;

    /* tmp == 7 means armv6 architecture. */
    if (tmp == 0xc07) /* armv7, also RaspberryPi 2 */
    {
        __arm_periiobase = BCM2836_PERIPHYSBASE;
        plus_board = 1;

        /* Clear terminal screen */
        kprintf("\033[0H\033[0J");

        /* prepare map for core boot vector(s) */
        mmu_map_section(0x40000000, 0x40000000, 0x100000, 0, 0, 3, 0);
    }
    else
    {
        __arm_periiobase = BCM2835_PERIPHYSBASE;
        /* Need to detect the plus board here in order to control LEDs properly */
        
        kprintf("\033[0H\033[0J");
    }

    /* Prepare map for MMIO registers */
    mmu_map_section(__arm_periiobase, __arm_periiobase, ARM_PERIIOSIZE, 1, 0, 3, 0);

    mem_init();

    boottag = tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE;

    /* first of all, store the arch for the kernel to use .. */
    boottag->ti_Tag = KRN_Platform;
    boottag->ti_Data = (IPTR)arch;
    boottag++;

    /* Init LED */
    {
        if (plus_board)
        {
            /*
             * Either B+ or rpi2 board. Uses two leds (power and activity) on GPIOs
             * 47 and 35. Enable both leds as output and turn both of them off.
             *
             * The power led will be brought back up once AROS boots.
             */

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL4);
            tmp &= ~(7 << 21); // GPIO 47 = 001 - output
            tmp |= (1 << 21);
            *(volatile unsigned int *)GPFSEL4 = AROS_LONG2LE(tmp);

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL3);
            tmp &= ~(7 << 15); // GPIO 35 = 001 - output
            tmp |= (1 << 15);
            *(volatile unsigned int *)GPFSEL3 = AROS_LONG2LE(tmp);

            /* LEDS off */
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE((1 << (47-32)));
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE((1 << (35-32)));
        }
        else
        {
            /*
             * Classic rpi board has only one controlable LED - activity on GPIO 16. Turn it
             * off now, kernel.resource will bring it back later.
             */

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL1);
            tmp &= ~(7 << 18); // GPIO 16 = 001 - output
            tmp |= (1 << 18);
            *(volatile unsigned int *)GPFSEL1 = AROS_LONG2LE(tmp);

            *(volatile unsigned int *)GPSET0 = AROS_LONG2LE((1 << 16));
        }
    }

    serInit();

    boottag->ti_Tag = KRN_BootLoader;
    boottag->ti_Data = (IPTR)bootstrapName;
    boottag++;

#if 0
    if (vcfb_init())
    {
        boottag->ti_Tag = KRN_FuncPutC;
        boottag->ti_Data = (IPTR)fb_Putc;
        boottag++;
    }
#endif

    kprintf("[BOOT] Big-Endian AROS %s\n", bootstrapName);
    kprintf("[BOOT] Arguments: %08x, %08x, %08x, %08x\n", dummy, arch, atags, a);

    DBOOT({
        kprintf("[BOOT] UART clock speed: %d\n", uartclock);
        kprintf("[BOOT] using %d.%d divisor for %d baud\n", uartdivint, uartdivfrac, uartbaud);

        asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
        kprintf("[BOOT] control register init:%08x, now:%08x\n", initcr, tmp);

        asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r"(tmp));
        kprintf("[BOOT] main id register: %08x\n", tmp);
    })

    if (atags == NULL)
        atags = (void*)0x100;

    parse_atags(atags);
    query_vmem();

    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);

    boottag->ti_Tag = KRN_ProtAreaStart;
    boottag->ti_Data = (IPTR)&__bootstrap_start;
    boottag++;

    boottag->ti_Tag = KRN_ProtAreaEnd;
    boottag->ti_Data = (IPTR)&__bootstrap_end;
    boottag++;

    kprintf("[BOOT] Topmost address for kernel: %p\n", *mem_upper);

if (mem_upper)
    {
        *mem_upper = *mem_upper & ~4095;

        unsigned long kernel_phys = *mem_upper;
        unsigned long kernel_virt = kernel_phys;

        unsigned long total_size_ro, total_size_rw;
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

        total_size_ro = (total_size_ro + 1024*1024-1) & 0xfff00000;
        total_size_rw = (total_size_rw + 1024*1024-1) & 0xfff00000;

        kernel_phys = *mem_upper - total_size_ro - total_size_rw;
        kernel_virt = 0xf8000000;

        kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
        kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

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

                getElfSize(base, &size_rw, &size_ro);

                total_size_ro += (size_ro + 4095) & ~4095;
                total_size_rw += (size_rw + 4095) & ~4095;
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

                    total_size_ro += (size_ro + 4095) & ~4095;
                    total_size_rw += (size_rw + 4095) & ~4095;

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
while(1);
}