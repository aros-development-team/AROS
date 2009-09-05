/* raid6_recover.c - module to recover from faulty RAID6 arrays.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/raid.h>

static grub_uint8_t raid6_table1[256][256];
static grub_uint8_t raid6_table2[256][256];

static void
grub_raid_block_mul (grub_uint8_t mul, char *buf, int size)
{
  int i;
  grub_uint8_t *p;

  p = (grub_uint8_t *) buf;
  for (i = 0; i < size; i++, p++)
    *p = raid6_table1[mul][*p];
}

static void
grub_raid6_init_table (void)
{
  int i, j;

  for (i = 0; i < 256; i++)
    raid6_table1[i][1] = raid6_table1[1][i] = i;

  for (i = 2; i < 256; i++)
    for (j = i; j < 256; j++)
      {
        int n;
        grub_uint8_t c;

        n = i >> 1;

        c = raid6_table1[n][j];
        c = (c << 1) ^ ((c & 0x80) ? 0x1d : 0);
        if (i & 1)
          c ^= j;

        raid6_table1[j][i] = raid6_table1[i][j] = c;
      }

  raid6_table2[0][0] = 1;
  for (i = 1; i < 256; i++)
    raid6_table2[i][i] = raid6_table1[raid6_table2[i - 1][i - 1]][2];

  for (i = 0; i < 254; i++)
    for (j = 0; j < 254; j++)
      {
        grub_uint8_t c, n;
        int k;

        if (i == j)
          continue;

        k = i - j;
        if (k < 0)
          k += 255;

        c = n = raid6_table2[k][k] ^ 1;
        for (k = 0; k < 253; k++)
          c = raid6_table1[c][n];

        raid6_table2[i][j] = raid6_table1[raid6_table2[255 - j][255 - j]][c];
      }
}

static grub_err_t
grub_raid6_recover (struct grub_raid_array *array, int disknr, int p,
                    char *buf, grub_disk_addr_t sector, int size)
{
  int i, q, pos;
  int bad1 = -1, bad2 = -1;
  char *pbuf = 0, *qbuf = 0;

  size <<= GRUB_DISK_SECTOR_BITS;
  pbuf = grub_zalloc (size);
  if (!pbuf)
    goto quit;

  qbuf = grub_zalloc (size);
  if (!qbuf)
    goto quit;

  q = p + 1;
  if (q == (int) array->total_devs)
    q = 0;

  pos = q + 1;
  if (pos == (int) array->total_devs)
    pos = 0;

  for (i = 0; i < (int) array->total_devs - 2; i++)
    {
      if (pos == disknr)
        bad1 = i;
      else
        {
          if ((array->device[pos]) &&
              (! grub_disk_read (array->device[pos], sector, 0, size, buf)))
            {
              grub_raid_block_xor (pbuf, buf, size);
              grub_raid_block_mul (raid6_table2[i][i], buf, size);
              grub_raid_block_xor (qbuf, buf, size);
            }
          else
            {
              /* Too many bad devices */
              if (bad2 >= 0)
                goto quit;

              bad2 = i;
              grub_errno = GRUB_ERR_NONE;
            }
        }

      pos++;
      if (pos == (int) array->total_devs)
        pos = 0;
    }

  /* Invalid disknr or p */
  if (bad1 < 0)
    goto quit;

  if (bad2 < 0)
    {
      /* One bad device */
      if ((array->device[p]) &&
          (! grub_disk_read (array->device[p], sector, 0, size, buf)))
        {
          grub_raid_block_xor (buf, pbuf, size);
          goto quit;
        }

      if (! array->device[q])
        {
          grub_error (GRUB_ERR_READ_ERROR, "Not enough disk to restore");
          goto quit;
        }

      grub_errno = GRUB_ERR_NONE;
      if (grub_disk_read (array->device[q], sector, 0, size, buf))
        goto quit;

      grub_raid_block_xor (buf, qbuf, size);
      grub_raid_block_mul (raid6_table2[255 - bad1][255 - bad1], buf,
                           size);
    }
  else
    {
      /* Two bad devices */
      grub_uint8_t c;

      if ((! array->device[p]) || (! array->device[q]))
        {
          grub_error (GRUB_ERR_READ_ERROR, "Not enough disk to restore");
          goto quit;
        }

      if (grub_disk_read (array->device[p], sector, 0, size, buf))
        goto quit;

      grub_raid_block_xor (pbuf, buf, size);

      if (grub_disk_read (array->device[q], sector, 0, size, buf))
        goto quit;

      grub_raid_block_xor (qbuf, buf, size);

      c = raid6_table2[bad2][bad1];
      grub_raid_block_mul (c, qbuf, size);

      c = raid6_table1[raid6_table2[bad2][bad2]][c];
      grub_raid_block_mul (c, pbuf, size);

      grub_raid_block_xor (pbuf, qbuf, size);
      grub_memcpy (buf, pbuf, size);
    }

quit:
  grub_free (pbuf);
  grub_free (qbuf);

  return grub_errno;
}

GRUB_MOD_INIT(raid6rec)
{
  grub_raid6_init_table ();
  grub_raid6_recover_func = grub_raid6_recover;
}

GRUB_MOD_FINI(raid6rec)
{
  grub_raid6_recover_func = 0;
}
