/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/memory.h>
#include <grub/normal.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/cpu/linux.h>
#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/command.h>
#include <grub/i386/relocator.h>
#include <grub/i18n.h>
#include <grub/lib/cmdline.h>

#ifdef GRUB_MACHINE_PCBIOS
#include <grub/i386/pc/vesa_modes_table.h>
#endif

#ifdef GRUB_MACHINE_EFI
#include <grub/efi/efi.h>
#define HAS_VGA_TEXT 0
#define DEFAULT_VIDEO_MODE "auto"
#elif defined (GRUB_MACHINE_IEEE1275)
#include <grub/ieee1275/ieee1275.h>
#define HAS_VGA_TEXT 0
#define DEFAULT_VIDEO_MODE "text"
#else
#include <grub/i386/pc/vbe.h>
#include <grub/i386/pc/console.h>
#define HAS_VGA_TEXT 1
#define DEFAULT_VIDEO_MODE "text"
#endif

#define GRUB_LINUX_CL_OFFSET		0x1000
#define GRUB_LINUX_CL_END_OFFSET	0x2000

static grub_dl_t my_mod;

static grub_size_t linux_mem_size;
static int loaded;
static void *real_mode_mem;
static grub_addr_t real_mode_target;
static void *prot_mode_mem;
static grub_addr_t prot_mode_target;
static void *initrd_mem;
static grub_addr_t initrd_mem_target;
static grub_uint32_t real_mode_pages;
static grub_uint32_t prot_mode_pages;
static grub_uint32_t initrd_pages;
static struct grub_relocator *relocator = NULL;
static void *efi_mmap_buf;
#ifdef GRUB_MACHINE_EFI
static grub_efi_uintn_t efi_mmap_size;
#else
static const grub_size_t efi_mmap_size = 0;
#endif

/* FIXME */
#if 0
struct idt_descriptor
{
  grub_uint16_t limit;
  void *base;
} __attribute__ ((packed));

static struct idt_descriptor idt_desc =
  {
    0,
    0
  };
#endif

static inline grub_size_t
page_align (grub_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

#ifdef GRUB_MACHINE_EFI
/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static grub_efi_uintn_t
find_efi_mmap_size (void)
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

#endif

/* Find the optimal number of pages for the memory map. */
static grub_size_t
find_mmap_size (void)
{
  grub_size_t count = 0, mmap_size;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				  grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr __attribute__ ((unused)),
			     grub_uint64_t size __attribute__ ((unused)),
			     grub_memory_type_t type __attribute__ ((unused)))
    {
      count++;
      return 0;
    }

  grub_mmap_iterate (hook);

  mmap_size = count * sizeof (struct grub_e820_mmap);

  /* Increase the size a bit for safety, because GRUB allocates more on
     later.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

static void
free_pages (void)
{
  grub_relocator_unload (relocator);
  relocator = NULL;
  real_mode_mem = prot_mode_mem = initrd_mem = 0;
  real_mode_target = prot_mode_target = initrd_mem_target = 0;
}

/* Allocate pages for the real mode code and the protected mode code
   for linux as well as a memory map buffer.  */
static grub_err_t
allocate_pages (grub_size_t prot_size)
{
  grub_size_t real_size, mmap_size;
  grub_err_t err;

  /* Make sure that each size is aligned to a page boundary.  */
  real_size = GRUB_LINUX_CL_END_OFFSET;
  prot_size = page_align (prot_size);
  mmap_size = find_mmap_size ();

#ifdef GRUB_MACHINE_EFI
  efi_mmap_size = find_efi_mmap_size ();
#endif

  grub_dprintf ("linux", "real_size = %x, prot_size = %x, mmap_size = %x\n",
		(unsigned) real_size, (unsigned) prot_size, (unsigned) mmap_size);

  /* Calculate the number of pages; Combine the real mode code with
     the memory map buffer for simplicity.  */
  real_mode_pages = ((real_size + mmap_size + efi_mmap_size) >> 12);
  prot_mode_pages = (prot_size >> 12);

  /* Initialize the memory pointers with NULL for convenience.  */
  free_pages ();

  relocator = grub_relocator_new ();
  if (!relocator)
    {
      err = grub_errno;
      goto fail;
    }

  /* FIXME: Should request low memory from the heap when this feature is
     implemented.  */

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				  grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size,
			     grub_memory_type_t type)
    {
      /* We must put real mode code in the traditional space.  */

      if (type == GRUB_MEMORY_AVAILABLE
	  && addr <= 0x90000)
	{
	  if (addr < 0x10000)
	    {
	      size += addr - 0x10000;
	      addr = 0x10000;
	    }

	  if (addr + size > 0x90000)
	    size = 0x90000 - addr;

	  if (real_size + mmap_size + efi_mmap_size > size)
	    return 0;

	  real_mode_target = ((addr + size) - (real_size + mmap_size + efi_mmap_size));
	  return 1;
	}

      return 0;
    }
  grub_mmap_iterate (hook);
  if (! real_mode_target)
    {
      err = grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot allocate real mode pages");
      goto fail;
    }

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					   real_mode_target,
					   (real_size + mmap_size 
					    + efi_mmap_size));
    if (err)
      goto fail;
    real_mode_mem = get_virtual_current_address (ch);
  }
  efi_mmap_buf = (grub_uint8_t *) real_mode_mem + real_size + mmap_size;

  prot_mode_target = GRUB_LINUX_BZIMAGE_ADDR;

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					   prot_mode_target, prot_size);
    if (err)
      goto fail;
    prot_mode_mem = get_virtual_current_address (ch);
  }

  grub_dprintf ("linux", "real_mode_mem = %lx, real_mode_pages = %x, "
                "prot_mode_mem = %lx, prot_mode_pages = %x\n",
                (unsigned long) real_mode_mem, (unsigned) real_mode_pages,
                (unsigned long) prot_mode_mem, (unsigned) prot_mode_pages);

  return GRUB_ERR_NONE;

 fail:
  free_pages ();
  return err;
}

