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

grub_uint16_t grub_console_getwh (struct grub_term_output *term);
void grub_console_setcolorstate (struct grub_term_output *term,
				 grub_term_color_state state);

#endif /* ! GRUB_VGA_COMMON_CPU_HEADER */
