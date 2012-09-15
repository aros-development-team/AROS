/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/memory.h>
#ifdef GRUB_MACHINE_PCBIOS
#include <grub/machine/biosnum.h>
#include <grub/machine/apm.h>
#include <grub/machine/memory.h>
#endif
#include <grub/multiboot.h>
#include <grub/cpu/multiboot.h>
#include <grub/cpu/relocator.h>
#include <grub/disk.h>
#include <grub/device.h>
#include <grub/partition.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/video.h>
#include <grub/acpi.h>
#include <grub/i18n.h>

#if defined (GRUB_MACHINE_EFI)
#include <grub/efi/efi.h>
#endif

#if defined (GRUB_MACHINE_PCBIOS) || defined (GRUB_MACHINE_COREBOOT) || defined (GRUB_MACHINE_MULTIBOOT) || defined (GRUB_MACHINE_QEMU)
#include <grub/i386/pc/vbe.h>
#define HAS_VGA_TEXT 1
#else
#define HAS_VGA_TEXT 0
#endif

struct module
{
  struct module *next;
  grub_addr_t start;
  grub_size_t size;
  char *cmdline;
  int cmdline_size;
};

static struct module *modules, *modules_last;
static grub_size_t cmdline_size;
static grub_size_t total_modcmd;
static unsigned modcnt;
static char *cmdline = NULL;
static int bootdev_set;
static grub_uint32_t biosdev, slice, part;
static grub_size_t elf_sec_num, elf_sec_entsize;
static unsigned elf_sec_shstrndx;
static void *elf_sections;

void
grub_multiboot_add_elfsyms (grub_size_t num, grub_size_t entsize,
			    unsigned shndx, void *data)
{
  elf_sec_num = num;
  elf_sec_shstrndx = shndx;
  elf_sec_entsize = entsize;
  elf_sections = data;
}

