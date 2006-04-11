/* linux.c - boot Linux zImage or bzImage */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/machine/linux.h>

static grub_dl_t my_mod;

static int big_linux;
static grub_size_t linux_mem_size;
static int loaded;

static grub_err_t
grub_linux_boot (void)
{
  if (big_linux)
    grub_linux_boot_bzimage ();
  else
    grub_linux_boot_zimage ();

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

void
grub_rescue_cmd_linux (int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
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

  if (grub_file_size (file) > (grub_ssize_t) grub_os_area_size)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "too big kernel");
      goto fail;
    }

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

  big_linux = 0;
  setup_sects = lh.setup_sects;
  linux_mem_size = 0;
  
  if (lh.header == grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      && grub_le_to_cpu16 (lh.version) >= 0x0200)
    {
      big_linux = (lh.loadflags & GRUB_LINUX_FLAG_BIG_KERNEL);
      lh.type_of_loader = GRUB_LINUX_BOOT_LOADER_TYPE;
      
      /* Put the real mode part at as a high location as possible.  */
      grub_linux_real_addr = (char *) (grub_lower_mem
				       - GRUB_LINUX_SETUP_MOVE_SIZE);
      /* But it must not exceed the traditional area.  */
      if (grub_linux_real_addr > (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR)
	grub_linux_real_addr = (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR;
      
      if (grub_le_to_cpu16 (lh.version) >= 0x0201)
	{
	  lh.heap_end_ptr = grub_cpu_to_le16 (GRUB_LINUX_HEAP_END_OFFSET);
	  lh.loadflags |= GRUB_LINUX_FLAG_CAN_USE_HEAP;
	}
      
      if (grub_le_to_cpu16 (lh.version) >= 0x0202)
	lh.cmd_line_ptr = grub_linux_real_addr + GRUB_LINUX_CL_OFFSET;
      else
	{
	  lh.cl_magic = grub_cpu_to_le16 (GRUB_LINUX_CL_MAGIC);
	  lh.cl_offset = grub_cpu_to_le16 (GRUB_LINUX_CL_OFFSET);
	  lh.setup_move_size = grub_cpu_to_le16 (GRUB_LINUX_SETUP_MOVE_SIZE);
	}
    }
  else
    {
      /* Your kernel is quite old...  */
      lh.cl_magic = grub_cpu_to_le16 (GRUB_LINUX_CL_MAGIC);
      lh.cl_offset = grub_cpu_to_le16 (GRUB_LINUX_CL_OFFSET);
      
      setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;
      
      grub_linux_real_addr = (char *) GRUB_LINUX_OLD_REAL_MODE_ADDR;
    }
  
  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;
  
  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;
  
  grub_linux_tmp_addr = (char *) GRUB_LINUX_BZIMAGE_ADDR + prot_size;

  if (! big_linux
      && prot_size > (grub_size_t) (grub_linux_real_addr
				    - (char *) GRUB_LINUX_ZIMAGE_ADDR))
    {
      grub_error (GRUB_ERR_BAD_OS, "too big zImage, use bzImage instead");
      goto fail;
    }
  
  if (grub_linux_real_addr + GRUB_LINUX_SETUP_MOVE_SIZE
      > (char *) grub_lower_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "too small lower memory");
      goto fail;
    }

  grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
	       big_linux ? "bzImage" : "zImage", real_size, prot_size);

  for (i = 1; i < argc; i++)
    if (grub_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	grub_uint16_t vid_mode;
	char *val = argv[i] + 4;

	if (grub_strcmp (val, "normal") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_NORMAL;
	else if (grub_strcmp (val, "ext") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_EXTENDED;
	else if (grub_strcmp (val, "ask") == 0)
	  vid_mode = GRUB_LINUX_VID_MODE_ASK;
	else
	  vid_mode = (grub_uint16_t) grub_strtoul (val, 0, 0);

	if (grub_errno)
	  goto fail;

	lh.vid_mode = grub_cpu_to_le16 (vid_mode);
      }
    else if (grub_memcmp (argv[i], "mem=", 4) == 0)
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

  /* Put the real mode code at the temporary address.  */
  grub_memmove (grub_linux_tmp_addr, &lh, sizeof (lh));

  len = real_size + GRUB_DISK_SECTOR_SIZE - sizeof (lh);
  if (grub_file_read (file, grub_linux_tmp_addr + sizeof (lh), len) != len)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  if (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
      || grub_le_to_cpu16 (lh.version) < 0x0200)
    /* Clear the heap space.  */
    grub_memset (grub_linux_tmp_addr
		 + ((setup_sects + 1) << GRUB_DISK_SECTOR_BITS),
		 0,
		 ((GRUB_LINUX_MAX_SETUP_SECTS - setup_sects - 1)
		  << GRUB_DISK_SECTOR_BITS));

  /* Specify the boot file.  */
  dest = grub_stpcpy (grub_linux_tmp_addr + GRUB_LINUX_CL_OFFSET,
		      "BOOT_IMAGE=");
  dest = grub_stpcpy (dest, argv[0]);
  
  /* Copy kernel parameters.  */
  for (i = 1;
       i < argc
	 && dest + grub_strlen (argv[i]) + 1 < (grub_linux_tmp_addr
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
      grub_linux_prot_size = prot_size;
      grub_loader_set (grub_linux_boot, grub_linux_unload);
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
  grub_addr_t addr_max, addr_min, addr;
  struct linux_kernel_header *lh;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }
  
  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  lh = (struct linux_kernel_header *) grub_linux_tmp_addr;

  if (!(lh->header == grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)
	&& grub_le_to_cpu16 (lh->version) >= 0x0200))
    {
      grub_error (GRUB_ERR_BAD_OS, "The kernel is too old for initrd.");
      goto fail;
    }

  /* Get the highest address available for the initrd.  */
  if (grub_le_to_cpu16 (lh->version) >= 0x0203)
    addr_max = grub_cpu_to_le32 (lh->initrd_addr_max);
  else
    addr_max = GRUB_LINUX_INITRD_MAX_ADDRESS;

  if (!linux_mem_size && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  if (addr_max > grub_os_area_addr + grub_os_area_size)
    addr_max = grub_os_area_addr + grub_os_area_size;

  addr_min = (grub_addr_t) grub_linux_tmp_addr + GRUB_LINUX_CL_END_OFFSET;

  file = grub_file_open (argv[0]);
  if (!file)
    goto fail;

  size = grub_file_size (file);

  /* Put the initrd as high as possible, 4Ki aligned.  */
  addr = (addr_max - size) & ~0xFFF;

  if (addr < addr_min)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "The initrd is too big");
      goto fail;
    }

  if (grub_file_read (file, (void *)addr, size) != size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

  lh->ramdisk_image = addr;
  lh->ramdisk_size = size;
  
 fail:
  if (file)
    grub_file_close (file);
}


GRUB_MOD_INIT
{
  grub_rescue_register_command ("linux",
				grub_rescue_cmd_linux,
				"load linux");
  grub_rescue_register_command ("initrd",
				grub_rescue_cmd_initrd,
				"load initrd");
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_rescue_unregister_command ("linux");
  grub_rescue_unregister_command ("initrd");
}
