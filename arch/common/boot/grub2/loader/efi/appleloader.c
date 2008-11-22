/* appleloader.c - apple legacy boot loader.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

static grub_dl_t my_mod;

static grub_efi_handle_t image_handle;
static grub_efi_char16_t *cmdline;

static grub_err_t
grub_appleloader_unload (void)
{
  grub_efi_boot_services_t *b;

  b = grub_efi_system_table->boot_services;
  efi_call_1 (b->unload_image, image_handle);

  grub_free (cmdline);
  cmdline = 0;

  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_appleloader_boot (void)
{
  grub_efi_boot_services_t *b;

  b = grub_efi_system_table->boot_services;
  efi_call_3 (b->start_image, image_handle, 0, 0);

  grub_appleloader_unload ();

  return grub_errno;
}

/* early 2006 Core Duo / Core Solo models  */
static grub_uint8_t devpath_1[] =
{
  0x01, 0x03, 0x18, 0x00, 0x0B, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xF9, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00,
};

/* mid-2006 Mac Pro (and probably other Core 2 models)  */
static grub_uint8_t devpath_2[] =
{
  0x01, 0x03, 0x18, 0x00, 0x0B, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xF7, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00,
};

/* mid-2007 MBP ("Santa Rosa" based models)  */
static grub_uint8_t devpath_3[] =
{
  0x01, 0x03, 0x18, 0x00, 0x0B, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00,
};

/* early-2008 MBA  */
static grub_uint8_t devpath_4[] =
{
  0x01, 0x03, 0x18, 0x00, 0x0B, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xC0, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00,
};

struct devdata
{
  char *model;
  grub_efi_device_path_t *devpath;
};

struct devdata devs[] =
{
  {"Core Duo/Solo", (grub_efi_device_path_t *) devpath_1},
  {"Mac Pro", (grub_efi_device_path_t *) devpath_2},
  {"MBP", (grub_efi_device_path_t *) devpath_3},
  {"MBA", (grub_efi_device_path_t *) devpath_4},
  {NULL, NULL},
};

static grub_err_t
grub_cmd_appleloader (struct grub_arg_list *state __attribute__ ((unused)),
                      int argc, char *argv[])
{
  grub_efi_boot_services_t *b;
  grub_efi_loaded_image_t *loaded_image;
  struct devdata *pdev;

  grub_dl_ref (my_mod);

  /* Initialize some global variables.  */
  image_handle = 0;

  b = grub_efi_system_table->boot_services;

  for (pdev = devs ; pdev->devpath ; pdev++)
    if (efi_call_6 (b->load_image, 0, grub_efi_image_handle, pdev->devpath,
                    NULL, 0, &image_handle) == GRUB_EFI_SUCCESS)
      break;

  if (! pdev->devpath)
    {
      grub_error (GRUB_ERR_BAD_OS, "can't find model");
      goto fail;
    }

  grub_printf ("Model : %s\n", pdev->model);

  loaded_image = grub_efi_get_loaded_image (image_handle);
  if (! loaded_image)
    {
      grub_error (GRUB_ERR_BAD_OS, "no loaded image available");
      goto fail;
    }

  if (argc > 0)
    {
      int i, len;
      grub_efi_char16_t *p16;

      for (i = 0, len = 0; i < argc; i++)
        len += grub_strlen (argv[i]) + 1;

      len *= sizeof (grub_efi_char16_t);
      cmdline = p16 = grub_malloc (len);
      if (! cmdline)
        goto fail;

      for (i = 0; i < argc; i++)
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

  grub_loader_set (grub_appleloader_boot, grub_appleloader_unload, 0);

  return 0;

 fail:

  grub_dl_unref (my_mod);
  return grub_errno;
}

GRUB_MOD_INIT(appleloader)
{
  grub_register_command ("appleloader", grub_cmd_appleloader,
			 GRUB_COMMAND_FLAG_BOTH,
			 "appleloader [OPTS]",
			 "Boot legacy system.", 0);

  my_mod = mod;
}

GRUB_MOD_FINI(appleloader)
{
  grub_unregister_command ("appleloader");
}
