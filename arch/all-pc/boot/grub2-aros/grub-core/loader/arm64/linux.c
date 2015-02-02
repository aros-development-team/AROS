/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#include <grub/charset.h>
#include <grub/command.h>
#include <grub/err.h>
#include <grub/file.h>
#include <grub/fdt.h>
#include <grub/linux.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/cpu/linux.h>
#include <grub/efi/efi.h>
#include <grub/efi/pe32.h>
#include <grub/i18n.h>
#include <grub/lib/cmdline.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_EFI_PAGE_SHIFT	12
#define BYTES_TO_PAGES(bytes)   (((bytes) + 0xfff) >> GRUB_EFI_PAGE_SHIFT)
#define GRUB_EFI_PE_MAGIC	0x5A4D

static grub_efi_guid_t fdt_guid = GRUB_EFI_DEVICE_TREE_GUID;

static grub_dl_t my_mod;
static int loaded;

static void *kernel_addr;
static grub_uint64_t kernel_size;

static char *linux_args;
static grub_uint32_t cmdline_size;

static grub_addr_t initrd_start;
static grub_addr_t initrd_end;

static void *loaded_fdt;
static void *fdt;

static void *
get_firmware_fdt (void)
{
  grub_efi_configuration_table_t *tables;
  void *firmware_fdt = NULL;
  unsigned int i;

  /* Look for FDT in UEFI config tables. */
  tables = grub_efi_system_table->configuration_table;

  for (i = 0; i < grub_efi_system_table->num_table_entries; i++)
    if (grub_memcmp (&tables[i].vendor_guid, &fdt_guid, sizeof (fdt_guid)) == 0)
      {
	firmware_fdt = tables[i].vendor_table;
	grub_dprintf ("linux", "found registered FDT @ %p\n", firmware_fdt);
	break;
      }

  return firmware_fdt;
}

static void
get_fdt (void)
{
  void *raw_fdt;
  grub_size_t size;

  if (fdt)
    {
      size = BYTES_TO_PAGES (grub_fdt_get_totalsize (fdt));
      grub_efi_free_pages ((grub_efi_physical_address_t) fdt, size);
    }

  if (loaded_fdt)
    raw_fdt = loaded_fdt;
  else
    raw_fdt = get_firmware_fdt();

  size =
    raw_fdt ? grub_fdt_get_totalsize (raw_fdt) : GRUB_FDT_EMPTY_TREE_SZ;
  size += 0x400;

  grub_dprintf ("linux", "allocating %ld bytes for fdt\n", size);
  fdt = grub_efi_allocate_pages (0, BYTES_TO_PAGES (size));
  if (!fdt)
    return;

  if (raw_fdt)
    {
      grub_memmove (fdt, raw_fdt, size);
      grub_fdt_set_totalsize (fdt, size);
    }
  else
    {
      grub_fdt_create_empty_tree (fdt, size);
    }
}

static grub_err_t
check_kernel (struct grub_arm64_linux_kernel_header *lh)
{
  if (lh->magic != GRUB_ARM64_LINUX_MAGIC)
    return grub_error(GRUB_ERR_BAD_OS, "invalid magic number");

  if ((lh->code0 & 0xffff) != GRUB_EFI_PE_MAGIC)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       N_("plain image kernel not supported - rebuild with CONFIG_(U)EFI_STUB enabled"));

  grub_dprintf ("linux", "UEFI stub kernel:\n");
  grub_dprintf ("linux", "text_offset = 0x%012llx\n",
		(long long unsigned) lh->text_offset);
  grub_dprintf ("linux", "PE/COFF header @ %08x\n", lh->hdr_offset);

  return GRUB_ERR_NONE;
}

