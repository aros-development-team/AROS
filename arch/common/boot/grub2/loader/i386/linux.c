/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/machine/machine.h>
#include <grub/machine/memory.h>
#include <grub/machine/loader.h>
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

#define GRUB_LINUX_CL_OFFSET		0x1000
#define GRUB_LINUX_CL_END_OFFSET	0x2000

static grub_dl_t my_mod;

static grub_size_t linux_mem_size;
static int loaded;
static void *real_mode_mem;
static void *prot_mode_mem;
static void *initrd_mem;
static grub_uint32_t real_mode_pages;
static grub_uint32_t prot_mode_pages;
static grub_uint32_t initrd_pages;

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
  grub_uint16_t limit;
  void *base;
} __attribute__ ((packed));

static struct gdt_descriptor gdt_desc =
  {
    sizeof (gdt) - 1,
    gdt
  };

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

#ifdef GRUB_MACHINE_PCBIOS
struct linux_vesafb_res
{
  grub_uint16_t width;
  grub_uint16_t height;
};

struct linux_vesafb_mode
{
  grub_uint8_t res_index;
  grub_uint8_t depth;
};

enum vga_modes
  {
    VGA_320_200,
    VGA_640_400,
    VGA_640_480,
    VGA_800_500,
    VGA_800_600,
    VGA_896_672,
    VGA_1024_640,
    VGA_1024_768,
    VGA_1152_720,
    VGA_1280_1024,
    VGA_1440_900,
    VGA_1600_1200,
  };

static struct linux_vesafb_res linux_vesafb_res[] =
  {
    { 320, 200 },
    { 640, 400 },
    { 640, 480 },
    { 800, 500 },
    { 800, 600 },
    { 896, 672 },
    { 1024, 640 },
    { 1024, 768 },
    { 1152, 720 },
    { 1280, 1024 },
    { 1440, 900 },
    { 1600, 1200 },
  };

/* This is the reverse of the table in [linux]/Documentation/fb/vesafb.txt
   plus a few more modes based on the table in
   http://en.wikipedia.org/wiki/VESA_BIOS_Extensions  */
struct linux_vesafb_mode linux_vesafb_modes[] =
  {
    { VGA_640_400, 8 },		/* 0x300 */
    { VGA_640_480, 8 },		/* 0x301 */
    { VGA_800_600, 4 },		/* 0x302 */
    { VGA_800_600, 8 },		/* 0x303 */
    { VGA_1024_768, 4 },	/* 0x304 */
    { VGA_1024_768, 8 },	/* 0x305 */
    { VGA_1280_1024, 4 },	/* 0x306 */
    { VGA_1280_1024, 8 },	/* 0x307 */
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { VGA_320_200, 15 },	/* 0x30d */
    { VGA_320_200, 16 },	/* 0x30e */
    { VGA_320_200, 24 },	/* 0x30f */
    { VGA_640_480, 15 },	/* 0x310 */
    { VGA_640_480, 16 },	/* 0x311 */
    { VGA_640_480, 24 },	/* 0x312 */
    { VGA_800_600, 15 },	/* 0x313 */
    { VGA_800_600, 16 },	/* 0x314 */
    { VGA_800_600, 24 },	/* 0x315 */
    { VGA_1024_768, 15 },	/* 0x316 */
    { VGA_1024_768, 16 },	/* 0x317 */
    { VGA_1024_768, 24 },	/* 0x318 */
    { VGA_1280_1024, 15 },	/* 0x319 */
    { VGA_1280_1024, 16 },	/* 0x31a */
    { VGA_1280_1024, 24 },	/* 0x31b */
    { VGA_1600_1200, 8 },	/* 0x31c */
    { VGA_1600_1200, 15 },	/* 0x31d */
    { VGA_1600_1200, 16 },	/* 0x31e */
    { VGA_1600_1200, 24 },	/* 0x31f */
    { 0, 0 },
    { VGA_640_400, 15 },	/* 0x321 */
    { VGA_640_400, 16 },	/* 0x322 */
    { VGA_640_400, 24 },	/* 0x323 */
    { VGA_640_400, 32 },	/* 0x324 */
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { VGA_640_480, 32 },	/* 0x329 */
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { VGA_896_672, 8 },		/* 0x32f */
    { VGA_896_672, 15 },	/* 0x330 */
    { VGA_896_672, 16 },	/* 0x331 */
    { VGA_896_672, 24 },	/* 0x332 */
    { VGA_896_672, 32 },	/* 0x333 */
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { VGA_1600_1200, 32 },	/* 0x342 */
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { VGA_1440_900, 8 },	/* 0x360 */
    { VGA_1440_900, 15 },	/* 0x361 */
    { VGA_1440_900, 16 },	/* 0x362 */
    { VGA_1440_900, 24 },	/* 0x363 */
    { VGA_1440_900, 32 },	/* 0x364 */
    { VGA_1152_720, 8 },	/* 0x365 */
    { VGA_1152_720, 15 },	/* 0x366 */
    { VGA_1152_720, 16 },	/* 0x367 */
    { VGA_1152_720, 24 },	/* 0x368 */
    { VGA_1152_720, 32 },	/* 0x369 */
    { VGA_1024_640, 8 },	/* 0x36a */
    { VGA_1024_640, 15 },	/* 0x36b */
    { VGA_1024_640, 16 },	/* 0x36c */
    { VGA_1024_640, 24 },	/* 0x36d */
    { VGA_1024_640, 32 },	/* 0x36e */
    { VGA_800_500, 8 },		/* 0x36f */
    { VGA_800_500, 15 },	/* 0x370 */
    { VGA_800_500, 16 },	/* 0x371 */
    { VGA_800_500, 24 },	/* 0x372 */
    { VGA_800_500, 32 },	/* 0x373 */
  };
