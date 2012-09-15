/* raid6_recover.c - module to recover from faulty RAID6 arrays.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/diskfilter.h>
#include <grub/crypto.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* x**y.  */
static grub_uint8_t powx[255 * 2];
/* Such an s that x**s = y */
static int powx_inv[256];
static const grub_uint8_t poly = 0x1d;

static void
grub_raid_block_mulx (int mul, char *buf, int size)
{
  int i;
  grub_uint8_t *p;

  p = (grub_uint8_t *) buf;
  for (i = 0; i < size; i++, p++)
    if (*p)
      *p = powx[mul + powx_inv[*p]];
}

static void
grub_raid6_init_table (void)
{
  int i;

  grub_uint8_t cur = 1;
  for (i = 0; i < 255; i++)
    {
      powx[i] = cur;
      powx[i + 255] = cur;
      powx_inv[cur] = i;
      if (cur & 0x80)
	cur = (cur << 1) ^ poly;
      else
	cur <<= 1;
    }
}

static grub_err_t
grub_raid6_recover (struct grub_diskfilter_segment *array, int disknr, int p,
                    char *buf, grub_disk_addr_t sector, grub_size_t size)
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
  if (q == (int) array->node_count)
    q = 0;

  pos = q + 1;
  if (pos == (int) array->node_count)
    pos = 0;

  for (i = 0; i < (int) array->node_count - 2; i++)
    {
      int c;
      if (array->layout & GRUB_RAID_LAYOUT_MUL_FROM_POS)
	c = pos;
      else
	c = i;
      if (pos == disknr)
        bad1 = c;
      else
        {
          if (! grub_diskfilter_read_node (&array->nodes[pos], sector,
					   size >> GRUB_DISK_SECTOR_BITS, buf))
            {
              grub_crypto_xor (pbuf, pbuf, buf, size);
              grub_raid_block_mulx (c, buf, size);
              grub_crypto_xor (qbuf, qbuf, buf, size);
            }
          else
            {
              /* Too many bad devices */
              if (bad2 >= 0)
                goto quit;

              bad2 = c;
              grub_errno = GRUB_ERR_NONE;
            }
        }

      pos++;
      if (pos == (int) array->node_count)
        pos = 0;
    }

  /* Invalid disknr or p */
  if (bad1 < 0)
    goto quit;

  if (bad2 < 0)
    {
      /* One bad device */
      if ((! grub_diskfilter_read_node (&array->nodes[p], sector,
					size >> GRUB_DISK_SECTOR_BITS, buf)))
        {
          grub_crypto_xor (buf, buf, pbuf, size);
          goto quit;
        }

      grub_errno = GRUB_ERR_NONE;
      if (grub_diskfilter_read_node (&array->nodes[q], sector,
				     size >> GRUB_DISK_SECTOR_BITS, buf))
        goto quit;

      grub_crypto_xor (buf, buf, qbuf, size);
      grub_raid_block_mulx (255 - bad1, buf,
                           size);
    }
  else
    {
      /* Two bad devices */
      int c;

      if (grub_diskfilter_read_node (&array->nodes[p], sector,
				     size >> GRUB_DISK_SECTOR_BITS, buf))
        goto quit;

      grub_crypto_xor (pbuf, pbuf, buf, size);

      if (grub_diskfilter_read_node (&array->nodes[q], sector,
				     size >> GRUB_DISK_SECTOR_BITS, buf))
        goto quit;

      grub_crypto_xor (qbuf, qbuf, buf, size);

      c = (255 - bad1 + (255 - powx_inv[(powx[bad2 - bad1 + 255] ^ 1)])) % 255;
      grub_raid_block_mulx (c, qbuf, size);

      c = (bad2 + c) % 255;
      grub_raid_block_mulx (c, pbuf, size);

      grub_crypto_xor (pbuf, pbuf, qbuf, size);
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