grub_err_t
grub_multiboot_load (grub_file_t file, const char *filename)
{
  grub_properly_aligned_t *buffer;
  grub_ssize_t len;
  struct multiboot_header *header;
  grub_err_t err;
  struct multiboot_header_tag *tag;
  struct multiboot_header_tag_address *addr_tag = NULL;
  int entry_specified = 0;
  grub_addr_t entry = 0;
  grub_uint32_t console_required = 0;
  struct multiboot_header_tag_framebuffer *fbtag = NULL;
  int accepted_consoles = GRUB_MULTIBOOT_CONSOLE_EGA_TEXT;

  buffer = grub_malloc (MULTIBOOT_SEARCH);
  if (!buffer)
    return grub_errno;

  len = grub_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"), filename);
    }

  COMPILE_TIME_ASSERT (MULTIBOOT_HEADER_ALIGN % 4 == 0);

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
       ((char *) header <= (char *) buffer + len - 12) || (header = 0);
       header = (struct multiboot_header *) ((grub_uint32_t *) header + MULTIBOOT_HEADER_ALIGN / 4))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
	  && !(header->magic + header->architecture
	       + header->header_length + header->checksum)
	  && header->architecture == MULTIBOOT_ARCHITECTURE_CURRENT)
	break;
    }

  if (header == 0)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "no multiboot header found");
    }

  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN % 4 == 0);

  for (tag = (struct multiboot_header_tag *) (header + 1);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_header_tag *) ((grub_uint32_t *) tag + ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN) / 4))
    switch (tag->type)
      {
      case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:
	{
	  unsigned i;
	  struct multiboot_header_tag_information_request *request_tag
	    = (struct multiboot_header_tag_information_request *) tag;
	  if (request_tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)
	    break;
	  for (i = 0; i < (request_tag->size - sizeof (request_tag))
		 / sizeof (request_tag->requests[0]); i++)
	    switch (request_tag->requests[i])
	      {
	      case MULTIBOOT_TAG_TYPE_END:
	      case MULTIBOOT_TAG_TYPE_CMDLINE:
	      case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	      case MULTIBOOT_TAG_TYPE_MODULE:
	      case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
	      case MULTIBOOT_TAG_TYPE_BOOTDEV:
	      case MULTIBOOT_TAG_TYPE_MMAP:
	      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
	      case MULTIBOOT_TAG_TYPE_VBE:
	      case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
	      case MULTIBOOT_TAG_TYPE_APM:
	      case MULTIBOOT_TAG_TYPE_EFI32:
	      case MULTIBOOT_TAG_TYPE_EFI64:
	      case MULTIBOOT_TAG_TYPE_ACPI_OLD:
	      case MULTIBOOT_TAG_TYPE_ACPI_NEW:
		break;

	      default:
		grub_free (buffer);
		return grub_error (GRUB_ERR_UNKNOWN_OS,
				   "unsupported information tag: 0x%x",
				   request_tag->requests[i]);
	      }
	  break;
	}
	       
      case MULTIBOOT_HEADER_TAG_ADDRESS:
	addr_tag = (struct multiboot_header_tag_address *) tag;
	break;

      case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS:
	entry_specified = 1;
	entry = ((struct multiboot_header_tag_entry_address *) tag)->entry_addr;
	break;

      case MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS:
	if (!(((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED))
	  accepted_consoles &= ~GRUB_MULTIBOOT_CONSOLE_EGA_TEXT;
	if (((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED)
	  console_required = 1;
	break;

      case MULTIBOOT_HEADER_TAG_FRAMEBUFFER:
	fbtag = (struct multiboot_header_tag_framebuffer *) tag;
	accepted_consoles |= GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER;
	break;

	/* GRUB always page-aligns modules.  */
      case MULTIBOOT_HEADER_TAG_MODULE_ALIGN:
	break;

      default:
	if (! (tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL))
	  {
	    grub_free (buffer);
	    return grub_error (GRUB_ERR_UNKNOWN_OS,
			       "unsupported tag: 0x%x", tag->type);
	  }
	break;
      }

  if (addr_tag && !entry_specified)
    {
      grub_free (buffer);
      return grub_error (GRUB_ERR_UNKNOWN_OS,
			 "load address tag without entry address tag");
    }
 
  if (addr_tag)
    {
      int offset = ((char *) header - (char *) buffer -
		    (addr_tag->header_addr - addr_tag->load_addr));
      int load_size = ((addr_tag->load_end_addr == 0) ? file->size - offset :
		       addr_tag->load_end_addr - addr_tag->load_addr);
      grub_size_t code_size;
      void *source;
      grub_relocator_chunk_t ch;

      if (addr_tag->bss_end_addr)
	code_size = (addr_tag->bss_end_addr - addr_tag->load_addr);
      else
	code_size = load_size;

      err = grub_relocator_alloc_chunk_addr (grub_multiboot_relocator, 
					     &ch, addr_tag->load_addr,
					     code_size);
      if (err)
	{
	  grub_dprintf ("multiboot_loader", "Error loading aout kludge\n");
	  grub_free (buffer);
	  return err;
	}
      source = get_virtual_current_address (ch);

      if ((grub_file_seek (file, offset)) == (grub_off_t) -1)
	{
	  grub_free (buffer);
	  return grub_errno;
	}

      grub_file_read (file, source, load_size);
      if (grub_errno)
	{
	  grub_free (buffer);
	  return grub_errno;
	}

      if (addr_tag->bss_end_addr)
	grub_memset ((grub_uint32_t *) source + load_size, 0,
		     addr_tag->bss_end_addr - addr_tag->load_addr - load_size);
    }
  else
    {
      err = grub_multiboot_load_elf (file, filename, buffer);
      if (err)
	{
	  grub_free (buffer);
	  return err;
	}
    }

  if (entry_specified)
    grub_multiboot_payload_eip = entry;

  if (fbtag)
    err = grub_multiboot_set_console (GRUB_MULTIBOOT_CONSOLE_FRAMEBUFFER,
				      accepted_consoles,
				      fbtag->width, fbtag->height,
				      fbtag->depth, console_required);
  else
    err = grub_multiboot_set_console (GRUB_MULTIBOOT_CONSOLE_EGA_TEXT,
				      accepted_consoles,
				      0, 0, 0, console_required);
  return err;
}

static grub_size_t
acpiv2_size (void)
{
#if GRUB_MACHINE_HAS_ACPI
  struct grub_acpi_rsdp_v20 *p = grub_acpi_get_rsdpv2 ();

  if (!p)
    return 0;

  return ALIGN_UP (sizeof (struct multiboot_tag_old_acpi)
		   + p->length, MULTIBOOT_TAG_ALIGN);
#else
  return 0;
#endif
}

static grub_size_t
grub_multiboot_get_mbi_size (void)
{
  return 2 * sizeof (grub_uint32_t) + sizeof (struct multiboot_tag)
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (cmdline_size, MULTIBOOT_TAG_ALIGN))
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (sizeof (PACKAGE_STRING), MULTIBOOT_TAG_ALIGN))
    + (modcnt * sizeof (struct multiboot_tag_module) + total_modcmd)
    + ALIGN_UP (sizeof (struct multiboot_tag_basic_meminfo),
		MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_bootdev), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_elf_sections), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (elf_sec_entsize * elf_sec_num, MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP ((sizeof (struct multiboot_tag_mmap)
		 + grub_get_multiboot_mmap_count ()
		 * sizeof (struct multiboot_mmap_entry)), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_framebuffer), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi32), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi64), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_old_acpi)
		+ sizeof (struct grub_acpi_rsdp_v10), MULTIBOOT_TAG_ALIGN)
    + acpiv2_size ()
    + sizeof (struct multiboot_tag_vbe) + MULTIBOOT_TAG_ALIGN - 1
    + sizeof (struct multiboot_tag_apm) + MULTIBOOT_TAG_ALIGN - 1;
}

