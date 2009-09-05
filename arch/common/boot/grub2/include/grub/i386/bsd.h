/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_BSD_CPU_HEADER
#define GRUB_BSD_CPU_HEADER	1

#include <grub/types.h>

enum bsd_kernel_types
  {
    KERNEL_TYPE_NONE,
    KERNEL_TYPE_FREEBSD,
    KERNEL_TYPE_OPENBSD,
    KERNEL_TYPE_NETBSD,
  };

#define GRUB_BSD_TEMP_BUFFER   0x80000

#define FREEBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define FREEBSD_RB_SINGLE       (1 << 1)  /* reboot to single user only */
#define FREEBSD_RB_NOSYNC       (1 << 2)  /* dont sync before reboot */
#define FREEBSD_RB_HALT         (1 << 3)  /* don't reboot, just halt */
#define FREEBSD_RB_INITNAME     (1 << 4)  /* name given for /etc/init (unused) */
#define FREEBSD_RB_DFLTROOT     (1 << 5)  /* use compiled-in rootdev */
#define FREEBSD_RB_KDB          (1 << 6)  /* give control to kernel debugger */
#define FREEBSD_RB_RDONLY       (1 << 7)  /* mount root fs read-only */
#define FREEBSD_RB_DUMP         (1 << 8)  /* dump kernel memory before reboot */
#define FREEBSD_RB_MINIROOT     (1 << 9)  /* mini-root present in memory at boot time */
#define FREEBSD_RB_CONFIG       (1 << 10) /* invoke user configuration routing */
#define FREEBSD_RB_VERBOSE      (1 << 11) /* print all potentially useful info */
#define FREEBSD_RB_SERIAL       (1 << 12) /* user serial port as console */
#define FREEBSD_RB_CDROM        (1 << 13) /* use cdrom as root */
#define FREEBSD_RB_GDB		(1 << 15) /* use GDB remote debugger instead of DDB */
#define FREEBSD_RB_MUTE		(1 << 16) /* Come up with the console muted */
#define FREEBSD_RB_PAUSE	(1 << 20)
#define FREEBSD_RB_QUIET	(1 << 21)
#define FREEBSD_RB_NOINTR	(1 << 28)
#define FREENSD_RB_MULTIPLE	(1 << 29)  /* Use multiple consoles */
#define FREEBSD_RB_DUAL		FREENSD_RB_MULTIPLE
#define FREEBSD_RB_BOOTINFO     (1 << 31) /* have `struct bootinfo *' arg */

#define FREEBSD_B_DEVMAGIC	0xa0000000
#define FREEBSD_B_SLICESHIFT	20
#define FREEBSD_B_UNITSHIFT	16
#define FREEBSD_B_PARTSHIFT	8
#define FREEBSD_B_TYPESHIFT	0

#define FREEBSD_BOOTINFO_VERSION 1
#define FREEBSD_N_BIOS_GEOM	8

#define FREEBSD_MODINFO_END		0x0000	/* End of list */
#define FREEBSD_MODINFO_NAME		0x0001	/* Name of module (string) */
#define FREEBSD_MODINFO_TYPE		0x0002	/* Type of module (string) */
#define FREEBSD_MODINFO_ADDR		0x0003	/* Loaded address */
#define FREEBSD_MODINFO_SIZE		0x0004	/* Size of module */
#define FREEBSD_MODINFO_EMPTY		0x0005	/* Has been deleted */
#define FREEBSD_MODINFO_ARGS		0x0006	/* Parameters string */
#define FREEBSD_MODINFO_METADATA	0x8000	/* Module-specfic */

#define FREEBSD_MODINFOMD_AOUTEXEC	0x0001	/* a.out exec header */
#define FREEBSD_MODINFOMD_ELFHDR	0x0002	/* ELF header */
#define FREEBSD_MODINFOMD_SSYM		0x0003	/* start of symbols */
#define FREEBSD_MODINFOMD_ESYM		0x0004	/* end of symbols */
#define FREEBSD_MODINFOMD_DYNAMIC	0x0005	/* _DYNAMIC pointer */
#define FREEBSD_MODINFOMD_ENVP		0x0006	/* envp[] */
#define FREEBSD_MODINFOMD_HOWTO		0x0007	/* boothowto */
#define FREEBSD_MODINFOMD_KERNEND	0x0008	/* kernend */
#define FREEBSD_MODINFOMD_SHDR		0x0009	/* section header table */
#define FREEBSD_MODINFOMD_NOCOPY	0x8000	/* don't copy this metadata to the kernel */

#define FREEBSD_MODINFOMD_SMAP		0x1001

