/* cmain.c - Startup code for the PowerPC.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.
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

#include <alloca.h>
#include <stdint.h>
#include <grub/kernel.h>
#include <grub/machine/kernel.h>
#include <grub/ieee1275/ieee1275.h>

/* OpenFirmware entry point passed to us from the real bootloader.  */
int (*grub_ieee1275_entry_fn) (void *);

grub_ieee1275_phandle_t grub_ieee1275_chosen;

static grub_uint32_t grub_ieee1275_flags;



int
grub_ieee1275_test_flag (enum grub_ieee1275_flag flag)
{
  return (grub_ieee1275_flags & (1 << flag));
}

void
grub_ieee1275_set_flag (enum grub_ieee1275_flag flag)
{
  grub_ieee1275_flags |= (1 << flag);
}

static void
grub_ieee1275_find_options (void)
{
  grub_ieee1275_phandle_t options;
  grub_ieee1275_phandle_t openprom;
  int realmode;
  int smartfw;

  grub_ieee1275_finddevice ("/options", &options);
  grub_ieee1275_get_property (options, "real-mode?", &realmode,
			      sizeof (realmode), 0);
  if (realmode)
    grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_REAL_MODE);

  grub_ieee1275_finddevice ("/openprom", &openprom);
  smartfw = grub_ieee1275_get_property (openprom, "SmartFirmware-version",
					0, 0, 0);
  if (smartfw != -1)
    grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_0_BASED_PARTITIONS);
}

void cmain (uint32_t r3, uint32_t r4, uint32_t r5);
void
cmain (uint32_t r3, uint32_t r4 __attribute__((unused)), uint32_t r5)
{
  if (r5 == 0xdeadbeef)
    {
      /* Entered from Old World stage1.  */
      extern char _start;
      extern char _end;

      grub_ieee1275_entry_fn = (int (*)(void *)) r3;

      grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_NO_PARTITION_0);

      /* Old World Open Firmware may use 4M-5M without claiming it.  */
      grub_ieee1275_claim (0x00400000, 0x00100000, 0, 0);

      /* Need to claim ourselves so we don't cannibalize our memory later.  */
      if (grub_ieee1275_claim ((grub_addr_t) &_start, (grub_addr_t) &_end
          - (grub_addr_t) &_start, 0, 0))
	abort();
    }
  else
    {
      /* Assume we were entered from Open Firmware.  */
      grub_ieee1275_entry_fn = (int (*)(void *)) r5;
    }

  grub_ieee1275_finddevice ("/chosen", &grub_ieee1275_chosen);

  grub_ieee1275_find_options ();

  /* Now invoke the main function.  */
  grub_main ();

  /* Never reached.  */
}
