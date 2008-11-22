/* crc.c - crc function  */
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

#include <grub/types.h>
#include <grub/lib/crc.h>

static grub_uint32_t crc32_table [256];

static void
init_crc32_table (void)
{
  auto grub_uint32_t reflect (grub_uint32_t ref, int len);
  grub_uint32_t reflect (grub_uint32_t ref, int len)
    {
      grub_uint32_t result = 0;
      int i;

      for (i = 1; i <= len; i++)
        {
          if (ref & 1)
            result |= 1 << (len - i);
          ref >>= 1;
        }

      return result;
    }

  grub_uint32_t polynomial = 0x04c11db7;
  int i, j;

  for(i = 0; i < 256; i++)
    {
      crc32_table[i] = reflect(i, 8) << 24;
      for (j = 0; j < 8; j++)
        crc32_table[i] = (crc32_table[i] << 1) ^
            (crc32_table[i] & (1 << 31) ? polynomial : 0);
      crc32_table[i] = reflect(crc32_table[i], 32);
    }
}

grub_uint32_t
grub_getcrc32 (grub_uint32_t crc, void *buf, int size)
{
  int i;
  grub_uint8_t *data = buf;

  if (! crc32_table[1])
    init_crc32_table ();

  crc^= 0xffffffff;

  for (i = 0; i < size; i++)
    {
      crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *data];
      data++;
    }

  return crc ^ 0xffffffff;
}
