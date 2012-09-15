/* date.c - command to display/set current datetime.  */
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

#include <grub/dl.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/datetime.h>
#include <grub/command.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_DATETIME_SET_YEAR		1
#define GRUB_DATETIME_SET_MONTH		2
#define GRUB_DATETIME_SET_DAY		4
#define GRUB_DATETIME_SET_HOUR		8
#define GRUB_DATETIME_SET_MINUTE	16
#define GRUB_DATETIME_SET_SECOND	32

static grub_err_t
grub_cmd_date (grub_command_t cmd __attribute__ ((unused)),
               int argc, char **args)
{
  struct grub_datetime datetime;
  int limit[6][2] = {{1980, 2079}, {1, 12}, {1, 31}, {0, 23}, {0, 59}, {0, 59}};
  int value[6], mask;

  if (argc == 0)
    {
      if (grub_get_datetime (&datetime))
        return grub_errno;

      grub_printf ("%d-%02d-%02d %02d:%02d:%02d %s\n",
                   datetime.year, datetime.month, datetime.day,
                   datetime.hour, datetime.minute, datetime.second,
                   grub_get_weekday_name (&datetime));

      return 0;
    }

  grub_memset (&value, 0, sizeof (value));
  mask = 0;

  for (; argc; argc--, args++)
    {
      char *p, c;
      int m1, ofs, n, cur_mask;

      p = args[0];
      m1 = grub_strtoul (p, &p, 10);

      c = *p;
      if (c == '-')
        ofs = 0;
      else if (c == ':')
        ofs = 3;
      else
        goto fail;

      value[ofs] = m1;
      cur_mask = (1 << ofs);
      mask &= ~(cur_mask * (1 + 2 + 4));

      for (n = 1; (n < 3) && (*p); n++)
        {
          if (*p != c)
            goto fail;

          value[ofs + n] = grub_strtoul (p + 1, &p, 10);
          cur_mask |= (1 << (ofs + n));
        }

      if (*p)
        goto fail;

      if ((ofs == 0) && (n == 2))
        {
          value[ofs + 2] = value[ofs + 1];
          value[ofs + 1] = value[ofs];
          ofs++;
          cur_mask <<= 1;
        }

      for (; n; n--, ofs++)
        if ((value [ofs] < limit[ofs][0]) ||
            (value [ofs] > limit[ofs][1]))
          goto fail;

      mask |= cur_mask;
    }

  if (grub_get_datetime (&datetime))
    return grub_errno;

  if (mask & GRUB_DATETIME_SET_YEAR)
    datetime.year = value[0];

  if (mask & GRUB_DATETIME_SET_MONTH)
    datetime.month = value[1];

  if (mask & GRUB_DATETIME_SET_DAY)
    datetime.day = value[2];

  if (mask & GRUB_DATETIME_SET_HOUR)
    datetime.hour = value[3];

  if (mask & GRUB_DATETIME_SET_MINUTE)
    datetime.minute = value[4];

  if (mask & GRUB_DATETIME_SET_SECOND)
    datetime.second = value[5];

  return grub_set_datetime (&datetime);

fail:
  return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid datetime");
}

static grub_command_t cmd;

GRUB_MOD_INIT(date)
{
  cmd =
    grub_register_command ("date", grub_cmd_date,
			   N_("[[year-]month-day] [hour:minute[:second]]"),
			   N_("Display/set current datetime."));
}

GRUB_MOD_FINI(date)
{
  grub_unregister_command (cmd);
}
