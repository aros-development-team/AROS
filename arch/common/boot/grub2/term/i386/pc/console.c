/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>

static const struct grub_machine_bios_data_area *bios_data_area =
  (struct grub_machine_bios_data_area *) GRUB_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR;

#define KEYBOARD_LEFT_SHIFT	(1 << 0)
#define KEYBOARD_RIGHT_SHIFT	(1 << 1)
#define KEYBOARD_CTRL		(1 << 2)
#define KEYBOARD_ALT		(1 << 3)

static int
grub_console_getkeystatus (void)
{
  grub_uint8_t status = bios_data_area->keyboard_flag_lower;
  int mods = 0;

  if (status & (KEYBOARD_LEFT_SHIFT | KEYBOARD_RIGHT_SHIFT))
    mods |= GRUB_TERM_STATUS_SHIFT;
  if (status & KEYBOARD_CTRL)
    mods |= GRUB_TERM_STATUS_CTRL;
  if (status & KEYBOARD_ALT)
    mods |= GRUB_TERM_STATUS_ALT;

  return mods;
}

static struct grub_term_input grub_console_term_input =
  {
    .name = "console",
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
    .getkeystatus = grub_console_getkeystatus,
  };

static struct grub_term_output grub_console_term_output =
  {
    .name = "console",
    .putchar = grub_console_putchar,
    .getcharwidth = grub_console_getcharwidth,
    .getwh = grub_console_getwh,
    .getxy = grub_console_getxy,
    .gotoxy = grub_console_gotoxy,
    .cls = grub_console_cls,
    .setcolorstate = grub_console_setcolorstate,
    .setcolor = grub_console_setcolor,
    .getcolor = grub_console_getcolor,
    .setcursor = grub_console_setcursor,
    .flags = 0,
  };

void
grub_console_init (void)
{
  grub_term_register_output ("console", &grub_console_term_output);
  grub_term_register_input ("console", &grub_console_term_input);
}

void
grub_console_fini (void)
{
  /* This is to make sure the console is restored to text mode before
     we boot.  */
  grub_term_set_current_output (&grub_console_term_output);

  grub_term_unregister_input (&grub_console_term_input);
  grub_term_unregister_output (&grub_console_term_output);
}
