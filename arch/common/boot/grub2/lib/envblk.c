/* envblk.c - Common function for environment block.  */
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

#include <config.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/lib/envblk.h>

grub_envblk_t
grub_envblk_find (char *buf)
{
  grub_uint32_t *pd;
  int len;

  pd = (grub_uint32_t *) buf;

  for (len = GRUB_ENVBLK_MAXLEN - 6; len > 0; len -= 4, pd++)
    if (*pd == GRUB_ENVBLK_SIGNATURE)
      {
        grub_envblk_t p;

        p = (grub_envblk_t) pd;
        if (p->length <= len)
          return p;
      }

  return 0;
}

int
grub_envblk_insert (grub_envblk_t envblk, char *name, char *value)
{
  char *p, *pend;
  char *found = 0;
  int nl;

  nl = grub_strlen (name);
  p = envblk->data;
  pend = p + envblk->length;

  while (*p)
    {
      if ((! found) && (! grub_memcmp (name, p, nl)) && (p[nl] == '='))
        found = p + nl + 1;

      p += grub_strlen (p) + 1;
      if (p >= pend)
        return 1;
    }

  if (found)
    {
      int len1, len2;

      len1 = grub_strlen (found);
      len2 = grub_strlen (value);
      if ((p - envblk->data) + 1 - len1 + len2 > envblk->length)
        return 1;

      grub_memcpy (found + len2 + 1, found + len1 + 1, (p - found) - len1);
      grub_strcpy (found, value);
    }
  else
    {
      int len2 = grub_strlen (value);

      if ((p - envblk->data) + nl + 1 + len2 + 2 > envblk->length)
        return 1;

      grub_strcpy (p, name);
      p[nl] = '=';
      grub_strcpy (p + nl + 1, value);
      p[nl + 1 + len2 + 1] = 0;
    }

  return 0;
}

void
grub_envblk_delete (grub_envblk_t envblk, char *name)
{
  char *p, *pend;
  char *found = 0;
  int nl;

  nl = grub_strlen (name);
  p = envblk->data;
  pend = p + envblk->length;

  while (*p)
    {
      if ((! found) && (! grub_memcmp (name, p, nl)) && (p[nl] == '='))
        found = p;

      p += grub_strlen (p) + 1;
      if (p >= pend)
        return;
    }

  if (found)
    {
      int len;

      len = grub_strlen (found);
      grub_memcpy (found, found + len + 1, (p - found) - len);
    }
}

void
grub_envblk_iterate (grub_envblk_t envblk,
                     int hook (char *name, char *value))
{
  char *p, *pend;

  p = envblk->data;
  pend = p + envblk->length;

  while (*p)
    {
      char *v;
      int r;

      v = grub_strchr (p, '=');
      if (v)
        {
          *v = 0;
          r = hook (p, v + 1);
          *v = '=';
        }
      else
        r = hook (p, "");

      if (r)
        break;

      p += grub_strlen (p) + 1;
      if (p >= pend)
        break;
    }
}
