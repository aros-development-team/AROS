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

#include <grub/cpu/io.h>
#include <grub/cpu/at_keyboard.h>
#include <grub/cpu/reboot.h>
#include <grub/misc.h>

void
grub_reboot (void)
{
  /* Use the keyboard controller to reboot.  That's what keyboards were
     designed for, isn't it?  */
  grub_outb (KEYBOARD_COMMAND_REBOOT, KEYBOARD_REG_STATUS);

  grub_printf ("GRUB doesn't know how to reboot this machine yet!\n");
}
