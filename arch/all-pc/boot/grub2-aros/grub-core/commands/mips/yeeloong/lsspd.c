/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/cs5536.h>

static grub_err_t
grub_cmd_lsspd (grub_command_t cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  grub_pci_device_t dev;
  grub_port_t smbbase;
  int i;
  grub_err_t err;

  if (!grub_cs5536_find (&dev))
    {
      grub_printf ("No CS5536 found\n");
      return GRUB_ERR_NONE;
    }
  grub_printf ("CS5536 at %d:%d.%d\n", grub_pci_get_bus (dev),
	       grub_pci_get_device (dev), grub_pci_get_function (dev));
  
  err = grub_cs5536_init_smbus (dev, 0x7fff, &smbbase);
  if (err)
    return err;

  grub_printf ("SMB base = 0x%x\n", smbbase);

  for (i = GRUB_SMB_RAM_START_ADDR;
       i < GRUB_SMB_RAM_START_ADDR + GRUB_SMB_RAM_NUM_MAX; i++)
    {
      struct grub_smbus_spd spd;
      grub_memset (&spd, 0, sizeof (spd));
      grub_printf ("Device %d\n", i);
      err = grub_cs5536_read_spd (smbbase, i, &spd);
      if (err)
	{
	  grub_print_error ();
	  continue;
	}
      grub_printf ("Written SPD bytes: %d B.\n", spd.written_size);
      grub_printf ("Total flash size: %d B.\n", 1 << spd.log_total_flash_size);
      if (spd.memory_type == GRUB_SMBUS_SPD_MEMORY_TYPE_DDR2)
	{
	  char str[sizeof (spd.ddr2.part_number) + 1];
	  grub_printf ("Memory type: DDR2.\n");
	  grub_memcpy (str, spd.ddr2.part_number,
		       sizeof (spd.ddr2.part_number));
	  str[sizeof (spd.ddr2.part_number)] = 0;
	  grub_printf ("Part no: %s.\n", str);
	}
      else
	grub_printf ("Memory type: Unknown.\n");
    }

  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

GRUB_MOD_INIT(lsspd)
{
  cmd = grub_register_command ("lsspd", grub_cmd_lsspd, 0,
			       "Print Memory information.");
}

GRUB_MOD_FINI(lsspd)
{
  grub_unregister_command (cmd);
}