/* Fill previously allocated Multiboot mmap.  */
static void
grub_fill_multiboot_mmap (struct multiboot_tag_mmap *tag)
{
  struct multiboot_mmap_entry *mmap_entry = tag->entries;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				  grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size,
			     grub_memory_type_t type)
    {
      mmap_entry->addr = addr;
      mmap_entry->len = size;
      switch (type)
	{
	case GRUB_MEMORY_AVAILABLE:
 	  mmap_entry->type = MULTIBOOT_MEMORY_AVAILABLE;
 	  break;

	case GRUB_MEMORY_ACPI:
 	  mmap_entry->type = MULTIBOOT_MEMORY_ACPI_RECLAIMABLE;
 	  break;

	case GRUB_MEMORY_NVS:
 	  mmap_entry->type = MULTIBOOT_MEMORY_NVS;
 	  break;

	case GRUB_MEMORY_BADRAM:
 	  mmap_entry->type = MULTIBOOT_MEMORY_BADRAM;
 	  break;

 	default:
 	  mmap_entry->type = MULTIBOOT_MEMORY_RESERVED;
 	  break;
 	}
      mmap_entry++;

      return 0;
    }

  tag->type = MULTIBOOT_TAG_TYPE_MMAP;
  tag->size = sizeof (struct multiboot_tag_mmap)
    + sizeof (struct multiboot_mmap_entry) * grub_get_multiboot_mmap_count (); 
  tag->entry_size = sizeof (struct multiboot_mmap_entry);
  tag->entry_version = 0;

  grub_mmap_iterate (hook);
}

