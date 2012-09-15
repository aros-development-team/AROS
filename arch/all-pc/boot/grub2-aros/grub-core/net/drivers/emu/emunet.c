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

#include <grub/dl.h>
#include <grub/net/netbuff.h>
#include <sys/socket.h>
#include <grub/net.h>
#include <sys/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <grub/term.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static int fd;

static grub_err_t 
send_card_buffer (struct grub_net_card *dev __attribute__ ((unused)),
		  struct grub_net_buff *pack)
{
  ssize_t actual;

  actual = write (fd, pack->data, pack->tail - pack->data);
  if (actual < 0)
    return grub_error (GRUB_ERR_IO, N_("couldn't send network packet"));

  return GRUB_ERR_NONE;
}

static struct grub_net_buff *
get_card_packet (struct grub_net_card *dev __attribute__ ((unused)))
{
  ssize_t actual;
  struct grub_net_buff *nb;

  nb = grub_netbuff_alloc (1536 + 2);
  if (!nb)
    return NULL;

  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  grub_netbuff_reserve (nb, 2);
  if (!nb)
    {
      grub_netbuff_free (nb);
      return NULL;
    }

  actual = read (fd, nb->data, 1536);
  if (actual < 0)
    {
      grub_netbuff_free (nb);
      return NULL;
    }
  grub_netbuff_put (nb, actual);

  return nb;
}

static struct grub_net_card_driver emudriver = 
  {
    .name = "emu",
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static struct grub_net_card emucard = 
  {
    .name = "emu0",
    .driver = &emudriver,
    .mtu = 1500,
    .default_address = {
			 .type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET,
			 {.mac = {0, 1, 2, 3, 4, 5}}
		       },
    .flags = 0
  };

GRUB_MOD_INIT(emunet)
{
  struct ifreq ifr;
  fd = open ("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if (fd < 0)
    return;
  grub_memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if (ioctl (fd, TUNSETIFF, &ifr) < 0)
    {
      close (fd);
      fd = -1;
      return;
    }
  grub_net_card_register (&emucard);
}

GRUB_MOD_FINI(emunet)
{
  if (fd >= 0)
    {
      close (fd);
      grub_net_card_unregister (&emucard);
    }
}
