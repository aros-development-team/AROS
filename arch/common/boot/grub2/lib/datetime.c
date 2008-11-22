/* datetime.c - Module for common datetime function.  */
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

#include <grub/lib/datetime.h>

static char *grub_weekday_names[] =
{
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
};

int
grub_get_weekday (struct grub_datetime *datetime)
{
  int a, y, m;

  a = (14 - datetime->month) / 12;
  y = datetime->year - a;
  m = datetime->month + 12 * a - 2;

  return (datetime->day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
}

char *
grub_get_weekday_name (struct grub_datetime *datetime)
{
  return grub_weekday_names[grub_get_weekday (datetime)];
}
