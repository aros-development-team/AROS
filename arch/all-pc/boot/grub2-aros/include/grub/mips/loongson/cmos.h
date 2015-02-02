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

#ifndef	GRUB_CPU_CMOS_H
#define	GRUB_CPU_CMOS_H	1

#include <grub/types.h>
#include <grub/cpu/io.h>

#define GRUB_CMOS_ADDR_REG	0xbfd00070
#define GRUB_CMOS_DATA_REG	0xbfd00071
#define GRUB_CMOS_ADDR_REG_HI	0xbfd00072
#define GRUB_CMOS_DATA_REG_HI	0xbfd00073

#endif /* GRUB_CPU_CMOS_H */
