/* grub-mkimage.c - make a bootable image */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/types.h>
#include <grub/elf.h>
#include <grub/aout.h>
#include <grub/i18n.h>
#include <grub/kernel.h>
#include <grub/disk.h>
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/util/resolve.h>
#include <grub/misc.h>
#include <grub/offsets.h>
#include <time.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <grub/efi/pe32.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#include "progname.h"

#define ALIGN_ADDR(x) (ALIGN_UP((x), image_target->voidp_sizeof))

#ifdef HAVE_LIBLZMA
#include <lzma.h>
#endif

#define TARGET_NO_FIELD 0xffffffff

typedef enum {
  COMPRESSION_AUTO, COMPRESSION_NONE, COMPRESSION_XZ
} grub_compression_t;

struct image_target_desc
{
  const char *name;
  grub_size_t voidp_sizeof;
  int bigendian;
  enum {
    IMAGE_I386_PC, IMAGE_EFI, IMAGE_COREBOOT,
    IMAGE_SPARC64_AOUT, IMAGE_SPARC64_RAW, IMAGE_I386_IEEE1275,
    IMAGE_YEELOONG_ELF, IMAGE_QEMU, IMAGE_PPC, IMAGE_YEELOONG_FLASH,
    IMAGE_I386_PC_PXE
  } id;
  enum
    {
      PLATFORM_FLAGS_NONE = 0,
      PLATFORM_FLAGS_LZMA = 1,
      PLATFORM_FLAGS_DECOMPRESSORS = 2
    } flags;
  unsigned prefix;
  unsigned prefix_end;
  unsigned raw_size;
  unsigned total_module_size;
  unsigned kernel_image_size;
  unsigned compressed_size;
  unsigned link_align;
  grub_uint16_t elf_target;
  unsigned section_align;
  signed vaddr_offset;
  unsigned install_dos_part, install_bsd_part;
  grub_uint64_t link_addr;
  unsigned mod_gap, mod_align;
  grub_compression_t default_compression;
};

struct image_target_desc image_targets[] =
  {
    {
      .name = "i386-coreboot",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_I386_COREBOOT_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_COREBOOT_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = GRUB_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .name = "i386-multiboot",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_I386_MULTIBOOT_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_MULTIBOOT_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = GRUB_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .name = "i386-pc",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC, 
      .flags = PLATFORM_FLAGS_LZMA,
      .prefix = GRUB_KERNEL_I386_PC_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_PC_PREFIX_END,
      .raw_size = GRUB_KERNEL_I386_PC_RAW_SIZE,
      .total_module_size = GRUB_KERNEL_I386_PC_TOTAL_MODULE_SIZE,
      .kernel_image_size = GRUB_KERNEL_I386_PC_KERNEL_IMAGE_SIZE,
      .compressed_size = GRUB_KERNEL_I386_PC_COMPRESSED_SIZE,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = GRUB_KERNEL_I386_PC_INSTALL_DOS_PART,
      .install_bsd_part = GRUB_KERNEL_I386_PC_INSTALL_BSD_PART,
      .link_addr = GRUB_KERNEL_I386_PC_LINK_ADDR
    },
    {
      .name = "i386-pc-pxe",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC_PXE, 
      .flags = PLATFORM_FLAGS_LZMA,
      .prefix = GRUB_KERNEL_I386_PC_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_PC_PREFIX_END,
      .raw_size = GRUB_KERNEL_I386_PC_RAW_SIZE,
      .total_module_size = GRUB_KERNEL_I386_PC_TOTAL_MODULE_SIZE,
      .kernel_image_size = GRUB_KERNEL_I386_PC_KERNEL_IMAGE_SIZE,
      .compressed_size = GRUB_KERNEL_I386_PC_COMPRESSED_SIZE,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = GRUB_KERNEL_I386_PC_INSTALL_DOS_PART,
      .install_bsd_part = GRUB_KERNEL_I386_PC_INSTALL_BSD_PART,
      .link_addr = GRUB_KERNEL_I386_PC_LINK_ADDR
    },
    {
      .name = "i386-efi",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_EFI,
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_I386_EFI_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_EFI_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = GRUB_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				+ GRUB_PE32_SIGNATURE_SIZE
				+ sizeof (struct grub_pe32_coff_header)
				+ sizeof (struct grub_pe32_optional_header)
				+ 4 * sizeof (struct grub_pe32_section_table),
				GRUB_PE32_SECTION_ALIGNMENT),
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
    },
    {
      .name = "i386-ieee1275",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_IEEE1275, 
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_I386_IEEE1275_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_IEEE1275_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_I386_IEEE1275_LINK_ADDR,
      .elf_target = EM_386,
      .mod_gap = GRUB_KERNEL_I386_IEEE1275_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_IEEE1275_MOD_ALIGN,
      .link_align = 4,
    },
    {
      .name = "i386-qemu",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_QEMU, 
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_I386_QEMU_PREFIX,
      .prefix_end = GRUB_KERNEL_I386_QEMU_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .kernel_image_size = GRUB_KERNEL_I386_QEMU_KERNEL_IMAGE_SIZE,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_I386_QEMU_LINK_ADDR
    },
    {
      .name = "x86_64-efi",
      .voidp_sizeof = 8,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_X86_64_EFI_PREFIX,
      .prefix_end = GRUB_KERNEL_X86_64_EFI_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = GRUB_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				+ GRUB_PE32_SIGNATURE_SIZE
				+ sizeof (struct grub_pe32_coff_header)
				+ sizeof (struct grub_pe64_optional_header)
				+ 4 * sizeof (struct grub_pe32_section_table),
				GRUB_PE32_SECTION_ALIGNMENT),
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
    },
    {
      .name = "mipsel-yeeloong-flash",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_YEELOONG_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .prefix = GRUB_KERNEL_MIPS_YEELOONG_PREFIX,
      .prefix_end = GRUB_KERNEL_MIPS_YEELOONG_PREFIX_END,
      .raw_size = 0,
      .total_module_size = GRUB_KERNEL_MIPS_YEELOONG_TOTAL_MODULE_SIZE,
      .compressed_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_MIPS_YEELOONG_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_YEELOONG_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .name = "mipsel-yeeloong-elf",
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_YEELOONG_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .prefix = GRUB_KERNEL_MIPS_YEELOONG_PREFIX,
      .prefix_end = GRUB_KERNEL_MIPS_YEELOONG_PREFIX_END,
      .raw_size = 0,
      .total_module_size = GRUB_KERNEL_MIPS_YEELOONG_TOTAL_MODULE_SIZE,
      .compressed_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_MIPS_YEELOONG_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_YEELOONG_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .name = "powerpc-ieee1275",
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_PPC, 
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_POWERPC_IEEE1275_PREFIX,
      .prefix_end = GRUB_KERNEL_POWERPC_IEEE1275_PREFIX_END,
      .raw_size = 0,
      .total_module_size = TARGET_NO_FIELD,
      .kernel_image_size = TARGET_NO_FIELD,
      .compressed_size = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_POWERPC_IEEE1275_LINK_ADDR,
      .elf_target = EM_PPC,
      .mod_gap = GRUB_KERNEL_POWERPC_IEEE1275_MOD_GAP,
      .mod_align = GRUB_KERNEL_POWERPC_IEEE1275_MOD_ALIGN,
      .link_align = 4
    },
    {
      .name = "sparc64-ieee1275-raw",
      .voidp_sizeof = 8,
      .bigendian = 1, 
      .id = IMAGE_SPARC64_RAW,
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_SPARC64_IEEE1275_PREFIX,
      .prefix_end = GRUB_KERNEL_SPARC64_IEEE1275_PREFIX_END,
      .raw_size = GRUB_KERNEL_SPARC64_IEEE1275_RAW_SIZE,
      .total_module_size = GRUB_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .kernel_image_size = GRUB_KERNEL_SPARC64_IEEE1275_KERNEL_IMAGE_SIZE,
      .compressed_size = GRUB_KERNEL_SPARC64_IEEE1275_COMPRESSED_SIZE,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_SPARC64_IEEE1275_LINK_ADDR
    },
    {
      .name = "sparc64-ieee1275-aout",
      .voidp_sizeof = 8,
      .bigendian = 1,
      .id = IMAGE_SPARC64_AOUT,
      .flags = PLATFORM_FLAGS_NONE,
      .prefix = GRUB_KERNEL_SPARC64_IEEE1275_PREFIX,
      .prefix_end = GRUB_KERNEL_SPARC64_IEEE1275_PREFIX_END,
      .raw_size = GRUB_KERNEL_SPARC64_IEEE1275_RAW_SIZE,
      .total_module_size = GRUB_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .kernel_image_size = GRUB_KERNEL_SPARC64_IEEE1275_KERNEL_IMAGE_SIZE,
      .compressed_size = GRUB_KERNEL_SPARC64_IEEE1275_COMPRESSED_SIZE,
      .section_align = 1,
      .vaddr_offset = 0,
      .install_dos_part = TARGET_NO_FIELD,
      .install_bsd_part = TARGET_NO_FIELD,
      .link_addr = GRUB_KERNEL_SPARC64_IEEE1275_LINK_ADDR
    },
  };

