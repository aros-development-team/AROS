/* linux.c - boot Linux */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005, 2007, 2009  Free Software Foundation, Inc.
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

#include <grub/elf.h>
#include <grub/elfload.h>
#include <grub/loader.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/machine/loader.h>
#include <grub/gzio.h>
#include <grub/command.h>

static grub_dl_t my_mod;

static int loaded;

/* /virtual-memory/translations property layout  */
struct grub_ieee1275_translation {
  grub_uint64_t vaddr;
  grub_uint64_t size;
  grub_uint64_t data;
};

static struct grub_ieee1275_translation *of_trans;
static int of_num_trans;

static grub_addr_t phys_base;
static grub_addr_t grub_phys_start;
static grub_addr_t grub_phys_end;

static grub_addr_t initrd_addr;
static grub_addr_t initrd_paddr;
static grub_size_t initrd_size;

static Elf64_Addr linux_entry;
static grub_addr_t linux_addr;
static grub_addr_t linux_paddr;
static grub_size_t linux_size;

static char *linux_args;

typedef void (*kernel_entry_t) (unsigned long, unsigned long,
				unsigned long, unsigned long, int (void *));

struct linux_bootstr_info {
	int len, valid;
	char buf[];
};

struct linux_hdrs {
	/* All HdrS versions support these fields.  */
	unsigned int start_insns[2];
	char magic[4]; /* "HdrS" */
	unsigned int linux_kernel_version; /* LINUX_VERSION_CODE */
	unsigned short hdrs_version;
	unsigned short root_flags;
	unsigned short root_dev;
	unsigned short ram_flags;
	unsigned int __deprecated_ramdisk_image;
	unsigned int ramdisk_size;

	/* HdrS versions 0x0201 and higher only */
	char *reboot_command;

	/* HdrS versions 0x0202 and higher only */
	struct linux_bootstr_info *bootstr_info;

	/* HdrS versions 0x0301 and higher only */
	unsigned long ramdisk_image;
};

