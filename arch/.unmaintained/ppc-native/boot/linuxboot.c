/*
 *  linux/arch/m68k/boot/amiga/linuxboot.c -- Generic routine to boot Linux/m68k
 *					      on Amiga, used by both Amiboot and
 *					      Amiga-Lilo.
 *
 *	Created 1996 by Geert Uytterhoeven
 *
 *
 *  This file is based on the original bootstrap code (bootstrap.c):
 *
 *	Copyright (C) 1993, 1994 Hamish Macdonald
 *				 Greg Harp
 *
 *		    with work by Michael Rausch
 *				 Geert Uytterhoeven
 *				 Frank Neumann
 *				 Andreas Schwab
 *				 Jesper Skov
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 *
 *  History:
 *	26 Feb 1998 Added support for booting APUS systems.
 *	27 Mar 1997 FPU-less machines couldn't boot kernels that use bootinfo
 *		    interface version 1.0 (Geert)
 *	03 Feb 1997 Implemented kernel decompression (Geert, based on Roman's
 *		    code for ataboot)
 *	30 Dec 1996 Reverted the CPU detection to the old scheme
 *		    New boot parameter override scheme (Geert)
 *      27 Nov 1996 Compatibility with bootinfo interface version 1.0 (Geert)
 *       9 Sep 1996 Rewritten option parsing
 *		    New parameter passing to linuxboot() (linuxboot_args)
 *		    (Geert)
 *	18 Aug 1996 Updated for the new boot information structure (Geert)
 *	10 Jan 1996 The real Linux/m68k boot code moved to linuxboot.[ch]
 *		    (Geert)
 *	11 Jul 1995 Support for ELF kernel (untested!) (Andreas)
 *	 7 Mar 1995 Memory block sizes are rounded to a multiple of 256K
 *		    instead of 1M (Geert)
 *	31 May 1994 Memory thrash problem solved (Geert)
 *	11 May 1994 A3640 MapROM check (Geert)
 */


#ifndef __GNUC__
#error GNU CC is required to compile this program
#endif /* __GNUC__ */


#define BOOTINFO_COMPAT_1_0	/* bootinfo interface version 1.0 compatible */
/* support compressed kernels? */
#define ZKERNEL

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include <linux/a.out.h>
#include <linux/elf.h>
#include <linux/linkage.h>
#include <asm/bootinfo.h>
#include <asm/amigahw.h>
#include <asm/page.h>

#include "linuxboot.h"

#define APUS
#ifdef APUS

#define INTUITION_CLASSES_H 1

#include <powerup/ppclib/interface.h>
/*#include <powerup/gcclib/powerup_protos.h>*/
#include <powerup/ppclib/message.h>
#include <powerup/ppclib/tasks.h>
#include <powerup/ppclib/object.h>
#include "ppcboot_elf.h"

#endif

#undef custom
#define custom ((*(volatile struct CUSTOM *)(CUSTOM_PHYSADDR)))

/* temporary stack size */
#define TEMP_STACKSIZE	(256)

#define DEFAULT_BAUD	(9600)

extern char copyall, copyallend;

extern char getvbr, wedgie;

static struct exec kexec;
static Elf32_Ehdr kexec_elf;
static const struct linuxboot_args *linuxboot_args;

/* Bootinfo */
struct amiga_bootinfo bi;

#ifdef BOOTINFO_COMPAT_1_0
static struct compat_bootinfo compat_bootinfo;
#endif /* BOOTINFO_COMPAT_1_0 */

#define MAX_BI_SIZE	(4096)
static u_long bi_size;
static union {
    struct bi_record record;
    u_char fake[MAX_BI_SIZE];
} bi_union;

#define kernelname	linuxboot_args->kernelname
#define ramdiskname	linuxboot_args->ramdiskname
#define debugflag	linuxboot_args->debugflag
#define keep_video	linuxboot_args->keep_video
#define reset_boards	linuxboot_args->reset_boards
#define baud		linuxboot_args->baud

#define apus_boot	linuxboot_args->apus_boot
#define checksum	linuxboot_args->checksum

#define Puts		linuxboot_args->puts
#define GetChar		linuxboot_args->getchar
#define PutChar		linuxboot_args->putchar
#define Printf		linuxboot_args->printf
#define Open		linuxboot_args->open
#define Seek		linuxboot_args->seek
#define Read		linuxboot_args->read
#define Close		linuxboot_args->close
#define FileSize	linuxboot_args->filesize
#define Sleep		linuxboot_args->sleep

    /*
     *  Function Prototypes
     */

static u_long get_chipset(void);
static void get_processor(u_long *cpu, u_long *fpu, u_long *mmu);
static u_long get_model(u_long chipset);
static int probe_resident(const char *name);
static int probe_resource(const char *name);
static int create_bootinfo(void);
#ifdef BOOTINFO_COMPAT_1_0
static int create_compat_bootinfo(void);
#endif /* BOOTINFO_COMPAT_1_0 */
static int add_bi_record(u_short tag, u_short size, const void *data);
static int add_bi_string(u_short tag, const u_char *s);
static int check_bootinfo_version(const char *memptr);
static void start_kernel(void (*startfunc)(), char *stackp, char *memptr,
			 u_long start_mem, u_long mem_size, u_long rd_size,
			 u_long kernel_size) __attribute__ ((noreturn));
asmlinkage u_long probe_powerup(u_long);
asmlinkage u_long maprommed(void);
asmlinkage u_long check346(void);
#ifdef ZKERNEL
static int load_zkernel(int fd);
static int KRead(int fd, void *buf, int cnt);
static int KSeek(int fd, int offset);
static int KClose(int fd);
#else
#define KRead		Read
#define KSeek		Seek
#define KClose		Close
#endif


    /*
     *	Reset functions for nasty Zorro boards
     */

static void reset_rb3(const struct ConfigDev *cd);
static void reset_piccolo(const struct ConfigDev *cd);
static void reset_sd64(const struct ConfigDev *cd);
static void reset_a2065(const struct ConfigDev *cd);
static void reset_ariadne(const struct ConfigDev *cd);
static void reset_hydra(const struct ConfigDev *cd);
#if 0
static void reset_a2060(const struct ConfigDev *cd);
#endif

struct boardreset {
    u_short manuf;
    u_short prod;
    const char *name;
    void (*reset)(const struct ConfigDev *cd);
};

static struct boardreset boardresetdb[] = {
    { MANUF_HELFRICH1, PROD_RAINBOW3, "Rainbow 3", reset_rb3 },
    { MANUF_HELFRICH2, PROD_PICCOLO_REG, "Piccolo", reset_piccolo },
    { MANUF_HELFRICH2, PROD_SD64_REG, "SD64", reset_sd64 },
    { MANUF_COMMODORE, PROD_A2065, "A2065", reset_a2065 },
    { MANUF_VILLAGE_TRONIC, PROD_ARIADNE, "Ariadne", reset_ariadne },
    { MANUF_HYDRA_SYSTEMS, PROD_AMIGANET, "Hydra", reset_hydra },
#if 0
    { MANUF_COMMODORE, PROD_A2060, "A2060", reset_a2060 },
#endif
};
#define NUM_BOARDRESET	sizeof(boardresetdb)/sizeof(*boardresetdb)

static void (*boardresetfuncs[ZORRO_NUM_AUTO])(const struct ConfigDev *cd);


const char *amiga_models[] = {
    "Amiga 500", "Amiga 500+", "Amiga 600", "Amiga 1000", "Amiga 1200",
    "Amiga 2000", "Amiga 2500", "Amiga 3000", "Amiga 3000T", "Amiga 3000+",
    "Amiga 4000", "Amiga 4000T", "CDTV", "CD32", "Draco"
};
const u_long first_amiga_model = AMI_500;
const u_long last_amiga_model = AMI_DRACO;


#define MASK(model)	(1<<AMI_##model)

#define CLASS_A3000	(MASK(3000) | MASK(3000T))
#define CLASS_A4000	(MASK(4000) | MASK(4000T))
#define CLASS_ZKICK	(MASK(500) | MASK(1000) | MASK(2000) | MASK(2500))


    /*
     *	Boot the Linux/m68k Operating System
     */

