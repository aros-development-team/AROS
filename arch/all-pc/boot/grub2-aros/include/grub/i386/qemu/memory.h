/* memory.h - describe the memory map */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007  Free Software Foundation, Inc.
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

#ifndef _GRUB_MEMORY_MACHINE_HEADER
#define _GRUB_MEMORY_MACHINE_HEADER      1

#include <grub/symbol.h>
#include <grub/i386/coreboot/memory.h>

#ifndef ASM_FILE
#include <grub/err.h>
#include <grub/types.h>
#endif

#define GRUB_MEMORY_MACHINE_LOWER_USABLE		0x9fc00		/* 640 kiB - 1 kiB */

#define GRUB_MEMORY_MACHINE_UPPER_START			0x100000	/* 1 MiB */
#define GRUB_MEMORY_MACHINE_LOWER_SIZE			GRUB_MEMORY_MACHINE_UPPER_START

#endif /* ! _GRUB_MEMORY_MACHINE_HEADER */
