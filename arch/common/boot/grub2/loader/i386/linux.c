/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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
  
  grub_machine_mmap_iterate (hook);
  
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
  real_mode_mem = 0;
  prot_mode_mem = 0;
  
  /* i386-pc port adds lower memory to heap, which collides with
     `real_mode_mem' allocation below.  */
#ifdef GRUB_MACHINE_PCBIOS
#error
#endif

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

	  real_mode_mem = (addr + size) - (real_size + mmap_size);
	  return 1;
	}

      return 0;
    }
  grub_machine_mmap_iterate (hook);
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

#ifdef __x86_64__
struct
{
  grub_uint32_t kernel_entry;
  grub_uint32_t kernel_cs;
} jumpvector;
#endif

static grub_err_t
grub_linux32_boot (void)
{
  struct linux_kernel_params *params;
  int e820_num;
  
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

        default:
          grub_e820_add_region (params->e820_map, &e820_num,
                                addr, size, GRUB_E820_RESERVED);
        }
      return 0;
    }

  e820_num = 0;
  grub_machine_mmap_iterate (hook);
  params->mmap_size = e820_num;

  /* Hardware interrupts are not safe any longer.  */
  asm volatile ("cli" : : );
  
  /* Load the IDT and the GDT for the bootstrap.  */
  asm volatile ("lidt %0" : : "m" (idt_desc));
  asm volatile ("lgdt %0" : : "m" (gdt_desc));

#ifdef __x86_64__

  jumpvector.kernel_entry = (grub_uint64_t) grub_linux_real_boot;
  jumpvector.kernel_cs = 0x10;

  asm volatile ( "mov %0, %%rbx" : : "m" (params->code32_start));
  asm volatile ( "mov %0, %%rsi" : : "m" (real_mode_mem));

  asm volatile ( "ljmp *%0" : : "m" (jumpvector));

#else

  /* Pass parameters.  */
  asm volatile ("movl %0, %%ecx" : : "m" (params->code32_start));
  asm volatile ("movl %0, %%esi" : : "m" (real_mode_mem));

  asm volatile ("xorl %%ebx, %%ebx" : : );

  /* Enter Linux.  */
  asm volatile ("jmp *%%ecx" : : );
  
#endif

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
  int video_type;

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

  /* FIXME: Is 2.02 recent enough for 32-bit boot?  */
  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0203)
    {
      grub_error (GRUB_ERR_BAD_OS, "too old version");
      goto fail;
    }

  /* zImage doesn't support 32-bit boot.  */
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
  
  params->video_cursor_x = grub_getxy () >> 8;
  params->video_cursor_y = grub_getxy () & 0xff;
  params->video_page = 0; /* ??? */
  params->video_mode = 0;
  params->video_width = (grub_getwh () >> 8);
  params->video_ega_bx = 0;
  params->video_height = (grub_getwh () & 0xff);
  params->have_vga = 0;
  params->font_size = 16; /* XXX */

  /* The other parameters are filled when booting.  */

  grub_file_seek (file, real_size + GRUB_DISK_SECTOR_SIZE);

  grub_printf ("   [Linux-bzImage, setup=0x%x, size=0x%x]\n",
	       (unsigned) real_size, (unsigned) prot_size);

  /* Detect explicitly specified memory size, if any.  */
  linux_mem_size = 0;
  video_type = 0;
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
      grub_loader_set (grub_linux32_boot, grub_linux_unload, 1);
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
