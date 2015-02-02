/* lsefisystab.c  - Display EFI systab.  */
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
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/charset.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct guid_mapping
{
  grub_efi_guid_t guid;
  const char *name;
};

static const struct guid_mapping guid_mappings[] =
  {
    { GRUB_EFI_ACPI_20_TABLE_GUID, "ACPI-2.0"},
    { GRUB_EFI_ACPI_TABLE_GUID, "ACPI-1.0"},
    { GRUB_EFI_SAL_TABLE_GUID, "SAL"},
    { GRUB_EFI_SMBIOS_TABLE_GUID, "SMBIOS"},
    { GRUB_EFI_MPS_TABLE_GUID, "MPS"},
    { GRUB_EFI_HCDP_TABLE_GUID, "HCDP"}
  };

static grub_err_t
grub_cmd_lsefisystab (struct grub_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char **args __attribute__ ((unused)))
{
  const grub_efi_system_table_t *st = grub_efi_system_table;
  grub_efi_configuration_table_t *t;
  unsigned int i;

  grub_printf ("Address: %p\n", st);
  grub_printf ("Signature: %016" PRIxGRUB_UINT64_T " revision: %08x\n",
	       st->hdr.signature, st->hdr.revision);
  {
    char *vendor;
    grub_uint16_t *vendor_utf16;
    grub_printf ("Vendor: ");
    
    for (vendor_utf16 = st->firmware_vendor; *vendor_utf16; vendor_utf16++);
    vendor = grub_malloc (4 * (vendor_utf16 - st->firmware_vendor) + 1);
    if (!vendor)
      return grub_errno;
    *grub_utf16_to_utf8 ((grub_uint8_t *) vendor, st->firmware_vendor,
			 vendor_utf16 - st->firmware_vendor) = 0;
    grub_printf ("%s", vendor);
    grub_free (vendor);
  }

  grub_printf (", Version=%x\n", st->firmware_revision);

  grub_printf ("%lld tables:\n", (long long) st->num_table_entries);
  t = st->configuration_table;
  for (i = 0; i < st->num_table_entries; i++)
    {
      unsigned int j;

      grub_printf ("%p  ", t->vendor_table);

      grub_printf ("%08x-%04x-%04x-",
		   t->vendor_guid.data1, t->vendor_guid.data2,
		   t->vendor_guid.data3);
      for (j = 0; j < 8; j++)
	grub_printf ("%02x", t->vendor_guid.data4[j]);
      
      for (j = 0; j < ARRAY_SIZE (guid_mappings); j++)
	if (grub_memcmp (&guid_mappings[j].guid, &t->vendor_guid,
			 sizeof (grub_efi_guid_t)) == 0)
	  grub_printf ("   %s", guid_mappings[j].name);

      grub_printf ("\n");
      t++;
    }
  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

GRUB_MOD_INIT(lsefisystab)
{
  cmd = grub_register_command ("lsefisystab", grub_cmd_lsefisystab, 
			       "", "Display EFI system tables.");
}

GRUB_MOD_FINI(lsefisystab)
{
  grub_unregister_command (cmd);
}
