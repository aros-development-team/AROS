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
#include <grub/crypto.h>
#include <grub/dl.h>
#include <time.h>
#include <multiboot.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <grub/efi/pe32.h>

#define _GNU_SOURCE	1
#include <argp.h>

#include "progname.h"

#define ALIGN_ADDR(x) (ALIGN_UP((x), image_target->voidp_sizeof))

#ifdef HAVE_LIBLZMA
#include <lzma.h>
#endif

#define TARGET_NO_FIELD 0xffffffff

typedef enum {
  COMPRESSION_AUTO, COMPRESSION_NONE, COMPRESSION_XZ, COMPRESSION_LZMA
} grub_compression_t;

struct image_target_desc
{
  const char *dirname;
  const char *names[6];
  grub_size_t voidp_sizeof;
  int bigendian;
  enum {
    IMAGE_I386_PC, IMAGE_EFI, IMAGE_COREBOOT,
    IMAGE_SPARC64_AOUT, IMAGE_SPARC64_RAW, IMAGE_I386_IEEE1275,
    IMAGE_LOONGSON_ELF, IMAGE_QEMU, IMAGE_PPC, IMAGE_YEELOONG_FLASH,
    IMAGE_FULOONG2F_FLASH, IMAGE_I386_PC_PXE, IMAGE_MIPS_ARC,
    IMAGE_QEMU_MIPS_FLASH
  } id;
  enum
    {
      PLATFORM_FLAGS_NONE = 0,
      PLATFORM_FLAGS_DECOMPRESSORS = 2,
      PLATFORM_FLAGS_MODULES_BEFORE_KERNEL = 4,
    } flags;
  unsigned total_module_size;
  unsigned decompressor_compressed_size;
  unsigned decompressor_uncompressed_size;
  unsigned decompressor_uncompressed_addr;
  unsigned link_align;
  grub_uint16_t elf_target;
  unsigned section_align;
  signed vaddr_offset;
  grub_uint64_t link_addr;
  unsigned mod_gap, mod_align;
  grub_compression_t default_compression;
  grub_uint16_t pe_target;
};

#define EFI64_HEADER_SIZE ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE		\
				    + GRUB_PE32_SIGNATURE_SIZE		\
				    + sizeof (struct grub_pe32_coff_header) \
				    + sizeof (struct grub_pe64_optional_header) \
				    + 4 * sizeof (struct grub_pe32_section_table), \
				    GRUB_PE32_SECTION_ALIGNMENT)