u_long linuxboot(const struct linuxboot_args *args)
{
    int kfd = -1, rfd = -1, elf_kernel = 0, do_fast, do_chip;
    int i, j;
    const struct MemHeader *mnp;
    struct ConfigDev *cdp = NULL;
    char *memptr = NULL;
    u_long *stack = NULL;
    u_long fast_total, model_mask, startcodesize, start_mem, mem_size, rd_size;
    u_long kernel_size;
    u_int realbaud;
    u_long memreq = 0, text_offset = 0;
    Elf32_Phdr *kernel_phdrs = NULL;
    void (*startfunc)(void);
    u_short manuf;
    u_char prod;
    void *bi_ptr;
	unsigned long* info;
	struct ConfigDev *zdevs[ZORRO_NUM_AUTO];

	linuxboot_args = args;

    /* print the greet message */
    Puts("\nLinux/m68k Amiga Bootstrap version " AMIBOOT_VERSION "\n");
    Puts("Copyright 1993,1994 by Hamish Macdonald and Greg Harp\n\n");

    /* Note: Initial values in bi override detected values */
    bi = args->bi;

    /* machine is Amiga */
    bi.machtype = MACH_AMIGA;

    /* determine chipset */
    if (!bi.chipset)
	bi.chipset = get_chipset();

    /* determine CPU, FPU and MMU type */
    if (!bi.cputype)
	get_processor(&bi.cputype, &bi.fputype, &bi.mmutype);

    /* determine Amiga model */
    if (!bi.model)
	bi.model = get_model(bi.chipset);
    model_mask = (bi.model != AMI_UNKNOWN) ? 1<<bi.model : 0;

	/* Memory & AutoConfig based on 'unix_boot.c' by C= */

    /* find all of the autoconfig boards in the system */
    if (!bi.num_autocon)
	for (i = 0; (cdp = (struct ConfigDev *)FindConfigDev(cdp, -1, -1)); i++)
	    if (bi.num_autocon < ZORRO_NUM_AUTO) {
		zdevs[i] = cdp;
		/* copy the contents of each structure into our boot info and
		   count this device */
		memcpy(&bi.autocon[bi.num_autocon++], cdp,
		       sizeof(struct ConfigDev));
	    } else
		Printf("Warning: too many AutoConfig devices. Ignoring device "
		       "at 0x%08lx\n", cdp->cd_BoardAddr);

    do_fast = bi.num_memory ? 0 : 1;
    do_chip = bi.chip_size ? 0 : 1;
    /* find out the memory in the system */
    for (mnp = (struct MemHeader *)SysBase->MemList.lh_Head;
	 mnp->mh_Node.ln_Succ;
	 mnp = (struct MemHeader *)mnp->mh_Node.ln_Succ) {
	struct MemHeader mh;

	/* copy the information */
	mh = *mnp;

	/* skip virtual memory */
	if (!(mh.mh_Attributes & MEMF_PUBLIC))
	    continue;

	/* if we suspect that Kickstart is shadowed in an A3000,
	   modify the entry to show 512K more at the top of RAM
	   Check first for a MapROMmed A3640 board: overwriting the
	   Kickstart image causes an infinite lock-up on reboot! */
	if ((mh.mh_Upper == (void *)0x07f80000) &&
	    (model_mask & (CLASS_A3000 | CLASS_A4000)))
	    if ((bi.cputype & CPU_68040) && Supervisor(maprommed))
		Puts("A3640 MapROM detected.\n");
	    else if (model_mask & CLASS_A3000) {
		mh.mh_Upper = (void *)0x08000000;
		Puts("A3000 shadowed Kickstart detected.\n");
	    }

	/* if we suspect that Kickstart is zkicked,
	   modify the entry to show 512K more at the botton of RAM */
	if ((mh.mh_Lower == (void *)0x00280020) &&
	    (model_mask & CLASS_ZKICK)) {
	    mh.mh_Lower = (void *)0x00200000;
	    Puts("ZKick detected.\n");
	}

	/* mask the memory limit values */
	mh.mh_Upper = (void *)((u_long)mh.mh_Upper & 0xfffff000);
	mh.mh_Lower = (void *)((u_long)mh.mh_Lower & 0xfffff000);

	/* if fast memory */
	if (do_fast && mh.mh_Attributes & MEMF_FAST) {
	    /* set the size value to the size of this block and mask off to a
	       256K increment */
	    u_long size = ((u_long)mh.mh_Upper-(u_long)mh.mh_Lower)&0xfffc0000;
	    if (size > 0)
		if (bi.num_memory < NUM_MEMINFO) {
			/* record the start and size */
		    bi.memory[bi.num_memory].addr = (u_long)mh.mh_Lower;
		    bi.memory[bi.num_memory].size = size;
		    /* count this block */
		    bi.num_memory++;
		} else
		    Printf("Warning: too many memory blocks. Ignoring block "
		    	   "of %ldK at 0x%08x\n", size>>10,
			   (u_long)mh.mh_Lower);
	} else if (do_chip && mh.mh_Attributes & MEMF_CHIP)
	    /* if CHIP memory, record the size */
	    bi.chip_size = (u_long)mh.mh_Upper;
    }

    /* get info from ExecBase */
    if (!bi.vblank)
	bi.vblank = SysBase->VBlankFrequency;
    if (!bi.psfreq)
	bi.psfreq = SysBase->PowerSupplyFrequency;
    if (!bi.eclock)
	bi.eclock = SysBase->ex_EClockFrequency;

    /* serial port */
    if (!bi.serper) {
	realbaud = baud ? baud : DEFAULT_BAUD;
	bi.serper = (5*bi.eclock+realbaud/2)/realbaud-1;
    }

    /* display Amiga model */
    if (bi.model >= first_amiga_model && bi.model <= last_amiga_model)
	Printf("%s ", amiga_models[bi.model-first_amiga_model]);
    else
	Puts("Amiga ");

    /* display the CPU type */
    Puts("CPU: ");
    switch (bi.cputype) {
	case CPU_68020:
	    Puts("68020 (Do you have an MMU?)");
	    break;
	case CPU_68030:
	    Puts("68030");
	    break;
	case CPU_68040:
	    Puts("68040");
	    break;
	case CPU_68060:
	    Puts("68060");
	    break;
	default:
	    Puts("Insufficient for Linux.  Aborting...\n");
	    Printf("SysBase->AttnFlags = 0x%08lx\n", SysBase->AttnFlags);
	    goto Fail;
    }
    switch (bi.fputype) {
	case FPU_68881:
	    Puts(" with 68881 FPU");
	    break;
	case FPU_68882:
	    Puts(" with 68882 FPU");
	    break;
	case FPU_68040:
	case FPU_68060:
	    Puts(" with internal FPU");
	    break;
	default:
	    Puts(" without FPU");
	    break;
    }

    /* display the chipset */
    switch (bi.chipset) {
	case CS_STONEAGE:
	    Puts(", old or unknown chipset");
	    break;
	case CS_OCS:
	    Puts(", OCS");
	    break;
	case CS_ECS:
	    Puts(", ECS");
	    break;
	case CS_AGA:
	    Puts(", AGA chipset");
	    break;
    }

	/* Look for PowerUp PCI bridge. Signal result to kernel in model type.
	 * Not pretty, but it'll do for now.
	 */
	if (apus_boot) {
		unsigned long vbr;

		Puts("\n\n");
		vbr = (unsigned long) Supervisor((void*)&getvbr);
		if (probe_powerup(vbr)) {
  		    Puts("APUS: PCI Bridge detected.");
			bi.model += 0x100;
		} else {
  		    Puts("APUS: No PCI Bridge detected.");
		}
	}

	Puts("\n\n");

    /* display the command line */
    Printf("Command line is '%s'\n", bi.command_line);

    /* display the clock statistics */
    Printf("Vertical Blank Frequency: %ldHz\n", bi.vblank);
    Printf("Power Supply Frequency: %ldHz\n", bi.psfreq);
    Printf("EClock Frequency: %ldHz\n\n", bi.eclock);

    /* display autoconfig devices */
    if (bi.num_autocon) {
	Printf("Found %ld AutoConfig Device%s\n", bi.num_autocon,
	       bi.num_autocon > 1 ? "s" : "");
	for (i = 0; i < bi.num_autocon; i++) {
	    Printf("Device %ld: addr = 0x%08lx", i,
		   (u_long)zdevs[i]->cd_BoardAddr);
	    boardresetfuncs[i] = NULL;
	    if (reset_boards) {
		manuf = zdevs[i]->cd_Rom.er_Manufacturer;
		prod = zdevs[i]->cd_Rom.er_Product;
		for (j = 0; j < NUM_BOARDRESET; j++)
		    if ((manuf == boardresetdb[j].manuf) &&
			(prod == boardresetdb[j].prod)) {
			Printf(" [%s - will be reset at kernel boot time]",
			       boardresetdb[j].name);
			boardresetfuncs[i] = boardresetdb[j].reset;
			break;
		    }
	    }
	    PutChar('\n');
	}
    } else
	Puts("No AutoConfig Devices Found\n");

    /* display memory */
    if (bi.num_memory) {
	Printf("\nFound %ld Block%sof Memory\n", bi.num_memory,
	       bi.num_memory > 1 ? "s " : " ");
	for (i = 0; i < bi.num_memory; i++)
	    Printf("Block %ld: 0x%08lx to 0x%08lx (%ldK)\n", i,
		   bi.memory[i].addr, bi.memory[i].addr+bi.memory[i].size,
		   bi.memory[i].size>>10);
    } else {
	Puts("No memory found?!  Aborting...\n");
	goto Fail;
    }

    /* display chip memory size */
    Printf("%ldK of CHIP memory\n", bi.chip_size>>10);

    start_mem = bi.memory[0].addr;
    mem_size = bi.memory[0].size;

    /* tell us where the kernel will go */
    Printf("\nThe kernel will be located at 0x%08lx\n", start_mem);

    /* verify that there is enough Chip RAM */
    if (bi.chip_size < 512*1024) {
	Puts("Not enough Chip RAM in this system.  Aborting...\n");
	goto Fail;
    }

    /* verify that there is enough Fast RAM */
    for (fast_total = 0, i = 0; i < bi.num_memory; i++)
	fast_total += bi.memory[i].size;
    if (fast_total < 2*1024*1024) {
	Puts("Not enough Fast RAM in this system.  Aborting...\n");
	goto Fail;
    }

    /* support for ramdisk */
    if (ramdiskname) {
	int size;

	if ((size = FileSize(ramdiskname)) == -1) {
	    Printf("Unable to find size of ramdisk file `%s'\n", ramdiskname);
	    goto Fail;
	}
	/* record ramdisk size */
	bi.ramdisk.size = size;
    } else
	bi.ramdisk.size = 0;
    rd_size = bi.ramdisk.size;
    bi.ramdisk.addr = (u_long)start_mem+mem_size-rd_size;

    /* create the bootinfo structure */
    if (!create_bootinfo())
	goto Fail;

    /* open kernel executable and read exec header */
    if ((kfd = Open(kernelname)) == -1) {
	Printf("Unable to open kernel file `%s'\n", kernelname);
	goto Fail;
    }
    if (KRead(kfd, (void *)&kexec, sizeof(kexec)) != sizeof(kexec)) {
	Puts("Unable to read exec header from kernel file\n");
	goto Fail;
    }

#ifdef ZKERNEL
    if (((unsigned char *)&kexec)[0] == 037 &&
	(((unsigned char *)&kexec)[1] == 0213 ||
	 ((unsigned char *)&kexec)[1] == 0236)) {
	/* That's a compressed kernel */
	Puts("Kernel is compressed\n");
	if (load_zkernel(kfd)) {
	    Puts("Decompression error -- aborting\n");
	    goto Fail;
	}
    }
#endif

    switch (N_MAGIC(kexec)) {
	case ZMAGIC:
	    if (debugflag)
		Puts("\nLoading a.out (ZMAGIC) Linux/m68k kernel...\n");
	    text_offset = N_TXTOFF(kexec);
	    break;

	case QMAGIC:
	    if (debugflag)
		Puts("\nLoading a.out (QMAGIC) Linux/m68k kernel...\n");
	    text_offset = sizeof(kexec);
	    /* the text size includes the exec header; remove this */
	    kexec.a_text -= sizeof(kexec);
	    break;

	default:
	    /* Try to parse it as an ELF header */
	    KSeek(kfd, 0);
	    if ((KRead(kfd, (void *)&kexec_elf, sizeof(kexec_elf)) ==
		 sizeof(kexec_elf)) &&
		 (memcmp(&kexec_elf.e_ident[EI_MAG0], ELFMAG, SELFMAG) == 0)) {
		elf_kernel = 1;
		if (debugflag)
		    Puts("\nLoading ELF Linux/m68k kernel...\n");
		/* A few plausibility checks */
		if ((kexec_elf.e_type != ET_EXEC) ||
		    (kexec_elf.e_machine != EM_PPC && apus_boot) ||
		    (kexec_elf.e_machine != EM_68K && !apus_boot) ||
		    (kexec_elf.e_version != EV_CURRENT)) {
		    Puts("Invalid ELF header contents in kernel\n");
			Printf("machine : %d, type: %d, version: %d\n",
			 kexec_elf.e_machine, kexec_elf.e_type, kexec_elf.e_version);
		    goto Fail;
		}
		/* Load the program headers */
		if (!(kernel_phdrs =
		      (Elf32_Phdr *)AllocMem(kexec_elf.e_phnum*sizeof(Elf32_Phdr),
					     MEMF_FAST | MEMF_PUBLIC |
					     MEMF_CLEAR))) {
		    Puts("Unable to allocate memory for program headers\n");
		    goto Fail;
		}
		KSeek(kfd, kexec_elf.e_phoff);
		if (KRead(kfd, (void *)kernel_phdrs,
			 kexec_elf.e_phnum*sizeof(*kernel_phdrs)) !=
		    kexec_elf.e_phnum*sizeof(*kernel_phdrs)) {
		    Puts("Unable to read program headers from kernel file\n");
		    goto Fail;
		}
		break;
	    }
	    Printf("Wrong magic number 0x%08lx in kernel header\n",
		   N_MAGIC(kexec));
	    goto Fail;
    }

    /* Load the kernel at one page after start of mem */
    if (!apus_boot) {
	start_mem += PAGE_SIZE;
	mem_size -= PAGE_SIZE;
    }

    /* Align bss size to multiple of four */
    if (!elf_kernel)
	kexec.a_bss = (kexec.a_bss+3) & ~3;

    /* calculate the total required amount of memory */
    if (elf_kernel) {
	u_long min_addr = 0xffffffff, max_addr = 0;
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    if (min_addr > kernel_phdrs[i].p_vaddr)
		min_addr = kernel_phdrs[i].p_vaddr;
	    if (max_addr < kernel_phdrs[i].p_vaddr+kernel_phdrs[i].p_memsz)
		max_addr = kernel_phdrs[i].p_vaddr+kernel_phdrs[i].p_memsz;
	}
	/* This is needed for newer linkers that include the header in
	   the first segment.  */
	if (apus_boot) {
	    /* Avoid ugly hacks further down. */
	    kernel_phdrs[0].p_vaddr = PAGE_SIZE;
	} else if (min_addr == 0) {
	    min_addr = PAGE_SIZE;
	    kernel_phdrs[0].p_vaddr += PAGE_SIZE;
	    kernel_phdrs[0].p_offset += PAGE_SIZE;
	    kernel_phdrs[0].p_filesz -= PAGE_SIZE;
	    kernel_phdrs[0].p_memsz -= PAGE_SIZE;
	}
	kernel_size = max_addr-min_addr;
    } else
	kernel_size = kexec.a_text+kexec.a_data+kexec.a_bss;
    memreq = kernel_size+bi_size+rd_size;