#if defined (GRUB_MACHINE_PCBIOS)
static void
fill_vbe_tag (struct multiboot_tag_vbe *tag)
{
  grub_vbe_status_t status;
  void *scratch = (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

  tag->type = MULTIBOOT_TAG_TYPE_VBE;
  tag->size = 0;
    
  status = grub_vbe_bios_get_controller_info (scratch);
  if (status != GRUB_VBE_STATUS_OK)
    return;
  
  grub_memcpy (&tag->vbe_control_info, scratch,
	       sizeof (struct grub_vbe_info_block));
  
  status = grub_vbe_bios_get_mode (scratch);
  tag->vbe_mode = *(grub_uint32_t *) scratch;
  if (status != GRUB_VBE_STATUS_OK)
    return;

  /* get_mode_info isn't available for mode 3.  */
  if (tag->vbe_mode == 3)
    {
      struct grub_vbe_mode_info_block *mode_info = (void *) &tag->vbe_mode_info;
      grub_memset (mode_info, 0,
		   sizeof (struct grub_vbe_mode_info_block));
      mode_info->memory_model = GRUB_VBE_MEMORY_MODEL_TEXT;
      mode_info->x_resolution = 80;
      mode_info->y_resolution = 25;
    }
  else
    {
      status = grub_vbe_bios_get_mode_info (tag->vbe_mode, scratch);
      if (status != GRUB_VBE_STATUS_OK)
	return;
      grub_memcpy (&tag->vbe_mode_info, scratch,
		   sizeof (struct grub_vbe_mode_info_block));
    }      
  grub_vbe_bios_get_pm_interface (&tag->vbe_interface_seg,
				  &tag->vbe_interface_off,
				  &tag->vbe_interface_len);

  tag->size = sizeof (*tag);
}
#endif

static grub_err_t
retrieve_video_parameters (grub_properly_aligned_t **ptrorig)
{
  grub_err_t err;
  struct grub_video_mode_info mode_info;
  void *framebuffer;
  grub_video_driver_id_t driv_id;
  struct grub_video_palette_data palette[256];
  struct multiboot_tag_framebuffer *tag
    = (struct multiboot_tag_framebuffer *) *ptrorig;

  err = grub_multiboot_set_video_mode ();
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }

  grub_video_get_palette (0, ARRAY_SIZE (palette), palette);

  driv_id = grub_video_get_driver_id ();
#if HAS_VGA_TEXT
  if (driv_id == GRUB_VIDEO_DRIVER_NONE)
    {
      struct grub_vbe_mode_info_block vbe_mode_info;
      grub_uint32_t vbe_mode;

#if defined (GRUB_MACHINE_PCBIOS)
      {
	grub_vbe_status_t status;
	void *scratch = (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
	status = grub_vbe_bios_get_mode (scratch);
	vbe_mode = *(grub_uint32_t *) scratch;
	if (status != GRUB_VBE_STATUS_OK)
	  return GRUB_ERR_NONE;
      }
#else
      vbe_mode = 3;
#endif

      /* get_mode_info isn't available for mode 3.  */
      if (vbe_mode == 3)
	{
	  grub_memset (&vbe_mode_info, 0,
		       sizeof (struct grub_vbe_mode_info_block));
	  vbe_mode_info.memory_model = GRUB_VBE_MEMORY_MODEL_TEXT;
	  vbe_mode_info.x_resolution = 80;
	  vbe_mode_info.y_resolution = 25;
	}
#if defined (GRUB_MACHINE_PCBIOS)
      else
	{
	  grub_vbe_status_t status;
	  void *scratch = (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
	  status = grub_vbe_bios_get_mode_info (vbe_mode, scratch);
	  if (status != GRUB_VBE_STATUS_OK)
	    return GRUB_ERR_NONE;
	  grub_memcpy (&vbe_mode_info, scratch,
		       sizeof (struct grub_vbe_mode_info_block));
	}
#endif

      if (vbe_mode_info.memory_model == GRUB_VBE_MEMORY_MODEL_TEXT)
	{
	  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
	  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
	  tag->common.size = 0;

	  tag->common.framebuffer_addr = 0xb8000;
	  
	  tag->common.framebuffer_pitch = 2 * vbe_mode_info.x_resolution;	
	  tag->common.framebuffer_width = vbe_mode_info.x_resolution;
	  tag->common.framebuffer_height = vbe_mode_info.y_resolution;

	  tag->common.framebuffer_bpp = 16;
	  
	  tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
	  tag->common.size = sizeof (tag->common);
	  tag->common.reserved = 0;
	  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN)
	    / sizeof (grub_properly_aligned_t);
	}
      return GRUB_ERR_NONE;
    }
#else
  if (driv_id == GRUB_VIDEO_DRIVER_NONE)
    return GRUB_ERR_NONE;
#endif

#if GRUB_MACHINE_HAS_VBE
  {
    struct multiboot_tag_vbe *tag_vbe = (struct multiboot_tag_vbe *) *ptrorig;

    fill_vbe_tag (tag_vbe);

    *ptrorig += ALIGN_UP (tag_vbe->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }
#endif

  err = grub_video_get_info_and_fini (&mode_info, &framebuffer);
  if (err)
    return err;

  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
  tag->common.size = 0;

  tag->common.framebuffer_addr = (grub_addr_t) framebuffer;
  tag->common.framebuffer_pitch = mode_info.pitch;

  tag->common.framebuffer_width = mode_info.width;
  tag->common.framebuffer_height = mode_info.height;

  tag->common.framebuffer_bpp = mode_info.bpp;

  tag->common.reserved = 0;
      
  if (mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      unsigned i;
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED;
      tag->framebuffer_palette_num_colors = mode_info.number_of_colors;
      if (tag->framebuffer_palette_num_colors > ARRAY_SIZE (palette))
	tag->framebuffer_palette_num_colors = ARRAY_SIZE (palette);
      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common)
	+ sizeof (multiboot_uint16_t) + tag->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      for (i = 0; i < tag->framebuffer_palette_num_colors; i++)
	{
	  tag->framebuffer_palette[i].red = palette[i].r;
	  tag->framebuffer_palette[i].green = palette[i].g;
	  tag->framebuffer_palette[i].blue = palette[i].b;
	}
    }
  else
    {
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      tag->framebuffer_red_field_position = mode_info.red_field_pos;
      tag->framebuffer_red_mask_size = mode_info.red_mask_size;
      tag->framebuffer_green_field_position = mode_info.green_field_pos;
      tag->framebuffer_green_mask_size = mode_info.green_mask_size;
      tag->framebuffer_blue_field_position = mode_info.blue_field_pos;
      tag->framebuffer_blue_mask_size = mode_info.blue_mask_size;

      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common) + 6;
    }
  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN)
    / sizeof (grub_properly_aligned_t);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_multiboot_make_mbi (grub_uint32_t *target)
{
  grub_properly_aligned_t *ptrorig;
  grub_properly_aligned_t *mbistart;
  grub_err_t err;
  grub_size_t bufsize;
  grub_relocator_chunk_t ch;

  bufsize = grub_multiboot_get_mbi_size ();

  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN % sizeof (grub_properly_aligned_t) == 0);

  err = grub_relocator_alloc_chunk_align (grub_multiboot_relocator, &ch,
					  0, 0xffffffff - bufsize,
					  bufsize, MULTIBOOT_TAG_ALIGN,
					  GRUB_RELOCATOR_PREFERENCE_NONE, 0);
  if (err)
    return err;

  ptrorig = get_virtual_current_address (ch);