static void
grub_e820_add_region (struct grub_e820_mmap *e820_map, int *e820_num,
                      grub_uint64_t start, grub_uint64_t size,
                      grub_uint32_t type)
{
  int n = *e820_num;

  if (n >= GRUB_E820_MAX_ENTRY)
    grub_fatal ("Too many e820 memory map entries");

  if ((n > 0) && (e820_map[n - 1].addr + e820_map[n - 1].size == start) &&
      (e820_map[n - 1].type == type))
      e820_map[n - 1].size += size;
  else
    {
      e820_map[n].addr = start;
      e820_map[n].size = size;
      e820_map[n].type = type;
      (*e820_num)++;
    }
}

static grub_err_t
grub_linux_setup_video (struct linux_kernel_params *params)
{
  struct grub_video_mode_info mode_info;
  void *framebuffer;
  grub_err_t err;

  err = grub_video_get_info_and_fini (&mode_info, &framebuffer);

  if (err)
    {
      grub_errno = GRUB_ERR_NONE;
      return 1;
    }

  params->lfb_width = mode_info.width;
  params->lfb_height = mode_info.height;
  params->lfb_depth = mode_info.bpp;
  params->lfb_line_len = mode_info.pitch;

  params->lfb_base = (grub_size_t) framebuffer;
  params->lfb_size = ALIGN_UP (params->lfb_line_len * params->lfb_height, 65536);

  params->red_mask_size = mode_info.red_mask_size;
  params->red_field_pos = mode_info.red_field_pos;
  params->green_mask_size = mode_info.green_mask_size;
  params->green_field_pos = mode_info.green_field_pos;
  params->blue_mask_size = mode_info.blue_mask_size;
  params->blue_field_pos = mode_info.blue_field_pos;
  params->reserved_mask_size = mode_info.reserved_mask_size;
  params->reserved_field_pos = mode_info.reserved_field_pos;


#ifdef GRUB_MACHINE_PCBIOS
  /* VESA packed modes may come with zeroed mask sizes, which need
     to be set here according to DAC Palette width.  If we don't,
     this results in Linux displaying a black screen.  */
  if (mode_info.bpp <= 8)
    {
      struct grub_vbe_info_block controller_info;
      int status;
      int width = 8;

      status = grub_vbe_bios_get_controller_info (&controller_info);

      if (status == GRUB_VBE_STATUS_OK &&
	  (controller_info.capabilities & GRUB_VBE_CAPABILITY_DACWIDTH))
	status = grub_vbe_bios_set_dac_palette_width (&width);

      if (status != GRUB_VBE_STATUS_OK)
	/* 6 is default after mode reset.  */
	width = 6;

      params->red_mask_size = params->green_mask_size
	= params->blue_mask_size = width;
      params->reserved_mask_size = 0;
    }
#endif

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_linux_boot (void)
{
  struct linux_kernel_params *params;
  int e820_num;
  grub_err_t err = 0;
  char *modevar, *tmp;
  struct grub_relocator32_state state;

  params = real_mode_mem;

#ifdef GRUB_MACHINE_IEEE1275
  {
    char *bootpath;
    grub_ssize_t len;

    bootpath = grub_env_get ("root");
    if (bootpath)
      grub_ieee1275_set_property (grub_ieee1275_chosen,
				  "bootpath", bootpath,
				  grub_strlen (bootpath) + 1,
				  &len);
  }
#endif

  grub_dprintf ("linux", "code32_start = %x\n",
		(unsigned) params->code32_start);

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				  grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, 
			     grub_memory_type_t type)
    {
      switch (type)
        {
        case GRUB_MEMORY_AVAILABLE:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_RAM);
	  break;

        case GRUB_MEMORY_ACPI:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_ACPI);
	  break;

        case GRUB_MEMORY_NVS:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_NVS);
	  break;

        case GRUB_MEMORY_BADRAM:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_BADRAM);
	  break;

        default:
          grub_e820_add_region (params->e820_map, &e820_num,
                                addr, size, GRUB_E820_RESERVED);
        }
      return 0;
    }

  e820_num = 0;
  grub_mmap_iterate (hook);
  params->mmap_size = e820_num;

  modevar = grub_env_get ("gfxpayload");

  /* Now all graphical modes are acceptable.
     May change in future if we have modes without framebuffer.  */
  if (modevar && *modevar != 0)
    {
      tmp = grub_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
      if (! tmp)
	return grub_errno;
      err = grub_video_set_mode (tmp, 0, 0);
      grub_free (tmp);
    }
  else
    err = grub_video_set_mode (DEFAULT_VIDEO_MODE, 0, 0);

  if (err)
    {
      grub_print_error ();
      grub_printf ("Booting however\n");
      grub_errno = GRUB_ERR_NONE;
    }

  if (! grub_linux_setup_video (params))
    {
      /* Use generic framebuffer unless VESA is known to be supported.  */
      if (params->have_vga != GRUB_VIDEO_LINUX_TYPE_VESA)
	params->have_vga = GRUB_VIDEO_LINUX_TYPE_SIMPLE;
      else
	params->lfb_size >>= 16;
    }
  else
    {
#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_QEMU)
      params->have_vga = GRUB_VIDEO_LINUX_TYPE_TEXT;
      params->video_mode = 0x3;
#else
      params->have_vga = 0;
      params->video_mode = 0;
      params->video_width = 0;
      params->video_height = 0;
#endif
    }

  /* Initialize these last, because terminal position could be affected by printfs above.  */
