/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2011  Free Software Foundation, Inc.
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

#include <grub/net.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>

struct icmp_header
{
  grub_uint8_t type;
  grub_uint8_t code;
  grub_uint16_t checksum;
} __attribute__ ((packed));

struct ping_header
{
  grub_uint16_t id;
  grub_uint16_t seq;
} __attribute__ ((packed));

enum
  {
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO = 8,
  };

grub_err_t
grub_net_recv_icmp_packet (struct grub_net_buff *nb,
			   struct grub_net_network_level_interface *inf,
			   const grub_net_link_level_address_t *ll_src,
			   const grub_net_network_level_address_t *src)
{
  struct icmp_header *icmph;
  grub_err_t err;
  grub_uint16_t checksum;

  /* Ignore broadcast.  */
  if (!inf)
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }

  icmph = (struct icmp_header *) nb->data;

  if (nb->tail - nb->data < (grub_ssize_t) sizeof (*icmph))
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }

  checksum = icmph->checksum;
  icmph->checksum = 0;
  if (checksum != grub_net_ip_chksum (nb->data, nb->tail - nb->data))
    {
      icmph->checksum = checksum;
      return GRUB_ERR_NONE;
    }
  icmph->checksum = checksum;

  err = grub_netbuff_pull (nb, sizeof (*icmph));
  if (err)
    return err;

  switch (icmph->type)
    {
    case ICMP_ECHO:
      {
	struct grub_net_buff *nb_reply;
	struct icmp_header *icmphr;
	if (icmph->code)
	  break;
	nb_reply = grub_netbuff_alloc (nb->tail - nb->data + 512);
	if (!nb_reply)
	  {
	    grub_netbuff_free (nb);
	    return grub_errno;
	  }
	err = grub_netbuff_reserve (nb_reply, nb->tail - nb->data + 512);
	if (err)
	  goto ping_fail;
	err = grub_netbuff_push (nb_reply, nb->tail - nb->data);
	if (err)
	  goto ping_fail;
	grub_memcpy (nb_reply->data, nb->data, nb->tail - nb->data);
	err = grub_netbuff_push (nb_reply, sizeof (*icmphr));
	if (err)
	  goto ping_fail;
	icmphr = (struct icmp_header *) nb_reply->data;
	icmphr->type = ICMP_ECHO_REPLY;
	icmphr->code = 0;
	icmphr->checksum = 0;
	icmphr->checksum = grub_net_ip_chksum ((void *) nb_reply->data,
					       nb_reply->tail - nb_reply->data);
	err = grub_net_send_ip_packet (inf, src, ll_src,
				       nb_reply, GRUB_NET_IP_ICMP);

      ping_fail:
	grub_netbuff_free (nb);
	grub_netbuff_free (nb_reply);
	return err;
      }
    };

  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
