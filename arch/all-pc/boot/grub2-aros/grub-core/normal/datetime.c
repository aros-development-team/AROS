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

#include <grub/datetime.h>
#include <grub/i18n.h>

static const char *const grub_weekday_names[] =
{
  N_("Sunday"),
  N_("Monday"),
  N_("Tuesday"),
  N_("Wednesday"),
  N_("Thursday"),
  N_("Friday"),
  N_("Saturday"),
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

const char *
grub_get_weekday_name (struct grub_datetime *datetime)
{
  return _ (grub_weekday_names[grub_get_weekday (datetime)]);
}

#define SECPERMIN 60
#define SECPERHOUR (60*SECPERMIN)
#define SECPERDAY (24*SECPERHOUR)
#define SECPERYEAR (365*SECPERDAY)
#define SECPER4YEARS (4*SECPERYEAR+SECPERDAY)


void
grub_unixtime2datetime (grub_int32_t nix, struct grub_datetime *datetime)
{
  int i;
  int div;
  grub_uint8_t months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  /* In the period of validity of unixtime all years divisible by 4
     are bissextile*/
  /* Convenience: let's have 3 consecutive non-bissextile years
     at the beginning of the epoch. So count from 1973 instead of 1970 */
  nix -= 3*SECPERYEAR + SECPERDAY;
  /* Transform C divisions and modulos to mathematical ones */
  div = nix / SECPER4YEARS;
  if (nix < 0)
    div--;
  datetime->year = 1973 + 4 * div;
  nix -= div * SECPER4YEARS;

  /* On 31st December of bissextile years 365 days from the beginning
     of the year elapsed but year isn't finished yet */
  if (nix / SECPERYEAR == 4)
    {
      datetime->year += 3;
      nix -= 3*SECPERYEAR;
    }
  else
    {
      datetime->year += nix / SECPERYEAR;
      nix %= SECPERYEAR;
    }
  for (i = 0; i < 12
	 && nix >= ((grub_int32_t) (i==1 && datetime->year % 4 == 0
				    ? 29 : months[i]))*SECPERDAY; i++)
    nix -= ((grub_int32_t) (i==1 && datetime->year % 4 == 0
			    ? 29 : months[i]))*SECPERDAY;
  datetime->month = i + 1;
  datetime->day = 1 + (nix / SECPERDAY);
  nix %= SECPERDAY;
  datetime->hour = (nix / SECPERHOUR);
  nix %= SECPERHOUR;
  datetime->minute = nix / SECPERMIN;
  datetime->second = nix % SECPERMIN;
}
