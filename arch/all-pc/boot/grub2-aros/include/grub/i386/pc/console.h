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

#ifndef ASM_FILE

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/term.h>
#include <grub/i386/vga_common.h>

/* These are global to share code between C and asm.  */
int grub_console_getkey (struct grub_term_input *term);
grub_uint16_t grub_console_getxy (struct grub_term_output *term);
void grub_console_gotoxy (struct grub_term_output *term,
			  grub_uint8_t x, grub_uint8_t y);
void grub_console_cls (struct grub_term_output *term);
void grub_console_setcursor (struct grub_term_output *term, int on);
void grub_console_putchar (struct grub_term_output *term,
			   const struct grub_unicode_glyph *c);

/* Initialize the console system.  */
void grub_console_init (void);

/* Finish the console system.  */
void grub_console_fini (void);

#endif

#endif /* ! GRUB_CONSOLE_MACHINE_HEADER */
