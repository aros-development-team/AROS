/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#ifndef GRUB_INIT_I386_LINUXBIOS_HEADER
#define GRUB_INIT_I386_LINUXBIOS_HEADER		1

#include <grub/symbol.h>
#include <grub/i386/pc/memory.h>

void EXPORT_FUNC(grub_stop) (void) __attribute__ ((noreturn));
void EXPORT_FUNC(grub_stop_floppy) (void);

#endif