struct image_target_desc image_targets[] =
  {
    {
      .dirname = "i386-coreboot",
      .names = { "i386-coreboot", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = GRUB_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .dirname = "i386-multiboot",
      .names = { "i386-multiboot", NULL},
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = GRUB_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .dirname = "i386-pc",
      .names = { "i386-pc", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_I386_PC_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_I386_PC_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_PC_LINK_ADDR,
      .default_compression = COMPRESSION_LZMA
    },
    {
      .dirname = "i386-pc",
      .names = { "i386-pc-pxe", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC_PXE, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_I386_PC_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_I386_PC_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_PC_LINK_ADDR,
      .default_compression = COMPRESSION_LZMA
    },
    {
      .dirname = "i386-efi",
      .names = { "i386-efi", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_EFI,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = GRUB_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = ALIGN_UP (GRUB_PE32_MSDOS_STUB_SIZE
				+ GRUB_PE32_SIGNATURE_SIZE
				+ sizeof (struct grub_pe32_coff_header)
				+ sizeof (struct grub_pe32_optional_header)
				+ 4 * sizeof (struct grub_pe32_section_table),
				GRUB_PE32_SECTION_ALIGNMENT),
      .pe_target = GRUB_PE32_MACHINE_I386,
      .elf_target = EM_386,
    },
    {
      .dirname = "i386-ieee1275",
      .names = { "i386-ieee1275", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_IEEE1275, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_IEEE1275_LINK_ADDR,
      .elf_target = EM_386,
      .mod_gap = GRUB_KERNEL_I386_IEEE1275_MOD_GAP,
      .mod_align = GRUB_KERNEL_I386_IEEE1275_MOD_ALIGN,
      .link_align = 4,
    },
    {
      .dirname = "i386-qemu",
      .names = { "i386-qemu", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_QEMU, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_I386_QEMU_LINK_ADDR
    },
    {
      .dirname = "x86_64-efi",
      .names = { "x86_64-efi", NULL },
      .voidp_sizeof = 8,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = GRUB_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI64_HEADER_SIZE,
      .pe_target = GRUB_PE32_MACHINE_X86_64,
      .elf_target = EM_X86_64,
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-yeeloong-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_YEELOONG_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-fuloong2f-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_FULOONG2F_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-loongson-elf", "mipsel-yeeloong-elf",
		 "mipsel-fuloong2f-elf", "mipsel-fuloong2e-elf",
		 "mipsel-fuloong-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "powerpc-ieee1275",
      .names = { "powerpc-ieee1275", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_PPC, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_POWERPC_IEEE1275_LINK_ADDR,
      .elf_target = EM_PPC,
      .mod_gap = GRUB_KERNEL_POWERPC_IEEE1275_MOD_GAP,
      .mod_align = GRUB_KERNEL_POWERPC_IEEE1275_MOD_ALIGN,
      .link_align = 4
    },
    {
      .dirname = "sparc64-ieee1275",
      .names = { "sparc64-ieee1275-raw", NULL },
      .voidp_sizeof = 8,
      .bigendian = 1, 
      .id = IMAGE_SPARC64_RAW,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = GRUB_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_SPARC64_IEEE1275_LINK_ADDR
    },
    {
      .dirname = "sparc64-ieee1275",
      .names = { "sparc64-ieee1275-aout", NULL },
      .voidp_sizeof = 8,
      .bigendian = 1,
      .id = IMAGE_SPARC64_AOUT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = GRUB_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_SPARC64_IEEE1275_LINK_ADDR
    },
    {
      .dirname = "ia64-efi",
      .names = {"ia64-efi", NULL},
      .voidp_sizeof = 8,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = GRUB_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI64_HEADER_SIZE,
      .pe_target = GRUB_PE32_MACHINE_IA64,
      .elf_target = EM_IA_64,
    },
    {
      .dirname = "mips-arc",
      .names = {"mips-arc", NULL},
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_MIPS_ARC, 
      .flags = (PLATFORM_FLAGS_DECOMPRESSORS
		| PLATFORM_FLAGS_MODULES_BEFORE_KERNEL),
      .total_module_size = GRUB_KERNEL_MIPS_ARC_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_ARC_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_ARC_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-qemu_mips",
      .names = { "mipsel-qemu_mips-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mips-qemu_mips",
      .names = { "mips-qemu_mips-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_QEMU_MIPS_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-qemu_mips",
      .names = { "mipsel-qemu_mips-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_QEMU_MIPS_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
    },
    {
      .dirname = "mips-qemu_mips",
      .names = { "mips-qemu_mips-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = GRUB_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = GRUB_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = GRUB_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = COMPRESSION_NONE
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
		      char **core_img, size_t *core_size)
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

  *core_img = xmalloc (kernel_size);

  *core_size = kernel_size;
  if (LzmaEncode ((unsigned char *) *core_img, core_size,
		  (unsigned char *) kernel_img,
		  kernel_size,
		  &props, out_props, &out_props_size,
		  0, NULL, &g_Alloc, &g_Alloc) != SZ_OK)
    grub_util_error ("%s", _("cannot compress the kernel image"));
}

#ifdef HAVE_LIBLZMA
static void
compress_kernel_xz (char *kernel_img, size_t kernel_size,
		    char **core_img, size_t *core_size)
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

  xzret = lzma_stream_encoder (&strm, fltrs, LZMA_CHECK_NONE);
  if (xzret != LZMA_OK)
    grub_util_error ("%s", _("cannot compress the kernel image"));

  *core_img = xmalloc (kernel_size);

  *core_size = kernel_size;
  strm.next_in = (unsigned char *) kernel_img;
  strm.avail_in = kernel_size;
  strm.next_out = (unsigned char *) *core_img;
  strm.avail_out = *core_size;

  while (1)
    {
      xzret = lzma_code (&strm, LZMA_FINISH);
      if (xzret == LZMA_OK)
	continue;
      if (xzret == LZMA_STREAM_END)
	break;
      grub_util_error ("%s", _("cannot compress the kernel image"));
    }

  *core_size -= strm.avail_out;
}
#endif

static void
compress_kernel (struct image_target_desc *image_target, char *kernel_img,
		 size_t kernel_size, char **core_img, size_t *core_size,
		 grub_compression_t comp)
{
  if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
      && (comp == COMPRESSION_LZMA))
    {
      compress_kernel_lzma (kernel_img, kernel_size, core_img,
			    core_size);
      return;
    }

#ifdef HAVE_LIBLZMA
 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp == COMPRESSION_XZ))
   {
     compress_kernel_xz (kernel_img, kernel_size, core_img,
			 core_size);
     return;
   }
#endif

 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp != COMPRESSION_NONE))
   grub_util_error (_("unknown compression %d\n"), comp);

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

#pragma GCC diagnostic ignored "-Wcast-align"

#define MKIMAGE_ELF32 1
#include "grub-mkimagexx.c"
#undef MKIMAGE_ELF32

#define MKIMAGE_ELF64 1
#include "grub-mkimagexx.c"
#undef MKIMAGE_ELF64

static void
generate_image (const char *dir, const char *prefix,
		FILE *out, const char *outname, char *mods[],
		char *memdisk_path, char *config_path,
		struct image_target_desc *image_target, int note,
		grub_compression_t comp)
{
  char *kernel_img, *core_img;
  size_t kernel_size, total_module_size, core_size, exec_size;
  size_t memdisk_size = 0, config_size = 0, config_size_pure = 0;
  size_t prefix_size = 0;
  char *kernel_path;
  size_t offset;
  struct grub_util_path_list *path_list, *p, *next;
  grub_size_t bss_size;
  grub_uint64_t start_address;
  void *rel_section = 0;
  grub_size_t reloc_size = 0, align;
  size_t decompress_size = 0;

  if (comp == COMPRESSION_AUTO)
    comp = image_target->default_compression;

  if (image_target->id == IMAGE_I386_PC
      || image_target->id == IMAGE_I386_PC_PXE)
    comp = COMPRESSION_LZMA;

  path_list = grub_util_resolve_dependencies (dir, "moddep.lst", mods);

  kernel_path = grub_util_get_path (dir, "kernel.img");

  if (image_target->voidp_sizeof == 8)
    total_module_size = sizeof (struct grub_module_info64);
  else
    total_module_size = sizeof (struct grub_module_info32);

  if (memdisk_path)
    {
      memdisk_size = ALIGN_UP(grub_util_get_image_size (memdisk_path), 512);
      grub_util_info ("the size of memory disk is 0x%llx",
		      (unsigned long long) memdisk_size);
      total_module_size += memdisk_size + sizeof (struct grub_module_header);
    }

  if (config_path)
    {
      config_size_pure = grub_util_get_image_size (config_path) + 1;
      config_size = ALIGN_ADDR (config_size_pure);
      grub_util_info ("the size of config file is 0x%llx",
		      (unsigned long long) config_size);
      total_module_size += config_size + sizeof (struct grub_module_header);
    }

  if (prefix)
    {
      prefix_size = ALIGN_ADDR (strlen (prefix) + 1);
      total_module_size += prefix_size + sizeof (struct grub_module_header);
    }

  for (p = path_list; p; p = p->next)
    total_module_size += (ALIGN_ADDR (grub_util_get_image_size (p->name))
			  + sizeof (struct grub_module_header));

  grub_util_info ("the total module size is 0x%llx",
		  (unsigned long long) total_module_size);

  if (image_target->voidp_sizeof == 4)
    kernel_img = load_image32 (kernel_path, &exec_size, &kernel_size, &bss_size,
			       total_module_size, &start_address, &rel_section,
			       &reloc_size, &align, image_target);
  else
    kernel_img = load_image64 (kernel_path, &exec_size, &kernel_size, &bss_size,
			       total_module_size, &start_address, &rel_section,
			       &reloc_size, &align, image_target);

  if ((image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
      && (image_target->total_module_size != TARGET_NO_FIELD))
    *((grub_uint32_t *) (kernel_img + image_target->total_module_size))
      = grub_host_to_target32 (total_module_size);

  if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
    memmove (kernel_img + total_module_size, kernel_img, kernel_size);

  if (image_target->voidp_sizeof == 8)
    {
      /* Fill in the grub_module_info structure.  */
      struct grub_module_info64 *modinfo;
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	modinfo = (struct grub_module_info64 *) kernel_img;
      else
	modinfo = (struct grub_module_info64 *) (kernel_img + kernel_size);
      memset (modinfo, 0, sizeof (struct grub_module_info64));
      modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
      modinfo->offset = grub_host_to_target_addr (sizeof (struct grub_module_info64));
      modinfo->size = grub_host_to_target_addr (total_module_size);
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	offset = sizeof (struct grub_module_info64);
      else
	offset = kernel_size + sizeof (struct grub_module_info64);
    }
  else
    {
      /* Fill in the grub_module_info structure.  */
      struct grub_module_info32 *modinfo;
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	modinfo = (struct grub_module_info32 *) kernel_img;
      else
	modinfo = (struct grub_module_info32 *) (kernel_img + kernel_size);
      memset (modinfo, 0, sizeof (struct grub_module_info32));
      modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
      modinfo->offset = grub_host_to_target_addr (sizeof (struct grub_module_info32));
      modinfo->size = grub_host_to_target_addr (total_module_size);
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	offset = sizeof (struct grub_module_info32);
      else
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

  if (prefix)
    {
      struct grub_module_header *header;

      header = (struct grub_module_header *) (kernel_img + offset);
      memset (header, 0, sizeof (struct grub_module_header));
      header->type = grub_host_to_target32 (OBJ_TYPE_PREFIX);
      header->size = grub_host_to_target32 (prefix_size + sizeof (*header));
      offset += sizeof (*header);

      grub_memset (kernel_img + offset, 0, prefix_size);
      grub_strcpy (kernel_img + offset, prefix);
      offset += prefix_size;
    }

  grub_util_info ("kernel_img=%p, kernel_size=0x%llx", kernel_img,
		  (unsigned long long) kernel_size);
  compress_kernel (image_target, kernel_img, kernel_size + total_module_size,
		   &core_img, &core_size, comp);
  free (kernel_img);

  grub_util_info ("the core size is 0x%llx", (unsigned long long) core_size);

  if (!(image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS) 
      && image_target->total_module_size != TARGET_NO_FIELD)
    *((grub_uint32_t *) (core_img + image_target->total_module_size))
      = grub_host_to_target32 (total_module_size);

  if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
    {
      char *full_img;
      size_t full_size;
      char *decompress_path, *decompress_img;
      const char *name;

      switch (comp)
	{
	case COMPRESSION_XZ:
	  name = "xz_decompress.img";
	  break;
	case COMPRESSION_LZMA:
	  name = "lzma_decompress.img";
	  break;
	case COMPRESSION_NONE:
	  name = "none_decompress.img";
	  break;
	default:
	  grub_util_error (_("unknown compression %d\n"), comp);
	}
      
      decompress_path = grub_util_get_path (dir, name);
      decompress_size = grub_util_get_image_size (decompress_path);
      decompress_img = grub_util_read_image (decompress_path);

      if ((image_target->id == IMAGE_I386_PC
	   || image_target->id == IMAGE_I386_PC_PXE)
	  && decompress_size > GRUB_KERNEL_I386_PC_LINK_ADDR - 0x8200)
	grub_util_error ("%s", _("Decompressor is too big"));

      if (image_target->decompressor_compressed_size != TARGET_NO_FIELD)
	*((grub_uint32_t *) (decompress_img
			     + image_target->decompressor_compressed_size))
	  = grub_host_to_target32 (core_size);

      if (image_target->decompressor_uncompressed_size != TARGET_NO_FIELD)
	*((grub_uint32_t *) (decompress_img
			     + image_target->decompressor_uncompressed_size))
	  = grub_host_to_target32 (kernel_size + total_module_size);

      if (image_target->decompressor_uncompressed_addr != TARGET_NO_FIELD)
	{
	  if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	    *((grub_uint32_t *) (decompress_img + image_target->decompressor_uncompressed_addr))
	      = grub_host_to_target_addr (image_target->link_addr - total_module_size);
	  else
	    *((grub_uint32_t *) (decompress_img + image_target->decompressor_uncompressed_addr))
	      = grub_host_to_target_addr (image_target->link_addr);
	}
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

	if (GRUB_KERNEL_I386_PC_LINK_ADDR + core_size > 0x78000
	    || (core_size > (0xffff << GRUB_DISK_SECTOR_BITS)))
	  grub_util_error (_("core image is too big (0x%x > 0x%x)"),
			   GRUB_KERNEL_I386_PC_LINK_ADDR + core_size,
			   0x78000);

	num = ((core_size + GRUB_DISK_SECTOR_SIZE - 1) >> GRUB_DISK_SECTOR_BITS);
	if (image_target->id == IMAGE_I386_PC_PXE)
	  {
	    char *pxeboot_path, *pxeboot_img;
	    size_t pxeboot_size;
	    grub_uint32_t *ptr;
	    
	    pxeboot_path = grub_util_get_path (dir, "pxeboot.img");
	    pxeboot_size = grub_util_get_image_size (pxeboot_path);
	    pxeboot_img = grub_util_read_image (pxeboot_path);
	    
	    grub_util_write_image (pxeboot_img, pxeboot_size, out,
				   outname);
	    free (pxeboot_img);
	    free (pxeboot_path);

	    /* Remove Multiboot header to avoid confusing ipxe.  */
	    for (ptr = (grub_uint32_t *) core_img;
		 ptr < (grub_uint32_t *) (core_img + MULTIBOOT_SEARCH); ptr++)
	      if (*ptr == grub_host_to_target32 (MULTIBOOT_HEADER_MAGIC)
		  && grub_target_to_host32 (ptr[0])
		  + grub_target_to_host32 (ptr[1])
		  + grub_target_to_host32 (ptr[2]) == 0)
		{
		  *ptr = 0;
		  break;
		}
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

	grub_util_write_image (boot_img, boot_size, out, outname);
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
	  header_size = EFI64_HEADER_SIZE;

	reloc_addr = ALIGN_UP (header_size + core_size,
			       image_target->section_align);

	pe_size = ALIGN_UP (reloc_addr + reloc_size,
			    image_target->section_align);
	pe_img = xmalloc (reloc_addr + reloc_size);
	memset (pe_img, 0, header_size);
	memcpy ((char *) pe_img + header_size, core_img, core_size);
	memcpy ((char *) pe_img + reloc_addr, rel_section, reloc_size);
	header = pe_img;

	/* The magic.  */
	memcpy (header, stub, GRUB_PE32_MSDOS_STUB_SIZE);
	memcpy (header + GRUB_PE32_MSDOS_STUB_SIZE, "PE\0\0",
		GRUB_PE32_SIGNATURE_SIZE);

	/* The COFF file header.  */
	c = (struct grub_pe32_coff_header *) (header + GRUB_PE32_MSDOS_STUB_SIZE
					      + GRUB_PE32_SIGNATURE_SIZE);
	c->machine = grub_host_to_target16 (image_target->pe_target);

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
	grub_memset (aout_head, 0, sizeof (*aout_head));
	aout_head->a_midmag = grub_host_to_target32 ((AOUT_MID_SUN << 16)
						     | AOUT32_OMAGIC);
	aout_head->a_text = grub_host_to_target32 (core_size);
	aout_head->a_entry
	  = grub_host_to_target32 (GRUB_BOOT_SPARC64_IEEE1275_IMAGE_ADDRESS);
	memcpy ((char *) aout_img + sizeof (*aout_head), core_img, core_size);

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
	  grub_util_error (_("diskboot.img size must be %u bytes"),
			   GRUB_DISK_SECTOR_SIZE);

	boot_img = grub_util_read_image (boot_path);

	*((grub_uint32_t *) (boot_img + GRUB_DISK_SECTOR_SIZE
			     - GRUB_BOOT_SPARC64_IEEE1275_LIST_SIZE + 8))
	  = grub_host_to_target32 (num);

	grub_util_write_image (boot_img, boot_size, out, outname);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_YEELOONG_FLASH:
    case IMAGE_FULOONG2F_FLASH:
    {
      char *rom_img;
      size_t rom_size;
      char *boot_path, *boot_img;
      size_t boot_size;
      grub_uint8_t context[GRUB_MD_SHA512->contextsize];
      /* fwstart.img is the only part which can't be tested by using *-elf
	 target. Check it against the checksum. */
      const grub_uint8_t yeeloong_fwstart_good_hash[512 / 8] = 
	{ 
	  0x11, 0x7f, 0xfd, 0x7e, 0xd9, 0xbb, 0x82, 0xe7,
	  0x5f, 0xcc, 0xbf, 0x09, 0x1d, 0xfe, 0xfa, 0xd5,
	  0x97, 0xfb, 0xbb, 0xd8, 0x76, 0x4b, 0xfc, 0x0a,
	  0x4e, 0x3c, 0x91, 0x06, 0x98, 0xa0, 0xe0, 0xda,
	  0x4f, 0x74, 0x17, 0x6f, 0x95, 0xd2, 0xec, 0x1b,
	  0x7f, 0x12, 0x80, 0x23, 0xcb, 0xa0, 0x2d, 0x59,
	  0x15, 0x82, 0x70, 0x3d, 0x23, 0xbf, 0xee, 0x93,
	  0x5e, 0x5c, 0xbd, 0x1c, 0x51, 0x0b, 0x0b, 0x45 };
      const grub_uint8_t fuloong2f_fwstart_good_hash[512 / 8] = 
	{ 
	  0x76, 0x9b, 0xad, 0x6e, 0xa2, 0x39, 0x47, 0x62,
	  0x1f, 0xc9, 0x3a, 0x6d, 0x05, 0x5c, 0x43, 0x5c,
	  0x29, 0x4a, 0x7e, 0x08, 0x2a, 0x31, 0x8f, 0x5d,
	  0x02, 0x84, 0xa0, 0x85, 0xf2, 0xd1, 0xb9, 0x53,
	  0xa2, 0xbc, 0xf2, 0xe1, 0x39, 0x1e, 0x51, 0xb5,
	  0xaf, 0xec, 0x9e, 0xf2, 0xf1, 0xf3, 0x0a, 0x2f,
	  0xe6, 0xf1, 0x08, 0x89, 0xbe, 0xbc, 0x73, 0xab,
	  0x46, 0x50, 0xd6, 0x21, 0xce, 0x8e, 0x24, 0xa7
	};
      const grub_uint8_t *fwstart_good_hash;
            
      if (image_target->id == IMAGE_FULOONG2F_FLASH)
	{
	  fwstart_good_hash = fuloong2f_fwstart_good_hash;
	  boot_path = grub_util_get_path (dir, "fwstart_fuloong2f.img");
	}
      else
	{
	  fwstart_good_hash = yeeloong_fwstart_good_hash;
	  boot_path = grub_util_get_path (dir, "fwstart.img");
	}

      boot_size = grub_util_get_image_size (boot_path);
      boot_img = grub_util_read_image (boot_path);

      grub_memset (context, 0, sizeof (context));
      GRUB_MD_SHA512->init (context);
      GRUB_MD_SHA512->write (context, boot_img, boot_size);
      GRUB_MD_SHA512->final (context);
      if (grub_memcmp (GRUB_MD_SHA512->read (context), fwstart_good_hash,
		       GRUB_MD_SHA512->mdlen) != 0)
	/* TRANSLATORS: fwstart.img may still be good, just it wasn't checked.  */
	grub_util_warn ("%s",
			_("fwstart.img doesn't match the known good version. "
			  "proceed at your own risk"));

      if (core_size + boot_size > 512 * 1024)
	grub_util_error ("%s", _("firmware image is too big"));
      rom_size = 512 * 1024;

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
    case IMAGE_QEMU_MIPS_FLASH:
    {
      char *rom_img;
      size_t rom_size;

      if (core_size > 512 * 1024)
	grub_util_error ("%s", _("firmware image is too big"));
      rom_size = 512 * 1024;

      rom_img = xmalloc (rom_size);
      memset (rom_img, 0, rom_size); 

      memcpy (rom_img, core_img, core_size);

      memset (rom_img + core_size, 0,
	      rom_size - core_size);

      free (core_img);
      core_img = rom_img;
      core_size = rom_size;
    }
    break;
    case IMAGE_MIPS_ARC:
      {
	char *ecoff_img;
	struct ecoff_header {
	  grub_uint16_t magic;
	  grub_uint16_t nsec;
	  grub_uint32_t time;
	  grub_uint32_t syms;
	  grub_uint32_t nsyms;
	  grub_uint16_t opt;
	  grub_uint16_t flags;
	  grub_uint16_t magic2;
	  grub_uint16_t version;
	  grub_uint32_t textsize;
	  grub_uint32_t datasize;
	  grub_uint32_t bsssize;
	  grub_uint32_t entry;
	  grub_uint32_t text_start;
	  grub_uint32_t data_start;
	  grub_uint32_t bss_start;
	  grub_uint32_t gprmask;
	  grub_uint32_t cprmask[4];
	  grub_uint32_t gp_value;
	};
	struct ecoff_section
	{
	  char name[8];
	  grub_uint32_t paddr;
	  grub_uint32_t vaddr;
	  grub_uint32_t size;
	  grub_uint32_t file_offset;
	  grub_uint32_t reloc;
	  grub_uint32_t gp;
	  grub_uint16_t nreloc;
	  grub_uint16_t ngp;
	  grub_uint32_t flags;
	};
	struct ecoff_header *head;
	struct ecoff_section *section;
	grub_uint32_t target_addr;
	size_t program_size;

	program_size = ALIGN_ADDR (core_size);
	if (comp == COMPRESSION_NONE)
	  target_addr = (image_target->link_addr 
			 - total_module_size - decompress_size);
	else
	  target_addr = (image_target->link_addr 
			 - ALIGN_UP(total_module_size + core_size, 1048576)
			 - (1 << 20));

	ecoff_img = xmalloc (program_size + sizeof (*head) + sizeof (*section));
	grub_memset (ecoff_img, 0, program_size + sizeof (*head) + sizeof (*section));
	head = (void *) ecoff_img;
	section = (void *) (head + 1);
	head->magic = grub_host_to_target16 (0x160);
	head->nsec = grub_host_to_target16 (1);
	head->time = grub_host_to_target32 (0);
	head->opt = grub_host_to_target16 (0x38);
	head->flags = grub_host_to_target16 (0x207);
	head->magic2 = grub_host_to_target16 (0x107);
	head->textsize = grub_host_to_target32 (program_size);
	head->entry = grub_host_to_target32 (target_addr);
	head->text_start = grub_host_to_target32 (target_addr);
	head->data_start = grub_host_to_target32 (target_addr + program_size);
	grub_memcpy (section->name, ".text", sizeof (".text") - 1); 
	section->vaddr = grub_host_to_target32 (target_addr);
	section->size = grub_host_to_target32 (program_size);
	section->file_offset = grub_host_to_target32 (sizeof (*head) + sizeof (*section));
	memcpy (section + 1, core_img, core_size);
	free (core_img);
	core_img = ecoff_img;
	core_size = program_size + sizeof (*head) + sizeof (*section);
      }
      break;
    case IMAGE_LOONGSON_ELF:
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
	
	if (image_target->id != IMAGE_LOONGSON_ELF)
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
	if (image_target->id == IMAGE_LOONGSON_ELF)
	  ehdr->e_shentsize = grub_host_to_target16 (0);
	else
	  ehdr->e_shentsize = grub_host_to_target16 (sizeof (Elf32_Shdr));
	ehdr->e_shnum = grub_host_to_target16 (0);
	ehdr->e_shstrndx = grub_host_to_target16 (0);

	ehdr->e_ehsize = grub_host_to_target16 (sizeof (*ehdr));

	phdr->p_type = grub_host_to_target32 (PT_LOAD);
	phdr->p_offset = grub_host_to_target32 (header_size);
	phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);

	if (image_target->id == IMAGE_LOONGSON_ELF)
	  {
	    if (comp == COMPRESSION_NONE)
	      target_addr = (image_target->link_addr - decompress_size);
	    else
	      target_addr = ALIGN_UP (image_target->link_addr
				      + kernel_size + total_module_size, 32);
	  }
	else
	  target_addr = image_target->link_addr;
	ehdr->e_entry = grub_host_to_target32 (target_addr);
	phdr->p_vaddr = grub_host_to_target32 (target_addr);
	phdr->p_paddr = grub_host_to_target32 (target_addr);
	phdr->p_align = grub_host_to_target32 (align > image_target->link_align ? align : image_target->link_align);
	if (image_target->id == IMAGE_LOONGSON_ELF)
	  ehdr->e_flags = grub_host_to_target32 (0x1000 | EF_MIPS_NOREORDER 
						 | EF_MIPS_PIC | EF_MIPS_CPIC);
	else
	  ehdr->e_flags = 0;
	if (image_target->id == IMAGE_LOONGSON_ELF)
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
	    struct grub_ieee1275_note *note_ptr = (struct grub_ieee1275_note *) 
	      (elf_img + program_size + header_size);

	    grub_util_info ("adding CHRP NOTE segment");

	    note_ptr->header.namesz = grub_host_to_target32 (sizeof (GRUB_IEEE1275_NOTE_NAME));
	    note_ptr->header.descsz = grub_host_to_target32 (note_size);
	    note_ptr->header.type = grub_host_to_target32 (GRUB_IEEE1275_NOTE_TYPE);
	    strcpy (note_ptr->header.name, GRUB_IEEE1275_NOTE_NAME);
	    note_ptr->descriptor.real_mode = grub_host_to_target32 (0xffffffff);
	    note_ptr->descriptor.real_base = grub_host_to_target32 (0x00c00000);
	    note_ptr->descriptor.real_size = grub_host_to_target32 (0xffffffff);
	    note_ptr->descriptor.virt_base = grub_host_to_target32 (0xffffffff);
	    note_ptr->descriptor.virt_size = grub_host_to_target32 (0xffffffff);
	    note_ptr->descriptor.load_base = grub_host_to_target32 (0x00004000);

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

  grub_util_write_image (core_img, core_size, out, outname);
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



static struct argp_option options[] = {
  {"directory",  'd', N_("DIR"), 0,
   /* TRANSLATORS: platform here isn't identifier. It can be translated.  */
   N_("use images and modules under DIR [default=%s/<platform>]"), 0},
  {"prefix",  'p', N_("DIR"), 0, N_("set prefix directory [default=%s]"), 0},
  {"memdisk",  'm', N_("FILE"), 0,
   /* TRANSLATORS: "memdisk" here isn't an identifier, it can be translated.
    "embed" is a verb (command description).  "*/
   N_("embed FILE as a memdisk image"), 0},
   /* TRANSLATORS: "embed" is a verb (command description).  "*/
  {"config",   'c', N_("FILE"), 0, N_("embed FILE as an early config"), 0},
  /* TRANSLATORS: NOTE is a name of segment.  */
  {"note",   'n', 0, 0, N_("add NOTE segment for CHRP IEEE1275"), 0},
  {"output",  'o', N_("FILE"), 0, N_("output a generated image to FILE [default=stdout]"), 0},
  {"format",  'O', N_("FORMAT"), 0, 0, 0},
  {"compression",  'C', "(xz|none|auto)", 0, N_("choose the compression to use"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
    case 'd':
      return xasprintf (text, GRUB_PKGLIBDIR);
    case 'p':
      return xasprintf (text, DEFAULT_DIRECTORY);
    case 'O':
      {
	int format_len = 0;
	char *formats;
	char *ptr;
	char *ret;
	unsigned i;
	for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	  format_len += strlen (image_targets[i].names[0]) + 2;
	ptr = formats = xmalloc (format_len);
	for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	  {
	    strcpy (ptr, image_targets[i].names[0]);
	    ptr += strlen (image_targets[i].names[0]);
	    *ptr++ = ',';
	    *ptr++ = ' ';
	  }
	ptr[-2] = 0;
	ret = xasprintf ("%s\n%s %s", _("generate an image in FORMAT"),
			 _("available formats:"), formats);
	free (formats);
	return ret;
      }
    default:
      return (char *) text;
    }
}

struct arguments
{
  size_t nmodules;
  size_t modules_max;
  char **modules;
  char *output;
  char *dir;
  char *prefix;
  char *memdisk;
  char *font;
  char *config;
  int note;
  struct image_target_desc *image_target;
  grub_compression_t comp;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'o':
      if (arguments->output)
	free (arguments->output);

      arguments->output = xstrdup (arg);
      break;

    case 'O':
      {
	unsigned i, j;
	for (i = 0; i < ARRAY_SIZE (image_targets); i++)
	  for (j = 0; image_targets[i].names[j]
		 && j < ARRAY_SIZE (image_targets[i].names); j++)
	    if (strcmp (arg, image_targets[i].names[j]) == 0)
	      arguments->image_target = &image_targets[i];
	if (!arguments->image_target)
	  {
	    printf (_("unknown target format %s\n"), arg);
	    argp_usage (state);
	    exit (1);
	  }
	break;
      }
    case 'd':
      if (arguments->dir)
	free (arguments->dir);

      arguments->dir = xstrdup (arg);
      break;

    case 'n':
      arguments->note = 1;
      break;

    case 'm':
      if (arguments->memdisk)
	free (arguments->memdisk);

      arguments->memdisk = xstrdup (arg);

      if (arguments->prefix)
	free (arguments->prefix);

      arguments->prefix = xstrdup ("(memdisk)/boot/grub");
      break;

    case 'c':
      if (arguments->config)
	free (arguments->config);

      arguments->config = xstrdup (arg);
      break;

    case 'C':
      if (grub_strcmp (arg, "xz") == 0)
	{
#ifdef HAVE_LIBLZMA
	  arguments->comp = COMPRESSION_XZ;
#else
	  grub_util_error ("%s",
			   _("grub-mkimage is compiled without XZ support"));
#endif
	}
      else if (grub_strcmp (arg, "none") == 0)
	arguments->comp = COMPRESSION_NONE;
      else if (grub_strcmp (arg, "auto") == 0)
	arguments->comp = COMPRESSION_AUTO;
      else
	grub_util_error (_("Unknown compression format %s"), arg);
      break;

    case 'p':
      if (arguments->prefix)
	free (arguments->prefix);

      arguments->prefix = xstrdup (arg);
      break;

    case 'v':
      verbosity++;
      break;
    case ARGP_KEY_ARG:
      assert (arguments->nmodules < arguments->modules_max);
      arguments->modules[arguments->nmodules++] = xstrdup(arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTION]... [MODULES]"),
  N_("Make a bootable image of GRUB."),
  NULL, help_filter, NULL
};

int
main (int argc, char *argv[])
{
  FILE *fp = stdout;
  struct arguments arguments;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  memset (&arguments, 0, sizeof (struct arguments));
  arguments.comp = COMPRESSION_AUTO;
  arguments.modules_max = argc + 1;
  arguments.modules = xmalloc ((arguments.modules_max + 1)
			     * sizeof (arguments.modules[0]));
  memset (arguments.modules, 0, (arguments.modules_max + 1)
	  * sizeof (arguments.modules[0]));

  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (!arguments.image_target)
    {
      char *program = xstrdup(program_name);
      printf ("%s\n", _("Target format not specified (use the -O option)."));
      argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
      free (program);
      exit(1);
    }

  if (arguments.output)
    {
      fp = fopen (arguments.output, "wb");
      if (! fp)
	grub_util_error (_("cannot open `%s': %s"), arguments.output,
			 strerror (errno));
      free (arguments.output);
    }

  if (!arguments.dir)
    {
      arguments.dir = xmalloc (sizeof (GRUB_PKGLIBDIR)
			       + grub_strlen (arguments.image_target->dirname)
			       + 1);
      memcpy (arguments.dir, GRUB_PKGLIBDIR,
	      sizeof (GRUB_PKGLIBDIR) - 1);
      *(arguments.dir + sizeof (GRUB_PKGLIBDIR) - 1) = '/';
      strcpy (arguments.dir + sizeof (GRUB_PKGLIBDIR),
	      arguments.image_target->dirname);
    }

  generate_image (arguments.dir, arguments.prefix ? : DEFAULT_DIRECTORY, fp,
		  arguments.output,
		  arguments.modules, arguments.memdisk, arguments.config,
		  arguments.image_target, arguments.note, arguments.comp);

  fflush (fp);
  fsync (fileno (fp));
  fclose (fp);

  if (arguments.dir)
    free (arguments.dir);

  return 0;
}
