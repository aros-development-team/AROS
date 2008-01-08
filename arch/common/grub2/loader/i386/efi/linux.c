/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/cpu/linux.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

#define NEXT_MEMORY_DESCRIPTOR(desc, size)      \
  ((grub_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

static grub_dl_t my_mod;

static grub_size_t linux_mem_size;
static int loaded;
static void *real_mode_mem;
static void *prot_mode_mem;
static void *initrd_mem;
static grub_efi_uintn_t real_mode_pages;
static grub_efi_uintn_t prot_mode_pages;
static grub_efi_uintn_t initrd_pages;
static void *mmap_buf;

static grub_uint8_t gdt[] __attribute__ ((aligned(16))) =
  {
    /* NULL.  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Reserved.  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Code segment.  */
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00,
    /* Data segment.  */
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
  };

struct gdt_descriptor
{
  grub_uint16_t dummy;
  grub_uint16_t limit;
  grub_uint32_t base;
} __attribute__ ((aligned(4), packed));

static struct gdt_descriptor gdt_desc =
  {
    0,
    sizeof (gdt) - 1,
    (grub_addr_t) gdt
  };

struct idt_descriptor
{
  grub_uint16_t dummy;
  grub_uint16_t limit;
  grub_uint32_t base;
} __attribute__ ((aligned(4)));

static struct idt_descriptor idt_desc =
  {
    0,
    0,
    0
  };

static inline grub_size_t
page_align (grub_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static grub_efi_uintn_t
find_mmap_size (void)
{
  static grub_efi_uintn_t mmap_size = 0;

  if (mmap_size != 0)
    return mmap_size;
  
  mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      grub_efi_memory_descriptor_t *mmap;
      grub_efi_uintn_t desc_size;
      
      mmap = grub_malloc (mmap_size);
      if (! mmap)
	return 0;

      ret = grub_efi_get_memory_map (&mmap_size, mmap, 0, &desc_size, 0);
      grub_free (mmap);
      
      if (ret < 0)
	grub_fatal ("cannot get memory map");
      else if (ret > 0)
	break;

      mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because GRUB allocates more on
     later, and EFI itself may allocate more.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

static void
free_pages (void)
{
  if (real_mode_mem)
    {
      grub_efi_free_pages ((grub_addr_t) real_mode_mem, real_mode_pages);
      real_mode_mem = 0;
    }

  if (prot_mode_mem)
    {
      grub_efi_free_pages ((grub_addr_t) prot_mode_mem, prot_mode_pages);
      prot_mode_mem = 0;
    }
  
  if (initrd_mem)
    {
      grub_efi_free_pages ((grub_addr_t) initrd_mem, initrd_pages);
      initrd_mem = 0;
    }
}

/* Allocate pages for the real mode code and the protected mode code
   for linux as well as a memory map buffer.  */
static int
allocate_pages (grub_size_t real_size, grub_size_t prot_size)
{
  grub_efi_uintn_t desc_size;
  grub_efi_memory_descriptor_t *mmap, *mmap_end;
  grub_efi_uintn_t mmap_size, tmp_mmap_size;
  grub_efi_memory_descriptor_t *desc;
  
  /* Make sure that each size is aligned to a page boundary.  */
  real_size = page_align (real_size + GRUB_DISK_SECTOR_SIZE);
  prot_size = page_align (prot_size);
  mmap_size = find_mmap_size ();

  grub_dprintf ("linux", "real_size = %x, prot_size = %x, mmap_size = %x\n",
		real_size, prot_size, mmap_size);
  
  /* Calculate the number of pages; Combine the real mode code with
     the memory map buffer for simplicity.  */
  real_mode_pages = ((real_size + mmap_size) >> 12);
  prot_mode_pages = (prot_size >> 12);
  
  /* Initialize the memory pointers with NULL for convenience.  */
  real_mode_mem = 0;
  prot_mode_mem = 0;
  
  /* Read the memory map temporarily, to find free space.  */
  mmap = grub_malloc (mmap_size);
  if (! mmap)
    return 0;

  tmp_mmap_size = mmap_size;
  if (grub_efi_get_memory_map (&tmp_mmap_size, mmap, 0, &desc_size, 0) <= 0)
    grub_fatal ("cannot get memory map");

  mmap_end = NEXT_MEMORY_DESCRIPTOR (mmap, tmp_mmap_size);
  
  /* First, find free pages for the real mode code
     and the memory map buffer.  */
  for (desc = mmap;
       desc < mmap_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      /* Probably it is better to put the real mode code in the traditional
	 space for safety.  */
      if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY
	  && desc->physical_start <= 0x90000
	  && desc->num_pages >= real_mode_pages)
	{
	  grub_efi_physical_address_t physical_end;
	  grub_efi_physical_address_t addr;
	  
	  physical_end = desc->physical_start + (desc->num_pages << 12);
	  if (physical_end > 0x90000)
	    physical_end = 0x90000;

	  grub_dprintf ("linux", "physical_start = %x, physical_end = %x\n",
			(unsigned) desc->physical_start,
			(unsigned) physical_end);
	  addr = physical_end - real_size - mmap_size;
	  if (addr < 0x10000)
	    continue;

	  grub_dprintf ("linux", "trying to allocate %u pages at %x\n",
			real_mode_pages, (unsigned) addr);
	  real_mode_mem = grub_efi_allocate_pages (addr, real_mode_pages);
	  if (! real_mode_mem)
	    grub_fatal ("cannot allocate pages");
	  
	  desc->num_pages -= real_mode_pages;
	  break;
	}
    }

  if (! real_mode_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot allocate real mode pages");
      goto fail;
    }

  mmap_buf = (void *) ((char *) real_mode_mem + real_size);
	      
  /* Next, find free pages for the protected mode code.  */
  /* XXX what happens if anything is using this address?  */
  prot_mode_mem = grub_efi_allocate_pages (0x100000, prot_mode_pages);
  if (! prot_mode_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "cannot allocate protected mode pages");
      goto fail;
    }

  grub_free (mmap);
  return 1;

 fail:
  grub_free (mmap);
  free_pages ();
  return 0;
}

static grub_err_t
grub_linux_boot (void)
{
  struct linux_kernel_header *lh;
  struct linux_kernel_params *params;
  grub_efi_uintn_t mmap_size;
  grub_efi_uintn_t map_key;
  grub_efi_uintn_t desc_size;
  grub_efi_uint32_t desc_version;
  
  lh = real_mode_mem;
  params = real_mode_mem;

  grub_dprintf ("linux", "code32_start = %x, idt_desc = %x, gdt_desc = %x\n",
		(unsigned) lh->code32_start, (grub_addr_t) &(idt_desc.limit),
		(grub_addr_t) &(gdt_desc.limit));
  grub_dprintf ("linux", "idt = %x:%x, gdt = %x:%x\n",
		(unsigned) idt_desc.limit, (unsigned) idt_desc.base,
		(unsigned) gdt_desc.limit, (unsigned) gdt_desc.base);
  mmap_size = find_mmap_size ();
  if (grub_efi_get_memory_map (&mmap_size, mmap_buf, &map_key,
			       &desc_size, &desc_version) <= 0)
    grub_fatal ("cannot get memory map");

  if (! grub_efi_exit_boot_services (map_key))
    grub_fatal ("cannot exit boot services");

  /* Note that no boot services are available from here.  */

  /* Hardware interrupts are not safe any longer.  */
  asm volatile ("cli" : : );
  
  /* Pass EFI parameters.  */
  params->efi_mem_desc_size = desc_size;
  params->efi_mem_desc_version = desc_version;
  params->efi_mmap = (grub_addr_t) mmap_buf;
  params->efi_mmap_size = mmap_size;

  /* Pass parameters.  */
  asm volatile ("movl %0, %%esi" : : "m" (real_mode_mem));
  asm volatile ("movl %0, %%ecx" : : "m" (lh->code32_start));
  asm volatile ("xorl %%ebx, %%ebx" : : );

  /* Load the IDT and the GDT for the bootstrap.  */
  asm volatile ("lidt %0" : : "m" (idt_desc.limit));
  asm volatile ("lgdt %0" : : "m" (gdt_desc.limit));

  /* Enter Linux.  */
  asm volatile ("jmp *%%ecx" : : );
  
  /* Never reach here.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_unload (void)
{
  free_pages ();
  grub_dl_unref (my_mod);
  loaded = 0;
  return GRUB_ERR_NONE;
}

void
grub_rescue_cmd_linux (int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
  struct linux_kernel_params *params;
  grub_uint8_t setup_sects;
  grub_size_t real_size, prot_size;
  grub_ssize_t len;
  int i;
  char *dest;

  grub_dl_ref (my_mod);
  
  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  if (grub_file_read (file, (char *) &lh, sizeof (lh)) != sizeof (lh))
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read the linux header");
      goto fail;
    }

  if (lh.boot_flag != grub_cpu_to_le16 (0xaa55))
    {
      grub_error (GRUB_ERR_BAD_OS, "invalid magic number");
      goto fail;
    }

  if (lh.setup_sects > GRUB_LINUX_MAX_SETUP_SECTS)
    {
      grub_error (GRUB_ERR_BAD_OS, "too many setup sectors");
      goto fail;
    }

  /* EFI support is quite new, so reject old versions.  */
  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0203)
    {
      grub_error (GRUB_ERR_BAD_OS, "too old version");
      goto fail;
    }

  /* I'm not sure how to support zImage on EFI.  */
  if (! (lh.loadflags & GRUB_LINUX_FLAG_BIG_KERNEL))
    {
      grub_error (GRUB_ERR_BAD_OS, "zImage is not supported");
      goto fail;
    }

  setup_sects = lh.setup_sects;
  
  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;
  
  if (! allocate_pages (real_size, prot_size))
    goto fail;
  
  /* XXX Linux assumes that only elilo can boot Linux on EFI!!!  */
  lh.type_of_loader = 0x50;

  lh.cl_magic = GRUB_LINUX_CL_MAGIC;
  lh.cl_offset = GRUB_LINUX_CL_END_OFFSET;
  lh.cmd_line_ptr = (char *) real_mode_mem + GRUB_LINUX_CL_OFFSET;
  lh.ramdisk_image = 0;
  lh.ramdisk_size = 0;

  params = (struct linux_kernel_params *) &lh;

  /* These are not needed to be precise, because Linux uses these values
     only to raise an error when the decompression code cannot find good
     space.  */
  params->ext_mem = ((32 * 0x100000) >> 10);
  params->alt_mem = ((32 * 0x100000) >> 10);
  
  params->video_cursor_x = grub_efi_system_table->con_out->mode->cursor_column;
  params->video_cursor_y = grub_efi_system_table->con_out->mode->cursor_row;
  params->video_page = 0; /* ??? */
  params->video_mode = grub_efi_system_table->con_out->mode->mode;
  params->video_width = (grub_getwh () >> 8);
  params->video_ega_bx = 0;
  params->video_height = (grub_getwh () & 0xff);
  params->have_vga = 0;
  params->font_size = 16; /* XXX */

  /* No VBE on EFI.  */
  params->lfb_width = 0;
  params->lfb_height = 0;
  params->lfb_depth = 0;
  params->lfb_base = 0;
  params->lfb_size = 0;
  params->lfb_line_len = 0;
  params->red_mask_size = 0;
  params->red_field_pos = 0;
  params->green_mask_size = 0;
  params->green_field_pos = 0;
  params->blue_mask_size = 0;
  params->blue_field_pos = 0;
  params->reserved_mask_size = 0;
  params->reserved_field_pos = 0;
  params->vesapm_segment = 0;
  params->vesapm_offset = 0;
  params->lfb_pages = 0;
  params->vesa_attrib = 0;

  /* No APM on EFI.  */
  params->apm_version = 0;
  params->apm_code_segment = 0;
  params->apm_entry = 0;
  params->apm_16bit_code_segment = 0;
  params->apm_data_segment = 0;
  params->apm_flags = 0;
  params->apm_code_len = 0;
  params->apm_data_len = 0;

  /* XXX is there any way to use SpeedStep on EFI?  */
  params->ist_signature = 0;
  params->ist_command = 0;
  params->ist_event = 0;
  params->ist_perf_level = 0;

  /* Let the kernel probe the information.  */
  grub_memset (params->hd0_drive_info, 0, sizeof (params->hd0_drive_info));
  grub_memset (params->hd1_drive_info, 0, sizeof (params->hd1_drive_info));

  /* No MCA on EFI.  */
  params->rom_config_len = 0;

  params->efi_signature = GRUB_LINUX_EFI_SIGNATURE; /* XXX not used */
  params->efi_system_table = (grub_addr_t) grub_efi_system_table;
  /* The other EFI parameters are filled when booting.  */

  /* No need to fake the BIOS's memory map.  */
  params->mmap_size = 0;

  /* Let the kernel probe the information.  */
  params->ps_mouse = 0;

  /* Clear padding for future compatibility.  */
  grub_memset (params->padding1, 0, sizeof (params->padding1));
  grub_memset (params->padding2, 0, sizeof (params->padding2));
  grub_memset (params->padding3, 0, sizeof (params->padding3));
  grub_memset (params->padding4, 0, sizeof (params->padding4));
  grub_memset (params->padding5, 0, sizeof (params->padding5));
  grub_memset (params->padding6, 0, sizeof (params->padding6));
  grub_memset (params->padding7, 0, sizeof (params->padding7));
  grub_memset (params->padding8, 0, sizeof (params->padding8));
  grub_memset (params->padding9, 0, sizeof (params->padding9));
  
  /* Put the real mode code at the real location.  */
  grub_memmove (real_mode_mem, &lh, sizeof (lh));

  len = real_size + GRUB_DISK_SECTOR_SIZE - sizeof (lh);
  if (grub_file_read (file, (char *) real_mode_mem + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  /* XXX there is no way to know if the kernel really supports EFI.  */
  grub_printf ("   [Linux-EFI, setup=0x%x, size=0x%x]\n",
	       real_size, prot_size);

  /* Detect explicitly specified memory size, if any.  */
  linux_mem_size = 0;
  for (i = 1; i < argc; i++)
    if (grub_memcmp (argv[i], "mem=", 4) == 0)
      {
	char *val = argv[i] + 4;
	  
	linux_mem_size = grub_strtoul (val, &val, 0);
	
	if (grub_errno)
	  {
	    grub_errno = GRUB_ERR_NONE;
	    linux_mem_size = 0;
	  }
	else
	  {
	    int shift = 0;
	    
	    switch (grub_tolower (val[0]))
	      {
	      case 'g':
		shift += 10;
	      case 'm':
		shift += 10;
	      case 'k':
		shift += 10;
	      default:
		break;
	      }

	    /* Check an overflow.  */
	    if (linux_mem_size > (~0UL >> shift))
	      linux_mem_size = 0;
	    else
	      linux_mem_size <<= shift;
	  }
      }

  /* Specify the boot file.  */
  dest = grub_stpcpy ((char *) real_mode_mem + GRUB_LINUX_CL_OFFSET,
		      "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);
  
  /* Copy kernel parameters.  */
  for (i = 1;
       i < argc
	 && dest + grub_strlen (argv[i]) + 1 < ((char *) real_mode_mem
						+ GRUB_LINUX_CL_END_OFFSET);
       i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

  len = prot_size;
  if (grub_file_read (file, (char *) GRUB_LINUX_BZIMAGE_ADDR, len) != len)
    grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");

  if (grub_errno == GRUB_ERR_NONE)
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 1);
      loaded = 1;
    }

 fail:
  
  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_dl_unref (my_mod);
      loaded = 0;
    }
}

void
grub_rescue_cmd_initrd (int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t addr_min, addr_max;
  grub_addr_t addr;
  grub_efi_uintn_t mmap_size;
  grub_efi_memory_descriptor_t *desc;
  grub_efi_uintn_t desc_size;
  struct linux_kernel_header *lh;
  
  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }
  
  if (! loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  initrd_pages = (page_align (size) >> 12);

  lh = (struct linux_kernel_header *) real_mode_mem;
  
  addr_max = grub_cpu_to_le32 (lh->initrd_addr_max);
  if (linux_mem_size != 0 && linux_mem_size < addr_max)
    addr_max = linux_mem_size;
  
  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  /* Usually, the compression ratio is about 50%.  */
  addr_min = (grub_addr_t) prot_mode_mem + ((prot_mode_pages * 3) << 12);
  
  /* Find the highest address to put the initrd.  */
  mmap_size = find_mmap_size ();
  if (grub_efi_get_memory_map (&mmap_size, mmap_buf, 0, &desc_size, 0) <= 0)
    grub_fatal ("cannot get memory map");

  addr = 0;
  for (desc = mmap_buf;
       desc < NEXT_MEMORY_DESCRIPTOR (mmap_buf, mmap_size);
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY
	  && desc->physical_start >= addr_min
	  && desc->physical_start + size < addr_max
	  && desc->num_pages >= initrd_pages)
	{
	  grub_efi_physical_address_t physical_end;
	  
	  physical_end = desc->physical_start + (desc->num_pages << 12);
	  if (physical_end > addr_max)
	    physical_end = addr_max;

	  if (physical_end > addr)
	    addr = physical_end - page_align (size);
	}
    }

  if (addr == 0)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "no free pages available");
      goto fail;
    }
  
  initrd_mem = grub_efi_allocate_pages (addr, initrd_pages);
  if (! initrd_mem)
    grub_fatal ("cannot allocate pages");
  
  if (grub_file_read (file, initrd_mem, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  grub_printf ("   [Initrd, addr=0x%x, size=0x%x]\n",
	       addr, size);
  
  lh->ramdisk_image = addr;
  lh->ramdisk_size = size;
  lh->root_dev = 0x0100; /* XXX */
  
 fail:
  if (file)
    grub_file_close (file);
}


GRUB_MOD_INIT(linux)
{
  grub_rescue_register_command ("linux",
				grub_rescue_cmd_linux,
				"load linux");
  grub_rescue_register_command ("initrd",
				grub_rescue_cmd_initrd,
				"load initrd");
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_rescue_unregister_command ("linux");
  grub_rescue_unregister_command ("initrd");
}