#endif

static inline grub_size_t
page_align (grub_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

/* Find the optimal number of pages for the memory map. */
static grub_size_t
find_mmap_size (void)
{
  grub_size_t count = 0, mmap_size;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr __attribute__ ((unused)),
			     grub_uint64_t size __attribute__ ((unused)),
			     grub_uint32_t type __attribute__ ((unused)))
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
  real_mode_mem = prot_mode_mem = initrd_mem = 0;
}

/* Allocate pages for the real mode code and the protected mode code
   for linux as well as a memory map buffer.  */
static int
allocate_pages (grub_size_t prot_size)
{
  grub_size_t real_size, mmap_size;

  /* Make sure that each size is aligned to a page boundary.  */
  real_size = GRUB_LINUX_CL_END_OFFSET;
  prot_size = page_align (prot_size);
  mmap_size = find_mmap_size ();

  grub_dprintf ("linux", "real_size = %x, prot_size = %x, mmap_size = %x\n",
		(unsigned) real_size, (unsigned) prot_size, (unsigned) mmap_size);

  /* Calculate the number of pages; Combine the real mode code with
     the memory map buffer for simplicity.  */
  real_mode_pages = ((real_size + mmap_size) >> 12);
  prot_mode_pages = (prot_size >> 12);

  /* Initialize the memory pointers with NULL for convenience.  */
  free_pages ();

  /* FIXME: Should request low memory from the heap when this feature is
     implemented.  */

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
    {
      /* We must put real mode code in the traditional space.  */

      if (type == GRUB_MACHINE_MEMORY_AVAILABLE
	  && addr <= 0x90000)
	{
	  if (addr < 0x10000)
	    {
	      size += addr - 0x10000;
	      addr = 0x10000;
	    }

	  if (addr + size > 0x90000)
	    size = 0x90000 - addr;

	  if (real_size + mmap_size > size)
	    return 0;

	  real_mode_mem =
	    (void *) (grub_size_t) ((addr + size) - (real_size + mmap_size));
	  return 1;
	}

      return 0;
    }
  grub_mmap_iterate (hook);
  if (! real_mode_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "cannot allocate real mode pages");
      goto fail;
    }

  prot_mode_mem = (void *) 0x100000;

  grub_dprintf ("linux", "real_mode_mem = %lx, real_mode_pages = %x, "
                "prot_mode_mem = %lx, prot_mode_pages = %x\n",
                (unsigned long) real_mode_mem, (unsigned) real_mode_pages,
                (unsigned long) prot_mode_mem, (unsigned) prot_mode_pages);

  return 1;

 fail:
  free_pages ();
  return 0;
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

