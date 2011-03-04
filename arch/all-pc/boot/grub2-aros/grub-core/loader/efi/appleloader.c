/* appleloader.c - apple legacy boot loader.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/command.h>
#include <grub/i18n.h>

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

struct piwg_full_device_path
{
  struct grub_efi_memory_mapped_device_path comp1;
  struct grub_efi_piwg_device_path comp2;
  struct grub_efi_device_path end;
};

/* early 2006 Core Duo / Core Solo models  */
static struct piwg_full_device_path devpath_1 =
{
  .comp1 =
  {
    .header = {
      .type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_memory_mapped_device_path), 0}
    },
    .memory_type = GRUB_EFI_MEMORY_MAPPED_IO,
    .start_address = 0xffe00000,
    .end_address = 0xfff9ffff
  },
  .comp2 =
  {
    .header = {
      .type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_piwg_device_path), 0}
    },
    .guid = {0x2B0585EB, 0xD8B8, 0x49A9, {0x8B, 0x8C, 0xE2, 0x1B,
					  0x01, 0xAE, 0xF2, 0xB7}}
  },
  .end =
  {
    .type = GRUB_EFI_END_DEVICE_PATH_TYPE,
    .subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    .length = {sizeof (struct grub_efi_device_path), 0}
  }
};

/* mid-2006 Mac Pro (and probably other Core 2 models)  */
static struct piwg_full_device_path devpath_2 =
{
  .comp1 =
  {
    .header = {
      .type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_memory_mapped_device_path), 0}
    },
    .memory_type = GRUB_EFI_MEMORY_MAPPED_IO,
    .start_address = 0xffe00000,
    .end_address = 0xfff7ffff
  },
  .comp2 =
  {
    .header = {
      .type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_piwg_device_path), 0}
    },
    .guid = {0x2B0585EB, 0xD8B8, 0x49A9, {0x8B, 0x8C, 0xE2, 0x1B,
					  0x01, 0xAE, 0xF2, 0xB7}}
  },
  .end =
  {
    .type = GRUB_EFI_END_DEVICE_PATH_TYPE,
    .subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    .length = {sizeof (struct grub_efi_device_path), 0}
  }
};

/* mid-2007 MBP ("Santa Rosa" based models)  */
static struct piwg_full_device_path devpath_3 =
{
  .comp1 =
  {
    .header = {
      .type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_memory_mapped_device_path), 0}
    },
    .memory_type = GRUB_EFI_MEMORY_MAPPED_IO,
    .start_address = 0xffe00000,
    .end_address = 0xfff8ffff
  },
  .comp2 =
  {
    .header = {
      .type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_piwg_device_path), 0}
    },
    .guid = {0x2B0585EB, 0xD8B8, 0x49A9, {0x8B, 0x8C, 0xE2, 0x1B,
					  0x01, 0xAE, 0xF2, 0xB7}}
  },
  .end =
  {
    .type = GRUB_EFI_END_DEVICE_PATH_TYPE,
    .subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    .length = {sizeof (struct grub_efi_device_path), 0}
  }
};

/* early-2008 MBA  */
static struct piwg_full_device_path devpath_4 =
{
  .comp1 =
  {
    .header = {
      .type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_memory_mapped_device_path), 0}
    },
    .memory_type = GRUB_EFI_MEMORY_MAPPED_IO,
    .start_address = 0xffc00000,
    .end_address = 0xfff8ffff
  },
  .comp2 =
  {
    .header = {
      .type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_piwg_device_path), 0}
    },
    .guid = {0x2B0585EB, 0xD8B8, 0x49A9, {0x8B, 0x8C, 0xE2, 0x1B,
					  0x01, 0xAE, 0xF2, 0xB7}}
  },
  .end =
  {
    .type = GRUB_EFI_END_DEVICE_PATH_TYPE,
    .subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    .length = {sizeof (struct grub_efi_device_path), 0}
  }
};

/* late-2008 MB/MBP (NVidia chipset)  */
static struct piwg_full_device_path devpath_5 =
{
  .comp1 =
  {
    .header = {
      .type = GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_memory_mapped_device_path), 0}
    },
    .memory_type = GRUB_EFI_MEMORY_MAPPED_IO,
    .start_address = 0xffcb4000,
    .end_address = 0xffffbfff
  },
  .comp2 =
  {
    .header = {
      .type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      .subtype = GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE,
      .length = {sizeof (struct grub_efi_piwg_device_path), 0}
    },
    .guid = {0x2B0585EB, 0xD8B8, 0x49A9, {0x8B, 0x8C, 0xE2, 0x1B,
					  0x01, 0xAE, 0xF2, 0xB7}}
  },
  .end =
  {
    .type = GRUB_EFI_END_DEVICE_PATH_TYPE,
    .subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    .length = {sizeof (struct grub_efi_device_path), 0}
  }
};

struct devdata
{
  char *model;
  grub_efi_device_path_t *devpath;
};

struct devdata devs[] =
{
  {"Core Duo/Solo", (grub_efi_device_path_t *) &devpath_1},
  {"Mac Pro", (grub_efi_device_path_t *) &devpath_2},
  {"MBP", (grub_efi_device_path_t *) &devpath_3},
  {"MBA", (grub_efi_device_path_t *) &devpath_4},
  {"MB NV", (grub_efi_device_path_t *) &devpath_5},
  {NULL, NULL},
};

static grub_err_t
grub_cmd_appleloader (grub_command_t cmd __attribute__ ((unused)),
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

static grub_command_t cmd;

GRUB_MOD_INIT(appleloader)
{
  cmd = grub_register_command ("appleloader", grub_cmd_appleloader,
			       "[OPTS]", N_("Boot legacy system."));
  my_mod = mod;
}

GRUB_MOD_FINI(appleloader)
{
  grub_unregister_command (cmd);
}