#define FREEBSD_MODINFOMD_DEPLIST	(0x4001 | FREEBSD_MODINFOMD_NOCOPY)  /* depends on */

#define FREEBSD_MODTYPE_KERNEL		"elf kernel"
#define FREEBSD_MODTYPE_KERNEL64	"elf64 kernel"
#define FREEBSD_MODTYPE_ELF_MODULE	"elf module"
#define FREEBSD_MODTYPE_ELF_MODULE_OBJ	"elf obj module"
#define FREEBSD_MODTYPE_RAW		"raw"

struct grub_freebsd_bootinfo
{
  grub_uint32_t bi_version;
  grub_uint8_t *bi_kernelname;
  struct nfs_diskless *bi_nfs_diskless;
  grub_uint32_t bi_n_bios_used;
  grub_uint32_t bi_bios_geom[FREEBSD_N_BIOS_GEOM];
  grub_uint32_t bi_size;
  grub_uint8_t bi_memsizes_valid;
  grub_uint8_t bi_bios_dev;
  grub_uint8_t bi_pad[2];
  grub_uint32_t bi_basemem;
  grub_uint32_t bi_extmem;
  grub_uint32_t bi_symtab;
  grub_uint32_t bi_esymtab;
  grub_uint32_t bi_kernend;
  grub_uint32_t bi_envp;
  grub_uint32_t bi_modulep;
} __attribute__ ((packed));

#define OPENBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define OPENBSD_RB_SINGLE	(1 << 1)  /* reboot to single user only */
#define OPENBSD_RB_NOSYNC	(1 << 2)  /* dont sync before reboot */
#define OPENBSD_RB_HALT		(1 << 3)  /* don't reboot, just halt */
#define OPENBSD_RB_INITNAME	(1 << 4)  /* name given for /etc/init (unused) */
#define OPENBSD_RB_DFLTROOT	(1 << 5)  /* use compiled-in rootdev */
#define OPENBSD_RB_KDB		(1 << 6)  /* give control to kernel debugger */
#define OPENBSD_RB_RDONLY	(1 << 7)  /* mount root fs read-only */
#define OPENBSD_RB_DUMP		(1 << 8)  /* dump kernel memory before reboot */
#define OPENBSD_RB_MINIROOT	(1 << 9)  /* mini-root present in memory at boot time */
#define OPENBSD_RB_CONFIG	(1 << 10) /* change configured devices */
#define OPENBSD_RB_TIMEBAD	(1 << 11) /* don't call resettodr() in boot() */
#define OPENBSD_RB_POWERDOWN	(1 << 12) /* attempt to power down machine */
#define OPENBSD_RB_SERCONS	(1 << 13) /* use serial console if available */
#define OPENBSD_RB_USERREQ	(1 << 14) /* boot() called at user request (e.g. ddb) */

#define OPENBSD_B_DEVMAGIC	0xa0000000
#define OPENBSD_B_ADAPTORSHIFT	24
#define OPENBSD_B_CTRLSHIFT	20
#define OPENBSD_B_UNITSHIFT	16
#define OPENBSD_B_PARTSHIFT	8
#define OPENBSD_B_TYPESHIFT	0

#define OPENBSD_BOOTARG_APIVER	(OPENBSD_BAPIV_VECTOR | \
                                 OPENBSD_BAPIV_ENV | \
                                 OPENBSD_BAPIV_BMEMMAP)

#define OPENBSD_BAPIV_ANCIENT	0x0  /* MD old i386 bootblocks */
#define OPENBSD_BAPIV_VARS	0x1  /* MD structure w/ add info passed */
#define OPENBSD_BAPIV_VECTOR	0x2  /* MI vector of MD structures passed */
#define OPENBSD_BAPIV_ENV	0x4  /* MI environment vars vector */
#define OPENBSD_BAPIV_BMEMMAP	0x8  /* MI memory map passed is in bytes */

#define OPENBSD_BOOTARG_ENV	0x1000
#define OPENBSD_BOOTARG_END	-1

#define	OPENBSD_BOOTARG_MMAP	0

struct grub_openbsd_bios_mmap
{
  grub_uint64_t addr;
  grub_uint64_t len;
#define	OPENBSD_MMAP_AVAILABLE	1
#define	OPENBSD_MMAP_RESERVED 2
#define	OPENBSD_MMAP_ACPI	3
#define	OPENBSD_MMAP_NVS 	4
  grub_uint32_t type;
};

struct grub_openbsd_bootargs
{
  int ba_type;
  int ba_size;
  struct grub_openbsd_bootargs *ba_next;
} __attribute__ ((packed));

#define NETBSD_RB_AUTOBOOT	0  /* flags for system auto-booting itself */