#define grub_target_to_host32(x) (grub_target_to_host32_real (image_target, (x)))
#define grub_host_to_target32(x) (grub_host_to_target32_real (image_target, (x)))
#define grub_target_to_host64(x) (grub_target_to_host64_real (image_target, (x)))
#define grub_host_to_target64(x) (grub_host_to_target64_real (image_target, (x)))
#define grub_host_to_target_addr(x) (grub_host_to_target_addr_real (image_target, (x)))
#define grub_target_to_host16(x) (grub_target_to_host16_real (image_target, (x)))
#define grub_host_to_target16(x) (grub_host_to_target16_real (image_target, (x)))

static inline grub_uint32_t
grub_target_to_host32_real (struct image_target_desc *image_target, grub_uint32_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu32 (in);
  else
    return grub_le_to_cpu32 (in);
}

static inline grub_uint64_t
grub_target_to_host64_real (struct image_target_desc *image_target, grub_uint64_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu64 (in);
  else
    return grub_le_to_cpu64 (in);
}

static inline grub_uint64_t
grub_host_to_target64_real (struct image_target_desc *image_target, grub_uint64_t in)
{
  if (image_target->bigendian)
    return grub_cpu_to_be64 (in);
  else
    return grub_cpu_to_le64 (in);
}

static inline grub_uint32_t
grub_host_to_target32_real (struct image_target_desc *image_target, grub_uint32_t in)
{
  if (image_target->bigendian)
    return grub_cpu_to_be32 (in);
  else
    return grub_cpu_to_le32 (in);
}

static inline grub_uint16_t
grub_target_to_host16_real (struct image_target_desc *image_target, grub_uint16_t in)
{
  if (image_target->bigendian)
    return grub_be_to_cpu16 (in);
  else
    return grub_le_to_cpu16 (in);
}

static inline grub_uint16_t
grub_host_to_target16_real (struct image_target_desc *image_target, grub_uint16_t in)
{
  if (image_target->bigendian)
    return grub_cpu_to_be16 (in);
  else
    return grub_cpu_to_le16 (in);
}

static inline grub_uint64_t
grub_host_to_target_addr_real (struct image_target_desc *image_target, grub_uint64_t in)
{
  if (image_target->voidp_sizeof == 8)
    return grub_host_to_target64_real (image_target, in);
  else
    return grub_host_to_target32_real (image_target, in);
}

static inline grub_uint64_t
grub_target_to_host_real (struct image_target_desc *image_target, grub_uint64_t in)
{
  if (image_target->voidp_sizeof == 8)
    return grub_target_to_host64_real (image_target, in);
  else
    return grub_target_to_host32_real (image_target, in);
}

#define GRUB_IEEE1275_NOTE_NAME "PowerPC"
#define GRUB_IEEE1275_NOTE_TYPE 0x1275

/* These structures are defined according to the CHRP binding to IEEE1275,
   "Client Program Format" section.  */

struct grub_ieee1275_note_hdr
{
  grub_uint32_t namesz;
  grub_uint32_t descsz;
  grub_uint32_t type;
  char name[sizeof (GRUB_IEEE1275_NOTE_NAME)];
};

struct grub_ieee1275_note_desc
{
  grub_uint32_t real_mode;
  grub_uint32_t real_base;
  grub_uint32_t real_size;
  grub_uint32_t virt_base;
  grub_uint32_t virt_size;
  grub_uint32_t load_base;
};

struct grub_ieee1275_note
{
  struct grub_ieee1275_note_hdr header;
  struct grub_ieee1275_note_desc descriptor;
};

#define grub_target_to_host(val) grub_target_to_host_real(image_target, (val))

#include <grub/lib/LzmaEnc.h>

