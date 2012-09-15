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
#include <grub/env.h>
#include <grub/i18n.h>
#include <grub/command.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>
#include <grub/net/udp.h>
#include <grub/datetime.h>

static char *
grub_env_write_readonly (struct grub_env_var *var __attribute__ ((unused)),
			 const char *val __attribute__ ((unused)))
{
  return NULL;
}

static void
set_env_limn_ro (const char *intername, const char *suffix,
		 char *value, grub_size_t len)
{
  char c;
  char varname[sizeof ("net_") + grub_strlen (intername) + sizeof ("_")
	       + grub_strlen (suffix)];
  char *ptr;
  grub_snprintf (varname, sizeof (varname), "net_%s_%s", intername, suffix);
  for (ptr = varname; *ptr; ptr++)
    if (*ptr == ':')
      *ptr = '_';    
  c = value[len];
  value[len] = 0;
  grub_env_set (varname, value);
  value[len] = c;
  grub_register_variable_hook (varname, 0, grub_env_write_readonly);
}

static void
parse_dhcp_vendor (const char *name, void *vend, int limit, int *mask)
{
  grub_uint8_t *ptr, *ptr0;

  ptr = ptr0 = vend;

  if (ptr[0] != GRUB_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != GRUB_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != GRUB_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != GRUB_NET_BOOTP_RFC1048_MAGIC_3)
    return;
  ptr = ptr + sizeof (grub_uint32_t);
  while (ptr - ptr0 < limit)
    {
      grub_uint8_t tagtype;
      grub_uint8_t taglength;

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == GRUB_NET_BOOTP_PAD)
	continue;

      /* End tag.  */
      if (tagtype == GRUB_NET_BOOTP_END)
	return;

      taglength = *ptr++;

      switch (tagtype)
	{
	case GRUB_NET_BOOTP_NETMASK:
	  if (taglength == 4)
	    {
	      int i;
	      for (i = 0; i < 32; i++)
		if (!(ptr[i / 8] & (1 << (7 - (i % 8)))))
		  break;
	      *mask = i;
	    }
	  break;

	case GRUB_NET_BOOTP_ROUTER:
	  if (taglength == 4)
	    {
	      grub_net_network_level_netaddress_t target;
	      grub_net_network_level_address_t gw;
	      char rname[grub_strlen (name) + sizeof (":default")];
	      
	      target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      target.ipv4.base = 0;
	      target.ipv4.masksize = 0;
	      gw.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      grub_memcpy (&gw.ipv4, ptr, sizeof (gw.ipv4));
	      grub_snprintf (rname, sizeof (rname), "%s:default", name);
	      grub_net_add_route_gw (rname, target, gw);
	    }
	  break;
	case GRUB_NET_BOOTP_DNS:
	  {
	    int i;
	    for (i = 0; i < taglength / 4; i++)
	      {
		struct grub_net_network_level_address s;
		s.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
		s.ipv4 = grub_get_unaligned32 (ptr);
		grub_net_add_dns_server (&s);
		ptr += 4;
	      }
	  }
	  break;
	case GRUB_NET_BOOTP_HOSTNAME:
	  set_env_limn_ro (name, "hostname", (char *) ptr, taglength);
	  break;

	case GRUB_NET_BOOTP_DOMAIN:
	  set_env_limn_ro (name, "domain", (char *) ptr, taglength);
	  break;

	case GRUB_NET_BOOTP_ROOT_PATH:
	  set_env_limn_ro (name, "rootpath", (char *) ptr, taglength);
	  break;

	case GRUB_NET_BOOTP_EXTENSIONS_PATH:
	  set_env_limn_ro (name, "extensionspath", (char *) ptr, taglength);
	  break;

	  /* If you need any other options please contact GRUB
	     developpement team.  */
	}

      ptr += taglength;
    }
}

#define OFFSET_OF(x, y) ((grub_uint8_t *)((y)->x) - (grub_uint8_t *)(y))