#define NETBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define NETBSD_RB_SINGLE	(1 << 1)  /* reboot to single user only */
#define NETBSD_RB_NOSYNC	(1 << 2)  /* dont sync before reboot */
#define NETBSD_RB_HALT		(1 << 3)  /* don't reboot, just halt */
#define NETBSD_RB_INITNAME	(1 << 4)  /* name given for /etc/init (unused) */
#define NETBSD_RB_UNUSED1	(1 << 5)  /* was RB_DFLTROOT, obsolete */
#define NETBSD_RB_KDB		(1 << 6)  /* give control to kernel debugger */
#define NETBSD_RB_RDONLY	(1 << 7)  /* mount root fs read-only */
#define NETBSD_RB_DUMP		(1 << 8)  /* dump kernel memory before reboot */
#define NETBSD_RB_MINIROOT	(1 << 9)  /* mini-root present in memory at boot time */
#define NETBSD_RB_STRING	(1 << 10) /* use provided bootstr */
#define NETBSD_RB_POWERDOWN     ((1 << 11) | RB_HALT) /* turn power off (or at least halt) */
#define NETBSD_RB_USERCONFIG	(1 << 12) /* change configured devices */

#define NETBSD_AB_NORMAL	0  /* boot normally (default) */

#define NETBSD_AB_QUIET		(1 << 16) /* boot quietly */
#define NETBSD_AB_VERBOSE	(1 << 17) /* boot verbosely */
#define NETBSD_AB_SILENT	(1 << 18) /* boot silently */
#define NETBSD_AB_DEBUG		(1 << 19) /* boot with debug messages */
#define NETBSD_AB_NOSMP		(1 << 28) /* Boot without SMP support.  */
#define NETBSD_AB_NOACPI        (1 << 29) /* Boot without ACPI support.  */

struct grub_netbsd_bootinfo
{
  grub_uint32_t bi_count;
  void *bi_data[1];
};

#define NETBSD_BTINFO_BOOTPATH		0
#define NETBSD_BTINFO_ROOTDEVICE	1
#define NETBSD_BTINFO_BOOTDISK		3
#define NETBSD_BTINFO_MEMMAP		9

struct grub_netbsd_btinfo_common
{
  int len;
  int type;
};

struct grub_netbsd_btinfo_mmap_header
{
  struct grub_netbsd_btinfo_common common;
  grub_uint32_t count;
};

struct grub_netbsd_btinfo_mmap_entry
{
  grub_uint64_t addr;
  grub_uint64_t len;
#define	NETBSD_MMAP_AVAILABLE	1
#define	NETBSD_MMAP_RESERVED 	2
#define	NETBSD_MMAP_ACPI	3
#define	NETBSD_MMAP_NVS 	4
  grub_uint32_t type;
};

struct grub_netbsd_btinfo_bootpath
{
  struct grub_netbsd_btinfo_common common;
  char bootpath[80];
};

struct grub_netbsd_btinfo_rootdevice
{
  struct grub_netbsd_btinfo_common common;
  char devname[16];
};

struct grub_netbsd_btinfo_bootdisk
{
  struct grub_netbsd_btinfo_common common;
  int labelsector;  /* label valid if != -1 */
  struct
    {
      grub_uint16_t type, checksum;
      char packname[16];
    } label;
  int biosdev;
  int partition;
};

void grub_unix_real_boot (grub_addr_t entry, ...)
     __attribute__ ((cdecl,noreturn));
grub_err_t grub_freebsd_load_elfmodule32 (grub_file_t file, int argc,
					  char *argv[], grub_addr_t *kern_end);
grub_err_t grub_freebsd_load_elfmodule_obj64 (grub_file_t file, int argc,
					      char *argv[],
					      grub_addr_t *kern_end);
grub_err_t grub_freebsd_load_elf_meta32 (grub_file_t file,
					 grub_addr_t *kern_end);
grub_err_t grub_freebsd_load_elf_meta64 (grub_file_t file,
					 grub_addr_t *kern_end);

grub_err_t grub_freebsd_add_meta (grub_uint32_t type, void *data,
				  grub_uint32_t len);
grub_err_t grub_freebsd_add_meta_module (char *filename, char *type,
					 int argc, char **argv,
					 grub_addr_t addr, grub_uint32_t size);

extern grub_uint8_t grub_bsd64_trampoline_start, grub_bsd64_trampoline_end;
extern grub_uint32_t grub_bsd64_trampoline_selfjump;
extern grub_uint32_t grub_bsd64_trampoline_gdt;

#endif /* ! GRUB_BSD_CPU_HEADER */
