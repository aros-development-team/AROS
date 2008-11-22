/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_CPU_AT_KEYBOARD_HEADER
#define GRUB_CPU_AT_KEYBOARD_HEADER	1

#include <grub/machine/machine.h>

#define SHIFT_L		0x2a
#define SHIFT_R		0x36
#define CTRL		0x1d
#define ALT		0x38
#define CAPS_LOCK	0x3a

#define KEYBOARD_REG_DATA	0x60
#define KEYBOARD_REG_STATUS	0x64

/* Used for sending commands to the controller.  */
#define KEYBOARD_COMMAND_ISREADY(x)	!((x) & 0x02)
#define KEYBOARD_COMMAND_READ		0x20
#define KEYBOARD_COMMAND_WRITE		0x60
#define KEYBOARD_COMMAND_REBOOT		0xfe

#define KEYBOARD_SCANCODE_SET1		0x40

#define KEYBOARD_ISMAKE(x)	!((x) & 0x80)
#define KEYBOARD_ISREADY(x)	(((x) & 0x01) == 0)
#define KEYBOARD_SCANCODE(x)	((x) & 0x7f)

#ifdef GRUB_MACHINE_IEEE1275
#define OLPC_UP		GRUB_TERM_UP
#define OLPC_DOWN	GRUB_TERM_DOWN
#define OLPC_LEFT	GRUB_TERM_LEFT
#define OLPC_RIGHT	GRUB_TERM_RIGHT
#else
#define OLPC_UP		'\0'
#define OLPC_DOWN	'\0'
#define OLPC_LEFT	'\0'
#define OLPC_RIGHT	'\0'
#endif

#endif