struct grub_net_network_level_interface *
grub_net_configure_by_dhcp_ack (const char *name,
				struct grub_net_card *card,
				grub_net_interface_flags_t flags,
				const struct grub_net_bootp_packet *bp,
				grub_size_t size,
				int is_def, char **device, char **path)
{
  grub_net_network_level_address_t addr;
  grub_net_link_level_address_t hwaddr;
  struct grub_net_network_level_interface *inter;
  int mask = -1;

  addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  addr.ipv4 = bp->your_ip;

  if (device)
    *device = 0;
  if (path)
    *path = 0;

  grub_memcpy (hwaddr.mac, bp->mac_addr,
	       bp->hw_len < sizeof (hwaddr.mac) ? bp->hw_len
	       : sizeof (hwaddr.mac));
  hwaddr.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  inter = grub_net_add_addr (name, card, &addr, &hwaddr, flags);
  if (bp->gateway_ip)
    {
      grub_net_network_level_netaddress_t target;
      grub_net_network_level_address_t gw;
      char rname[grub_strlen (name) + sizeof (":gw")];
	  
      target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target.ipv4.base = bp->server_ip;
      target.ipv4.masksize = 32;
      gw.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      gw.ipv4 = bp->gateway_ip;
      grub_snprintf (rname, sizeof (rname), "%s:gw", name);
      grub_net_add_route_gw (rname, target, gw);

      target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target.ipv4.base = bp->gateway_ip;
      target.ipv4.masksize = 32;
      grub_net_add_route (name, target, inter);
    }

  if (size > OFFSET_OF (boot_file, bp))
    set_env_limn_ro (name, "boot_file", (char *) bp->boot_file,
		     sizeof (bp->boot_file));
  if (is_def)
    grub_net_default_server = 0;
  if (is_def && !grub_net_default_server && bp->server_ip)
    {
      grub_net_default_server = grub_xasprintf ("%d.%d.%d.%d",
						((grub_uint8_t *) &bp->server_ip)[0],
						((grub_uint8_t *) &bp->server_ip)[1],
						((grub_uint8_t *) &bp->server_ip)[2],
						((grub_uint8_t *) &bp->server_ip)[3]);
      grub_print_error ();
    }

  if (device && !*device && bp->server_ip)
    {
      *device = grub_xasprintf ("tftp,%d.%d.%d.%d",
				((grub_uint8_t *) &bp->server_ip)[0],
				((grub_uint8_t *) &bp->server_ip)[1],
				((grub_uint8_t *) &bp->server_ip)[2],
				((grub_uint8_t *) &bp->server_ip)[3]);
      grub_print_error ();
    }
  if (size > OFFSET_OF (server_name, bp)
      && bp->server_name[0])
    {
      set_env_limn_ro (name, "dhcp_server_name", (char *) bp->server_name,
		       sizeof (bp->server_name));
      if (is_def && !grub_net_default_server)
	{
	  grub_net_default_server = grub_strdup (bp->server_name);
	  grub_print_error ();
	}
      if (device && !*device)
	{
	  *device = grub_xasprintf ("tftp,%s", bp->server_name);
	  grub_print_error ();
	}
    }

  if (size > OFFSET_OF (boot_file, bp) && path)
    {
      *path = grub_strndup (bp->boot_file, sizeof (bp->boot_file));
      grub_print_error ();
      if (*path)
	{
	  char *slash;
	  slash = grub_strrchr (*path, '/');
	  if (slash)
	    *slash = 0;
	  else
	    **path = 0;
	}
    }
  if (size > OFFSET_OF (vendor, bp))
    parse_dhcp_vendor (name, &bp->vendor, size - OFFSET_OF (vendor, bp), &mask);
  grub_net_add_ipv4_local (inter, mask);
  
  inter->dhcp_ack = grub_malloc (size);
  if (inter->dhcp_ack)
    {
      grub_memcpy (inter->dhcp_ack, bp, size);
      inter->dhcp_acklen = size;
    }
  else
    grub_errno = GRUB_ERR_NONE;

  return inter;
}

void
grub_net_process_dhcp (struct grub_net_buff *nb,
		       struct grub_net_card *card)
{
  char *name;
  struct grub_net_network_level_interface *inf;

  name = grub_xasprintf ("%s:dhcp", card->name);
  if (!name)
    {
      grub_print_error ();
      return;
    }
  grub_net_configure_by_dhcp_ack (name, card,
				  0, (const struct grub_net_bootp_packet *) nb->data,
				  (nb->tail - nb->data), 0, 0, 0);
  grub_free (name);
  if (grub_errno)
    grub_print_error ();
  else
    {
      FOR_NET_NETWORK_LEVEL_INTERFACES(inf)
	if (grub_memcmp (inf->name, card->name, grub_strlen (card->name)) == 0
	    && grub_memcmp (inf->name + grub_strlen (card->name),
			    ":dhcp_tmp", sizeof (":dhcp_tmp") - 1) == 0)
	  {
	    grub_net_network_level_interface_unregister (inf);
	    break;
	  }
    }
}