static int
grub_linux_setup_video (struct linux_kernel_params *params)
{
  struct grub_video_mode_info mode_info;
  void *framebuffer;
  int ret;

  ret = grub_video_get_info_and_fini (&mode_info, &framebuffer);

  if (ret)
    return 1;

  params->lfb_width = mode_info.width;
  params->lfb_height = mode_info.height;
  params->lfb_depth = mode_info.bpp;
  params->lfb_line_len = mode_info.pitch;

  params->lfb_base = (grub_size_t) framebuffer;
  params->lfb_size = (params->lfb_line_len * params->lfb_height + 65535) >> 16;

  params->red_mask_size = mode_info.red_mask_size;
  params->red_field_pos = mode_info.red_field_pos;
  params->green_mask_size = mode_info.green_mask_size;
  params->green_field_pos = mode_info.green_field_pos;
  params->blue_mask_size = mode_info.blue_mask_size;
  params->blue_field_pos = mode_info.blue_field_pos;
  params->reserved_mask_size = mode_info.reserved_mask_size;
  params->reserved_field_pos = mode_info.reserved_field_pos;

  return 0;
}

#ifdef __x86_64__
extern grub_uint8_t grub_linux_trampoline_start[];
extern grub_uint8_t grub_linux_trampoline_end[];
#endif

static grub_err_t
grub_linux_boot (void)
{
  struct linux_kernel_params *params;
  int e820_num;
  grub_err_t err = 0;
  char *modevar, *tmp;

  params = real_mode_mem;

  grub_dprintf ("linux", "code32_start = %x, idt_desc = %lx, gdt_desc = %lx\n",
		(unsigned) params->code32_start,
                (unsigned long) &(idt_desc.limit),
		(unsigned long) &(gdt_desc.limit));
  grub_dprintf ("linux", "idt = %x:%lx, gdt = %x:%lx\n",
		(unsigned) idt_desc.limit, (unsigned long) idt_desc.base,
		(unsigned) gdt_desc.limit, (unsigned long) gdt_desc.base);

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
    {
      switch (type)
        {
        case GRUB_MACHINE_MEMORY_AVAILABLE:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_RAM);
	  break;

#ifdef GRUB_MACHINE_MEMORY_ACPI
        case GRUB_MACHINE_MEMORY_ACPI:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_ACPI);
	  break;
#endif

#ifdef GRUB_MACHINE_MEMORY_NVS
        case GRUB_MACHINE_MEMORY_NVS:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_NVS);
	  break;
#endif

#ifdef GRUB_MACHINE_MEMORY_CODE
        case GRUB_MACHINE_MEMORY_CODE:
	  grub_e820_add_region (params->e820_map, &e820_num,
				addr, size, GRUB_E820_EXEC_CODE);
	  break;
#endif

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
      tmp = grub_malloc (grub_strlen (modevar)
			 + sizeof (";text"));
      if (! tmp)
	return grub_errno;
      grub_sprintf (tmp, "%s;text", modevar);
      err = grub_video_set_mode (tmp, 0);
      grub_free (tmp);
    }
  else
    err = grub_video_set_mode ("text", 0);

  if (err)
    {
      grub_print_error ();
      grub_printf ("Booting however\n");
      grub_errno = GRUB_ERR_NONE;
    }

  if (! grub_linux_setup_video (params))
    params->have_vga = GRUB_VIDEO_TYPE_VLFB;
  else
    {
      params->have_vga = GRUB_VIDEO_TYPE_TEXT;
      params->video_width = 80;
      params->video_height = 25;
    }

  /* Initialize these last, because terminal position could be affected by printfs above.  */
  if (params->have_vga == GRUB_VIDEO_TYPE_TEXT)
    {
      params->video_cursor_x = grub_getxy () >> 8;
      params->video_cursor_y = grub_getxy () & 0xff;
    }

#ifdef __x86_64__

  grub_memcpy ((char *) prot_mode_mem + (prot_mode_pages << 12),
	       grub_linux_trampoline_start,
	       grub_linux_trampoline_end - grub_linux_trampoline_start);

  ((void (*) (unsigned long, void *)) ((char *) prot_mode_mem
				       + (prot_mode_pages << 12)))
    (params->code32_start, real_mode_mem);
