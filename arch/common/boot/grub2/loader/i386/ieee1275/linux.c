/* linux.c - boot Linux zImage or bzImage */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008  Free Software Foundation, Inc.
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
#include <grub/machine/memory.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/rescue.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/term.h>
#include <grub/cpu/linux.h>
#include <grub/ieee1275/ieee1275.h>

#define GRUB_OFW_LINUX_PARAMS_ADDR	0x90000
#define GRUB_OFW_LINUX_KERNEL_ADDR	0x100000
#define GRUB_OFW_LINUX_INITRD_ADDR	0x800000

#define GRUB_OFW_LINUX_CL_OFFSET	0x1e00
#define GRUB_OFW_LINUX_CL_LENGTH	0x100

static grub_dl_t my_mod;

static grub_size_t kernel_size;
static char *kernel_addr, *kernel_cmdline;
static grub_size_t initrd_size;

static grub_err_t
grub_linux_unload (void)
{
  grub_free (kernel_cmdline);
  grub_free (kernel_addr);
  kernel_cmdline = 0;
  kernel_addr = 0;
  initrd_size = 0;

  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

/*
static int
grub_ieee1275_debug (void)
{
  struct enter_args
  {
    struct grub_ieee1275_common_hdr common;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "enter", 0, 0);

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return 0;
}
*/

static grub_err_t
grub_linux_boot (void)
{
  struct linux_kernel_params *params;
  struct linux_kernel_header *lh;
  char *prot_code;
  char *bootpath;
  grub_ssize_t len;

  bootpath = grub_env_get ("root");
  if (bootpath)
    grub_ieee1275_set_property (grub_ieee1275_chosen,
                                "bootpath", bootpath,
                                grub_strlen (bootpath) + 1,
                                &len);

  params = (struct linux_kernel_params *) GRUB_OFW_LINUX_PARAMS_ADDR;
  lh = (struct linux_kernel_header *) params;

  grub_memset ((char *) params, 0, GRUB_OFW_LINUX_CL_OFFSET);

  params->alt_mem = grub_upper_mem >> 10;
  params->ext_mem = params->alt_mem;

  lh->cmd_line_ptr = (char *)
        (GRUB_OFW_LINUX_PARAMS_ADDR + GRUB_OFW_LINUX_CL_OFFSET);

  params->cl_magic = GRUB_LINUX_CL_MAGIC;
  params->cl_offset = GRUB_OFW_LINUX_CL_OFFSET;

  params->video_width = (grub_getwh () >> 8);
  params->video_height = (grub_getwh () & 0xff);
  params->font_size = 16;

  params->ofw_signature = GRUB_LINUX_OFW_SIGNATURE;
  params->ofw_num_items = 1;
  params->ofw_cif_handler = (grub_uint32_t) grub_ieee1275_entry_fn;
  params->ofw_idt = 0;

  if (initrd_size)
    {
      lh->type_of_loader = 1;
      lh->ramdisk_image = GRUB_OFW_LINUX_INITRD_ADDR;
      lh->ramdisk_size = initrd_size;
    }

  if (kernel_cmdline)
    grub_strcpy (lh->cmd_line_ptr, kernel_cmdline);

  prot_code = (char *) GRUB_OFW_LINUX_KERNEL_ADDR;
  grub_memcpy (prot_code, kernel_addr, kernel_size);

  asm volatile ("movl %0, %%esi" : : "m" (params));
  asm volatile ("movl %%esi, %%esp" : : );
  asm volatile ("movl %0, %%ecx" : : "m" (prot_code));
  asm volatile ("xorl %%ebx, %%ebx" : : );
  asm volatile ("jmp *%%ecx" : : );

  return GRUB_ERR_NONE;
}

void
grub_rescue_cmd_linux (int argc, char *argv[])
{
  grub_file_t file = 0;
  struct linux_kernel_header lh;
  grub_uint8_t setup_sects;
  grub_size_t real_size, prot_size;
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

  if ((lh.boot_flag != grub_cpu_to_le16 (0xaa55)) ||
      (lh.header != grub_cpu_to_le32 (GRUB_LINUX_MAGIC_SIGNATURE)))
    {
      grub_error (GRUB_ERR_BAD_OS, "invalid magic number");
      goto fail;
    }

  setup_sects = lh.setup_sects;
  if (! setup_sects)
    setup_sects = GRUB_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << GRUB_DISK_SECTOR_BITS;
  prot_size = grub_file_size (file) - real_size - GRUB_DISK_SECTOR_SIZE;

  grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
               "bzImage", real_size, prot_size);

  grub_file_seek (file, real_size + GRUB_DISK_SECTOR_SIZE);
  if (grub_errno)
    goto fail;

  kernel_cmdline = grub_malloc (GRUB_OFW_LINUX_CL_LENGTH);
  if (! kernel_cmdline)
    goto fail;

  dest = kernel_cmdline;
  for (i = 1;
       i < argc
       && dest + grub_strlen (argv[i]) + 1 < (kernel_cmdline
                                              + GRUB_OFW_LINUX_CL_LENGTH);
       i++)
    {
      *dest++ = ' ';
      dest = grub_stpcpy (dest, argv[i]);
    }

  kernel_addr = grub_malloc (prot_size);
  if (! kernel_addr)
    goto fail;

  kernel_size = prot_size;
  if (grub_file_read (file, kernel_addr, prot_size) != (int) prot_size)
    grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");

  if (grub_errno == GRUB_ERR_NONE)
    grub_loader_set (grub_linux_boot, grub_linux_unload, 1);

fail:

  if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    {
      grub_free (kernel_cmdline);
      grub_free (kernel_addr);
      kernel_cmdline = 0;
      kernel_addr = 0;

      grub_dl_unref (my_mod);
    }
}

void
grub_rescue_cmd_initrd (int argc, char *argv[])
{
  grub_file_t file = 0;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "No module specified");
      goto fail;
    }

  if (! kernel_addr)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "You need to load the kernel first.");
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (! file)
    goto fail;

  initrd_size = grub_file_size (file);
  if (grub_file_read (file, (char *) GRUB_OFW_LINUX_INITRD_ADDR,
                      initrd_size) != (int) initrd_size)
    {
      grub_error (GRUB_ERR_FILE_READ_ERROR, "Couldn't read file");
      goto fail;
    }

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