static char
hexdigit (grub_uint8_t val)
{
  if (val < 10)
    return val + '0';
  return val + 'a' - 10;
}

static grub_err_t
grub_cmd_dhcpopt (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_network_level_interface *inter;
  int num;
  grub_uint8_t *ptr;
  grub_uint8_t taglength;

  if (argc < 4)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("four arguments expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (grub_strcmp (inter->name, args[1]) == 0)
      break;

  if (!inter)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("unrecognised network interface `%s'"), args[1]);

  if (!inter->dhcp_ack)
    return grub_error (GRUB_ERR_IO, N_("no DHCP info found"));

  if (inter->dhcp_acklen <= OFFSET_OF (vendor, inter->dhcp_ack))
    return grub_error (GRUB_ERR_IO, N_("no DHCP options found"));

  num = grub_strtoul (args[2], 0, 0);
  if (grub_errno)
    return grub_errno;

  ptr = inter->dhcp_ack->vendor;

  if (ptr[0] != GRUB_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != GRUB_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != GRUB_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != GRUB_NET_BOOTP_RFC1048_MAGIC_3)
    return grub_error (GRUB_ERR_IO, N_("no DHCP options found"));
  ptr = ptr + sizeof (grub_uint32_t);
  while (1)
    {
      grub_uint8_t tagtype;

      if (ptr >= ((grub_uint8_t *) inter->dhcp_ack) + inter->dhcp_acklen)
	return grub_error (GRUB_ERR_IO, N_("no DHCP option %d found"), num);

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == 0)
	continue;

      /* End tag.  */
      if (tagtype == 0xff)
	return grub_error (GRUB_ERR_IO, N_("no DHCP option %d found"), num);

      taglength = *ptr++;
	
      if (tagtype == num)
	break;
      ptr += taglength;
    }

  if (grub_strcmp (args[3], "string") == 0)
    {
      char *val = grub_malloc (taglength + 1);
      if (!val)
	return grub_errno;
      grub_memcpy (val, ptr, taglength);
      val[taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%s\n", val);
      else
	return grub_env_set (args[0], val);
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[3], "number") == 0)
    {
      grub_uint64_t val = 0;
      int i;
      for (i = 0; i < taglength; i++)
	val = (val << 8) | ptr[i];
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%llu\n", (unsigned long long) val);
      else
	{
	  char valn[64];
	  grub_printf (valn, sizeof (valn), "%lld\n", (unsigned long long) val);
	  return grub_env_set (args[0], valn);
	}
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[3], "hex") == 0)
    {
      char *val = grub_malloc (2 * taglength + 1);
      int i;
      if (!val)
	return grub_errno;
      for (i = 0; i < taglength; i++)
	{
	  val[2 * i] = hexdigit (ptr[i] >> 4);
	  val[2 * i + 1] = hexdigit (ptr[i] & 0xf);
	}
      val[2 * taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%s\n", val);
      else
	return grub_env_set (args[0], val);
      return GRUB_ERR_NONE;
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     N_("unrecognised DHCP option format specification `%s'"),
		     args[3]);
}