#if defined (__i386__) || defined (__x86_64__)
  *target = get_physical_target_address (ch);
#elif defined (__mips)
  *target = get_physical_target_address (ch) | 0x80000000;
#else
#error Please complete this
#endif

  mbistart = ptrorig;
  COMPILE_TIME_ASSERT ((2 * sizeof (grub_uint32_t))
		       % sizeof (grub_properly_aligned_t) == 0);
  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN
		       % sizeof (grub_properly_aligned_t) == 0);
  ptrorig += (2 * sizeof (grub_uint32_t)) / sizeof (grub_properly_aligned_t);

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_CMDLINE;
    tag->size = sizeof (struct multiboot_tag_string) + cmdline_size; 
    grub_memcpy (tag->string, cmdline, cmdline_size);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
       / sizeof (grub_properly_aligned_t);
  }

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME;
    tag->size = sizeof (struct multiboot_tag_string) + sizeof (PACKAGE_STRING); 
    grub_memcpy (tag->string, PACKAGE_STRING, sizeof (PACKAGE_STRING));
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }

#ifdef GRUB_MACHINE_PCBIOS
  {
    struct grub_apm_info info;
    if (grub_apm_get_info (&info))
      {
	struct multiboot_tag_apm *tag = (struct multiboot_tag_apm *) ptrorig;

	tag->type = MULTIBOOT_TAG_TYPE_APM;
	tag->size = sizeof (struct multiboot_tag_apm); 

	tag->cseg = info.cseg;
	tag->offset = info.offset;
	tag->cseg_16 = info.cseg_16;
	tag->dseg = info.dseg;
	tag->flags = info.flags;
	tag->cseg_len = info.cseg_len;
	tag->dseg_len = info.dseg_len;
	tag->cseg_16_len = info.cseg_16_len;
	tag->version = info.version;

	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (grub_properly_aligned_t);
      }
  }
