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

#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/net.h>
#include <grub/time.h>
#include <grub/net/arp.h>

#define LLCADDRMASK 0x7f

struct etherhdr
{
  grub_uint8_t dst[6];
  grub_uint8_t src[6];
  grub_uint16_t type;
} GRUB_PACKED;

struct llchdr
{
  grub_uint8_t dsap;
  grub_uint8_t ssap;
  grub_uint8_t ctrl;
} GRUB_PACKED;

struct snaphdr
{
  grub_uint8_t oui[3]; 
  grub_uint16_t type;
} GRUB_PACKED;

grub_err_t
send_ethernet_packet (struct grub_net_network_level_interface *inf,
		      struct grub_net_buff *nb,
		      grub_net_link_level_address_t target_addr,
		      grub_net_ethertype_t ethertype)
{
  struct etherhdr *eth;
  grub_err_t err;

  COMPILE_TIME_ASSERT (sizeof (*eth) < GRUB_NET_MAX_LINK_HEADER_SIZE);

  err = grub_netbuff_push (nb, sizeof (*eth));
  if (err)
    return err;
  eth = (struct etherhdr *) nb->data;
  grub_memcpy (eth->dst, target_addr.mac, 6);
  grub_memcpy (eth->src, inf->hwaddress.mac, 6);

  eth->type = grub_cpu_to_be16 (ethertype);
  if (!inf->card->opened)
    {
      err = GRUB_ERR_NONE;
      if (inf->card->driver->open)
	err = inf->card->driver->open (inf->card);
      if (err)
	return err;
      inf->card->opened = 1;
    }
  return inf->card->driver->send (inf->card, nb);
}

grub_err_t
grub_net_recv_ethernet_packet (struct grub_net_buff *nb,
			       struct grub_net_card *card)
{
  struct etherhdr *eth;
  struct llchdr *llch;
  struct snaphdr *snaph;
  grub_net_ethertype_t type;
  grub_net_link_level_address_t hwaddress;
  grub_net_link_level_address_t src_hwaddress;
  grub_err_t err;

  eth = (struct etherhdr *) nb->data;
  type = grub_be_to_cpu16 (eth->type);
  err = grub_netbuff_pull (nb, sizeof (*eth));
  if (err)
    return err;

  if (type <= 1500)
    {
      llch = (struct llchdr *) nb->data;
      type = llch->dsap & LLCADDRMASK;

      if (llch->dsap == 0xaa && llch->ssap == 0xaa && llch->ctrl == 0x3)
	{
	  err = grub_netbuff_pull (nb, sizeof (*llch));
	  if (err)
	    return err;
	  snaph = (struct snaphdr *) nb->data;
	  type = snaph->type;
	}
    }

  hwaddress.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  grub_memcpy (hwaddress.mac, eth->dst, sizeof (hwaddress.mac));
  src_hwaddress.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  grub_memcpy (src_hwaddress.mac, eth->src, sizeof (src_hwaddress.mac));

  switch (type)
    {
      /* ARP packet. */
    case GRUB_NET_ETHERTYPE_ARP:
      grub_net_arp_receive (nb, card);
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
      /* IP packet.  */
    case GRUB_NET_ETHERTYPE_IP:
    case GRUB_NET_ETHERTYPE_IP6:
      return grub_net_recv_ip_packets (nb, card, &hwaddress, &src_hwaddress);
    }
  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
