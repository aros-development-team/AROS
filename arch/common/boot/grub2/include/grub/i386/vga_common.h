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

#ifndef GRUB_VGA_COMMON_CPU_HEADER
#define GRUB_VGA_COMMON_CPU_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/term.h>

extern grub_uint8_t grub_console_cur_color;

void grub_console_putchar (grub_uint32_t c);
grub_ssize_t grub_console_getcharwidth (grub_uint32_t c);
grub_uint16_t grub_console_getwh (void);
void grub_console_setcolorstate (grub_term_color_state state);
void grub_console_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color);
void grub_console_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color);

/* Implemented in both kern/i386/pc/startup.S and vga_text.c;  this symbol
   is not exported, so there's no collision, but vga_common.c expects this
   prototype to be the same.  */
void grub_console_real_putchar (int c);

#endif /* ! GRUB_VGA_COMMON_CPU_HEADER */