#ifdef BOOTINFO_COMPAT_1_0
    if (sizeof(compat_bootinfo) > bi_size)
	memreq = kernel_size+sizeof(compat_bootinfo)+rd_size;
#endif /* BOOTINFO_COMPAT_1_0 */
    if (!(memptr = (char *)AllocMem(memreq, MEMF_FAST | MEMF_PUBLIC |
					    MEMF_CLEAR))) {
	Puts("Unable to allocate memory\n");
	goto Fail;
    }

    /* read the text and data segments from the kernel image */
    if (elf_kernel)
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    if (KSeek(kfd, kernel_phdrs[i].p_offset) == -1) {
		Printf("Failed to seek to segment %ld\n", i);
		goto Fail;
	    }
	    if (KRead(kfd, memptr+kernel_phdrs[i].p_vaddr-PAGE_SIZE,
		      kernel_phdrs[i].p_filesz) != kernel_phdrs[i].p_filesz) {
		Printf("Failed to read segment %ld\n", i);
		goto Fail;
	    }
	}
    else {
	if (KSeek(kfd, text_offset) == -1) {
	    Puts("Failed to seek to text\n");
	    goto Fail;
	}
	if (KRead(kfd, memptr, kexec.a_text) != kexec.a_text) {
	    Puts("Failed to read text\n");
	    goto Fail;
	}
	/* data follows immediately after text */
	if (KRead(kfd, memptr+kexec.a_text, kexec.a_data) != kexec.a_data) {
	    Puts("Failed to read data\n");
	    goto Fail;
	}
    }
    KClose(kfd);
    kfd = -1;

    /* Check kernel's bootinfo version */
    switch (apus_boot ? BI_VERSION_MAJOR(AMIGA_BOOTI_VERSION) :
	    check_bootinfo_version(memptr)) {
	case BI_VERSION_MAJOR(AMIGA_BOOTI_VERSION):
	    bi_ptr = &bi_union.record;
	    break;

#ifdef BOOTINFO_COMPAT_1_0
	case BI_VERSION_MAJOR(COMPAT_AMIGA_BOOTI_VERSION):
	    if (!create_compat_bootinfo())
		goto Fail;
	    bi_ptr = &compat_bootinfo;
	    bi_size = sizeof(compat_bootinfo);
	    break;
#endif /* BOOTINFO_COMPAT_1_0 */

	default:
	    goto Fail;
    }

    /* copy the bootinfo to the end of the kernel image */
    memcpy((void *)(memptr+kernel_size), bi_ptr, bi_size);

    if (ramdiskname) {
	if ((rfd = Open(ramdiskname)) == -1) {
	    Printf("Unable to open ramdisk file `%s'\n", ramdiskname);
	    goto Fail;
	}
	if (Read(rfd, memptr+kernel_size+bi_size, rd_size) != rd_size) {
	    Puts("Failed to read ramdisk file\n");
	    goto Fail;
	}
	Close(rfd);
	rfd = -1;
    }

    /* allocate temporary chip ram stack */
    if (!(stack = (u_long *)AllocMem(TEMP_STACKSIZE, MEMF_CHIP | MEMF_CLEAR))) {
	Puts("Unable to allocate memory for stack\n");
	goto Fail;
    }

    /* allocate chip ram for copy of startup code */
    startcodesize = &copyallend-&copyall;
    if (!(startfunc = (void (*)(void))AllocMem(startcodesize,
					       MEMF_CHIP | MEMF_CLEAR))) {
	Puts("Unable to allocate memory for startcode\n");
	goto Fail;
    }

    /* copy startup code to CHIP RAM */
    memcpy(startfunc, &copyall, startcodesize);

    if (debugflag) {
	if (bi.ramdisk.size)
	    Printf("RAM disk at 0x%08lx, size is %ldK\n",
		   (u_long)memptr+kernel_size, bi.ramdisk.size>>10);

	if (elf_kernel) {
	    PutChar('\n');
	    for (i = 0; i < kexec_elf.e_phnum; i++)
		Printf("Kernel segment %ld at 0x%08lx, size %ld\n", i,
		       start_mem+kernel_phdrs[i].p_vaddr-PAGE_SIZE,
		       kernel_phdrs[i].p_memsz);
	    Printf("Boot info        at 0x%08lx\n", start_mem+kernel_size);
	} else {
	    Printf("\nKernel text at 0x%08lx, code size 0x%08lx\n", start_mem,
		   kexec.a_text);
	    Printf("Kernel data at 0x%08lx, data size 0x%08lx\n",
		   start_mem+kexec.a_text, kexec.a_data);
	    Printf("Kernel bss  at 0x%08lx, bss  size 0x%08lx\n",
		   start_mem+kexec.a_text+kexec.a_data, kexec.a_bss);
	    Printf("Boot info   at 0x%08lx\n", start_mem+kernel_size);
	}
	Printf("\nKernel entry is 0x%08lx\n", elf_kernel ? kexec_elf.e_entry :
							   kexec.a_entry);

	Printf("ramdisk dest top is 0x%08lx\n", start_mem+mem_size);
	Printf("ramdisk lower limit is 0x%08lx\n",
	       (u_long)(memptr+kernel_size));
	Printf("ramdisk src top is 0x%08lx\n",
	       (u_long)(memptr+kernel_size)+rd_size);

	Puts("\nType a key to continue the Linux/m68k boot...");
	GetChar();
	PutChar('\n');
    }

    /* wait for things to settle down */
    Sleep(1000000);


	{
		void* PPCPort;
		void* PPCMsg;

#define CHIP_PTR (0xfff00)
#define INFO_SIZE (0x3000 + 0x0100)
    if (apus_boot) {
		/* Let the PowerPC handle the actual kernel start */
	
		/* Store some relevant information about the kernel image
		 * and RAM disk at *CHIP_PTR.
		 */
		unsigned long boot_msg;
		
		/* Make sure the CHIP_PTR will not be destroyed. */
		if (!AllocAbs (0x10, CHIP_PTR)){
			Puts("Unable to allocate CHIP memory at 0xfff00.\n"
		         "Try booting with a smaller resolution.\n");
		}
		
		if (!(boot_msg = (unsigned long) AllocMem (0x200, 
		                                           MEMF_REVERSE 
												   | MEMF_CHIP 
												   | MEMF_CLEAR))){
												   
			Puts("Unable to allocate CHIP memory for boot message\n");
			goto Fail;
		}
 
		if (!(info = (unsigned long*) AllocMem (INFO_SIZE, 
		                                        MEMF_CHIP | MEMF_CLEAR))){
			Puts("Unable to allocate CHIP memory for info table and"
		         " reloc code\n");
	        goto Fail;
		}		

		/* Leave the pointer in chip mem at address CHIP_PTR. */
		*((unsigned long*) CHIP_PTR) = (unsigned long) info;
		info[0] = (unsigned long) (memptr+kernel_phdrs[0].p_vaddr-PAGE_SIZE);
		info[1] = start_mem;
		info[2] = kernel_size;
		info[3] = bi_size;
		info[4] = rd_size;
		info[5] = mem_size;
		info[6] = boot_msg + 0x100;
		info[7] = 0; /* checksum */
		info[8] = (unsigned long)info + 0x2000; /* stack */
		
		/* Calculate a simple kernel checksum. */
		if (checksum)
		{
			int i = (info[2] + info[3]) / 4;
			unsigned char* k_p = (unsigned char*) info[0];
			unsigned long kcs = 0;
			
			while (i--)
			{
				unsigned long w = 0;
				
				w |= *k_p++;
				w <<= 8;
				w |= *k_p++;
				w <<= 8;
				w |= *k_p++;
				w <<= 8;
				w |= *k_p++;

				
				kcs = kcs ^ i;
				kcs = kcs ^ w;
			}
			info[7] = kcs;
		}
		
		/* Set LVL7 vector ("expose the top rim of our shorts") */
		{
			unsigned long* vbr;

			vbr = (unsigned long*) Supervisor((void*)&getvbr);
			vbr[31] = (unsigned long) &wedgie;
		}
		
		
		{
			void* elfObject;
			void* task;
			struct TagItem MyTags[2];
			struct TagItem tag;
			
			/* Load PPC elf object. */
			MyTags[0].ti_Tag	=	PPCELFLOADTAG_ELFADDRESS;
			MyTags[0].ti_Data	=	(u_long)myppc_boot;
			MyTags[1].ti_Tag	=	TAG_DONE;
		
			elfObject = (void*) PPCLoadObjectTagList(MyTags);
		
			/* Start PPC code, asking for a MSGPORT to be created. */
			MyTags[0].ti_Tag	=	PPCTASKTAG_MSGPORT;
			MyTags[0].ti_Data	=	TRUE;
			MyTags[1].ti_Tag	=	TAG_DONE;
			task = PPCCreateTask(elfObject, MyTags);

			tag.ti_Tag = PPCTASKINFOTAG_MSGPORT;
			if (PPCPort=(void*) PPCGetTaskAttrs(task, &tag))
			{
				PPCMsg = PPCCreateMessage(NULL, 0);
			}

			
			if (PPCPort == 0 || PPCMsg == 0)
			{
				Puts ("Didn't get contact with PPC task...\n");
				goto Fail;
			}
		}
	}

	/* turn off caches */
    CacheControl(0, ~0);
	
    if (!keep_video)
	/* set graphics mode to a nice normal one */
	LoadView(NULL);

	/* reset nasty Zorro boards */
	if (reset_boards)
	for (i = 0; i < bi.num_autocon; i++)
    	if (boardresetfuncs[i])
			boardresetfuncs[i](zdevs[i]);

	if (apus_boot && PPCPort && PPCMsg)
	{
		/* Notify the PPC that we are done ("bend over") */
		PPCSendMessage(PPCPort, PPCMsg,	NULL, 0, 0);
		for (;;) ;
	}

	/* Go into supervisor state */
    SuperState();

	}
	/* Turn off interrupts. */
    Disable();

	/* Turn off all DMA */
	custom.dmacon = DMAF_ALL | DMAF_MASTER;
	
	/* turn off any mmu translation */
    disable_mmu();

    /* execute the copy-and-go code (from CHIP RAM) */
    start_kernel(startfunc, (char *)stack+TEMP_STACKSIZE, memptr, start_mem,
		 mem_size, rd_size, kernel_size);

    /* Clean up and exit in case of a failure */
