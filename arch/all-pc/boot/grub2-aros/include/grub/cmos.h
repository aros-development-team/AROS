/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008, 2009  Free Software Foundation, Inc.
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

#ifndef	GRUB_CMOS_H
#define	GRUB_CMOS_H	1

#include <grub/types.h>
#include <grub/cpu/io.h>
#include <grub/cpu/cmos.h>

#define GRUB_CMOS_INDEX_SECOND		0
#define GRUB_CMOS_INDEX_SECOND_ALARM	1
#define GRUB_CMOS_INDEX_MINUTE		2
#define GRUB_CMOS_INDEX_MINUTE_ALARM	3
#define GRUB_CMOS_INDEX_HOUR		4
#define GRUB_CMOS_INDEX_HOUR_ALARM	5
#define GRUB_CMOS_INDEX_DAY_OF_WEEK	6
#define GRUB_CMOS_INDEX_DAY_OF_MONTH	7
#define GRUB_CMOS_INDEX_MONTH		8
#define GRUB_CMOS_INDEX_YEAR		9

#define GRUB_CMOS_INDEX_STATUS_A	0xA
#define GRUB_CMOS_INDEX_STATUS_B	0xB
#define GRUB_CMOS_INDEX_STATUS_C	0xC
#define GRUB_CMOS_INDEX_STATUS_D	0xD

#define GRUB_CMOS_STATUS_B_DAYLIGHT	1
#define GRUB_CMOS_STATUS_B_24HOUR	2
#define GRUB_CMOS_STATUS_B_BINARY	4

static inline grub_uint8_t
grub_bcd_to_num (grub_uint8_t a)
{
  return ((a >> 4) * 10 + (a & 0xF));
}

static inline grub_uint8_t
grub_num_to_bcd (grub_uint8_t a)
{
  return (((a / 10) << 4) + (a % 10));
}

static inline grub_uint8_t
grub_cmos_read (grub_uint8_t index)
{
  grub_outb (index, GRUB_CMOS_ADDR_REG);
  return grub_inb (GRUB_CMOS_DATA_REG);
}

static inline void
grub_cmos_write (grub_uint8_t index, grub_uint8_t value)
{
  grub_outb (index, GRUB_CMOS_ADDR_REG);
  grub_outb (value, GRUB_CMOS_DATA_REG);
}

#endif /* GRUB_CMOS_H */