static grub_err_t
finalize_params (void)
{
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  int node, retval;

  get_fdt ();
  if (!fdt)
    goto failure;

  node = grub_fdt_find_subnode (fdt, 0, "chosen");
  if (node < 0)
    node = grub_fdt_add_subnode (fdt, 0, "chosen");

  if (node < 1)
    goto failure;

  /* Set initrd info */
  if (initrd_start && initrd_end > initrd_start)
    {
      grub_dprintf ("linux", "Initrd @ 0x%012lx-0x%012lx\n",
		    initrd_start, initrd_end);

      retval = grub_fdt_set_prop64 (fdt, node, "linux,initrd-start",
				    initrd_start);
      if (retval)
	goto failure;
      retval = grub_fdt_set_prop64 (fdt, node, "linux,initrd-end",
				    initrd_end);
      if (retval)
	goto failure;
    }

  b = grub_efi_system_table->boot_services;
  status = b->install_configuration_table (&fdt_guid, fdt);
  if (status != GRUB_EFI_SUCCESS)
    goto failure;

  grub_dprintf ("linux", "Installed/updated FDT configuration table @ %p\n",
		fdt);

  return GRUB_ERR_NONE;

failure:
  grub_efi_free_pages ((grub_efi_physical_address_t) fdt,
		       BYTES_TO_PAGES (grub_fdt_get_totalsize (fdt)));
  fdt = NULL;
  return grub_error(GRUB_ERR_BAD_OS, "failed to install/update FDT");
}

static grub_err_t
grub_cmd_devicetree (grub_command_t cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  grub_file_t dtb;
  void *blob = NULL;
  int size;

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  N_("you need to load the kernel first"));
      return GRUB_ERR_BAD_OS;
    }

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (loaded_fdt)
    grub_free (loaded_fdt);
  loaded_fdt = NULL;

  dtb = grub_file_open (argv[0]);
  if (!dtb)
    goto out;

  size = grub_file_size (dtb);
  blob = grub_malloc (size);
  if (!blob)
    goto out;

  if (grub_file_read (dtb, blob, size) < size)
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
      goto out;
    }

  if (grub_fdt_check_header (blob, size) != 0)
    {
      grub_error (GRUB_ERR_BAD_OS, N_("invalid device tree"));
      goto out;
    }

out:
  if (dtb)
    grub_file_close (dtb);

  if (blob)
    {
      if (grub_errno == GRUB_ERR_NONE)
	loaded_fdt = blob;
      else
	grub_free (blob);
    }

  return grub_errno;
}

static grub_err_t
grub_linux_boot (void)
{
  grub_efi_memory_mapped_device_path_t *mempath;
  grub_efi_handle_t image_handle;
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  grub_err_t retval;
  grub_efi_loaded_image_t *loaded_image;
  int len;

  retval = finalize_params();
  if (retval != GRUB_ERR_NONE)
    return retval;

  mempath = grub_malloc (2 * sizeof (grub_efi_memory_mapped_device_path_t));
  if (!mempath)
    return grub_errno;

  mempath[0].header.type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE;
  mempath[0].header.subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE;
  mempath[0].header.length = grub_cpu_to_le16_compile_time (sizeof (*mempath));
  mempath[0].memory_type = GRUB_EFI_LOADER_DATA;
  mempath[0].start_address = (grub_addr_t) kernel_addr;
  mempath[0].end_address = (grub_addr_t) kernel_addr + kernel_size;

  mempath[1].header.type = GRUB_EFI_END_DEVICE_PATH_TYPE;
  mempath[1].header.subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  mempath[1].header.length = sizeof (grub_efi_device_path_t);

  b = grub_efi_system_table->boot_services;
  status = b->load_image (0, grub_efi_image_handle,
			  (grub_efi_device_path_t *) mempath,
                          kernel_addr, kernel_size, &image_handle);
  if (status != GRUB_EFI_SUCCESS)
    return grub_error (GRUB_ERR_BAD_OS, "cannot load image");

  grub_dprintf ("linux", "linux command line: '%s'\n", linux_args);

  /* Convert command line to UCS-2 */
  loaded_image = grub_efi_get_loaded_image (image_handle);
  loaded_image->load_options_size = len =
    (grub_strlen (linux_args) + 1) * sizeof (grub_efi_char16_t);
  loaded_image->load_options =
    grub_efi_allocate_pages (0,
			     BYTES_TO_PAGES (loaded_image->load_options_size));
  if (!loaded_image->load_options)
    return grub_errno;

  loaded_image->load_options_size =
    2 * grub_utf8_to_utf16 (loaded_image->load_options, len,
			    (grub_uint8_t *) linux_args, len, NULL);

  grub_dprintf("linux", "starting image %p\n", image_handle);
  status = b->start_image (image_handle, 0, NULL);

  /* When successful, not reached */
  b->unload_image (image_handle);
  grub_efi_free_pages ((grub_efi_physical_address_t) loaded_image->load_options,
		       BYTES_TO_PAGES (loaded_image->load_options_size));

  return grub_errno;
}