Fail:
    if (kfd != -1)
	KClose(kfd);
    if (rfd != -1)
	Close(rfd);
    if (info)
	FreeMem((void *) info, INFO_SIZE);
    if (memptr)
	FreeMem((void *)memptr, memreq);
    if (stack)
	FreeMem((void *)stack, TEMP_STACKSIZE);
    if (kernel_phdrs)
	FreeMem((void *)kernel_phdrs, kexec_elf.e_phnum*sizeof(Elf32_Phdr));
    return(FALSE);
}


    /*
     *	Determine the Chipset
     */

static u_long get_chipset(void)
{
    u_char cs;
    u_long chipset;

    if (GfxBase->Version >= 39)
	cs = SetChipRev(SETCHIPREV_BEST);
    else
	cs = GfxBase->ChipRevBits0;
    if ((cs & GFXG_AGA) == GFXG_AGA)
	chipset = CS_AGA;
    else if ((cs & GFXG_ECS) == GFXG_ECS)
	chipset = CS_ECS;
    else if ((cs & GFXG_OCS) == GFXG_OCS)
	chipset = CS_OCS;
    else
	chipset = CS_STONEAGE;
    return(chipset);
}


    /*
     *	Determine the CPU Type
     */

static void get_processor(u_long *cpu, u_long *fpu, u_long *mmu)
{
    *cpu = *fpu = 0;

    if (SysBase->AttnFlags & AFF_68060)
	*cpu = CPU_68060;
    else if (SysBase->AttnFlags & AFF_68040)
	*cpu = CPU_68040;
    else if (SysBase->AttnFlags & AFF_68030)
	*cpu = CPU_68030;
    else if (SysBase->AttnFlags & AFF_68020)
	*cpu = CPU_68020;

    if (*cpu == CPU_68040 || *cpu == CPU_68060) {
	if (SysBase->AttnFlags & AFF_FPU40)
	    *fpu = *cpu;
    } else if (SysBase->AttnFlags & AFF_68882)
	*fpu = FPU_68882;
    else if (SysBase->AttnFlags & AFF_68881)
	*fpu = FPU_68881;

    *mmu = *cpu;
}

    /*
     *	Determine the Amiga Model
     */

