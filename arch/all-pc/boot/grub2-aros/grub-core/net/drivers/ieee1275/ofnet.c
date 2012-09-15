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

#include <grub/net/netbuff.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/dl.h>
#include <grub/net.h>
#include <grub/time.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct grub_ofnetcard_data
{
  char *path;
  grub_ieee1275_ihandle_t handle;
};

static grub_err_t
card_open (struct grub_net_card *dev)
{
  int status;
  struct grub_ofnetcard_data *data = dev->data;

  if (!grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_OFNET_SUFFIX))
    {
      char path[grub_strlen (data->path) +
		sizeof (":speed=auto,duplex=auto,1.1.1.1,dummy,1.1.1.1,1.1.1.1,5,5,1.1.1.1,512")];
      
      /* The full string will prevent a bootp packet to be sent. Just put some valid ip in there.  */
      grub_snprintf (path, sizeof (path), "%s%s", data->path,
		     ":speed=auto,duplex=auto,1.1.1.1,dummy,1.1.1.1,1.1.1.1,5,5,1.1.1.1,512");
      status = grub_ieee1275_open (path, &(data->handle));
    }
  else
    status = grub_ieee1275_open (data->path, &(data->handle));

  if (status)
    return grub_error (GRUB_ERR_IO, "Couldn't open network card.");

  return GRUB_ERR_NONE;
}

static void
card_close (struct grub_net_card *dev)
{
  struct grub_ofnetcard_data *data = dev->data;

  if (data->handle)
    grub_ieee1275_close (data->handle);
}

static grub_err_t
send_card_buffer (struct grub_net_card *dev, struct grub_net_buff *pack)
{
  grub_ssize_t actual;
  int status;
  struct grub_ofnetcard_data *data = dev->data;
  grub_size_t len;

  len = (pack->tail - pack->data);
  if (len > dev->mtu)
    len = dev->mtu;

  grub_memcpy (dev->txbuf, pack->data, len);
  status = grub_ieee1275_write (data->handle, dev->txbuf,
				pack->tail - pack->data, &actual);

  if (status)
    return grub_error (GRUB_ERR_IO, N_("couldn't send network packet"));
  return GRUB_ERR_NONE;
}

static struct grub_net_buff *
get_card_packet (struct grub_net_card *dev)
{
  grub_ssize_t actual;
  int rc;
  struct grub_ofnetcard_data *data = dev->data;
  grub_uint64_t start_time;
  struct grub_net_buff *nb;

  nb = grub_netbuff_alloc (dev->mtu + 64 + 2);
  if (!nb)
    {
      grub_netbuff_free (nb);
      return NULL;
    }
  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  grub_netbuff_reserve (nb, 2);

  start_time = grub_get_time_ms ();
  do
    rc = grub_ieee1275_read (data->handle, nb->data, dev->mtu + 64, &actual);
  while ((actual <= 0 || rc < 0) && (grub_get_time_ms () - start_time < 200));
  if (actual > 0)
    {
      grub_netbuff_put (nb, actual);
      return nb;
    }
  grub_netbuff_free (nb);
  return NULL;
}