#ifndef GRUB_MACHINE_IEEE1275
  if (params->have_vga == GRUB_VIDEO_LINUX_TYPE_TEXT)
#endif
    {
      grub_term_output_t term;
      int found = 0;
      FOR_ACTIVE_TERM_OUTPUTS(term)
	if (grub_strcmp (term->name, "vga_text") == 0
	    || grub_strcmp (term->name, "console") == 0
	    || grub_strcmp (term->name, "ofconsole") == 0)
	  {
	    grub_uint16_t pos = grub_term_getxy (term);
	    params->video_cursor_x = pos >> 8;
	    params->video_cursor_y = pos & 0xff;
	    params->video_width = grub_term_width (term);
	    params->video_height = grub_term_height (term);
	    found = 1;
	    break;
	  }
      if (!found)
	{
	  params->video_cursor_x = 0;
	  params->video_cursor_y = 0;
	  params->video_width = 80;
	  params->video_height = 25;
	}
    }

#ifdef GRUB_MACHINE_IEEE1275
  {
    params->ofw_signature = GRUB_LINUX_OFW_SIGNATURE;
    params->ofw_num_items = 1;
    params->ofw_cif_handler = (grub_uint32_t) grub_ieee1275_entry_fn;
    params->ofw_idt = 0;
  }
