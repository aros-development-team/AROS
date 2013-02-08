/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
*/

#include <inttypes.h>
#include <asm/cpu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <string.h>
#include <stdlib.h>

#include "boot.h"
#include "serialdebug.h"
#include "bootconsole.h"
#include "atags.h"
#include "elf.h"

#include "vc_fb.h"

#define DBOOT(x)

extern void mem_init(void);
extern unsigned int uartclock;
extern unsigned int uartdivint;
extern unsigned int uartdivfrac;
extern unsigned int uartbaud;

static unsigned char __stack[BOOT_STACK_SIZE] __attribute__((used,aligned(16)));

asm("	.section .aros.startup		     \n"
"		.globl bootstrap	     \n"
"		.type bootstrap,%function    \n"
"bootstrap:				     \n"
"               cps     #0x13                \n" /* Should be in SVC (supervisor) mode already, but just incase.. */
"               ldr     sp, tmp_stack_ptr    \n"
"		b       boot		     \n"
);

static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup"))) = &__stack[BOOT_STACK_SIZE-16];

static struct TagItem tags[128];
static struct TagItem *tag;
static unsigned long mem_upper;
static void *pkg_image;
static uint32_t pkg_size;

static void parse_atags(struct tag *tags)
{
    struct tag *t = NULL;

    kprintf("[BOOT] Parsing ATAGS\n");

    for_each_tag(t, tags)
    {
        kprintf("[BOOT]   %08x: ", t->hdr.tag, t->hdr.size);
        switch (t->hdr.tag)
        {
        case 0:
            kprintf("ATAG_NONE - Ignored\n");
            break;

        case ATAG_CORE:
            kprintf("ATAG_CORE - Ignored\n");
            break;

        case ATAG_MEM:
            kprintf("ATAG_MEM (%08x-%08x)\n", t->u.mem.start, t->u.mem.size + t->u.mem.start - 1);
            tag->ti_Tag = KRN_MEMLower;
            tag->ti_Data = t->u.mem.start;
            if (tag->ti_Data == 0)
                tag->ti_Data = 0x1000; // Leave space for the cpu vectors.

            tag++;
            tag->ti_Tag = KRN_MEMUpper;
            tag->ti_Data = t->u.mem.start + t->u.mem.size;
            tag++;

            mem_upper = t->u.mem.start + t->u.mem.size;

            break;

        case ATAG_VIDEOTEXT:
            kprintf("ATAG_VIDEOTEXT - Ignored\n");
            break;

        case ATAG_RAMDISK:
            kprintf("ATAG_RAMDISK - Ignored\n");
            break;

        case ATAG_INITRD2:
            kprintf("ATAG_INITRD2 (%08x-%08x)\n", t->u.initrd.start, t->u.initrd.size + t->u.initrd.start - 1);
            pkg_image = (void *)t->u.initrd.start;
            pkg_size = t->u.initrd.size;
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

                tag->ti_Tag = KRN_CmdLine;
                tag->ti_Data = (intptr_t)cmdline;
                tag++;
            }
            break;

        default:
            kprintf("(UNKNOWN)...\n");
            break;
        }
    }
}

static const char bootstrapName[] = "Bootstrap/RasPI ARM";

volatile unsigned int *gpioGPFSEL1 = (unsigned int *) 0x20200004;
volatile unsigned int *gpioGPSET0 = (unsigned int *) 0x2020001c;
volatile unsigned int *gpioGPPUD = (unsigned int *) 0x20200094;
volatile unsigned int *gpioPUDCLK0 = (unsigned int *) 0x20200098;
volatile unsigned int *gpioPUDCLK1 = (unsigned int *) 0x2020009c;