#endif

  {
    unsigned i;
    struct module *cur;

    for (i = 0, cur = modules; i < modcnt; i++, cur = cur->next)
      {
	struct multiboot_tag_module *tag
	  = (struct multiboot_tag_module *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_MODULE;
	tag->size = sizeof (struct multiboot_tag_module) + cur->cmdline_size;
	tag->mod_start = cur->start;
	tag->mod_end = tag->mod_start + cur->size;
	grub_memcpy (tag->cmdline, cur->cmdline, cur->cmdline_size);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (grub_properly_aligned_t);
      }
  }

  {
    struct multiboot_tag_mmap *tag = (struct multiboot_tag_mmap *) ptrorig;
    grub_fill_multiboot_mmap (tag);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }

  {
    struct multiboot_tag_elf_sections *tag
      = (struct multiboot_tag_elf_sections *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_ELF_SECTIONS;
    tag->size = sizeof (struct multiboot_tag_elf_sections)
      + elf_sec_entsize * elf_sec_num;
    grub_memcpy (tag->sections, elf_sections, elf_sec_entsize * elf_sec_num);
    tag->num = elf_sec_num;
    tag->entsize = elf_sec_entsize;
    tag->shndx = elf_sec_shstrndx;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }

  {
    struct multiboot_tag_basic_meminfo *tag
      = (struct multiboot_tag_basic_meminfo *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_BASIC_MEMINFO;
    tag->size = sizeof (struct multiboot_tag_basic_meminfo); 

    /* Convert from bytes to kilobytes.  */
    tag->mem_lower = grub_mmap_get_lower () / 1024;
    tag->mem_upper = grub_mmap_get_upper () / 1024;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
       / sizeof (grub_properly_aligned_t);
  }

  if (bootdev_set)
    {
      struct multiboot_tag_bootdev *tag
	= (struct multiboot_tag_bootdev *) ptrorig;
      tag->type = MULTIBOOT_TAG_TYPE_BOOTDEV;
      tag->size = sizeof (struct multiboot_tag_bootdev); 

      tag->biosdev = biosdev;
      tag->slice = slice;
      tag->part = part;
      ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	/ sizeof (grub_properly_aligned_t);
    }

  {
    err = retrieve_video_parameters (&ptrorig);
    if (err)
      {
	grub_print_error ();
	grub_errno = GRUB_ERR_NONE;
      }
  }

#if defined (GRUB_MACHINE_EFI) && defined (__x86_64__)
  {
    struct multiboot_tag_efi64 *tag = (struct multiboot_tag_efi64 *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_EFI64;
    tag->size = sizeof (*tag);
    tag->pointer = (grub_addr_t) grub_efi_system_table;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }
#endif

#if defined (GRUB_MACHINE_EFI) && defined (__i386__)
  {
    struct multiboot_tag_efi32 *tag = (struct multiboot_tag_efi32 *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_EFI32;
    tag->size = sizeof (*tag);
    tag->pointer = (grub_addr_t) grub_efi_system_table;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }
#endif

#if GRUB_MACHINE_HAS_ACPI
  {
    struct multiboot_tag_old_acpi *tag = (struct multiboot_tag_old_acpi *)
      ptrorig;
    struct grub_acpi_rsdp_v10 *a = grub_acpi_get_rsdpv1 ();
    if (a)
      {
	tag->type = MULTIBOOT_TAG_TYPE_ACPI_OLD;
	tag->size = sizeof (*tag) + sizeof (*a);
	grub_memcpy (tag->rsdp, a, sizeof (*a));
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (grub_properly_aligned_t);
      }
  }

  {
    struct multiboot_tag_new_acpi *tag = (struct multiboot_tag_new_acpi *)
      ptrorig;
    struct grub_acpi_rsdp_v20 *a = grub_acpi_get_rsdpv2 ();
    if (a)
      {
	tag->type = MULTIBOOT_TAG_TYPE_ACPI_NEW;
	tag->size = sizeof (*tag) + a->length;
	grub_memcpy (tag->rsdp, a, a->length);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (grub_properly_aligned_t);
      }
  }
#endif

  {
    struct multiboot_tag *tag = (struct multiboot_tag *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_END;
    tag->size = sizeof (struct multiboot_tag);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (grub_properly_aligned_t);
  }

  ((grub_uint32_t *) mbistart)[0] = (char *) ptrorig - (char *) mbistart;
  ((grub_uint32_t *) mbistart)[1] = 0;

  return GRUB_ERR_NONE;
}

void
grub_multiboot_free_mbi (void)
{
  struct module *cur, *next;

  cmdline_size = 0;
  total_modcmd = 0;
  modcnt = 0;
  grub_free (cmdline);
  cmdline = NULL;
  bootdev_set = 0;

  for (cur = modules; cur; cur = next)
    {
      next = cur->next;
      grub_free (cur->cmdline);
      grub_free (cur);
    }
  modules = NULL;
  modules_last = NULL;
}

grub_err_t
grub_multiboot_init_mbi (int argc, char *argv[])
{
  grub_ssize_t len = 0;
  char *p;
  int i;

  grub_multiboot_free_mbi ();

  for (i = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;
  if (len == 0)
    len = 1;

  cmdline = p = grub_malloc (len);
  if (! cmdline)
    return grub_errno;
  cmdline_size = len;

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != cmdline)
    p--;
  *p = '\0';

  return GRUB_ERR_NONE;
}

grub_err_t
grub_multiboot_add_module (grub_addr_t start, grub_size_t size,
			   int argc, char *argv[])
{
  struct module *newmod;
  char *p;
  grub_ssize_t len = 0;
  int i;

  newmod = grub_malloc (sizeof (*newmod));
  if (!newmod)
    return grub_errno;
  newmod->start = start;
  newmod->size = size;

  for (i = 0; i < argc; i++)
    len += grub_strlen (argv[i]) + 1;

  if (len == 0)
    len = 1;

  newmod->cmdline = p = grub_malloc (len);
  if (! newmod->cmdline)
    {
      grub_free (newmod);
      return grub_errno;
    }
  newmod->cmdline_size = len;
  total_modcmd += ALIGN_UP (len, MULTIBOOT_TAG_ALIGN);

  for (i = 0; i < argc; i++)
    {
      p = grub_stpcpy (p, argv[i]);
      *(p++) = ' ';
    }

  /* Remove the space after the last word.  */
  if (p != newmod->cmdline)
    p--;
  *p = '\0';

  if (modules_last)
    modules_last->next = newmod;
  else
    {
      modules = newmod;
      modules_last->next = NULL;
    }
  modules_last = newmod;

  modcnt++;

  return GRUB_ERR_NONE;
}

void
grub_multiboot_set_bootdev (void)
{
  grub_device_t dev;

  slice = ~0;
  part = ~0;

#ifdef GRUB_MACHINE_PCBIOS
  biosdev = grub_get_root_biosnumber ();
#else
  biosdev = 0xffffffff;
#endif

  if (biosdev == 0xffffffff)
    return;

  dev = grub_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {
      if (dev->disk->partition->parent)
 	{
	  part = dev->disk->partition->number;
	  slice = dev->disk->partition->parent->number;
	}
      else
	slice = dev->disk->partition->number;
    }
  if (dev)
    grub_device_close (dev);

  bootdev_set = 1;
}