static u_long get_model(u_long chipset)
{
    u_long model = AMI_UNKNOWN;

    if (debugflag)
	Puts("Amiga model identification:\n");
    if (probe_resource("draco.resource"))
	model = AMI_DRACO;
    else {
	if (debugflag)
	    Puts("    Chipset: ");
	switch (chipset) {
	    case CS_STONEAGE:
		if (debugflag)
		    Puts("Old or unknown\n");
		goto OCS;
		break;

	    case CS_OCS:
		if (debugflag)
		    Puts("OCS\n");
OCS:		if (probe_resident("cd.device"))
		    model = AMI_CDTV;
		else
		    /* let's call it an A2000 (may be A500, A1000, A2500) */
		    model = AMI_2000;
		break;

	    case CS_ECS:
		if (debugflag)
		    Puts("ECS\n");
		if (probe_resident("Magic 36.7") ||
		    probe_resident("kickad 36.57") ||
		    probe_resident("A3000 Bonus") ||
		    probe_resident("A3000 bonus"))
		    /* let's call it an A3000 (may be A3000T) */
		    model = AMI_3000;
		else if (probe_resource("card.resource"))
		    model = AMI_600;
		else
		    /* let's call it an A2000 (may be A500[+], A1000, A2500) */
		    model = AMI_2000;
		break;

	    case CS_AGA:
		if (debugflag)
		    Puts("AGA\n");
		if (probe_resident("A1000 Bonus") ||
		    probe_resident("A4000 bonus"))
		    model = probe_resident("NCR scsi.device") ? AMI_4000T :
								AMI_4000;
		else if (probe_resource("card.resource"))
		    model = AMI_1200;
		else if (probe_resident("cd.device"))
		    model = AMI_CD32;
		else
		    model = AMI_3000PLUS;
		break;
	}
    }
    if (debugflag) {
	Puts("\nType a key to continue...");
	GetChar();
	Puts("\n\n");
    }
    return(model);
}

	
	/*
     *	Probe for a Resident Modules
     */

static int probe_resident(const char *name)
{
    const struct Resident *res;

    if (debugflag)
	Printf("    Module `%s': ", name);
    res = FindResident(name);
    if (debugflag)
	if (res)
	    Printf("0x%08lx\n", res);
	else
	    Puts("not present\n");
    return(res ? TRUE : FALSE);
}


    /*
     *	Probe for an available Resource
     */

static int probe_resource(const char *name)
{
    const void *res;

    if (debugflag)
	Printf("    Resource `%s': ", name);
    res = OpenResource(name);
    if (debugflag)
	if (res)
	    Printf("0x%08lx\n", res);
	else
	    Puts("not present\n");
    return(res ? TRUE : FALSE);
}


    /*
     *  Create the Bootinfo structure
     */

static int create_bootinfo(void)
{
    int i;
    struct bi_record *record;

    /* Initialization */
    bi_size = 0;

    /* Generic tags */
    if (!add_bi_record(BI_MACHTYPE, sizeof(bi.machtype), &bi.machtype))
	return(0);
    if (!add_bi_record(BI_CPUTYPE, sizeof(bi.cputype), &bi.cputype))
	return(0);
    if (!add_bi_record(BI_FPUTYPE, sizeof(bi.fputype), &bi.fputype))
	return(0);
    if (!add_bi_record(BI_MMUTYPE, sizeof(bi.mmutype), &bi.mmutype))
	return(0);
    for (i = 0; i < bi.num_memory; i++)
	if (!add_bi_record(BI_MEMCHUNK, sizeof(bi.memory[i]), &bi.memory[i]))
	    return(0);
    if (bi.ramdisk.size)
	if (!add_bi_record(BI_RAMDISK, sizeof(bi.ramdisk), &bi.ramdisk))
	    return(0);
    if (!add_bi_string(BI_COMMAND_LINE, bi.command_line))
	return(0);

    /* Amiga tags */
    if (!add_bi_record(BI_AMIGA_MODEL, sizeof(bi.model), &bi.model))
	return(0);
    for (i = 0; i < bi.num_autocon; i++)
	if (!add_bi_record(BI_AMIGA_AUTOCON, sizeof(bi.autocon[i]),
			    &bi.autocon[i]))
	    return(0);
    if (!add_bi_record(BI_AMIGA_CHIP_SIZE, sizeof(bi.chip_size), &bi.chip_size))
	return(0);
    if (!add_bi_record(BI_AMIGA_VBLANK, sizeof(bi.vblank), &bi.vblank))
	return(0);
    if (!add_bi_record(BI_AMIGA_PSFREQ, sizeof(bi.psfreq), &bi.psfreq))
	return(0);
    if (!add_bi_record(BI_AMIGA_ECLOCK, sizeof(bi.eclock), &bi.eclock))
	return(0);
    if (!add_bi_record(BI_AMIGA_CHIPSET, sizeof(bi.chipset), &bi.chipset))
	return(0);
    if (!add_bi_record(BI_AMIGA_SERPER, sizeof(bi.serper), &bi.serper))
	return(0);

    /* Trailer */
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = BI_LAST;
    bi_size += sizeof(bi_union.record.tag);

    return(1);
}


    /*
     *  Add a Record to the Bootinfo Structure
     */