void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags)
{
    uint32_t tmp, initcr;
    void (*entry)(struct TagItem *tags);

    /* Enable unaligned memory access */
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    initcr = tmp;
    tmp |= 1 << 22;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(tmp));

    mem_init();

    tag = &tags[0];

    /* Init LED */
    {
	unsigned int delay;

        tmp = *gpioGPFSEL1;
        tmp &= ~(7<<18); // GPIO 16 = 001 - output
        tmp |= 1<<18;
        tmp &= ~(7<<12); // GPIO 14 = 000 - input
        *gpioGPFSEL1 = tmp;

        *gpioGPPUD = 2; // enable pull-up control
        delay = 150; // wait at least 150 cycles
	while(delay--)
            asm ("mov r0, r0");

        *gpioPUDCLK0 = 1<<14; // set Pull Up/Down CLocK0 for pin 14
        *gpioPUDCLK1 = 0;
        delay = 150; // wait at least 150 cycles
	while(delay--)
            asm ("mov r0, r0");

        *gpioGPPUD = 0; // disable pull-up control
        *gpioPUDCLK0 = 0; // reset the clock registers
        *gpioPUDCLK1 = 0;
        
        *gpioGPSET0 = 1<<16; // turn it off.. kernel.resource will turn it back on.
    }
    serInit();

    tag->ti_Tag = KRN_BootLoader;
    tag->ti_Data = (IPTR)bootstrapName;
    tag++;

    if (vcfb_init())
    {
        tag->ti_Tag = KRN_FuncPutC;
        tag->ti_Data = fb_Putc;
        tag++;
    }

    kprintf("[BOOT] AROS %s\n", bootstrapName);

    kprintf("[BOOT] UART clock speed: %d\n", uartclock);
    kprintf("[BOOT] using %d.%d divisor for %d baud\n", uartdivint, uartdivfrac, uartbaud);
    
    DBOOT(asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    kprintf("[BOOT] control register init:%08x, now:%08x\n", initcr, tmp);)

    parse_atags(atags);

    kprintf("[BOOT] Bootstrap @ %08x-%08x\n", &__bootstrap_start, &__bootstrap_end);

    tag->ti_Tag = KRN_ProtAreaStart;
    tag->ti_Data = &__bootstrap_start;
    tag++;

    tag->ti_Tag = KRN_ProtAreaEnd;
    tag->ti_Data = &__bootstrap_end;
    tag++;

    kprintf("[BOOT] Topmost address for kernel: %p\n", mem_upper);

    if (mem_upper)
    {
    	mem_upper = mem_upper & ~4095;

    	unsigned long kernel_phys = mem_upper;
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
    				getElfSize(file, &size_rw, &size_ro);

    				total_size_ro += (size_ro + 4095) & ~4095;
    				total_size_rw += (size_rw + 4095) & ~4095;

    				/* go to the next file */
    				file += len;
                                cnt++;
    			}
                        kprintf("\n");
    		}
    	}

    	kernel_phys = mem_upper - total_size_ro - total_size_rw;
    	kernel_virt = kernel_phys;

    	kprintf("[BOOT] Physical address of kernel: %p\n", kernel_phys);
    	kprintf("[BOOT] Virtual address of kernel: %p\n", kernel_virt);

    	entry = (void (*)(struct TagItem))kernel_virt;

        initAllocator(kernel_phys, kernel_phys  + total_size_ro, kernel_virt - kernel_phys);

    	tag->ti_Tag = KRN_KernelLowest;
    	tag->ti_Data = kernel_phys;
    	tag++;

    	tag->ti_Tag = KRN_KernelHighest;
    	tag->ti_Data = kernel_phys + ((total_size_ro + 4095) & ~4095) + ((total_size_rw + 4095) & ~4095);
    	tag++;

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

        tag->ti_Tag = KRN_KernelBss;
        tag->ti_Data = (IPTR)tracker;
        tag++;
    }

    tag->ti_Tag = TAG_DONE;
    tag->ti_Data = 0;

    kprintf("[BOOT] Kernel taglist contains %d entries\n", ((intptr_t)tag - (intptr_t)tags)/sizeof(struct TagItem));
    kprintf("[BOOT] Bootstrap wasted %d bytes of memory for kernels use\n", mem_used()   );

    kprintf("[BOOT] Heading over to AROS kernel @ %08x\n", entry);

    entry(tags);

    kprintf("[BOOT] Back? Something wrong happened...\n");

    while(1);
}
