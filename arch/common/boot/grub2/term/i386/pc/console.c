/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>

static struct grub_term_input grub_console_term_input =
  {
    .name = "console",
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
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
  grub_term_register_output (&grub_console_term_output);
  grub_term_register_input (&grub_console_term_input);
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