static int add_bi_record(u_short tag, u_short size, const void *data)
{
    struct bi_record *record;
    u_int size2;

    size2 = (sizeof(struct bi_record)+size+3)&-4;
    if (bi_size+size2+sizeof(bi_union.record.tag) > MAX_BI_SIZE) {
	Puts("Can't add bootinfo record. Ask a wizard to enlarge me.\n");
	return(0);
    }
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = tag;
    record->size = size2;
    memcpy(record->data, data, size);
    bi_size += size2;
    return(1);
}


    /*
     *  Add a String Record to the Bootinfo Structure
     */

static int add_bi_string(u_short tag, const u_char *s)
{
    return(add_bi_record(tag, strlen(s)+1, (void *)s));
}


#ifdef BOOTINFO_COMPAT_1_0

    /*
     *  Create the Bootinfo structure for backwards compatibility mode
     */

static int create_compat_bootinfo(void)
{
    u_int i;

    compat_bootinfo.machtype = bi.machtype;
    if (bi.cputype & CPU_68020)
	compat_bootinfo.cputype = COMPAT_CPU_68020;
    else if (bi.cputype & CPU_68030)
	compat_bootinfo.cputype = COMPAT_CPU_68030;
    else if (bi.cputype & CPU_68040)
	compat_bootinfo.cputype = COMPAT_CPU_68040;
    else if (bi.cputype & CPU_68060)
	compat_bootinfo.cputype = COMPAT_CPU_68060;
    else {
	Printf("CPU type 0x%08lx not supported by kernel\n", bi.cputype);
	return(0);
    }
    if (bi.fputype & FPU_68881)
	compat_bootinfo.cputype |= COMPAT_FPU_68881;
    else if (bi.fputype & FPU_68882)
	compat_bootinfo.cputype |= COMPAT_FPU_68882;
    else if (bi.fputype & FPU_68040)
	compat_bootinfo.cputype |= COMPAT_FPU_68040;
    else if (bi.fputype & FPU_68060)
	compat_bootinfo.cputype |= COMPAT_FPU_68060;
    else if (bi.fputype) {
	Printf("FPU type 0x%08lx not supported by kernel\n", bi.fputype);
	return(0);
    }
    compat_bootinfo.num_memory = bi.num_memory;
    if (compat_bootinfo.num_memory > COMPAT_NUM_MEMINFO) {
	Printf("Warning: using only %ld blocks of memory\n",
	       COMPAT_NUM_MEMINFO);
	compat_bootinfo.num_memory = COMPAT_NUM_MEMINFO;
    }
    for (i = 0; i < compat_bootinfo.num_memory; i++) {
	compat_bootinfo.memory[i].addr = bi.memory[i].addr;
	compat_bootinfo.memory[i].size = bi.memory[i].size;
    }
    if (bi.ramdisk.size) {
	compat_bootinfo.ramdisk_size = (bi.ramdisk.size+1023)/1024;
	compat_bootinfo.ramdisk_addr = bi.ramdisk.addr;
    } else {
	compat_bootinfo.ramdisk_size = 0;
	compat_bootinfo.ramdisk_addr = 0;
    }
    strncpy(compat_bootinfo.command_line, bi.command_line, COMPAT_CL_SIZE);
    compat_bootinfo.command_line[COMPAT_CL_SIZE-1] = '\0';

    compat_bootinfo.bi_amiga.model = bi.model;
    compat_bootinfo.bi_amiga.num_autocon = bi.num_autocon;
    if (compat_bootinfo.bi_amiga.num_autocon > COMPAT_NUM_AUTO) {
	Printf("Warning: using only %ld AutoConfig devices\n",
	       COMPAT_NUM_AUTO);
	compat_bootinfo.bi_amiga.num_autocon = COMPAT_NUM_AUTO;
    }
    for (i = 0; i < compat_bootinfo.bi_amiga.num_autocon; i++)
	compat_bootinfo.bi_amiga.autocon[i] = bi.autocon[i];
    compat_bootinfo.bi_amiga.chip_size = bi.chip_size;
    compat_bootinfo.bi_amiga.vblank = bi.vblank;
    compat_bootinfo.bi_amiga.psfreq = bi.psfreq;
    compat_bootinfo.bi_amiga.eclock = bi.eclock;
    compat_bootinfo.bi_amiga.chipset = bi.chipset;
    compat_bootinfo.bi_amiga.hw_present = 0;
    return(1);
}
#endif /* BOOTINFO_COMPAT_1_0 */


    /*
     *  Compare the Bootstrap and Kernel Versions
     */

static int check_bootinfo_version(const char *memptr)
{
    const struct bootversion *bv = (struct bootversion *)memptr;
    unsigned long version = 0;
    int i, kernel_major, kernel_minor, boots_major, boots_minor;

    if (bv->magic == BOOTINFOV_MAGIC)
	for (i = 0; bv->machversions[i].machtype != 0; ++i)
	    if (bv->machversions[i].machtype == MACH_AMIGA) {
		version = bv->machversions[i].version;
		break;
	    }
    if (!version)
	Puts("Kernel has no bootinfo version info, assuming 0.0\n");

    kernel_major = BI_VERSION_MAJOR(version);
    kernel_minor = BI_VERSION_MINOR(version);
    boots_major  = BI_VERSION_MAJOR(AMIGA_BOOTI_VERSION);
    boots_minor  = BI_VERSION_MINOR(AMIGA_BOOTI_VERSION);
    Printf("Bootstrap's bootinfo version: %ld.%ld\n", boots_major,
	   boots_minor);
    Printf("Kernel's bootinfo version   : %ld.%ld\n", kernel_major,
	   kernel_minor);

    switch (kernel_major) {
	case BI_VERSION_MAJOR(AMIGA_BOOTI_VERSION):
	    if (kernel_minor > boots_minor) {
		Puts("Warning: Bootinfo version of bootstrap and kernel "
		       "differ!\n");
		Puts("         Certain features may not work.\n");
	    }
	    break;

#ifdef BOOTINFO_COMPAT_1_0
	case BI_VERSION_MAJOR(COMPAT_AMIGA_BOOTI_VERSION):
	    Puts("(using backwards compatibility mode)\n");
	    break;
#endif /* BOOTINFO_COMPAT_1_0 */

	default:
	    Printf("\nThis bootstrap is too %s for this kernel!\n",
		   boots_major < kernel_major ? "old" : "new");
	    return(0);
    }
    return(kernel_major);
}


    /*
     *	Call the copy-and-go-code
     */

static void start_kernel(void (*startfunc)(), char *stackp, char *memptr,
			 u_long start_mem, u_long mem_size, u_long rd_size,
			 u_long kernel_size)
{
    register void (*a0)() __asm("a0") = startfunc;
    register char *a2 __asm("a2") = stackp;
    register char *a3 __asm("a3") = memptr;
    register u_long a4 __asm("a4") = start_mem;
    register u_long d0 __asm("d0") = mem_size;
    register u_long d1 __asm("d1") = rd_size;
    register u_long d2 __asm("d2") = kernel_size;
    register u_long d3 __asm("d3") = bi_size;

    __asm __volatile ("movel a2,sp;"
		      "jmp a0@"
		      : /* no outputs */
		      : "r" (a0), "r" (a2), "r" (a3), "r" (a4), "r" (d0),
			"r" (d1), "r" (d2), "r" (d3)
		      /* no return */);
    /* fake a noreturn */
    for (;;);
}