static grub_err_t
grub_linux_boot (void)
{
  struct linux_bootstr_info *bp;
  kernel_entry_t linuxmain;
  struct linux_hdrs *hp;
  grub_addr_t addr;

  hp = (struct linux_hdrs *) linux_addr;

  /* Any pointer we dereference in the kernel image must be relocated
     to where we actually loaded the kernel.  */
  addr = (grub_addr_t) hp->bootstr_info;
  addr += (linux_addr - linux_entry);
  bp = (struct linux_bootstr_info *) addr;

  /* Set the command line arguments, unless the kernel has been
     built with a fixed CONFIG_CMDLINE.  */
  if (!bp->valid)
    {
      int len = grub_strlen (linux_args) + 1;
      if (bp->len < len)
	len = bp->len;
      memcpy(bp->buf, linux_args, len);
      bp->buf[len-1] = '\0';
      bp->valid = 1;
    }

  if (initrd_addr)
    {
      /* The kernel expects the physical address, adjusted relative
	 to the lowest address advertised in "/memory"'s available
	 property.

	 The history of this is that back when the kernel only supported
	 specifying a 32-bit ramdisk address, this was the way to still
	 be able to specify the ramdisk physical address even if memory
	 started at some place above 4GB.

	 The magic 0x400000 is KERNBASE, I have no idea why SILO adds
	 that term into the address, but it does and thus we have to do
	 it too as this is what the kernel expects.  */
      hp->ramdisk_image = initrd_paddr - phys_base + 0x400000;
      hp->ramdisk_size = initrd_size;
    }

  grub_dprintf ("loader", "Entry point: 0x%lx\n", linux_addr);
  grub_dprintf ("loader", "Initrd at: 0x%lx, size 0x%lx\n", initrd_addr,
		initrd_size);
  grub_dprintf ("loader", "Boot arguments: %s\n", linux_args);
  grub_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.  */
  linuxmain = (kernel_entry_t) linux_addr;
  linuxmain (0, 0, 0, 0, grub_ieee1275_entry_fn);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_release_mem (void)
{
  grub_free (linux_args);
  linux_args = 0;
  linux_addr = 0;
  initrd_addr = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_unload (void)
{
  grub_err_t err;

  err = grub_linux_release_mem ();
  grub_dl_unref (my_mod);

  loaded = 0;

  return err;
}

#define FOUR_MB	(4 * 1024 * 1024)

static grub_addr_t
align_addr(grub_addr_t val, grub_addr_t align)
{
  return (val + (align - 1)) & ~(align - 1);
}

static grub_addr_t
alloc_phys (grub_addr_t size)
{
  grub_addr_t ret = (grub_addr_t) -1;

  auto int NESTED_FUNC_ATTR choose (grub_uint64_t addr, grub_uint64_t len __attribute__((unused)), grub_uint32_t type);
  int NESTED_FUNC_ATTR choose (grub_uint64_t addr, grub_uint64_t len __attribute__((unused)), grub_uint32_t type)
  {
    grub_addr_t end = addr + len;

    if (type != 1)
      return 0;

    addr = align_addr (addr, FOUR_MB);
    if (addr >= end)
      return 0;

    if (addr >= grub_phys_start && addr < grub_phys_end)
      {
	addr = align_addr (grub_phys_end, FOUR_MB);
	if (addr >= end)
	  return 0;
      }
    if ((addr + size) >= grub_phys_start
	&& (addr + size) < grub_phys_end)
      {
	addr = align_addr (grub_phys_end, FOUR_MB);
	if (addr >= end)
	  return 0;
      }

    if (loaded)
      {
	grub_addr_t linux_end = align_addr (linux_paddr + linux_size, FOUR_MB);

	if (addr >= linux_paddr && addr < linux_end)
	  {
	    addr = linux_end;
	    if (addr >= end)
	      return 0;
	  }
	if ((addr + size) >= linux_paddr
	    && (addr + size) < linux_end)
	  {
	    addr = linux_end;
	    if (addr >= end)
	      return 0;
	  }
      }

    ret = addr;
    return 1;
  }

  grub_machine_mmap_iterate (choose);

  return ret;
}

static grub_err_t
grub_linux_load64 (grub_elf_t elf)
{
  grub_addr_t off, paddr, base;
  int ret;

  linux_entry = elf->ehdr.ehdr64.e_entry;
  linux_addr = 0x40004000;
  off = 0x4000;
  linux_size = grub_elf64_size (elf);
  if (linux_size == 0)
    return grub_errno;

  grub_dprintf ("loader", "Attempting to claim at 0x%lx, size 0x%lx.\n",
		linux_addr, linux_size);

  paddr = alloc_phys (linux_size + off);
  if (paddr == (grub_addr_t) -1)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "Could not allocate physical memory.");
  ret = grub_ieee1275_map_physical (paddr, linux_addr - off,
				    linux_size + off, IEEE1275_MAP_DEFAULT);
  if (ret)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "Could not map physical memory.");

  grub_dprintf ("loader", "Loading linux at vaddr 0x%lx, paddr 0x%lx, size 0x%lx\n",
		linux_addr, paddr, linux_size);

  linux_paddr = paddr;

  base = linux_entry - off;

  /* Now load the segments into the area we claimed.  */
  auto grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load);
  grub_err_t offset_phdr (Elf64_Phdr *phdr, grub_addr_t *addr, int *do_load)
    {
      if (phdr->p_type != PT_LOAD)
	{
	  *do_load = 0;
	  return 0;
	}
      *do_load = 1;

      /* Adjust the program load address to linux_addr.  */
      *addr = (phdr->p_paddr - base) + (linux_addr - off);
      return 0;
    }
  return grub_elf64_load (elf, offset_phdr, 0, 0);
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_elf_t elf = 0;
  int i;
  int size;
  char *dest;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto out;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (!file)
    goto out;

  elf = grub_elf_file (file);
  if (! elf)
    goto out;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC)
    {
      grub_error (GRUB_ERR_UNKNOWN_OS,
		  "This ELF file is not of the right type\n");
      goto out;
    }

  /* Release the previously used memory.  */
  grub_loader_unset ();

  if (grub_elf_is_elf64 (elf))
    grub_linux_load64 (elf);
  else
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "Unknown ELF class");
      goto out;
    }

  size = sizeof ("BOOT_IMAGE=") + grub_strlen (argv[0]);
  for (i = 0; i < argc; i++)
    size += grub_strlen (argv[i]) + 1;

  linux_args = grub_malloc (size);
  if (! linux_args)
    goto out;

  /* Specify the boot file.  */
  dest = grub_stpcpy (linux_args, "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);

  for (i = 1; i < argc; i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

out:
  if (elf)
    grub_elf_close (elf);
  else if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_linux_release_mem ();
      grub_dl_unref (my_mod);
      loaded = 0;
    }
  else
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 1);
      initrd_addr = 0;
      loaded = 1;
    }

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t paddr;
  grub_addr_t addr;
  int ret;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no initrd specified");
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  addr = 0x60000000;
  size = grub_file_size (file);

  paddr = alloc_phys (size);
  if (paddr == (grub_addr_t) -1)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "Could not allocate physical memory.");
      goto fail;
    }
  ret = grub_ieee1275_map_physical (paddr, addr, size, IEEE1275_MAP_DEFAULT);
  if (ret)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "Could not map physical memory.");
      goto fail;
    }

  grub_dprintf ("loader", "Loading initrd at vaddr 0x%lx, paddr 0x%lx, size 0x%lx\n",
		addr, paddr, size);

  if (grub_file_read (file, (void *) addr, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  initrd_addr = addr;
  initrd_paddr = paddr;
  initrd_size = size;

 fail:
  if (file)
    grub_file_close (file);

  return grub_errno;
}

static void
determine_phys_base (void)
{
  auto int NESTED_FUNC_ATTR get_physbase (grub_uint64_t addr, grub_uint64_t len __attribute__((unused)), grub_uint32_t type);
  int NESTED_FUNC_ATTR get_physbase (grub_uint64_t addr, grub_uint64_t len __attribute__((unused)), grub_uint32_t type)
  {
    if (type != 1)
      return 0;
    if (addr < phys_base)
      phys_base = addr;
    return 0;
  }

  phys_base = ~(grub_uint64_t) 0;
  grub_machine_mmap_iterate (get_physbase);
}

static void
fetch_translations (void)
{
  grub_ieee1275_phandle_t node;
  grub_ssize_t actual;
  int i;

  if (grub_ieee1275_finddevice ("/virtual-memory", &node))
    {
      grub_printf ("Cannot find /virtual-memory node.\n");
      return;
    }

  if (grub_ieee1275_get_property_length (node, "translations", &actual))
    {
      grub_printf ("Cannot find /virtual-memory/translations size.\n");
      return;
    }

  of_trans = grub_malloc (actual);
  if (!of_trans)
    {
      grub_printf ("Cannot allocate translations buffer.\n");
      return;
    }

  if (grub_ieee1275_get_property (node, "translations", of_trans, actual, &actual))
    {
      grub_printf ("Cannot fetch /virtual-memory/translations property.\n");
      return;
    }

  of_num_trans = actual / sizeof(struct grub_ieee1275_translation);

  for (i = 0; i < of_num_trans; i++)
    {
      struct grub_ieee1275_translation *p = &of_trans[i];

      if (p->vaddr == 0x2000)
	{
	  grub_addr_t phys, tte = p->data;

	  phys = tte & ~(0xff00000000001fffULL);

	  grub_phys_start = phys;
	  grub_phys_end = grub_phys_start + p->size;
	  grub_dprintf ("loader", "Grub lives at phys_start[%lx] phys_end[%lx]\n",
			(unsigned long) grub_phys_start,
			(unsigned long) grub_phys_end);
	  break;
	}
    }
}


static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux)
{
  determine_phys_base ();
  fetch_translations ();

  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, "load a linux kernel");
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, "load an initrd");
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