static void *SzAlloc(void *p, size_t size) { p = p; return xmalloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static void
compress_kernel_lzma (char *kernel_img, size_t kernel_size,
		      char **core_img, size_t *core_size, size_t raw_size)
{
  CLzmaEncProps props;
  unsigned char out_props[5];
  size_t out_props_size = 5;

  LzmaEncProps_Init(&props);
  props.dictSize = 1 << 16;
  props.lc = 3;
  props.lp = 0;
  props.pb = 2;
  props.numThreads = 1;

  if (kernel_size < raw_size)
    grub_util_error (_("the core image is too small"));

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, raw_size);

  *core_size = kernel_size - raw_size;
  if (LzmaEncode ((unsigned char *) *core_img + raw_size, core_size,
		  (unsigned char *) kernel_img + raw_size,
		  kernel_size - raw_size,
		  &props, out_props, &out_props_size,
		  0, NULL, &g_Alloc, &g_Alloc) != SZ_OK)
    grub_util_error (_("cannot compress the kernel image"));

  *core_size += raw_size;
}

#ifdef HAVE_LIBLZMA
static void
compress_kernel_xz (char *kernel_img, size_t kernel_size,
		    char **core_img, size_t *core_size, size_t raw_size)
{
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret xzret;
  lzma_options_lzma lzopts = {
    .dict_size = 1 << 16,
    .preset_dict = NULL,
    .preset_dict_size = 0,
    .lc = 3,
    .lp = 0,
    .pb = 2,
    .mode = LZMA_MODE_NORMAL,
    .nice_len = 64,
    .mf = LZMA_MF_BT4,
    .depth = 0,
  };
  lzma_filter fltrs[] = {
    { .id = LZMA_FILTER_LZMA2, .options = &lzopts},
    { .id = LZMA_VLI_UNKNOWN, .options = NULL}
  };

  if (kernel_size < raw_size)
    grub_util_error (_("the core image is too small"));

  xzret = lzma_stream_encoder (&strm, fltrs, LZMA_CHECK_NONE);
  if (xzret != LZMA_OK)
    grub_util_error (_("cannot compress the kernel image"));

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, raw_size);

  *core_size = kernel_size - raw_size;
  strm.next_in = (unsigned char *) kernel_img + raw_size;
  strm.avail_in = kernel_size - raw_size;
  strm.next_out = (unsigned char *) *core_img + raw_size;
  strm.avail_out = *core_size;

  while (1)
    {
      xzret = lzma_code (&strm, LZMA_FINISH);
      if (xzret == LZMA_OK)
	continue;
      if (xzret == LZMA_STREAM_END)
	break;
      grub_util_error (_("cannot compress the kernel image"));
    }

  *core_size -= strm.avail_out;

  *core_size += raw_size;
}
#endif

static void
compress_kernel (struct image_target_desc *image_target, char *kernel_img,
		 size_t kernel_size, char **core_img, size_t *core_size,
		 grub_compression_t comp)
{
 if (image_target->flags & PLATFORM_FLAGS_LZMA)
   {
     compress_kernel_lzma (kernel_img, kernel_size, core_img,
			   core_size, image_target->raw_size);
     return;
   }

#ifdef HAVE_LIBLZMA
 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp == COMPRESSION_XZ))
   {
     compress_kernel_xz (kernel_img, kernel_size, core_img,
			 core_size, image_target->raw_size);
     return;
   }
#endif

 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp != COMPRESSION_NONE))
   grub_util_error ("unknown compression %d\n", comp);

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, kernel_size);
  *core_size = kernel_size;
}

struct fixup_block_list
{
  struct fixup_block_list *next;
  int state;
  struct grub_pe32_fixup_block b;
};

#define MKIMAGE_ELF32 1
#include "grub-mkimagexx.c"
#undef MKIMAGE_ELF32

#define MKIMAGE_ELF64 1
#include "grub-mkimagexx.c"
#undef MKIMAGE_ELF64