asm(".text\n"
".align 4\n"
SYMBOL_NAME_STR(_getvbr) ":
 movec vbr,d0
 rte
");


/* LPSTOP cannot be used on the 68060. Apparently the 
   CyberStorm bus design cannot cope with it.*/
/* Just let the 68k hang for now. */
asm(".text\n"
".align 4\n"
SYMBOL_NAME_STR(_wedgie) ":
stop #0x2700
");


asm(".text\n"
".align 4\n"
SYMBOL_NAME_STR(_probe_powerup) ":
	moveml		d2/a5/a6,a7@-
	movel       d0,a0
	leal		99f,a1
	movel       a0@(8),d1
	movel		a1,a0@(8)
	moveq		#1,d0

	leal		0xfffe0000,a1
	nop
	movel		a1@,d2
	nop
	swap		d2
	andil		#0xff,d2
	bne 		20f

	nop
	clrl		a1@(0x30)
	nop
	movel		a1@,d2
	nop
	andil		#3,d2
	bne 		20f

	nop
	movel		#3,a1@(0x30)
	nop
	movel		a1@,d2
	nop
	andil		#3,d2
	cmpw		#3,d2
	bne 		20f

10:	movel		d1,a0@(8)
	moveml 		a7@+,d2/a5/a6
	rts

20:	moveq		#0,d0
	bra         10b

99:	addql		#2,a7@(2)
	moveq		#0,d0
	rte
");

    /*
     *	This assembler code is copied to chip ram, and then executed.
     *	It copies the kernel to it's final resting place.
     *
     *	It is called with:
     *
     *	    a3 = memptr
     *	    a4 = start_mem
     *	    d0 = mem_size
     *	    d1 = rd_size
     *	    d2 = kernel_size
     *	    d3 = bi_size
     */

asm(".text\n"
".align 4\n"
SYMBOL_NAME_STR(_copyall) ":
 				| /* copy kernel text and data */
	movel	a3,a0		| src = (u_long *)memptr;
	movel	a0,a2		| limit = (u_long *)(memptr+kernel_size);
	addl	d2,a2
	movel	a4,a1		| dest = (u_long *)start_mem;
1:	cmpl	a0,a2
	jeq	2f		| while (src < limit)
	moveb	a0@+,a1@+	|  *dest++ = *src++;
	jra	1b
2:
				| /* copy bootinfo to end of bss */
	movel	a3,a0		| src = (u_long *)(memptr+kernel_size);
	addl	d2,a0		| dest = end of bss (already in a1)
	movel	d3,d7		| count = bi_size
	subql	#1,d7
1:	moveb	a0@+,a1@+	| while (--count > -1)
	dbra	d7,1b		|     *dest++ = *src++

				| /* copy the ramdisk to the top of memory */
				| /* (from back to front) */
	movel	a4,a1		| dest = (u_long *)(start_mem+mem_size);
	addl	d0,a1
	movel	a3,a2		| limit = (u_long *)(memptr+kernel_size +
	addl	d2,a2		|		     bi_size);
	addl	d3,a2
	movel	a2,a0		| src = (u_long *)((u_long)limit+rd_size);
	addl	d1,a0
1:	cmpl	a0,a2
	beqs	2f		| while (src > limit)
	moveb	a0@-,a1@-	|     *--dest = *--src;
	bras	1b
2:
				| /* jump to start of kernel */
	movel	a4,a0		| jump_to (start_mem);
	jmp	a0@
"
SYMBOL_NAME_STR(_copyallend) ":
");


    /*
     *	Test for a MapROMmed A3640 Board
     */

asm(".text\n"
".align 4\n"
SYMBOL_NAME_STR(_maprommed) ":
	oriw	#0x0700,sr
	moveml	#0x3f20,sp@-
				| /* Save cache settings */
	.long	0x4e7a1002	| movec cacr,d1 */
				| /* Save MMU settings */
	.long	0x4e7a2003	| movec tc,d2
	.long	0x4e7a3004	| movec itt0,d3
	.long	0x4e7a4005	| movec itt1,d4
	.long	0x4e7a5006	| movec dtt0,d5
	.long	0x4e7a6007	| movec dtt1,d6
	moveq	#0,d0
	movel	d0,a2
				| /* Disable caches */
	.long	0x4e7b0002	| movec d0,cacr
				| /* Disable MMU */
	.long	0x4e7b0003	| movec d0,tc
	.long	0x4e7b0004	| movec d0,itt0
	.long	0x4e7b0005	| movec d0,itt1
	.long	0x4e7b0006	| movec d0,dtt0
	.long	0x4e7b0007	| movec d0,dtt1
	lea	0x07f80000,a0
	lea	0x00f80000,a1
	movel	a0@,d7
	cmpl	a1@,d7
	jne	1f
	movel	d7,d0
	notl	d0
	movel	d0,a0@
	nop			| /* Thanks to Jörg Mayer! */
	cmpl	a1@,d0
	jne	1f
	moveq	#-1,d0		| /* MapROMmed A3640 present */
	movel	d0,a2
1:	movel	d7,a0@
				| /* Restore MMU settings */
	.long	0x4e7b2003	| movec d2,tc
	.long	0x4e7b3004	| movec d3,itt0
	.long	0x4e7b4005	| movec d4,itt1
	.long	0x4e7b5006	| movec d5,dtt0
	.long	0x4e7b6007	| movec d6,dtt1
				| /* Restore cache settings */
	.long	0x4e7b1002	| movec d1,cacr
	movel	a2,d0
	moveml	sp@+,#0x04fc
	rte
");


    /*
     *	Reset functions for nasty Zorro boards
     */

static void reset_rb3(const struct ConfigDev *cd)
{
    volatile u_char *rb3_reg = (u_char *)(cd->cd_BoardAddr+0x01002000);

    /* FN: If a Rainbow III board is present, reset it to disable */
    /* its (possibly activated) vertical blank interrupts as the */
    /* kernel is not yet prepared to handle them (level 6). */

    /* set RESET bit in special function register */
    *rb3_reg = 0x01;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit */
    *rb3_reg = 0x00;
}

static void reset_piccolo(const struct ConfigDev *cd)
{
    volatile u_char *piccolo_reg = (u_char *)(cd->cd_BoardAddr+0x8000);

    /* FN: the same stuff as above, for the Piccolo board. */
    /* this also has the side effect of resetting the board's */
    /* output selection logic to use the Amiga's display in single */
    /* monitor systems - which is currently what we want. */

    /* set RESET bit in special function register */
    *piccolo_reg = 0x01;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit */
    *piccolo_reg = 0x51;
}

static void reset_sd64(const struct ConfigDev *cd)
{
    volatile u_char *sd64_reg = (u_char *)(cd->cd_BoardAddr+0x8000);

    /* FN: the same stuff as above, for the SD64 board. */
    /* just as on the Piccolo, this also resets the monitor switch */

    /* set RESET bit in special function register */
    *sd64_reg = 0x1f;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit AND switch monitor bit (0x20) */
    *sd64_reg = 0x4f;
}

static void reset_a2065(const struct ConfigDev *cd)
{
	volatile u_short *lance_rdp = (u_short *)(cd->cd_BoardAddr+0x4000);
	volatile u_short *lance_rap = (u_short *)(cd->cd_BoardAddr+0x4002);

    Disable();

	/* Stop the card */
	*lance_rap = 0; /* PCnet-ISA Controller Status (CSR0) */
	*lance_rdp = 4; /* STOP */

	Enable();
}
	
static void reset_ariadne(const struct ConfigDev *cd)
{
    volatile u_short *lance_rdp = (u_short *)(cd->cd_BoardAddr+0x0370);
    volatile u_short *lance_rap = (u_short *)(cd->cd_BoardAddr+0x0372);
    volatile u_short *lance_reset = (u_short *)(cd->cd_BoardAddr+0x0374);

    volatile u_char *pit_paddr = (u_char *)(cd->cd_BoardAddr+0x1004);
    volatile u_char *pit_pbddr = (u_char *)(cd->cd_BoardAddr+0x1006);
    volatile u_char *pit_pacr = (u_char *)(cd->cd_BoardAddr+0x100b);
    volatile u_char *pit_pbcr = (u_char *)(cd->cd_BoardAddr+0x100e);
    volatile u_char *pit_psr = (u_char *)(cd->cd_BoardAddr+0x101a);

    u_short in;

    Disable();

    /*
     *	Reset the Ethernet part (Am79C960 PCnet-ISA)
     */

    in = *lance_reset;   /* Reset Chip on Read Access */
    *lance_rap = 0x0000; /* PCnet-ISA Controller Status (CSR0) */
    *lance_rdp = 0x0400; /* STOP */

    /*
     *	Reset the Parallel part (MC68230 PI/T)
     */

    *pit_pacr &= 0xfd;   /* Port A Control Register */
    *pit_pbcr &= 0xfd;   /* Port B Control Register */
    *pit_psr = 0x05;     /* Port Status Register */
    *pit_paddr = 0x00;   /* Port A Data Direction Register */
    *pit_pbddr = 0x00;   /* Port B Data Direction Register */

    Enable();
}

static void reset_hydra(const struct ConfigDev *cd)
{
    volatile u_char *nic_cr  = (u_char *)(cd->cd_BoardAddr+0xffe1);
    volatile u_char *nic_isr = (u_char *)(cd->cd_BoardAddr+0xffe1 + 14);
    int n = 5000;

    Disable();
 
    *nic_cr = 0x21;	/* nic command register: software reset etc. */
    while (((*nic_isr & 0x80) == 0) && --n)  /* wait for reset to complete */
	;
 
    Enable();
}

#if 0
static void reset_a2060(const struct ConfigDev *cd)
{
#error reset_a2060: not yet implemented
}
#endif


#ifdef ZKERNEL

#define	ZFILE_CHUNK_BITS	16  /* chunk is 64 KB */
#define	ZFILE_CHUNK_SIZE	(1 << ZFILE_CHUNK_BITS)
#define	ZFILE_CHUNK_MASK	(ZFILE_CHUNK_SIZE-1)
#define	ZFILE_N_CHUNKS		(2*1024*1024/ZFILE_CHUNK_SIZE)

/* variables for storing the uncompressed data */
static char *ZFile[ZFILE_N_CHUNKS];
static int ZFileSize = 0;
static int ZFpos = 0;
static int Zwpos = 0;

static int Zinfd = 0;	     /* fd of compressed file */

/*
 * gzip declarations
 */

#define OF(args)  args

#define memzero(s, n)     memset ((s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define INBUFSIZ 4096
#define WSIZE 0x8000    /* window size--must be a power of two, and */
			/*  at least 32K for zip's deflate method */

static uch *inbuf;
static uch *window;

static unsigned insize = 0;  /* valid bytes in inbuf */
static unsigned inptr = 0;   /* index of next byte to be processed in inbuf */
static unsigned outcnt = 0;  /* bytes in output buffer */
static int exit_code = 0;
static long bytes_out = 0;

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())
		
/* Diagnostic functions (stubbed out) */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

#define STATIC static

static int  fill_inbuf(void);
static void flush_window(void);
static void error(char *m);
static void gzip_mark(void **);
static void gzip_release(void **);

#define malloc(x)	AllocVec(x, MEMF_FAST | MEMF_PUBLIC)
#define free(x)		FreeVec(x)

#ifdef LILO
#include "inflate.c"
#else
#include "../../../../lib/inflate.c"
#endif

static void gzip_mark(void **ptr)
{
}

static void gzip_release(void **ptr)
{
}


/*
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 */
static int fill_inbuf(void)
{
    if (exit_code)
	return -1;

    insize = Read(Zinfd, inbuf, INBUFSIZ);
    if (insize <= 0)
	return -1;

    inptr = 1;
    return(inbuf[0]);
}

/*
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
static void flush_window(void)
{
    ulg c = crc;         /* temporary variable */
    unsigned n;
    uch *in, ch;
    int chunk = Zwpos >> ZFILE_CHUNK_BITS;

    if (exit_code)
	return;

    if (chunk >= ZFILE_N_CHUNKS) {
	error("Compressed image too large! Aborting.\n");
	return;
    }
    if (!ZFile[chunk]) {
	if (!(ZFile[chunk] = (char *)AllocMem(ZFILE_CHUNK_SIZE,
					      MEMF_FAST | MEMF_PUBLIC))) {
	    error("Out of memory for decompresing kernel image\n");
	    return;
	}
    }
    memcpy(ZFile[chunk] + (Zwpos & ZFILE_CHUNK_MASK), window, outcnt);
    Zwpos += outcnt;
    
#define	DISPLAY_BITS 10
    if ((Zwpos & ((1 << DISPLAY_BITS)-1)) == 0)
	PutChar('.');
    
    in = window;
    for (n = 0; n < outcnt; n++) {
	ch = *in++;
	c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
    }
    crc = c;
    bytes_out += (ulg)outcnt;
    outcnt = 0;
}

static void error(char *x)
{
    Printf("\n%s", x);
    exit_code = 1;
}

static inline int call_sub(int (*func)(void), void *stackp)
{
    register int _res __asm("d0");
    register int (*a0)(void) __asm("a0") = func;
    register int (*a1)(void) __asm("a1") = stackp;

    __asm __volatile ("movel sp,a2;"
    		      "movel a1,sp;"
    		      "jsr a0@;"
    		      "movel a2,sp"
		      : "=r" (_res)
		      : "r" (a0), "r" (a1)
		      : "a0", "a1", "a2", "d0", "d1", "memory");
    return(_res);
}

static int load_zkernel(int fd)
{
    int i, err = -1;
#define ZSTACKSIZE	(16384)
    u_long *zstack;
    
    for (i = 0; i < ZFILE_N_CHUNKS; ++i)
	ZFile[i] = NULL;
    Zinfd = fd;
    Seek(fd, 0);
    
    if (!(inbuf = (uch *)AllocMem(INBUFSIZ, MEMF_FAST | MEMF_PUBLIC)))
	Puts("Couldn't allocate gunzip buffer\n");
    else {
	if (!(window = (uch *)AllocMem(WSIZE, MEMF_FAST | MEMF_PUBLIC)))
	    Puts("Couldn't allocate gunzip window\n");
	else {
	    if (!(zstack = (u_long *)AllocMem(ZSTACKSIZE,
	    				      MEMF_FAST | MEMF_PUBLIC)))
		Puts("Couldn't allocate gunzip stack\n");
	    else {
		Puts("Uncompressing kernel image ");
		makecrc();
		if (!(err = call_sub(gunzip, (char *)zstack+ZSTACKSIZE)))
		    Puts("done\n");
		ZFileSize = Zwpos;
		FreeMem(zstack, ZSTACKSIZE);
	    }
	    FreeMem(window, WSIZE);
	    window = NULL;
	}
	FreeMem(inbuf, INBUFSIZ);
	inbuf = NULL;
    }
    Close(Zinfd);	/* input file not needed anymore */
    return(err);
}


/* Note about the read/lseek wrapper and its memory management: It assumes
 * that all seeks are only forward, and thus data already read or skipped can
 * be freed. This is true for current organization of bootstrap and kernels.
 * Little exception: The struct kexec at the start of the file. After reading
 * it, there may be a seek back to the end of the file. But this currently
 * doesn't hurt. (Roman)
 */

static int KRead(int fd, void *buf, int cnt)
{
    unsigned done = 0;
	
    if (!ZFileSize)
	return(Read(fd, buf, cnt));
    
    if (ZFpos + cnt > ZFileSize)
	cnt = ZFileSize - ZFpos;
    
    while (cnt > 0) {
	unsigned chunk = ZFpos >> ZFILE_CHUNK_BITS;
	unsigned endchunk = (chunk+1) << ZFILE_CHUNK_BITS;
	unsigned n = cnt;

	if (ZFpos + n > endchunk)
	    n = endchunk - ZFpos;
	memcpy(buf, ZFile[chunk] + (ZFpos & ZFILE_CHUNK_MASK), n);
	cnt -= n;
	buf += n;
	done += n;
	ZFpos += n;

	if (ZFpos == endchunk) {
	    FreeMem(ZFile[chunk], ZFILE_CHUNK_SIZE);
	    ZFile[chunk] = NULL;
	}
    }

    return(done);
}


static int KSeek(int fd, int offset)
{
    unsigned oldpos, oldchunk, newchunk;

    if (!ZFileSize)
	return(Seek(fd, offset));

    oldpos = ZFpos;
    ZFpos = offset;
    if (ZFpos < 0) {
	ZFpos = 0;
	return(-1);
    } else if (ZFpos > ZFileSize) {
	ZFpos = ZFileSize;
	return(-1);
    }

    /* free memory of skipped-over data */
    oldchunk = oldpos >> ZFILE_CHUNK_BITS;
    newchunk = ZFpos  >> ZFILE_CHUNK_BITS;
    while(oldchunk < newchunk) {
	if (ZFile[oldchunk]) {
	    FreeMem(ZFile[oldchunk], ZFILE_CHUNK_SIZE);
	    ZFile[oldchunk] = NULL;
	}
	++oldchunk;
    }
    return(ZFpos);
}


static void free_zfile(void)
{
    int i;

    for (i = 0; i < ZFILE_N_CHUNKS; ++i)
	if (ZFile[i]) {
	    FreeMem(ZFile[i], ZFILE_CHUNK_SIZE);
	    ZFile[i] = NULL;
	}
}

static int KClose(int fd)
{
    if (ZFileSize) {
	free_zfile();
	ZFileSize = 0;
    } else
	Close(fd);
    return(0);
}
#endif /* ZKERNEL */
