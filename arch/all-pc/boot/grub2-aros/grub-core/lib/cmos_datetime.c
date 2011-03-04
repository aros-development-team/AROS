/* kern/cmos_datetime.c - CMOS datetime function.
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

#include <grub/datetime.h>
#include <grub/cmos.h>

grub_err_t
grub_get_datetime (struct grub_datetime *datetime)
{
  int is_bcd, is_12hour;
  grub_uint8_t value, flag;

  flag = grub_cmos_read (GRUB_CMOS_INDEX_STATUS_B);

  is_bcd = ! (flag & GRUB_CMOS_STATUS_B_BINARY);

  value = grub_cmos_read (GRUB_CMOS_INDEX_YEAR);
  if (is_bcd)
    value = grub_bcd_to_num (value);

  datetime->year = value;
  datetime->year += (value < 80) ? 2000 : 1900;

  value = grub_cmos_read (GRUB_CMOS_INDEX_MONTH);
  if (is_bcd)
    value = grub_bcd_to_num (value);

  datetime->month = value;

  value = grub_cmos_read (GRUB_CMOS_INDEX_DAY_OF_MONTH);
  if (is_bcd)
    value = grub_bcd_to_num (value);

  datetime->day = value;

  is_12hour = ! (flag & GRUB_CMOS_STATUS_B_24HOUR);

  value = grub_cmos_read (GRUB_CMOS_INDEX_HOUR);
  if (is_12hour)
    {
      is_12hour = (value & 0x80);

      value &= 0x7F;
      value--;
    }

  if (is_bcd)
    value = grub_bcd_to_num (value);

  if (is_12hour)
    value += 12;

  datetime->hour = value;

  value = grub_cmos_read (GRUB_CMOS_INDEX_MINUTE);
  if (is_bcd)
    value = grub_bcd_to_num (value);

  datetime->minute = value;

  value = grub_cmos_read (GRUB_CMOS_INDEX_SECOND);
  if (is_bcd)
    value = grub_bcd_to_num (value);

  datetime->second = value;

  return 0;
}

grub_err_t
grub_set_datetime (struct grub_datetime *datetime)
{
  int is_bcd, is_12hour;
  grub_uint8_t value, flag;

  flag = grub_cmos_read (GRUB_CMOS_INDEX_STATUS_B);

  is_bcd = ! (flag & GRUB_CMOS_STATUS_B_BINARY);

  value = ((datetime->year >= 2000) ? datetime->year - 2000 :
           datetime->year - 1900);

  if (is_bcd)
    value = grub_num_to_bcd (value);

  grub_cmos_write (GRUB_CMOS_INDEX_YEAR, value);

  value = datetime->month;

  if (is_bcd)
    value = grub_num_to_bcd (value);

  grub_cmos_write (GRUB_CMOS_INDEX_MONTH, value);

  value = datetime->day;

  if (is_bcd)
    value = grub_num_to_bcd (value);

  grub_cmos_write (GRUB_CMOS_INDEX_DAY_OF_MONTH, value);

  value = datetime->hour;

  is_12hour = (! (flag & GRUB_CMOS_STATUS_B_24HOUR));

  if (is_12hour)
    {
      value++;

      if (value > 12)
        value -= 12;
      else
        is_12hour = 0;
    }

  if (is_bcd)
    value = grub_num_to_bcd (value);

  if (is_12hour)
    value |= 0x80;

  grub_cmos_write (GRUB_CMOS_INDEX_HOUR, value);

  value = datetime->minute;

  if (is_bcd)
    value = grub_num_to_bcd (value);

  grub_cmos_write (GRUB_CMOS_INDEX_MINUTE, value);

  value = datetime->second;

  if (is_bcd)
    value = grub_num_to_bcd (value);

  grub_cmos_write (GRUB_CMOS_INDEX_SECOND, value);

  return 0;
}
