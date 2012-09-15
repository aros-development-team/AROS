/* chainloader.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2006,2007,2008  Free Software Foundation, Inc.
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

/* TODO: support load options.  */

#include <grub/loader.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/charset.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/disk.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/net.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_dl_t my_mod;

static grub_efi_physical_address_t address;
static grub_efi_uintn_t pages;
static grub_efi_device_path_t *file_path;
static grub_efi_handle_t image_handle;
static grub_efi_char16_t *cmdline;

static grub_err_t
grub_chainloader_unload (void)
{
  grub_efi_boot_services_t *b;

  b = grub_efi_system_table->boot_services;
  efi_call_1 (b->unload_image, image_handle);
  efi_call_2 (b->free_pages, address, pages);

  grub_free (file_path);
  grub_free (cmdline);
  cmdline = 0;
  file_path = 0;

  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_chainloader_boot (void)
{
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  grub_efi_uintn_t exit_data_size;
  grub_efi_char16_t *exit_data = NULL;

  b = grub_efi_system_table->boot_services;
  status = efi_call_3 (b->start_image, image_handle, &exit_data_size, &exit_data);
  if (status != GRUB_EFI_SUCCESS)
    {
      if (exit_data)
	{
	  char *buf;

	  buf = grub_malloc (exit_data_size * 4 + 1);
	  if (buf)
	    {
	      *grub_utf16_to_utf8 ((grub_uint8_t *) buf,
				   exit_data, exit_data_size) = 0;

	      grub_error (GRUB_ERR_BAD_OS, buf);
	      grub_free (buf);
	    }
	}
      else
	grub_error (GRUB_ERR_BAD_OS, "unknown error");
    }

  if (exit_data)
    efi_call_1 (b->free_pool, exit_data);

  grub_loader_unset ();

  return grub_errno;
}

static void
copy_file_path (grub_efi_file_path_device_path_t *fp,
		const char *str, grub_efi_uint16_t len)
{
  grub_efi_char16_t *p;
  grub_efi_uint16_t size;

  fp->header.type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE;
  fp->header.subtype = GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE;

  size = grub_utf8_to_utf16 (fp->path_name, len * GRUB_MAX_UTF16_PER_UTF8,
			     (const grub_uint8_t *) str, len, 0);
  for (p = fp->path_name; p < fp->path_name + size; p++)
    if (*p == '/')
      *p = '\\';

  size = size * sizeof (grub_efi_char16_t) + sizeof (*fp);
  fp->header.length[0] = (grub_efi_uint8_t) (size & 0xff);
  fp->header.length[1] = (grub_efi_uint8_t) (size >> 8);
}

static grub_efi_device_path_t *
make_file_path (grub_efi_device_path_t *dp, const char *filename)
{
  char *dir_start;
  char *dir_end;
  grub_size_t size;
  grub_efi_device_path_t *d;

  dir_start = grub_strchr (filename, ')');
  if (! dir_start)
    dir_start = (char *) filename;
  else
    dir_start++;

  dir_end = grub_strrchr (dir_start, '/');
  if (! dir_end)
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "invalid EFI file path");
      return 0;
    }

  size = 0;
  d = dp;
  while (1)
    {
      size += GRUB_EFI_DEVICE_PATH_LENGTH (d);
      if ((GRUB_EFI_END_ENTIRE_DEVICE_PATH (d)))
	break;
      d = GRUB_EFI_NEXT_DEVICE_PATH (d);
    }

  file_path = grub_malloc (size
			   + ((grub_strlen (dir_start) + 1)
			      * GRUB_MAX_UTF16_PER_UTF8
			      * sizeof (grub_efi_char16_t))
			   + sizeof (grub_efi_file_path_device_path_t) * 2);
  if (! file_path)
    return 0;

  grub_memcpy (file_path, dp, size);

  /* Fill the file path for the directory.  */
  d = (grub_efi_device_path_t *) ((char *) file_path
				  + ((char *) d - (char *) dp));
  grub_efi_print_device_path (d);
  copy_file_path ((grub_efi_file_path_device_path_t *) d,
		  dir_start, dir_end - dir_start);

  /* Fill the file path for the file.  */
  d = GRUB_EFI_NEXT_DEVICE_PATH (d);
  copy_file_path ((grub_efi_file_path_device_path_t *) d,
		  dir_end + 1, grub_strlen (dir_end + 1));

  /* Fill the end of device path nodes.  */
  d = GRUB_EFI_NEXT_DEVICE_PATH (d);
  d->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
  d->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  d->length[0] = sizeof (*d);
  d->length[1] = 0;

  return file_path;
}