#else

  /* Hardware interrupts are not safe any longer.  */
  asm volatile ("cli" : : );

  /* Load the IDT and the GDT for the bootstrap.  */
  asm volatile ("lidt %0" : : "m" (idt_desc));
  asm volatile ("lgdt %0" : : "m" (gdt_desc));

  /* Enter Linux.  */
  asm volatile ("jmp *%2" : : "b" (0), "S" (real_mode_mem), "g" (params->code32_start));

#endif

  /* Never reach here.  */
  return GRUB_ERR_NONE;
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

  if (grub_file_read (file, &lh, sizeof (lh)) != sizeof (lh))
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

  if (! allocate_pages (prot_size))
    goto fail;

  params = (struct linux_kernel_params *) real_mode_mem;
  grub_memset (params, 0, GRUB_LINUX_CL_END_OFFSET);
  grub_memcpy (&params->setup_sects, &lh.setup_sects, sizeof (lh) - 0x1F1);

  params->ps_mouse = params->padding10 =  0;

  len = 0x400 - sizeof (lh);
  if (grub_file_read (file, (char *) real_mode_mem + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  params->type_of_loader = (LINUX_LOADER_ID_GRUB << 4);

  /* These two are used (instead of cmd_line_ptr) by older versions of Linux,
     and otherwise ignored.  */
  params->cl_magic = GRUB_LINUX_CL_MAGIC;
  params->cl_offset = 0x1000;

  params->cmd_line_ptr = (unsigned long) real_mode_mem + 0x1000;
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

  /* Must be non-zero even in text mode, or Linux will think there's no VGA.  */
  params->video_mode = 0x3;

  /* Only used when `video_mode == 0x7', otherwise ignored.  */
  params->video_ega_bx = 0;

  params->font_size = 16; /* XXX */

  /* The other parameters are filled when booting.  */

  grub_file_seek (file, real_size + GRUB_DISK_SECTOR_SIZE);

  grub_printf ("   [Linux-bzImage, setup=0x%x, size=0x%x]\n",
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
	struct linux_vesafb_mode *linux_mode;
	grub_err_t err;
	char *buf;

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
	    if (vid_mode < GRUB_LINUX_VID_MODE_VESA_START ||
		vid_mode >= GRUB_LINUX_VID_MODE_VESA_START +
		ARRAY_SIZE (linux_vesafb_modes))
	      {
		grub_env_set ("gfxpayload", "text");
		grub_printf ("%s is deprecated. Mode %d isn't recognized. "
			     "Use set gfxpayload=WIDTHxHEIGHT[xDEPTH] before "
			     "linux command instead.\n",
			     argv[i], vid_mode);
		break;
	      }

	    buf = grub_malloc (sizeof ("WWWWxHHHHxDD;WWWWxHHHH"));
	    if (! buf)
	      goto fail;

	    linux_mode
	      = &linux_vesafb_modes[vid_mode - GRUB_LINUX_VID_MODE_VESA_START];

	    grub_sprintf (buf, "%ux%ux%u,%ux%u",
			  linux_vesafb_res[linux_mode->res_index].width,
			  linux_vesafb_res[linux_mode->res_index].height,
			  linux_mode->depth,
			  linux_vesafb_res[linux_mode->res_index].width,
			  linux_vesafb_res[linux_mode->res_index].height);
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
  if (grub_file_read (file, (void *) GRUB_LINUX_BZIMAGE_ADDR, len) != len)
    grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");

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
  addr_min = (grub_addr_t) prot_mode_mem + ((prot_mode_pages * 3) << 12)
             + page_align (size);

  if (addr_max > grub_os_area_addr + grub_os_area_size)
    addr_max = grub_os_area_addr + grub_os_area_size;

  /* Put the initrd as high as possible, 4KiB aligned.  */
  addr = (addr_max - size) & ~0xFFF;

  if (addr < addr_min)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "The initrd is too big");
      goto fail;
    }

  initrd_mem = (void *) addr;

  if (grub_file_read (file, initrd_mem, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  grub_printf ("   [Initrd, addr=0x%x, size=0x%x]\n",
	       (unsigned) addr, (unsigned) size);

  lh->ramdisk_image = addr;
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
				     0, "load linux");
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, "load initrd");
  my_mod = mod;
}

GRUB_MOD_FINI(linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
}