static struct grub_net_card_driver ofdriver =
  {
    .name = "ofnet",
    .open = card_open,
    .close = card_close,
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static const struct
{
  const char *name;
  int offset;
}

bootp_response_properties[] =
  {
    { .name = "bootp-response", .offset = 0},
    { .name = "dhcp-response", .offset = 0},
    { .name = "bootpreply-packet", .offset = 0x2a},
  };

static void
grub_ieee1275_net_config_real (const char *devpath, char **device, char **path)
{
  struct grub_net_card *card;

  /* FIXME: Check that it's the right card.  */
  FOR_NET_CARDS (card)
  {
    char *bootp_response;
    char *cardpath;
    char *canon;

    grub_ssize_t size = -1;
    unsigned int i;

    if (card->driver != &ofdriver)
      continue;

    cardpath = ((struct grub_ofnetcard_data *) card->data)->path;
    canon = grub_ieee1275_canonicalise_devname (cardpath);
    if (grub_strcmp (devpath, canon) != 0)
      {
	grub_free (canon);
	continue;
      }
    grub_free (canon);

    for (i = 0; i < ARRAY_SIZE (bootp_response_properties); i++)
      if (grub_ieee1275_get_property_length (grub_ieee1275_chosen,
					     bootp_response_properties[i].name,
					     &size) >= 0)
	break;

    if (size < 0)
      return;

    bootp_response = grub_malloc (size);
    if (!bootp_response)
      {
	grub_print_error ();
	return;
      }
    if (grub_ieee1275_get_property (grub_ieee1275_chosen,
				    bootp_response_properties[i].name,
				    bootp_response, size, 0) < 0)
      return;

    grub_net_configure_by_dhcp_ack (card->name, card, 0,
				    (struct grub_net_bootp_packet *)
				    (bootp_response
				     + bootp_response_properties[i].offset),
				    size - bootp_response_properties[i].offset,
				    1, device, path);
    grub_free (bootp_response);
    return;
  }
}

static char *
find_alias (const char *fullname)
{
  char *ret = NULL;
  auto int find_alias_hook (struct grub_ieee1275_devalias *alias);

  int find_alias_hook (struct grub_ieee1275_devalias *alias)
  {
    if (grub_strcmp (alias->path, fullname) == 0)
      {
	ret = grub_strdup (alias->name);
	return 1;
      }
    return 0;
  }

  grub_devalias_iterate (find_alias_hook);
  grub_errno = GRUB_ERR_NONE;
  return ret;
}

static int
search_net_devices (struct grub_ieee1275_devalias *alias)
{
  struct grub_ofnetcard_data *ofdata;
  struct grub_net_card *card;
  grub_ieee1275_phandle_t devhandle;
  grub_net_link_level_address_t lla;
  char *shortname;

  if (grub_strcmp (alias->type, "network") != 0)
    return 0;

  ofdata = grub_malloc (sizeof (struct grub_ofnetcard_data));
  if (!ofdata)
    {
      grub_print_error ();
      return 1;
    }
  card = grub_zalloc (sizeof (struct grub_net_card));
  if (!card)
    {
      grub_free (ofdata);
      grub_print_error ();
      return 1;
    }

  ofdata->path = grub_strdup (alias->path);

  grub_ieee1275_finddevice (ofdata->path, &devhandle);

  {
    grub_uint32_t t;
    if (grub_ieee1275_get_integer_property (devhandle,
					    "max-frame-size", &t,
					    sizeof (t), 0))
      card->mtu = 1500;
    else
      card->mtu = t;
  }

  if (grub_ieee1275_get_property (devhandle, "mac-address",
				  &(lla.mac), 6, 0)
      && grub_ieee1275_get_property (devhandle, "local-mac-address",
				     &(lla.mac), 6, 0))
    {
      grub_error (GRUB_ERR_IO, "Couldn't retrieve mac address.");
      grub_print_error ();
      return 0;
    }

  lla.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  card->default_address = lla;

  card->txbufsize = ALIGN_UP (card->mtu, 64) + 256;

  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_VIRT_TO_REAL_BROKEN))
    {
      struct alloc_args
      {
	struct grub_ieee1275_common_hdr common;
	grub_ieee1275_cell_t method;
	grub_ieee1275_cell_t len;
	grub_ieee1275_cell_t catch;
	grub_ieee1275_cell_t result;
      }
      args;
      INIT_IEEE1275_COMMON (&args.common, "interpret", 2, 2);
      args.len = card->txbufsize;
      args.method = (grub_ieee1275_cell_t) "alloc-mem";

      if (IEEE1275_CALL_ENTRY_FN (&args) == -1
	  || args.catch)
	{
	  card->txbuf = 0;
	  grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
	}
      else
	card->txbuf = (void *) args.result;
    }
  else
    card->txbuf = grub_zalloc (card->txbufsize);
  if (!card->txbuf)
    {
      grub_print_error ();
      return 0;
    }
  card->driver = NULL;
  card->data = ofdata;
  card->flags = 0;
  shortname = find_alias (alias->path);
  card->name = grub_xasprintf ("ofnet_%s", shortname ? : alias->path);
  card->idle_poll_delay_ms = 10;
  grub_free (shortname);

  card->driver = &ofdriver;
  grub_net_card_register (card);
  return 0;
}

static void
grub_ofnet_findcards (void)
{
  /* Look at all nodes for devices of the type network.  */
  grub_ieee1275_devices_iterate (search_net_devices);
}

GRUB_MOD_INIT(ofnet)
{
  grub_ofnet_findcards ();
  grub_ieee1275_net_config = grub_ieee1275_net_config_real;
}

GRUB_MOD_FINI(ofnet)
{
  struct grub_net_card *card, *next;

  FOR_NET_CARDS_SAFE (card, next) 
    if (card->driver && grub_strcmp (card->driver->name, "ofnet") == 0)
      grub_net_card_unregister (card);
  grub_ieee1275_net_config = 0;
}
