/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2008  Free Software Foundation, Inc.
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

#ifndef GRUB_CONSOLE_MACHINE_HEADER
#define GRUB_CONSOLE_MACHINE_HEADER	1

/* Define scan codes.  */
#define GRUB_CONSOLE_KEY_LEFT		0x4B00
#define GRUB_CONSOLE_KEY_RIGHT		0x4D00
#define GRUB_CONSOLE_KEY_UP		0x4800
#define GRUB_CONSOLE_KEY_DOWN		0x5000
#define GRUB_CONSOLE_KEY_IC		0x5200
#define GRUB_CONSOLE_KEY_DC		0x5300
#define GRUB_CONSOLE_KEY_BACKSPACE	0x0008
#define GRUB_CONSOLE_KEY_HOME		0x4700
#define GRUB_CONSOLE_KEY_END		0x4F00
#define GRUB_CONSOLE_KEY_NPAGE		0x5100
#define GRUB_CONSOLE_KEY_PPAGE		0x4900

#ifndef ASM_FILE

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/term.h>
#include <grub/i386/vga_common.h>

/* These are global to share code between C and asm.  */
int grub_console_checkkey (void);
int grub_console_getkey (void);
grub_uint16_t grub_console_getxy (void);
void grub_console_gotoxy (grub_uint8_t x, grub_uint8_t y);
void grub_console_cls (void);
void grub_console_setcursor (int on);

/* Initialize the console system.  */
void grub_console_init (void);

/* Finish the console system.  */
void grub_console_fini (void);

#endif

#endif /* ! GRUB_CONSOLE_MACHINE_HEADER */