static void
generate_image (const char *dir, char *prefix, FILE *out, char *mods[],
		char *memdisk_path, char *config_path,
		struct image_target_desc *image_target, int note,
		grub_compression_t comp)
{
  char *kernel_img, *core_img;
  size_t kernel_size, total_module_size, core_size, exec_size;
  size_t memdisk_size = 0, config_size = 0, config_size_pure = 0;
  char *kernel_path;
  size_t offset;
  struct grub_util_path_list *path_list, *p, *next;
  grub_size_t bss_size;
  grub_uint64_t start_address;
  void *rel_section;
  grub_size_t reloc_size, align;

  if (comp == COMPRESSION_AUTO)
    comp = image_target->default_compression;

  path_list = grub_util_resolve_dependencies (dir, "moddep.lst", mods);

  kernel_path = grub_util_get_path (dir, "kernel.img");

  if (image_target->voidp_sizeof == 8)
    total_module_size = sizeof (struct grub_module_info64);
  else
    total_module_size = sizeof (struct grub_module_info32);

  if (memdisk_path)
    {
      memdisk_size = ALIGN_UP(grub_util_get_image_size (memdisk_path), 512);
      grub_util_info ("the size of memory disk is 0x%x", memdisk_size);
      total_module_size += memdisk_size + sizeof (struct grub_module_header);
    }

  if (config_path)
    {
      config_size_pure = grub_util_get_image_size (config_path) + 1;
      config_size = ALIGN_ADDR (config_size_pure);
      grub_util_info ("the size of config file is 0x%x", config_size);
      total_module_size += config_size + sizeof (struct grub_module_header);
    }

  for (p = path_list; p; p = p->next)
    total_module_size += (ALIGN_ADDR (grub_util_get_image_size (p->name))
			  + sizeof (struct grub_module_header));

  grub_util_info ("the total module size is 0x%x", total_module_size);

  if (image_target->voidp_sizeof == 4)
    kernel_img = load_image32 (kernel_path, &exec_size, &kernel_size, &bss_size,
			       total_module_size, &start_address, &rel_section,
			       &reloc_size, &align, image_target);
  else
    kernel_img = load_image64 (kernel_path, &exec_size, &kernel_size, &bss_size,
			       total_module_size, &start_address, &rel_section,
			       &reloc_size, &align, image_target);

  if (image_target->prefix + strlen (prefix) + 1 > image_target->prefix_end)
    grub_util_error (_("prefix is too long"));
  strcpy (kernel_img + image_target->prefix, prefix);

  if (image_target->voidp_sizeof == 8)
    {
      /* Fill in the grub_module_info structure.  */
      struct grub_module_info64 *modinfo;
      modinfo = (struct grub_module_info64 *) (kernel_img + kernel_size);
      memset (modinfo, 0, sizeof (struct grub_module_info64));
      modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
      modinfo->offset = grub_host_to_target_addr (sizeof (struct grub_module_info64));
      modinfo->size = grub_host_to_target_addr (total_module_size);
      offset = kernel_size + sizeof (struct grub_module_info64);
    }
  else
    {
      /* Fill in the grub_module_info structure.  */
      struct grub_module_info32 *modinfo;
      modinfo = (struct grub_module_info32 *) (kernel_img + kernel_size);
      memset (modinfo, 0, sizeof (struct grub_module_info32));
      modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
      modinfo->offset = grub_host_to_target_addr (sizeof (struct grub_module_info32));
      modinfo->size = grub_host_to_target_addr (total_module_size);
      offset = kernel_size + sizeof (struct grub_module_info32);
    }

  for (p = path_list; p; p = p->next)
    {
      struct grub_module_header *header;
      size_t mod_size, orig_size;

      orig_size = grub_util_get_image_size (p->name);
      mod_size = ALIGN_ADDR (orig_size);

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_ELF);
      header->size = grub_host_to_target32 (mod_size + sizeof (*header));
      offset += sizeof (*header);
      memset (kernel_img + offset + orig_size, 0, mod_size - orig_size);

      grub_util_load_image (p->name, kernel_img + offset);
      offset += mod_size;
    }

  if (memdisk_path)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_MEMDISK);
      header->size = grub_host_to_target32 (memdisk_size + sizeof (*header));
      offset += sizeof (*header);

      grub_util_load_image (memdisk_path, kernel_img + offset);
      offset += memdisk_size;
    }

  if (config_path)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_CONFIG);
      header->size = grub_host_to_target32 (config_size + sizeof (*header));
      offset += sizeof (*header);

      grub_util_load_image (config_path, kernel_img + offset);
      *(kernel_img + offset + config_size_pure - 1) = 0;
      offset += config_size;
    }

  if ((image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
      && (image_target->total_module_size != TARGET_NO_FIELD))
    *((grub_uint32_t *) (kernel_img + image_target->total_module_size))
      = grub_host_to_target32 (total_module_size);

  grub_util_info ("kernel_img=%p, kernel_size=0x%x", kernel_img, kernel_size);
  compress_kernel (image_target, kernel_img, kernel_size + total_module_size,
		   &core_img, &core_size, comp);

  grub_util_info ("the core size is 0x%x", core_size);

  if (!(image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS) 
      && image_target->total_module_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->total_module_size))
      = grub_host_to_target32 (total_module_size);
  if (image_target->kernel_image_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->kernel_image_size))
      = grub_host_to_target32 (kernel_size);
  if (image_target->compressed_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->compressed_size))
      = grub_host_to_target32 (core_size - image_target->raw_size);

  /* If we included a drive in our prefix, let GRUB know it doesn't have to
     prepend the drive told by BIOS.  */
  if (image_target->install_dos_part != TARGET_NO_FIELD
      && image_target->install_bsd_part != TARGET_NO_FIELD && prefix[0] == '(')
    {
      *((grub_int32_t *) (core_img + image_target->install_dos_part))
	= grub_host_to_target32 (-2);
      *((grub_int32_t *) (core_img + image_target->install_bsd_part))
	= grub_host_to_target32 (-2);
    }

  if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
    {
      char *full_img;
      size_t full_size;
      char *decompress_path, *decompress_img;
      size_t decompress_size;
      const char *name;

      switch (comp)
	{
	case COMPRESSION_XZ:
	  name = "xz_decompress.img";
	  break;
	case COMPRESSION_NONE:
	  name = "none_decompress.img";
	  break;
	default:
	  grub_util_error ("unknown compression %d\n", comp);
	}
      
      decompress_path = grub_util_get_path (dir, name);
      decompress_size = grub_util_get_image_size (decompress_path);
      decompress_img = grub_util_read_image (decompress_path);

      *((grub_uint32_t *) (decompress_img + GRUB_KERNEL_MIPS_YEELOONG_COMPRESSED_SIZE))
	= grub_host_to_target32 (core_size);

      *((grub_uint32_t *) (decompress_img + GRUB_KERNEL_MIPS_YEELOONG_UNCOMPRESSED_SIZE))
	= grub_host_to_target32 (kernel_size + total_module_size);

      full_size = core_size + decompress_size;

      full_img = xmalloc (full_size);
      memset (full_img, 0, full_size); 

      memcpy (full_img, decompress_img, decompress_size);

      memcpy (full_img + decompress_size, core_img, core_size);

      memset (full_img + decompress_size + core_size, 0,
	      full_size - (decompress_size + core_size));

      free (core_img);
      core_img = full_img;
      core_size = full_size;
    }

  switch (image_target->id)
    {
    case IMAGE_I386_PC:
    case IMAGE_I386_PC_PXE:
      {
	unsigned num;
	char *boot_path, *boot_img;
	size_t boot_size;

	if (GRUB_KERNEL_I386_PC_LINK_ADDR + core_size > GRUB_MEMORY_I386_PC_UPPER)
	  grub_util_error (_("core image is too big (%p > %p)"),
			   GRUB_KERNEL_I386_PC_LINK_ADDR + core_size,
			   GRUB_MEMORY_I386_PC_UPPER);

	num = ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);
	if (num > 0xffff)
	  grub_util_error (_("the core image is too big"));

	if (image_target->id == IMAGE_I386_PC_PXE)
	  {
	    char *pxeboot_path, *pxeboot_img;
	    size_t pxeboot_size;
	    
	    pxeboot_path = grub_util_get_path (dir, "pxeboot.img");
	    pxeboot_size = grub_util_get_image_size (pxeboot_path);
	    pxeboot_img = grub_util_read_image (pxeboot_path);
	    
	    grub_util_write_image (pxeboot_img, pxeboot_size, out);
	    free (pxeboot_img);
	    free (pxeboot_path);
	  }

	boot_path = grub_util_get_path (dir, "diskboot.img");
	boot_size = grub_util_get_image_size (boot_path);
	if (boot_size != GRUB_DISK_SECTOR_SIZE)
	  grub_util_error (_("diskboot.img size must be %u bytes"),
			   GRUB_DISK_SECTOR_SIZE);

	boot_img = grub_util_read_image (boot_path);

	{
	  struct grub_pc_bios_boot_blocklist *block;
	  block = (struct grub_pc_bios_boot_blocklist *) (boot_img
							  + GRUB_DISK_SECTOR_SIZE
							  - sizeof (*block));
	  block->len = grub_host_to_target16 (num);

	  /* This is filled elsewhere.  Verify it just in case.  */
	  assert (block->segment
		  == grub_host_to_target16 (GRUB_BOOT_I386_PC_KERNEL_SEG
					    + (GRUB_DISK_SECTOR_SIZE >> 4)));
	}

	grub_util_write_image (boot_img, boot_size, out);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_EFI:
      {
	void *pe_img;
	grub_uint8_t *header;
	void *sections;
	size_t pe_size;
	struct grub_pe32_coff_header *c;
	struct grub_pe32_section_table *text_section, *data_section;
	struct grub_pe32_section_table *mods_section, *reloc_section;
	static const grub_uint8_t stub[] = GRUB_PE32_MSDOS_STUB;
	int header_size;
	int reloc_addr;

	if (image_target->voidp_sizeof == 4)
	  header_size = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				  + GRUB_PE32_SIGNATURE_SIZE
				  + sizeof (struct grub_pe32_coff_header)
				  + sizeof (struct grub_pe32_optional_header)
				  + 4 * sizeof (struct grub_pe32_section_table),
				  GRUB_PE32_SECTION_ALIGNMENT);
	else
	  header_size = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				  + GRUB_PE32_SIGNATURE_SIZE
				  + sizeof (struct grub_pe32_coff_header)
				  + sizeof (struct grub_pe64_optional_header)
				  + 4 * sizeof (struct grub_pe32_section_table),
				  GRUB_PE32_SECTION_ALIGNMENT);

	reloc_addr = ALIGN_UP (header_size + core_size,
			       image_target->section_align);

	pe_size = ALIGN_UP (reloc_addr + reloc_size,
			    image_target->section_align);
	pe_img = xmalloc (reloc_addr + reloc_size);
	memset (pe_img, 0, header_size);
	memcpy (pe_img + header_size, core_img, core_size);
	memcpy (pe_img + reloc_addr, rel_section, reloc_size);
	header = pe_img;

	/* The magic.  */
	memcpy (header, stub, GRUB_PE32_MSDOS_STUB_SIZE);
	memcpy (header + GRUB_PE32_MSDOS_STUB_SIZE, "PE\0\0",
		GRUB_PE32_SIGNATURE_SIZE);

	/* The COFF file header.  */
	c = (struct grub_pe32_coff_header *) (header + GRUB_PE32_MSDOS_STUB_SIZE
					      + GRUB_PE32_SIGNATURE_SIZE);
	if (image_target->voidp_sizeof == 4)
	  c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_I386);
	else
	  c->machine = grub_host_to_target16 (GRUB_PE32_MACHINE_X86_64);

	c->num_sections = grub_host_to_target16 (4);
	c->time = grub_host_to_target32 (time (0));
	c->characteristics = grub_host_to_target16 (GRUB_PE32_EXECUTABLE_IMAGE
						    | GRUB_PE32_LINE_NUMS_STRIPPED
						    | ((image_target->voidp_sizeof == 4)
						       ? GRUB_PE32_32BIT_MACHINE
						       : 0)
						    | GRUB_PE32_LOCAL_SYMS_STRIPPED
						    | GRUB_PE32_DEBUG_STRIPPED);

	/* The PE Optional header.  */
	if (image_target->voidp_sizeof == 4)
	  {
	    struct grub_pe32_optional_header *o;

	    c->optional_header_size = grub_host_to_target16 (sizeof (struct grub_pe32_optional_header));

	    o = (struct grub_pe32_optional_header *)
	      (header + GRUB_PE32_MSDOS_STUB_SIZE + GRUB_PE32_SIGNATURE_SIZE
	       + sizeof (struct grub_pe32_coff_header));
	    o->magic = grub_host_to_target16 (GRUB_PE32_PE32_MAGIC);
	    o->code_size = grub_host_to_target32 (exec_size);
	    o->data_size = grub_cpu_to_le32 (reloc_addr - exec_size
					     - header_size);
	    o->bss_size = grub_cpu_to_le32 (bss_size);
	    o->entry_addr = grub_cpu_to_le32 (start_address);
	    o->code_base = grub_cpu_to_le32 (header_size);

	    o->data_base = grub_host_to_target32 (header_size + exec_size);

	    o->image_base = 0;
	    o->section_alignment = grub_host_to_target32 (image_target->section_align);
	    o->file_alignment = grub_host_to_target32 (image_target->section_align);
	    o->image_size = grub_host_to_target32 (pe_size);
	    o->header_size = grub_host_to_target32 (header_size);
	    o->subsystem = grub_host_to_target16 (GRUB_PE32_SUBSYSTEM_EFI_APPLICATION);

	    /* Do these really matter? */
	    o->stack_reserve_size = grub_host_to_target32 (0x10000);
	    o->stack_commit_size = grub_host_to_target32 (0x10000);
	    o->heap_reserve_size = grub_host_to_target32 (0x10000);
	    o->heap_commit_size = grub_host_to_target32 (0x10000);
    
	    o->num_data_directories = grub_host_to_target32 (GRUB_PE32_NUM_DATA_DIRECTORIES);

	    o->base_relocation_table.rva = grub_host_to_target32 (reloc_addr);
	    o->base_relocation_table.size = grub_host_to_target32 (reloc_size);
	    sections = o + 1;
	  }
	else
	  {
	    struct grub_pe64_optional_header *o;

	    c->optional_header_size = grub_host_to_target16 (sizeof (struct grub_pe64_optional_header));

	    o = (struct grub_pe64_optional_header *) 
	      (header + GRUB_PE32_MSDOS_STUB_SIZE + GRUB_PE32_SIGNATURE_SIZE
	       + sizeof (struct grub_pe32_coff_header));
	    o->magic = grub_host_to_target16 (GRUB_PE32_PE64_MAGIC);
	    o->code_size = grub_host_to_target32 (exec_size);
	    o->data_size = grub_cpu_to_le32 (reloc_addr - exec_size
					     - header_size);
	    o->bss_size = grub_cpu_to_le32 (bss_size);
	    o->entry_addr = grub_cpu_to_le32 (start_address);
	    o->code_base = grub_cpu_to_le32 (header_size);
	    o->image_base = 0;
	    o->section_alignment = grub_host_to_target32 (image_target->section_align);
	    o->file_alignment = grub_host_to_target32 (image_target->section_align);
	    o->image_size = grub_host_to_target32 (pe_size);
	    o->header_size = grub_host_to_target32 (header_size);
	    o->subsystem = grub_host_to_target16 (GRUB_PE32_SUBSYSTEM_EFI_APPLICATION);

	    /* Do these really matter? */
	    o->stack_reserve_size = grub_host_to_target32 (0x10000);
	    o->stack_commit_size = grub_host_to_target32 (0x10000);
	    o->heap_reserve_size = grub_host_to_target32 (0x10000);
	    o->heap_commit_size = grub_host_to_target32 (0x10000);
    
	    o->num_data_directories
	      = grub_host_to_target32 (GRUB_PE32_NUM_DATA_DIRECTORIES);

	    o->base_relocation_table.rva = grub_host_to_target32 (reloc_addr);
	    o->base_relocation_table.size = grub_host_to_target32 (reloc_size);
	    sections = o + 1;
	  }
	/* The sections.  */
	text_section = sections;
	strcpy (text_section->name, ".text");
	text_section->virtual_size = grub_cpu_to_le32 (exec_size);
	text_section->virtual_address = grub_cpu_to_le32 (header_size);
	text_section->raw_data_size = grub_cpu_to_le32 (exec_size);
	text_section->raw_data_offset = grub_cpu_to_le32 (header_size);
	text_section->characteristics = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_CODE
							  | GRUB_PE32_SCN_MEM_EXECUTE
							  | GRUB_PE32_SCN_MEM_READ);

	data_section = text_section + 1;
	strcpy (data_section->name, ".data");
	data_section->virtual_size = grub_cpu_to_le32 (kernel_size - exec_size);
	data_section->virtual_address = grub_cpu_to_le32 (header_size + exec_size);
	data_section->raw_data_size = grub_cpu_to_le32 (kernel_size - exec_size);
	data_section->raw_data_offset = grub_cpu_to_le32 (header_size + exec_size);
	data_section->characteristics
	  = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			      | GRUB_PE32_SCN_MEM_READ
			      | GRUB_PE32_SCN_MEM_WRITE);