#endif

#ifdef GRUB_MACHINE_EFI
  {
    grub_efi_uintn_t efi_desc_size;
    grub_efi_uint32_t efi_desc_version;
    err = grub_efi_finish_boot_services (&efi_mmap_size, efi_mmap_buf, NULL,
					 &efi_desc_size, &efi_desc_version);
    if (err)
      return err;
    
    /* Note that no boot services are available from here.  */

    /* Pass EFI parameters.  */
    if (grub_le_to_cpu16 (params->version) >= 0x0206)
      {
	params->v0206.efi_mem_desc_size = efi_desc_size;
	params->v0206.efi_mem_desc_version = efi_desc_version;
	params->v0206.efi_mmap = (grub_uint32_t) (unsigned long) efi_mmap_buf;
	params->v0206.efi_mmap_size = efi_mmap_size;
#ifdef __x86_64__
	params->v0206.efi_mmap_hi = (grub_uint32_t) ((grub_uint64_t) efi_mmap_buf >> 32);
#endif
      }
    else if (grub_le_to_cpu16 (params->version) >= 0x0204)
      {
	params->v0204.efi_mem_desc_size = efi_desc_size;
	params->v0204.efi_mem_desc_version = efi_desc_version;
	params->v0204.efi_mmap = (grub_uint32_t) (unsigned long) efi_mmap_buf;
	params->v0204.efi_mmap_size = efi_mmap_size;
      }
  }
#endif

  /* FIXME.  */
  /*  asm volatile ("lidt %0" : : "m" (idt_desc)); */
  state.ebp = state.edi = state.ebx = 0;
  state.esi = real_mode_target;
  state.esp = real_mode_target;
  state.eip = params->code32_start;
  return grub_relocator32_boot (relocator, state);
}

