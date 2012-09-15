/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/netbuff.h>

grub_err_t
grub_netbuff_put (struct grub_net_buff *nb, grub_size_t len)
{
  nb->tail += len;
  if (nb->tail > nb->end)
    return grub_error (GRUB_ERR_BUG, "put out of the packet range.");
  return GRUB_ERR_NONE;
}

grub_err_t
grub_netbuff_unput (struct grub_net_buff *nb, grub_size_t len)
{
  nb->tail -= len;
  if (nb->tail < nb->head)
    return grub_error (GRUB_ERR_BUG,
		       "unput out of the packet range.");
  return GRUB_ERR_NONE;
}

grub_err_t
grub_netbuff_push (struct grub_net_buff *nb, grub_size_t len)
{
  nb->data -= len;
  if (nb->data < nb->head)
    return grub_error (GRUB_ERR_BUG,
		       "push out of the packet range.");
  return GRUB_ERR_NONE;
}

grub_err_t
grub_netbuff_pull (struct grub_net_buff *nb, grub_size_t len)
{
  nb->data += len;
  if (nb->data > nb->end)
    return grub_error (GRUB_ERR_BUG,
		       "pull out of the packet range.");
  return GRUB_ERR_NONE;
}

grub_err_t
grub_netbuff_reserve (struct grub_net_buff *nb, grub_size_t len)
{
  nb->data += len;
  nb->tail += len;
  if ((nb->tail > nb->end) || (nb->data > nb->end))
    return grub_error (GRUB_ERR_BUG,
		       "reserve out of the packet range.");
  return GRUB_ERR_NONE;
}

struct grub_net_buff *
grub_netbuff_alloc (grub_size_t len)
{
  struct grub_net_buff *nb;
  void *data;

  COMPILE_TIME_ASSERT (NETBUFF_ALIGN % sizeof (grub_properly_aligned_t) == 0);

  if (len < NETBUFFMINLEN)
    len = NETBUFFMINLEN;

  len = ALIGN_UP (len, NETBUFF_ALIGN);
  data = grub_memalign (NETBUFF_ALIGN, len + sizeof (*nb));
  if (!data)
    return NULL;
  nb = (struct grub_net_buff *) ((grub_properly_aligned_t *) data
				 + len / sizeof (grub_properly_aligned_t));
  nb->head = nb->data = nb->tail = data;
  nb->end = (grub_uint8_t *) nb;
  return nb;
}

void
grub_netbuff_free (struct grub_net_buff *nb)
{
  if (!nb)
    return;
  grub_free (nb->head);
}

grub_err_t
grub_netbuff_clear (struct grub_net_buff *nb)
{
  nb->data = nb->tail = nb->head;
  return GRUB_ERR_NONE;
}