#if 0
	bss_section = data_section + 1;
	strcpy (bss_section->name, ".bss");
	bss_section->virtual_size = grub_cpu_to_le32 (bss_size);
	bss_section->virtual_address = grub_cpu_to_le32 (header_size + kernel_size);
	bss_section->raw_data_size = 0;
	bss_section->raw_data_offset = 0;
	bss_section->characteristics
	  = grub_cpu_to_le32 (GRUB_PE32_SCN_MEM_READ
			      | GRUB_PE32_SCN_MEM_WRITE
			      | GRUB_PE32_SCN_ALIGN_64BYTES
			      | GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			      | 0x80);
#endif
    
	mods_section = data_section + 1;
	strcpy (mods_section->name, "mods");
	mods_section->virtual_size = grub_cpu_to_le32 (reloc_addr - kernel_size - header_size);
	mods_section->virtual_address = grub_cpu_to_le32 (header_size + kernel_size + bss_size);
	mods_section->raw_data_size = grub_cpu_to_le32 (reloc_addr - kernel_size - header_size);
	mods_section->raw_data_offset = grub_cpu_to_le32 (header_size + kernel_size);
	mods_section->characteristics
	  = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			      | GRUB_PE32_SCN_MEM_READ
			      | GRUB_PE32_SCN_MEM_WRITE);

	reloc_section = mods_section + 1;
	strcpy (reloc_section->name, ".reloc");
	reloc_section->virtual_size = grub_cpu_to_le32 (reloc_size);
	reloc_section->virtual_address = grub_cpu_to_le32 (reloc_addr + bss_size);
	reloc_section->raw_data_size = grub_cpu_to_le32 (reloc_size);
	reloc_section->raw_data_offset = grub_cpu_to_le32 (reloc_addr);
	reloc_section->characteristics
	  = grub_cpu_to_le32 (GRUB_PE32_SCN_CNT_INITIALIZED_DATA
			      | GRUB_PE32_SCN_MEM_DISCARDABLE
			      | GRUB_PE32_SCN_MEM_READ);
	free (core_img);
	core_img = pe_img;
	core_size = pe_size;
      }
      break;
    case IMAGE_QEMU:
      {
	char *rom_img;
	size_t rom_size;
	char *boot_path, *boot_img;
	size_t boot_size;

	boot_path = grub_util_get_path (dir, "boot.img");
	boot_size = grub_util_get_image_size (boot_path);
	boot_img = grub_util_read_image (boot_path);

	/* Rom sizes must be 64k-aligned.  */
	rom_size = ALIGN_UP (core_size + boot_size, 64 * 1024);

	rom_img = xmalloc (rom_size);
	memset (rom_img, 0, rom_size);

	*((grub_int32_t *) (core_img + GRUB_KERNEL_I386_QEMU_CORE_ENTRY_ADDR))
	  = grub_host_to_target32 ((grub_uint32_t) -rom_size);

	memcpy (rom_img, core_img, core_size);

	*((grub_int32_t *) (boot_img + GRUB_BOOT_I386_QEMU_CORE_ENTRY_ADDR))
	  = grub_host_to_target32 ((grub_uint32_t) -rom_size);

	memcpy (rom_img + rom_size - boot_size, boot_img, boot_size);

	free (core_img);
	core_img = rom_img;
	core_size = rom_size;

	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_SPARC64_AOUT:
      {
	void *aout_img;
	size_t aout_size;
	struct grub_aout32_header *aout_head;

	aout_size = core_size + sizeof (*aout_head);
	aout_img = xmalloc (aout_size);
	aout_head = aout_img;
	aout_head->a_midmag = grub_host_to_target32 ((AOUT_MID_SUN << 16)
						     | AOUT32_OMAGIC);
	aout_head->a_text = grub_host_to_target32 (core_size);
	aout_head->a_entry
	  = grub_host_to_target32 (GRUB_BOOT_SPARC64_IEEE1275_IMAGE_ADDRESS);
	memcpy (aout_img + sizeof (*aout_head), core_img, core_size);

	free (core_img);
	core_img = aout_img;
	core_size = aout_size;
      }
      break;
    case IMAGE_SPARC64_RAW:
      {
	unsigned int num;
	char *boot_path, *boot_img;
	size_t boot_size;

	num = ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);
	num <<= GRUB_DISK_SECTOR_BITS;

	boot_path = grub_util_get_path (dir, "diskboot.img");
	boot_size = grub_util_get_image_size (boot_path);
	if (boot_size != GRUB_DISK_SECTOR_SIZE)
	  grub_util_error ("diskboot.img is not one sector size");

	boot_img = grub_util_read_image (boot_path);

	*((grub_uint32_t *) (boot_img + GRUB_DISK_SECTOR_SIZE
			     - GRUB_BOOT_SPARC64_IEEE1275_LIST_SIZE + 8))
	  = grub_host_to_target32 (num);

	grub_util_write_image (boot_img, boot_size, out);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_YEELOONG_FLASH:
    {
      char *rom_img;
      size_t rom_size;
      char *boot_path, *boot_img;
      size_t boot_size;
      
      boot_path = grub_util_get_path (dir, "fwstart.img");
      boot_size = grub_util_get_image_size (boot_path);
      boot_img = grub_util_read_image (boot_path);

      rom_size = ALIGN_UP (core_size + boot_size, 512 * 1024);

      rom_img = xmalloc (rom_size);
      memset (rom_img, 0, rom_size); 

      memcpy (rom_img, boot_img, boot_size);

      memcpy (rom_img + boot_size, core_img, core_size);

      memset (rom_img + boot_size + core_size, 0,
	      rom_size - (boot_size + core_size));

      free (core_img);
      core_img = rom_img;
      core_size = rom_size;
    }
    break;
    case IMAGE_YEELOONG_ELF:
    case IMAGE_PPC:
    case IMAGE_COREBOOT:
    case IMAGE_I386_IEEE1275:
      {
	char *elf_img;
	size_t program_size;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	grub_uint32_t target_addr;
	int header_size, footer_size = 0;
	int phnum = 1;
	
	if (image_target->id != IMAGE_YEELOONG_ELF)
	  phnum += 2;

	if (note)
	  {
	    phnum++;
	    footer_size += sizeof (struct grub_ieee1275_note);
	  }
	header_size = ALIGN_ADDR (sizeof (*ehdr) + phnum * sizeof (*phdr));

	program_size = ALIGN_ADDR (core_size);

	elf_img = xmalloc (program_size + header_size + footer_size);
	memset (elf_img, 0, program_size + header_size);
	memcpy (elf_img  + header_size, core_img, core_size);
	ehdr = (void *) elf_img;
	phdr = (void *) (elf_img + sizeof (*ehdr));
	memcpy (ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	if (!image_target->bigendian)
	  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	else
	  ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
	ehdr->e_type = grub_host_to_target16 (ET_EXEC);
	ehdr->e_machine = grub_host_to_target16 (image_target->elf_target);
	ehdr->e_version = grub_host_to_target32 (EV_CURRENT);

	ehdr->e_phoff = grub_host_to_target32 ((char *) phdr - (char *) ehdr);
	ehdr->e_phentsize = grub_host_to_target16 (sizeof (*phdr));
	ehdr->e_phnum = grub_host_to_target16 (phnum);

	/* No section headers.  */
	ehdr->e_shoff = grub_host_to_target32 (0);
	if (image_target->id == IMAGE_YEELOONG_ELF)
	  ehdr->e_shentsize = grub_host_to_target16 (0);
	else
	  ehdr->e_shentsize = grub_host_to_target16 (sizeof (Elf32_Shdr));
	ehdr->e_shnum = grub_host_to_target16 (0);
	ehdr->e_shstrndx = grub_host_to_target16 (0);

	ehdr->e_ehsize = grub_host_to_target16 (sizeof (*ehdr));

	phdr->p_type = grub_host_to_target32 (PT_LOAD);
	phdr->p_offset = grub_host_to_target32 (header_size);
	phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);

	if (image_target->id == IMAGE_YEELOONG_ELF)
	  target_addr = ALIGN_UP (image_target->link_addr
				  + kernel_size + total_module_size, 32);
	else
	  target_addr = image_target->link_addr;
	ehdr->e_entry = grub_host_to_target32 (target_addr);
	phdr->p_vaddr = grub_host_to_target32 (target_addr);
	phdr->p_paddr = grub_host_to_target32 (target_addr);
	phdr->p_align = grub_host_to_target32 (align > image_target->link_align ? align : image_target->link_align);
	if (image_target->id == IMAGE_YEELOONG_ELF)
	  ehdr->e_flags = grub_host_to_target32 (0x1000 | EF_MIPS_NOREORDER 
						 | EF_MIPS_PIC | EF_MIPS_CPIC);
	else
	  ehdr->e_flags = 0;
	if (image_target->id == IMAGE_YEELOONG_ELF)
	  {
	    phdr->p_filesz = grub_host_to_target32 (core_size);
	    phdr->p_memsz = grub_host_to_target32 (core_size);
	  }
	else
	  {
	    grub_uint32_t target_addr_mods;
	    phdr->p_filesz = grub_host_to_target32 (kernel_size);
	    phdr->p_memsz = grub_host_to_target32 (kernel_size + bss_size);

	    phdr++;
	    phdr->p_type = grub_host_to_target32 (PT_GNU_STACK);
	    phdr->p_offset = grub_host_to_target32 (header_size + kernel_size);
	    phdr->p_paddr = phdr->p_vaddr = phdr->p_filesz = phdr->p_memsz = 0;
	    phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);
	    phdr->p_align = grub_host_to_target32 (image_target->link_align);

	    phdr++;
	    phdr->p_type = grub_host_to_target32 (PT_LOAD);
	    phdr->p_offset = grub_host_to_target32 (header_size + kernel_size);
	    phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);
	    phdr->p_filesz = phdr->p_memsz
	      = grub_host_to_target32 (core_size - kernel_size);

	    target_addr_mods = ALIGN_UP (target_addr + kernel_size + bss_size
					 + image_target->mod_gap,
					 image_target->mod_align);
	    phdr->p_vaddr = grub_host_to_target32 (target_addr_mods);
	    phdr->p_paddr = grub_host_to_target32 (target_addr_mods);
	    phdr->p_align = grub_host_to_target32 (image_target->link_align);
	  }

	if (note)
	  {
	    int note_size = sizeof (struct grub_ieee1275_note);
	    struct grub_ieee1275_note *note = (struct grub_ieee1275_note *) 
	      (elf_img + program_size + header_size);

	    grub_util_info ("adding CHRP NOTE segment");

	    note->header.namesz = grub_host_to_target32 (sizeof (GRUB_IEEE1275_NOTE_NAME));
	    note->header.descsz = grub_host_to_target32 (note_size);
	    note->header.type = grub_host_to_target32 (GRUB_IEEE1275_NOTE_TYPE);
	    strcpy (note->header.name, GRUB_IEEE1275_NOTE_NAME);
	    note->descriptor.real_mode = grub_host_to_target32 (0xffffffff);
	    note->descriptor.real_base = grub_host_to_target32 (0x00c00000);
	    note->descriptor.real_size = grub_host_to_target32 (0xffffffff);
	    note->descriptor.virt_base = grub_host_to_target32 (0xffffffff);
	    note->descriptor.virt_size = grub_host_to_target32 (0xffffffff);
	    note->descriptor.load_base = grub_host_to_target32 (0x00004000);

	    phdr++;
	    phdr->p_type = grub_host_to_target32 (PT_NOTE);
	    phdr->p_flags = grub_host_to_target32 (PF_R);
	    phdr->p_align = grub_host_to_target32 (image_target->voidp_sizeof);
	    phdr->p_vaddr = 0;
	    phdr->p_paddr = 0;
	    phdr->p_filesz = grub_host_to_target32 (note_size);
	    phdr->p_memsz = 0;
	    phdr->p_offset = grub_host_to_target32 (header_size + program_size);
	  }

	free (core_img);
	core_img = elf_img;
	core_size = program_size + header_size + footer_size;
      }
      break;
    }

  grub_util_write_image (core_img, core_size, out);
  free (kernel_img);
  free (core_img);
  free (kernel_path);

  while (path_list)
    {
      next = path_list->next;
      free ((void *) path_list->name);
      free (path_list);
      path_list = next;
    }
}