static grub_err_t
grub_cmd_chainloader (grub_command_t cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  grub_file_t file = 0;
  grub_ssize_t size;
  grub_efi_status_t status;
  grub_efi_boot_services_t *b;
  grub_device_t dev = 0;
  grub_efi_device_path_t *dp = 0;
  grub_efi_loaded_image_t *loaded_image;
  char *filename;
  grub_efi_handle_t dev_handle = 0;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
  filename = argv[0];

  grub_dl_ref (my_mod);

  /* Initialize some global variables.  */
  address = 0;
  image_handle = 0;
  file_path = 0;

  b = grub_efi_system_table->boot_services;

  file = grub_file_open (filename);
  if (! file)
    goto fail;

  /* Get the root device's device path.  */
  dev = grub_device_open (0);
  if (! dev)
    goto fail;

  if (dev->disk)
    dev_handle = grub_efidisk_get_device_handle (dev->disk);
  else if (dev->net && dev->net->server)
    {
      grub_net_network_level_address_t addr;
      struct grub_net_network_level_interface *inf;
      grub_net_network_level_address_t gateway;
      grub_err_t err;

      err = grub_net_resolve_address (dev->net->server, &addr);
      if (err)
	goto fail;

      err = grub_net_route_address (addr, &gateway, &inf);
      if (err)
	goto fail;

      dev_handle = grub_efinet_get_device_handle (inf->card);
    }

  if (dev_handle)
    dp = grub_efi_get_device_path (dev_handle);

  if (! dp)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "not a valid root device");
      goto fail;
    }

  file_path = make_file_path (dp, filename);
  if (! file_path)
    goto fail;

  grub_printf ("file path: ");
  grub_efi_print_device_path (file_path);

  size = grub_file_size (file);
  if (!size)
    {
      grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
		  filename);
      goto fail;
    }
  pages = (((grub_efi_uintn_t) size + ((1 << 12) - 1)) >> 12);

  status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,
			      GRUB_EFI_LOADER_CODE,
			      pages, &address);
  if (status != GRUB_EFI_SUCCESS)
    {
      grub_dprintf ("chain", "Failed to allocate %u pages\n",
		    (unsigned int) pages);
      grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  if (grub_file_read (file, (void *) ((grub_addr_t) address), size) != size)
    {
      if (grub_errno == GRUB_ERR_NONE)
	grub_error (GRUB_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);

      goto fail;
    }

  status = efi_call_6 (b->load_image, 0, grub_efi_image_handle, file_path,
			  (void *) ((grub_addr_t) address), size,
			  &image_handle);
  if (status != GRUB_EFI_SUCCESS)
    {
      if (status == GRUB_EFI_OUT_OF_RESOURCES)
	grub_error (GRUB_ERR_OUT_OF_MEMORY, "out of resources");
      else
	grub_error (GRUB_ERR_BAD_OS, "cannot load image");

      goto fail;
    }

  /* LoadImage does not set a device handler when the image is
     loaded from memory, so it is necessary to set it explicitly here.
     This is a mess.  */
  loaded_image = grub_efi_get_loaded_image (image_handle);
  if (! loaded_image)
    {
      grub_error (GRUB_ERR_BAD_OS, "no loaded image available");
      goto fail;
    }
  loaded_image->device_handle = dev_handle;

  grub_file_close (file);

  if (argc > 1)
    {
      int i, len;
      grub_efi_char16_t *p16;

      for (i = 1, len = 0; i < argc; i++)
        len += grub_strlen (argv[i]) + 1;

      len *= sizeof (grub_efi_char16_t);
      cmdline = p16 = grub_malloc (len);
      if (! cmdline)
        goto fail;

      for (i = 1; i < argc; i++)
        {
          char *p8;

          p8 = argv[i];
          while (*p8)
            *(p16++) = *(p8++);

          *(p16++) = ' ';
        }
      *(--p16) = 0;

      loaded_image->load_options = cmdline;
      loaded_image->load_options_size = len;
    }

  grub_loader_set (grub_chainloader_boot, grub_chainloader_unload, 0);
  return 0;

 fail:

  if (dev)
    grub_device_close (dev);

  if (file)
    grub_file_close (file);

  grub_free (file_path);

  if (address)
    efi_call_2 (b->free_pages, address, pages);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_command_t cmd;

GRUB_MOD_INIT(chainloader)
{
  cmd = grub_register_command ("chainloader", grub_cmd_chainloader,
			       0, N_("Load another boot loader."));
  my_mod = mod;
}

GRUB_MOD_FINI(chainloader)
{
  grub_unregister_command (cmd);
}
