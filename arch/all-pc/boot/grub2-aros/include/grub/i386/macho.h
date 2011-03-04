/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_CPU_MACHO_H
#define GRUB_CPU_MACHO_H 1

#include <grub/macho.h>

#define GRUB_MACHO_CPUTYPE_IS_HOST32(x) ((x)==0x00000007)
#define GRUB_MACHO_CPUTYPE_IS_HOST64(x) ((x)==0x01000007)

struct grub_macho_thread32
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t unknown1[48];
  grub_uint32_t entry_point;
  grub_uint8_t unknown2[20];
} __attribute__ ((packed));


struct grub_macho_thread64
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t unknown1[0x88];
  grub_uint64_t entry_point;
  grub_uint8_t unknown2[0x20];
} __attribute__ ((packed));

#endif