/* FIXME: allow to specify mac address.  */
static grub_err_t
grub_cmd_bootp (struct grub_command *cmd __attribute__ ((unused)),
		int argc, char **args)
{
  struct grub_net_card *card;
  struct grub_net_network_level_interface *ifaces;
  grub_size_t ncards = 0;
  unsigned j = 0;
  int interval;
  grub_err_t err;

  FOR_NET_CARDS (card)
  {
    if (argc > 0 && grub_strcmp (card->name, args[0]) != 0)
      continue;
    ncards++;
  }

  ifaces = grub_zalloc (ncards * sizeof (ifaces[0]));
  if (!ifaces)
    return grub_errno;

  j = 0;
  FOR_NET_CARDS (card)
  {
    if (argc > 0 && grub_strcmp (card->name, args[0]) != 0)
      continue;
    ifaces[j].card = card;
    ifaces[j].next = &ifaces[j+1];
    if (j)
      ifaces[j].prev = &ifaces[j-1].next;
    ifaces[j].name = grub_xasprintf ("%s:dhcp_tmp", card->name);
    card->num_ifaces++;
    if (!ifaces[j].name)
      {
	unsigned i;
	for (i = 0; i < j; i++)
	  grub_free (ifaces[i].name);
	grub_free (ifaces);
	return grub_errno;
      }
    ifaces[j].address.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV;
    grub_memcpy (&ifaces[j].hwaddress, &card->default_address, 
		 sizeof (ifaces[j].hwaddress));
    j++;
  }
  ifaces[ncards - 1].next = grub_net_network_level_interfaces;
  if (grub_net_network_level_interfaces)
    grub_net_network_level_interfaces->prev = & ifaces[ncards - 1].next;
  grub_net_network_level_interfaces = &ifaces[0];
  ifaces[0].prev = &grub_net_network_level_interfaces;
  for (interval = 200; interval < 10000; interval *= 2)
    {
      int done = 0;
      for (j = 0; j < ncards; j++)
	{
	  struct grub_net_bootp_packet *pack;
	  struct grub_datetime date;
	  grub_int32_t t = 0;
	  struct grub_net_buff *nb;
	  struct udphdr *udph;
	  grub_net_network_level_address_t target;
	  grub_net_link_level_address_t ll_target;

	  if (!ifaces[j].prev)
	    continue;
	  nb = grub_netbuff_alloc (sizeof (*pack) + 64 + 128);
	  if (!nb)
	    {
	      grub_netbuff_free (nb);
	      return grub_errno;
	    }
	  err = grub_netbuff_reserve (nb, sizeof (*pack) + 64 + 128);
	  if (err)
	    {
	      grub_netbuff_free (nb);
	      return err;
	    }
	  err = grub_netbuff_push (nb, sizeof (*pack) + 64);
	  if (err)
	    {
	      grub_netbuff_free (nb);
	      return err;
	    }
	  pack = (void *) nb->data;
	  done = 1;
	  grub_memset (pack, 0, sizeof (*pack) + 64);
	  pack->opcode = 1;
	  pack->hw_type = 1;
	  pack->hw_len = 6;
	  err = grub_get_datetime (&date);
	  if (err || !grub_datetime2unixtime (&date, &t))
	    {
	      grub_errno = GRUB_ERR_NONE;
	      t = 0;
	    }
	  pack->ident = grub_cpu_to_be32 (t);
	  pack->seconds = grub_cpu_to_be16 (t);

	  grub_memcpy (&pack->mac_addr, &ifaces[j].hwaddress.mac, 6); 

	  grub_netbuff_push (nb, sizeof (*udph));

	  udph = (struct udphdr *) nb->data;
	  udph->src = grub_cpu_to_be16 (68);
	  udph->dst = grub_cpu_to_be16 (67);
	  udph->chksum = 0;
	  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);
	  target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	  target.ipv4 = 0xffffffff;
	  err = grub_net_link_layer_resolve (&ifaces[j], &target, &ll_target);
	  if (err)
	    return err;

	  udph->chksum = grub_net_ip_transport_checksum (nb, GRUB_NET_IP_UDP,
							 &ifaces[j].address,
							 &target);

	  err = grub_net_send_ip_packet (&ifaces[j], &target, &ll_target, nb,
					 GRUB_NET_IP_UDP);
	  grub_netbuff_free (nb);
	  if (err)
	    return err;
	}
      if (!done)
	break;
      grub_net_poll_cards (interval, 0);
    }

  err = GRUB_ERR_NONE;
  for (j = 0; j < ncards; j++)
    {
      grub_free (ifaces[j].name);
      if (!ifaces[j].prev)
	continue;
      grub_error_push ();
      grub_net_network_level_interface_unregister (&ifaces[j]);
      err = grub_error (GRUB_ERR_FILE_NOT_FOUND,
			N_("couldn't autoconfigure %s"),
			ifaces[j].card->name);
    }

  grub_free (ifaces);
  return err;
}

static grub_command_t cmd_getdhcp, cmd_bootp;

void
grub_bootp_init (void)
{
  cmd_bootp = grub_register_command ("net_bootp", grub_cmd_bootp,
				     N_("[CARD]"),
				     N_("perform a bootp autoconfiguration"));
  cmd_getdhcp = grub_register_command ("net_get_dhcp_option", grub_cmd_dhcpopt,
				       N_("VAR INTERFACE NUMBER DESCRIPTION"),
				       N_("retrieve DHCP option and save it into VAR. If VAR is - then print the value."));
}

void
grub_bootp_fini (void)
{
  grub_unregister_command (cmd_getdhcp);
  grub_unregister_command (cmd_bootp);
}