static grub_err_t
grub_linux_unload (void)
{
  grub_dl_unref (my_mod);
  loaded = 0;
  if (initrd_start)
    grub_efi_free_pages ((grub_efi_physical_address_t) initrd_start,
			 BYTES_TO_PAGES (initrd_end - initrd_start));
  initrd_start = initrd_end = 0;
  grub_free (linux_args);
  if (kernel_addr)
    grub_efi_free_pages ((grub_efi_physical_address_t) kernel_addr,
			 BYTES_TO_PAGES (kernel_size));
  if (fdt)
    grub_efi_free_pages ((grub_efi_physical_address_t) fdt,
			 BYTES_TO_PAGES (grub_fdt_get_totalsize (fdt)));

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  struct grub_linux_initrd_context initrd_ctx = { 0, 0, 0 };
  int initrd_size, initrd_pages;
  void *initrd_mem = NULL;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  N_("you need to load the kernel first"));
      goto fail;
    }

  if (grub_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  initrd_size = grub_get_initrd_size (&initrd_ctx);
  grub_dprintf ("linux", "Loading initrd\n");

  initrd_pages = (BYTES_TO_PAGES (initrd_size));
  initrd_mem = grub_efi_allocate_pages (0, initrd_pages);
  if (!initrd_mem)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  if (grub_initrd_load (&initrd_ctx, argv, initrd_mem))
    goto fail;

  initrd_start = (grub_addr_t) initrd_mem;
  initrd_end = initrd_start + initrd_size;
  grub_dprintf ("linux", "[addr=%p, size=0x%x]\n",
		(void *) initrd_start, initrd_size);

 fail:
  grub_initrd_close (&initrd_ctx);
  if (initrd_mem && !initrd_start)
    grub_efi_free_pages ((grub_efi_physical_address_t) initrd_mem,
			 initrd_pages);

  return grub_errno;
}

static grub_err_t
grub_cmd_linux (grub_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  grub_file_t file = 0;
  struct grub_arm64_linux_kernel_header lh;

  grub_dl_ref (my_mod);

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = grub_file_open (argv[0]);
  if (!file)
    goto fail;

  kernel_size = grub_file_size (file);

  if (grub_file_read (file, &lh, sizeof (lh)) < (long) sizeof (lh))
    return grub_errno;

  if (check_kernel (&lh) != GRUB_ERR_NONE)
    goto fail;

  grub_loader_unset();

  grub_dprintf ("linux", "kernel file size: %lld\n", (long long) kernel_size);
  kernel_addr = grub_efi_allocate_pages (0, BYTES_TO_PAGES (kernel_size));
  grub_dprintf ("linux", "kernel numpages: %lld\n",
		(long long) BYTES_TO_PAGES (kernel_size));
  if (!kernel_addr)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  grub_file_seek (file, 0);
  if (grub_file_read (file, kernel_addr, kernel_size)
      < (grub_int64_t) kernel_size)
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
      goto fail;
    }

  grub_dprintf ("linux", "kernel @ %p\n", kernel_addr);

  cmdline_size = grub_loader_cmdline_size (argc, argv) + sizeof (LINUX_IMAGE);
  linux_args = grub_malloc (cmdline_size);
  if (!linux_args)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }
  grub_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  grub_create_loader_cmdline (argc, argv,
			      linux_args + sizeof (LINUX_IMAGE) - 1,
			      cmdline_size);

  if (grub_errno == GRUB_ERR_NONE)
    {
      grub_loader_set (grub_linux_boot, grub_linux_unload, 0);
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

  if (linux_args && !loaded)
    grub_free (linux_args);

  if (kernel_addr && !loaded)
    grub_efi_free_pages ((grub_efi_physical_address_t) kernel_addr,
			 BYTES_TO_PAGES (kernel_size));

  return grub_errno;
}


static grub_command_t cmd_linux, cmd_initrd, cmd_devicetree;

GRUB_MOD_INIT (linux)
{
  cmd_linux = grub_register_command ("linux", grub_cmd_linux, 0,
				     N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd, 0,
				      N_("Load initrd."));
  cmd_devicetree =
    grub_register_command ("devicetree", grub_cmd_devicetree, 0,
			   N_("Load DTB file."));
  my_mod = mod;
}

GRUB_MOD_FINI (linux)
{
  grub_unregister_command (cmd_linux);
  grub_unregister_command (cmd_initrd);
  grub_unregister_command (cmd_devicetree);
}