static grub_err_t
grub_linux_unload (void)
{
  grub_dl_unref (my_mod);
  loaded = 0;
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
  struct linux_kernel_params *params;
  grub_uint8_t setup_sects;
  grub_size_t real_size, prot_size;
  grub_ssize_t len;
  int i;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  if (grub_file_read (file, &lh, sizeof (lh)) != sizeof (lh))
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read the Linux header");
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

  if (! (lh.loadflags & GRUB_LINUX_FLAG_BIG_KERNEL))
    {
      grub_error (GRUB_ERR_BAD_OS, "zImage doesn't support 32-bit boot"
#ifdef GRUB_MACHINE_PCBIOS
		  " (try with `linux16')"
#endif
		  );
      goto fail;
    }

  /* FIXME: 2.03 is not always good enough (Linux 2.4 can be 2.03 and
     still not support 32-bit boot.  */
  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0203)
    {
      grub_error (GRUB_ERR_BAD_OS, "version too old for 32-bit boot"
#ifdef GRUB_MACHINE_PCBIOS
		  " (try with `linux16')"
#endif
		  );
      goto fail;
    }

  setup_sects = lh.setup_sects;

  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;

  if (allocate_pages (prot_size))
    goto fail;

  params = (struct linux_kernel_params *) real_mode_mem;
  grub_memset (params, 0, GRUB_LINUX_CL_END_OFFSET);
  grub_memcpy (&params->setup_sects, &lh.setup_sects, sizeof (lh) - 0x1F1);

  params->ps_mouse = params->padding10 =  0;

  len = 0x400 - sizeof (lh);
  if (grub_file_read (file, (char *) real_mode_mem + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  params->type_of_loader = GRUB_LINUX_BOOT_LOADER_TYPE;

  /* These two are used (instead of cmd_line_ptr) by older versions of Linux,
     and otherwise ignored.  */
  params->cl_magic = GRUB_LINUX_CL_MAGIC;
  params->cl_offset = 0x1000;

  params->cmd_line_ptr = real_mode_target + 0x1000;
  params->ramdisk_image = 0;
  params->ramdisk_size = 0;

  params->heap_end_ptr = GRUB_LINUX_HEAP_END_OFFSET;
  params->loadflags |= GRUB_LINUX_FLAG_CAN_USE_HEAP;

  /* These are not needed to be precise, because Linux uses these values
     only to raise an error when the decompression code cannot find good
     space.  */
  params->ext_mem = ((32 * 0x100000) >> 10);
  params->alt_mem = ((32 * 0x100000) >> 10);

  /* Ignored by Linux.  */
  params->video_page = 0;

  /* Only used when `video_mode == 0x7', otherwise ignored.  */
  params->video_ega_bx = 0;

  params->font_size = 16; /* XXX */

#ifdef GRUB_MACHINE_EFI
  if (grub_le_to_cpu16 (params->version) >= 0x0206)
    {
      params->v0206.efi_signature = GRUB_LINUX_EFI_SIGNATURE;
      params->v0206.efi_system_table = (grub_uint32_t) (unsigned long) grub_efi_system_table;
#ifdef __x86_64__
      params->v0206.efi_system_table_hi = (grub_uint32_t) ((grub_uint64_t) grub_efi_system_table >> 32);
#endif
    }
  else if (grub_le_to_cpu16 (params->version) >= 0x0204)
    {
      params->v0204.efi_signature = GRUB_LINUX_EFI_SIGNATURE_0204;
      params->v0204.efi_system_table = (grub_uint32_t) (unsigned long) grub_efi_system_table;
    }
#endif

  /* The other parameters are filled when booting.  */

  grub_file_seek (file, real_size + GRUB_DISK_SECTOR_SIZE);

  grub_dprintf ("linux", "bzImage, setup=0x%x, size=0x%x\n",
		(unsigned) real_size, (unsigned) prot_size);

  /* Look for memory size and video mode specified on the command line.  */
  linux_mem_size = 0;
  for (i = 1; i < argc; i++)
#ifdef GRUB_MACHINE_PCBIOS
    if (grub_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	char *val = argv[i] + 4;
	unsigned vid_mode = GRUB_LINUX_VID_MODE_NORMAL;
	struct grub_vesa_mode_table_entry *linux_mode;
	grub_err_t err;
	char *buf;

	grub_dl_load ("vbe");

	if (grub_strcmp (val, "normal") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_NORMAL;
	else if (grub_strcmp (val, "ext") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_EXTENDED;
	else if (grub_strcmp (val, "ask") == 0)
	  {
	    grub_printf ("Legacy `ask' parameter no longer supported.\n");

	    /* We usually would never do this in a loader, but "vga=ask" means user
	       requested interaction, so it can't hurt to request keyboard input.  */
	    grub_wait_after_message ();

	    goto fail;
	  }
	else
	  vid_mode = (grub_uint16_t) grub_strtoul (val, 0, 0);

	switch (vid_mode)
	  {
	  case 0:
	  case GRUB_LINUX_VID_MODE_NORMAL:
	    grub_env_set ("gfxpayload", "text");
	    grub_printf ("%s is deprecated. "
			 "Use set gfxpayload=text before "
			 "linux command instead.\n",
			 argv[i]);
	    break;

	  case 1:
	  case GRUB_LINUX_VID_MODE_EXTENDED:
	    /* FIXME: support 80x50 text. */
	    grub_env_set ("gfxpayload", "text");
	    grub_printf ("%s is deprecated. "
			 "Use set gfxpayload=text before "
			 "linux command instead.\n",
			 argv[i]);
	    break;
	  default:
	    /* Ignore invalid values.  */
	    if (vid_mode < GRUB_VESA_MODE_TABLE_START ||
		vid_mode > GRUB_VESA_MODE_TABLE_END)
	      {
		grub_env_set ("gfxpayload", "text");
		grub_printf ("%s is deprecated. Mode %d isn't recognized. "
			     "Use set gfxpayload=WIDTHxHEIGHT[xDEPTH] before "
			     "linux command instead.\n",
			     argv[i], vid_mode);
		break;
	      }

	    /* We can't detect VESA, but user is implicitly telling us that it
	       is built-in because `vga=' parameter was used.  */
	    params->have_vga = GRUB_VIDEO_LINUX_TYPE_VESA;

	    linux_mode = &grub_vesa_mode_table[vid_mode
					       - GRUB_VESA_MODE_TABLE_START];

	    buf = grub_xasprintf ("%ux%ux%u,%ux%u",
				 linux_mode->width, linux_mode->height,
				 linux_mode->depth,
				 linux_mode->width, linux_mode->height);
	    if (! buf)
	      goto fail;

	    grub_printf ("%s is deprecated. "
			 "Use set gfxpayload=%s before "
			 "linux command instead.\n",
			 argv[i], buf);
	    err = grub_env_set ("gfxpayload", buf);
	    grub_free (buf);
	    if (err)
	      goto fail;
	  }
      }
    else
#endif /* GRUB_MACHINE_PCBIOS */
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
    else if (grub_memcmp (argv[i], "quiet", sizeof ("quiet") - 1) == 0)
      {
	params->loadflags |= GRUB_LINUX_FLAG_QUIET;
      }

  /* Create kernel command line.  */
  grub_memcpy ((char *)real_mode_mem + GRUB_LINUX_CL_OFFSET, LINUX_IMAGE,
	      sizeof (LINUX_IMAGE));
  grub_create_loader_cmdline (argc, argv,
			      (char *)real_mode_mem + GRUB_LINUX_CL_OFFSET
			      + sizeof (LINUX_IMAGE) - 1,
			      GRUB_LINUX_CL_END_OFFSET - GRUB_LINUX_CL_OFFSET
			      - (sizeof (LINUX_IMAGE) - 1));

  len = prot_size;
  if (grub_file_read (file, prot_mode_mem, len) != len)
    grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");

  if (grub_errno == GRUB_ERR_NONE)
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload,
		       0 /* set noreturn=0 in order to avoid grub_console_fini() */);
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

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_addr_t addr_min, addr_max;
  grub_addr_t addr;
  grub_err_t err;
  struct linux_kernel_header *lh;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no module specified");
      goto fail;
    }

  if (! loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "you need to load the kernel first");
      goto fail;
    }

  grub_file_filter_disable_compression ();
  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  size = grub_file_size (file);
  initrd_pages = (page_align (size) >> 12);

  lh = (struct linux_kernel_header *) real_mode_mem;

  /* Get the highest address available for the initrd.  */
  if (grub_le_to_cpu16 (lh->version) >= 0x0203)
    {
      addr_max = grub_cpu_to_le32 (lh->initrd_addr_max);

      /* XXX in reality, Linux specifies a bogus value, so
	 it is necessary to make sure that ADDR_MAX does not exceed
	 0x3fffffff.  */
      if (addr_max > GRUB_LINUX_INITRD_MAX_ADDRESS)
	addr_max = GRUB_LINUX_INITRD_MAX_ADDRESS;
    }
  else
    addr_max = GRUB_LINUX_INITRD_MAX_ADDRESS;

  if (linux_mem_size != 0 && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  /* Usually, the compression ratio is about 50%.  */
  addr_min = (grub_addr_t) prot_mode_target + ((prot_mode_pages * 3) << 12)
             + page_align (size);

  /* Put the initrd as high as possible, 4KiB aligned.  */
  addr = (addr_max - size) & ~0xFFF;

  if (addr < addr_min)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "the initrd is too big");
      goto fail;
    }

  {
    grub_relocator_chunk_t ch;
    err = grub_relocator_alloc_chunk_align (relocator, &ch,
					    addr_min, addr, size, 0x1000,
					    GRUB_RELOCATOR_PREFERENCE_HIGH);
    if (err)
      return err;
    initrd_mem = get_virtual_current_address (ch);
    initrd_mem_target = get_physical_target_address (ch);
  }

  if (grub_file_read (file, initrd_mem, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "couldn't read file");
      goto fail;
    }

  grub_dprintf ("linux", "Initrd, addr=0x%x, size=0x%x\n",
		(unsigned) addr, (unsigned) size);

  lh->ramdisk_image = initrd_mem_target;
  lh->ramdisk_size = size;
  lh->root_dev = 0x0100; /* XXX */

 fail:
  if (file)
    grub_file_close (file);

  return grub_errno;
}

static grub_command_t cmd_linux, cmd_initrd;

GRUB_MOD_INIT(linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, N_("Load initrd."));
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
