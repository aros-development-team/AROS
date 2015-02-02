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

#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/mm.h>
#include <grub/net.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/time.h>

/* ARP header operation codes */
enum
  {
    ARP_REQUEST = 1,
    ARP_REPLY = 2
  };

enum
  {
    /* IANA ARP constant to define hardware type as ethernet. */
    GRUB_NET_ARPHRD_ETHERNET = 1
  };

struct arphdr {
  grub_uint16_t hrd;
  grub_uint16_t pro;
  grub_uint8_t hln;
  grub_uint8_t pln;
  grub_uint16_t op;
} GRUB_PACKED;

static int have_pending;
static grub_uint32_t pending_req;

grub_err_t
grub_net_arp_send_request (struct grub_net_network_level_interface *inf,
			   const grub_net_network_level_address_t *proto_addr)
{
  struct grub_net_buff nb;
  struct arphdr *arp_header;
  grub_net_link_level_address_t target_hw_addr;
  grub_uint8_t *aux, arp_data[128];
  grub_err_t err;
  int i;
  grub_size_t addrlen;
  grub_uint16_t etherpro;
  grub_uint8_t *nbd;

  if (proto_addr->type == GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    {
      addrlen = 4;
      etherpro = GRUB_NET_ETHERTYPE_IP;
    }
  else
    return grub_error (GRUB_ERR_BUG, "unsupported address family");

  /* Build a request packet.  */
  nb.head = arp_data;
  nb.end = arp_data + sizeof (arp_data);
  grub_netbuff_clear (&nb);
  grub_netbuff_reserve (&nb, 128);

  err = grub_netbuff_push (&nb, sizeof (*arp_header) + 2 * (6 + addrlen));
  if (err)
    return err;

  arp_header = (struct arphdr *) nb.data;
  arp_header->hrd = grub_cpu_to_be16_compile_time (GRUB_NET_ARPHRD_ETHERNET);
  arp_header->hln = 6;
  arp_header->pro = grub_cpu_to_be16 (etherpro);
  arp_header->pln = addrlen;
  arp_header->op = grub_cpu_to_be16_compile_time (ARP_REQUEST);
  aux = (grub_uint8_t *) arp_header + sizeof (*arp_header);
  /* Sender hardware address.  */
  grub_memcpy (aux, &inf->hwaddress.mac, 6);

  aux += 6;
  /* Sender protocol address */
  grub_memcpy (aux, &inf->address.ipv4, 4);
  aux += addrlen;
  /* Target hardware address */
  for (i = 0; i < 6; i++)
    aux[i] = 0x00;
  aux += 6;
  /* Target protocol address */
  grub_memcpy (aux, &proto_addr->ipv4, 4);
  grub_memset (&target_hw_addr.mac, 0xff, 6);

  nbd = nb.data;
  send_ethernet_packet (inf, &nb, target_hw_addr, GRUB_NET_ETHERTYPE_ARP);
  for (i = 0; i < GRUB_NET_TRIES; i++)
    {
      if (grub_net_link_layer_resolve_check (inf, proto_addr))
	return GRUB_ERR_NONE;
      pending_req = proto_addr->ipv4;
      have_pending = 0;
      grub_net_poll_cards (GRUB_NET_INTERVAL + (i * GRUB_NET_INTERVAL_ADDITION),
                           &have_pending);
      if (grub_net_link_layer_resolve_check (inf, proto_addr))
	return GRUB_ERR_NONE;
      nb.data = nbd;
      send_ethernet_packet (inf, &nb, target_hw_addr, GRUB_NET_ETHERTYPE_ARP);
    }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_net_arp_receive (struct grub_net_buff *nb,
		      struct grub_net_card *card)
{
  struct arphdr *arp_header = (struct arphdr *) nb->data;
  grub_uint8_t *sender_hardware_address;
  grub_uint8_t *target_hardware_address;
  grub_net_network_level_address_t sender_addr, target_addr;
  grub_net_link_level_address_t sender_hw_addr;
  struct grub_net_network_level_interface *inf;
  grub_uint8_t *sender_protocol_address, *target_protocol_address;

  sender_hardware_address =
    (grub_uint8_t *) arp_header + sizeof (*arp_header);
  sender_protocol_address = sender_hardware_address + arp_header->hln;
  target_hardware_address = sender_protocol_address + arp_header->pln;
  target_protocol_address = target_hardware_address + arp_header->hln;
  if (grub_be_to_cpu16 (arp_header->pro) == GRUB_NET_ETHERTYPE_IP
      && arp_header->pln == 4)
    {
      sender_addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target_addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      grub_memcpy (&sender_addr.ipv4, sender_protocol_address, 4);
      grub_memcpy (&target_addr.ipv4, target_protocol_address, 4);
      if (grub_memcmp (sender_protocol_address, &pending_req, 4) == 0)
	have_pending = 1;
    }
  else
    return GRUB_ERR_NONE;

  sender_hw_addr.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  grub_memcpy (sender_hw_addr.mac, sender_hardware_address,
	       sizeof (sender_hw_addr.mac));
  grub_net_link_layer_add_address (card, &sender_addr, &sender_hw_addr, 1);

  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    /* Am I the protocol address target? */
    if (grub_net_addr_cmp (&inf->address, &target_addr) == 0
	&& grub_be_to_cpu16 (arp_header->op) == ARP_REQUEST)
      {
	grub_net_link_level_address_t target;
	/* We've already checked that pln is either 4 or 16.  */
	char tmp[16];
	grub_size_t pln = arp_header->pln;

	if (pln > 16)
	  pln = 16;

	target.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
	grub_memcpy (target.mac, sender_hardware_address, 6);
	grub_memcpy (target_hardware_address, target.mac, 6);
	grub_memcpy (sender_hardware_address, inf->hwaddress.mac, 6);

	grub_memcpy (tmp, sender_protocol_address, pln);
	grub_memcpy (sender_protocol_address, target_protocol_address, pln);
	grub_memcpy (target_protocol_address, tmp, pln);

	/* Change operation to REPLY and send packet */
	arp_header->op = grub_be_to_cpu16 (ARP_REPLY);
	send_ethernet_packet (inf, nb, target, GRUB_NET_ETHERTYPE_ARP);
      }
  }
  return GRUB_ERR_NONE;
}