static struct option options[] =
  {
    {"directory", required_argument, 0, 'd'},
    {"prefix", required_argument, 0, 'p'},
    {"memdisk", required_argument, 0, 'm'},
    {"font", required_argument, 0, 'f'},
    {"config", required_argument, 0, 'c'},
    {"output", required_argument, 0, 'o'},
    {"note", no_argument, 0, 'n'},
    {"format", required_argument, 0, 'O'},
    {"compression", required_argument, 0, 'C'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, _("Try `%s --help' for more information.\n"), program_name);
  else
    {
      int format_len = 0;
      char *formats;
      char *ptr;
      unsigned i;
      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	format_len += strlen (image_targets[i].name) + 2;
      ptr = formats = xmalloc (format_len);
      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	{
	  strcpy (ptr, image_targets[i].name);
	  ptr += strlen (image_targets[i].name);
	  *ptr++ = ',';
	  *ptr++ = ' ';
	}
      ptr[-2] = 0;

      printf (_("\
Usage: %s [OPTION]... [MODULES]\n\
\n\
Make a bootable image of GRUB.\n\
\n\
  -d, --directory=DIR     use images and modules under DIR [default=%s/@platform@]\n\
  -p, --prefix=DIR        set grub_prefix directory [default=%s]\n\
  -m, --memdisk=FILE      embed FILE as a memdisk image\n\
  -c, --config=FILE       embed FILE as boot config\n\
  -n, --note              add NOTE segment for CHRP Open Firmware\n\
  -o, --output=FILE       output a generated image to FILE [default=stdout]\n\
  -O, --format=FORMAT     generate an image in format\n\
                          available formats: %s\n\
  -C, --compression=(xz|none|auto)  choose the compression to use\n\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
"), 
	      program_name, GRUB_PKGLIBROOTDIR, DEFAULT_DIRECTORY,
	      formats,
	      PACKAGE_BUGREPORT);
      free (formats);
    }
  exit (status);
}

int
main (int argc, char *argv[])
{
  char *output = NULL;
  char *dir = NULL;
  char *prefix = NULL;
  char *memdisk = NULL;
  char *font = NULL;
  char *config = NULL;
  FILE *fp = stdout;
  int note = 0;
  struct image_target_desc *image_target = NULL;
  grub_compression_t comp = COMPRESSION_AUTO;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  while (1)
    {
      int c = getopt_long (argc, argv, "d:p:m:c:o:O:f:C:hVvn", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'o':
	    if (output)
	      free (output);

	    output = xstrdup (optarg);
	    break;

	  case 'O':
	    {
	      unsigned i;
	      for (i = 0; i < ARRAY_SIZE (image_targets); i++)
		if (strcmp (optarg, image_targets[i].name) == 0)
		  image_target = &image_targets[i];
	      if (!image_target)
		{
		  printf ("unknown target format %s\n", optarg);
		  usage (1);
		}
	      break;
	    }
	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;

	  case 'n':
	    note = 1;
	    break;

	  case 'm':
	    if (memdisk)
	      free (memdisk);

	    memdisk = xstrdup (optarg);

	    if (prefix)
	      free (prefix);

	    prefix = xstrdup ("(memdisk)/boot/grub");
	    break;

	  case 'c':
	    if (config)
	      free (config);

	    config = xstrdup (optarg);
	    break;

	  case 'C':
	    if (grub_strcmp (optarg, "xz") == 0)
	      {
#ifdef HAVE_LIBLZMA
		comp = COMPRESSION_XZ;
#else
		grub_util_error ("grub-mkimage is compiled without XZ support",
				 optarg);
#endif
	      }
	    else if (grub_strcmp (optarg, "none") == 0)
	      comp = COMPRESSION_NONE;
	    else
	      grub_util_error ("Unknown compression format %s", optarg);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'p':
	    if (prefix)
	      free (prefix);

	    prefix = xstrdup (optarg);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (!image_target)
    {
      printf ("Target format not specified (use the -O option).\n");
      usage (1);
    }

  if (output)
    {
      fp = fopen (output, "wb");
      if (! fp)
	grub_util_error (_("cannot open %s"), output);
      free (output);
    }

  if (!dir)
    {
      const char *last;
      last = strchr (image_target->name, '-');
      if (last)
	last = strchr (last + 1, '-');
      if (!last)
	last = image_target->name + strlen (image_target->name);
      dir = xmalloc (sizeof (GRUB_PKGLIBROOTDIR) + (last - image_target->name)
		     + 1);
      memcpy (dir, GRUB_PKGLIBROOTDIR, sizeof (GRUB_PKGLIBROOTDIR) - 1);
      *(dir + sizeof (GRUB_PKGLIBROOTDIR) - 1) = '/';
      memcpy (dir + sizeof (GRUB_PKGLIBROOTDIR), image_target->name,
	      last - image_target->name);
      *(dir + sizeof (GRUB_PKGLIBROOTDIR) + (last - image_target->name)) = 0;
    }

  generate_image (dir, prefix ? : DEFAULT_DIRECTORY, fp,
		  argv + optind, memdisk, config,
		  image_target, note, comp);

  fclose (fp);

  if (dir)
    free (dir);

  return 0;
}
