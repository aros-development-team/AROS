/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Based on "src/main.c" in etherboot-4.6.4.  */

/**************************************************************************
ETHERBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93
  
Literature dealing with the network protocols:
       ARP - RFC826
       RARP - RFC903
       UDP - RFC768
       BOOTP - RFC951, RFC2132 (vendor extensions)
       DHCP - RFC2131, RFC2132 (options)
       TFTP - RFC1350, RFC2347 (options), RFC2348 (blocksize), RFC2349 (tsize)
       RPC - RFC1831, RFC1832 (XDR), RFC1833 (rpcbind/portmapper)
       NFS - RFC1094, RFC1813 (v3, useful for clarifications, not implemented)

**************************************************************************/

/* #define MDEBUG */

#include <etherboot.h>
#include <nic.h>

struct arptable_t arptable[MAX_ARP];

/* Set if the user pushes Control-C.  */
int ip_abort = 0;
/* Set if an ethernet card is probed and IP addresses are set.  */
int network_ready = 0;

struct rom_info rom;

static int vendorext_isvalid;
static unsigned long netmask;
static struct bootpd_t bootp_data;
static unsigned long xid;
static unsigned char *end_of_rfc1533 = NULL;

#ifndef	NO_DHCP_SUPPORT
static int dhcp_reply;
static in_addr dhcp_server = {0L};
static in_addr dhcp_addr = {0L};
#endif /* NO_DHCP_SUPPORT */

/* äEth */
static unsigned char vendorext_magic[] = {0xE4, 0x45, 0x74, 0x68};

#ifdef	NO_DHCP_SUPPORT

static char rfc1533_cookie[5] = {RFC1533_COOKIE, RFC1533_END};

#else /* ! NO_DHCP_SUPPORT */

static char rfc1533_cookie[] = {RFC1533_COOKIE};
static char rfc1533_end[] = {RFC1533_END};

static const char dhcpdiscover[] =
{
  RFC2132_MSG_TYPE, 1, DHCPDISCOVER,	
  RFC2132_MAX_SIZE,2,	/* request as much as we can */
  sizeof(struct bootpd_t) / 256, sizeof(struct bootpd_t) % 256,
  RFC2132_PARAM_LIST, 4, RFC1533_NETMASK, RFC1533_GATEWAY,
  RFC1533_HOSTNAME, RFC1533_EXTENSIONPATH
};

static const char dhcprequest[] =
{
  RFC2132_MSG_TYPE, 1, DHCPREQUEST,
  RFC2132_SRV_ID, 4, 0, 0, 0, 0,
  RFC2132_REQ_ADDR, 4, 0, 0, 0, 0,
  RFC2132_MAX_SIZE,2,	/* request as much as we can */
  sizeof(struct bootpd_t) / 256, sizeof(struct bootpd_t) % 256,
  /* request parameters */
  RFC2132_PARAM_LIST,
#ifdef GRUB
  /* 4 standard + 2 vendortags */
  4 + 2,
#else
# ifdef  IMAGE_FREEBSD
  /* 4 standard + 4 vendortags + 8 motd + 16 menu items */
  4 + 4 + 8 + 16,
# else
  /* 4 standard + 3 vendortags + 8 motd + 16 menu items */
  4 + 3 + 8 + 16,
# endif
#endif
  /* Standard parameters */
  RFC1533_NETMASK, RFC1533_GATEWAY,
  RFC1533_HOSTNAME, RFC1533_EXTENSIONPATH,
  /* Etherboot vendortags */
  RFC1533_VENDOR_MAGIC,
#ifdef GRUB
  RFC1533_VENDOR_CONFIGFILE,
#else /* ! GRUB */
# ifdef IMAGE_FREEBSD
  RFC1533_VENDOR_HOWTO,
# endif
  RFC1533_VENDOR_MNUOPTS, RFC1533_VENDOR_SELECTION,
  /* 8 MOTD entries */
  RFC1533_VENDOR_MOTD,
  RFC1533_VENDOR_MOTD + 1,
  RFC1533_VENDOR_MOTD + 2,
  RFC1533_VENDOR_MOTD + 3,
  RFC1533_VENDOR_MOTD + 4,
  RFC1533_VENDOR_MOTD + 5,
  RFC1533_VENDOR_MOTD + 6,
  RFC1533_VENDOR_MOTD + 7,
  /* 16 image entries */
  RFC1533_VENDOR_IMG,
  RFC1533_VENDOR_IMG + 1,
  RFC1533_VENDOR_IMG + 2,
  RFC1533_VENDOR_IMG + 3,
  RFC1533_VENDOR_IMG + 4,
  RFC1533_VENDOR_IMG + 5,
  RFC1533_VENDOR_IMG + 6,
  RFC1533_VENDOR_IMG + 7,
  RFC1533_VENDOR_IMG + 8,
  RFC1533_VENDOR_IMG + 9,
  RFC1533_VENDOR_IMG + 10,
  RFC1533_VENDOR_IMG + 11,
  RFC1533_VENDOR_IMG + 12,
  RFC1533_VENDOR_IMG + 13,
  RFC1533_VENDOR_IMG + 14,
  RFC1533_VENDOR_IMG + 15,
#endif /* ! GRUB */
};

#endif /* ! NO_DHCP_SUPPORT */

static const char broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void
print_network_configuration (void)
{
  static void sprint_ip_addr (char *buf, unsigned long addr)
    {
      grub_sprintf (buf, "%d.%d.%d.%d",
		    addr & 0xFF, (addr >> 8) & 0xFF,
		    (addr >> 16) & 0xFF, addr >> 24);
    }
  
  if (! eth_probe ())
    grub_printf ("No ethernet card found.\n");
  else if (! network_ready)
    grub_printf ("Not initialized yet.\n");
  else
    {
      char me[16], my_mask[16], server[16], gw[16];

      sprint_ip_addr (me, arptable[ARP_CLIENT].ipaddr.s_addr);
      sprint_ip_addr (my_mask, netmask);
      sprint_ip_addr (server, arptable[ARP_SERVER].ipaddr.s_addr);
      sprint_ip_addr (gw, arptable[ARP_GATEWAY].ipaddr.s_addr);
		       
      grub_printf ("Address: %s    Netmask: %s\nServer: %s    Gateway: %s\n",
		   me, my_mask, server, gw);
    }
}

/* Override the server IP address.  */
int
arp_server_override (const char *buffer)
{
  in_addr in;

  if (! inet_aton ((char *) buffer, &in))
    return 0;

  arptable[ARP_SERVER].ipaddr.s_addr = in.s_addr;
  return 1;
}

/**************************************************************************
DEFAULT_NETMASK - Return default netmask for IP address
**************************************************************************/
static inline unsigned long 
default_netmask (void)
{
  int net = ntohl (arptable[ARP_CLIENT].ipaddr.s_addr) >> 24;
  if (net <= 127)
    return (htonl (0xff000000));
  else if (net < 192)
    return (htonl (0xffff0000));
  else
    return (htonl (0xffffff00));
}

/**************************************************************************
UDP_TRANSMIT - Send a UDP datagram
**************************************************************************/
int 
udp_transmit (unsigned long destip, unsigned int srcsock,
	      unsigned int destsock, int len, const void *buf)
{
  struct iphdr *ip;
  struct udphdr *udp;
  struct arprequest arpreq;
  int arpentry, i;
  int retry;

  ip = (struct iphdr *) buf;
  udp = (struct udphdr *) ((unsigned long) buf + sizeof (struct iphdr));
  ip->verhdrlen = 0x45;
  ip->service = 0;
  ip->len = htons (len);
  ip->ident = 0;
  ip->frags = 0;
  ip->ttl = 60;
  ip->protocol = IP_UDP;
  ip->chksum = 0;
  ip->src.s_addr = arptable[ARP_CLIENT].ipaddr.s_addr;
  ip->dest.s_addr = destip;
  ip->chksum = ipchksum ((unsigned short *) buf, sizeof (struct iphdr));
  udp->src = htons (srcsock);
  udp->dest = htons (destsock);
  udp->len = htons (len - sizeof (struct iphdr));
  udp->chksum = 0;
  
  if (destip == IP_BROADCAST)
    {
      eth_transmit (broadcast, IP, len, buf);
    }
  else
    {
      if (((destip & netmask)
	   != (arptable[ARP_CLIENT].ipaddr.s_addr & netmask))
	  && arptable[ARP_GATEWAY].ipaddr.s_addr)
	destip = arptable[ARP_GATEWAY].ipaddr.s_addr;
      
      for (arpentry = 0; arpentry < MAX_ARP; arpentry++)
	if (arptable[arpentry].ipaddr.s_addr == destip)
	  break;
      
      if (arpentry == MAX_ARP)
	{
	  grub_printf ("%x is not in my arp table!\n", destip);
	  return 0;
	}
      
      for (i = 0; i < ETHER_ADDR_SIZE; i++)
	if (arptable[arpentry].node[i])
	  break;
      
      if (i == ETHER_ADDR_SIZE)
	{
	  /* Need to do arp request.  */
	  arpreq.hwtype = htons (1);
	  arpreq.protocol = htons (IP);
	  arpreq.hwlen = ETHER_ADDR_SIZE;
	  arpreq.protolen = 4;
	  arpreq.opcode = htons (ARP_REQUEST);
	  grub_memmove (arpreq.shwaddr, arptable[ARP_CLIENT].node,
			ETHER_ADDR_SIZE);
	  grub_memmove (arpreq.sipaddr, (char *) &arptable[ARP_CLIENT].ipaddr,
			sizeof (in_addr));
	  grub_memset (arpreq.thwaddr, 0, ETHER_ADDR_SIZE);
	  grub_memmove (arpreq.tipaddr, (char *) &destip, sizeof (in_addr));
	  
	  for (retry = 1; retry <= MAX_ARP_RETRIES; retry++)
	    {
	      eth_transmit (broadcast, ARP, sizeof (arpreq), &arpreq);
	      if (await_reply (AWAIT_ARP, arpentry, arpreq.tipaddr, TIMEOUT))
		goto xmit;

	      if (ip_abort)
		return 0;
	      
	      rfc951_sleep (retry);
	      /* We have slept for a while - the packet may
	       * have arrived by now.  If not, we have at
	       * least some room in the Rx buffer for the
	       * next reply.  */
	      if (await_reply (AWAIT_ARP, arpentry, arpreq.tipaddr, 0))
		goto xmit;
	      
	      if (ip_abort)
		return 0;
	    }
	  
	  return 0;
	}
      
    xmit:
      eth_transmit (arptable[arpentry].node, IP, len, buf);
    }
  
  return 1;
}

/**************************************************************************
TFTP - Download extended BOOTP data, or kernel image
**************************************************************************/
static int
tftp (const char *name, int (*fnc) (unsigned char *, int, int, int))
{
  int retry = 0;
  static unsigned short iport = 2000;
  unsigned short oport;
  unsigned short len, block = 0, prevblock = 0;
  int bcounter = 0;
  struct tftp_t *tr;
  struct tftp_t tp;
  int rc;
  int packetsize = TFTP_DEFAULTSIZE_PACKET;
  
  /* Clear out the Rx queue first.  It contains nothing of interest,
   * except possibly ARP requests from the DHCP/TFTP server.  We use
   * polling throughout Etherboot, so some time may have passed since we
   * last polled the receive queue, which may now be filled with
   * broadcast packets.  This will cause the reply to the packets we are
   * about to send to be lost immediately.  Not very clever.  */
  await_reply (AWAIT_QDRAIN, 0, NULL, 0);
  
  tp.opcode = htons (TFTP_RRQ);
  len = (grub_sprintf ((char *)tp.u.rrq, "%s%coctet%cblksize%c%d",
		       name, 0, 0, 0, TFTP_MAX_PACKET)
	 + TFTP_MIN_PACKET + 1);
  if (! udp_transmit (arptable[ARP_SERVER].ipaddr.s_addr, ++iport,
		      TFTP_PORT, len, &tp))
    return 0;
  
  for (;;)
    {
#ifdef CONGESTED
      if (! await_reply (AWAIT_TFTP, iport, NULL,
			 (block ? TFTP_REXMT : TIMEOUT)))
#else
      if (! await_reply (AWAIT_TFTP, iport, NULL, TIMEOUT))
#endif
	{
	  if (! block && retry++ < MAX_TFTP_RETRIES)
	    {
	      /* Maybe initial request was lost.  */
	      rfc951_sleep (retry);
	      if (! udp_transmit (arptable[ARP_SERVER].ipaddr.s_addr,
				  ++iport, TFTP_PORT, len, &tp))
		return 0;
	      
	      continue;
	    }
	  
#ifdef CONGESTED
	  if (block && ((retry += TFTP_REXMT) < TFTP_TIMEOUT))
	    {
	      /* We resend our last ack.  */
#ifdef MDEBUG
	      grub_printf ("<REXMT>\n");
#endif
	      udp_transmit (arptable[ARP_SERVER].ipaddr.s_addr,
			    iport, oport,
			    TFTP_MIN_PACKET, &tp);
	      continue;
	    }
#endif
	  /* Timeout.  */
	  break;
	}
      
      tr = (struct tftp_t *) &nic.packet[ETHER_HDR_SIZE];
      if (tr->opcode == ntohs (TFTP_ERROR))
	{
	  grub_printf ("TFTP error %d (%s)\n",
		       ntohs (tr->u.err.errcode),
		       tr->u.err.errmsg);
	  break;
	}
      
      if (tr->opcode == ntohs (TFTP_OACK))
	{
	  char *p = tr->u.oack.data, *e;
	  
	  /* Shouldn't happen.  */
	  if (prevblock)
	    /* Ignore it.  */
	    continue;
	  
	  len = ntohs (tr->udp.len) - sizeof (struct udphdr) - 2;
	  if (len > TFTP_MAX_PACKET)
	    goto noak;
	  
	  e = p + len;
	  while (*p != '\000' && p < e)
	    {
	      if (! grub_strcmp ("blksize", p))
		{
		  p += 8;
		  if ((packetsize = getdec (&p)) < TFTP_DEFAULTSIZE_PACKET)
		    goto noak;
		  
		  while (p < e && *p)
		    p++;
		  
		  if (p < e)
		    p++;
		}
	      else
		{
		noak:
		  tp.opcode = htons (TFTP_ERROR);
		  tp.u.err.errcode = 8;
		  len = (grub_sprintf ((char *) tp.u.err.errmsg,
				       "RFC1782 error")
			 + TFTP_MIN_PACKET + 1);
		  udp_transmit (arptable[ARP_SERVER].ipaddr.s_addr,
				iport, ntohs (tr->udp.src),
				len, &tp);
		  return 0;
		}
	    }
	  
	  if (p > e)
	    goto noak;
	  
	  /* This ensures that the packet does not get processed as data!  */
	  block = tp.u.ack.block = 0; 
	}
      else if (tr->opcode == ntohs (TFTP_DATA))
	{
	  len = ntohs (tr->udp.len) - sizeof (struct udphdr) - 4;
	  /* Shouldn't happen.  */
	  if (len > packetsize)
	    /* Ignore it.  */
	    continue;
	  
	  block = ntohs (tp.u.ack.block = tr->u.data.block);
	}
      else
	/* Neither TFTP_OACK nor TFTP_DATA.  */
	break;
      
      if ((block || bcounter) && (block != prevblock + 1))
	/* Block order should be continuous */
	tp.u.ack.block = htons (block = prevblock);
      
      /* Should be continuous.  */
      tp.opcode = htons (TFTP_ACK);
      oport = ntohs (tr->udp.src);
      /* Ack.  */
      udp_transmit (arptable[ARP_SERVER].ipaddr.s_addr, iport,
		    oport, TFTP_MIN_PACKET, &tp);
      
      if ((unsigned short) (block - prevblock) != 1)
	/* Retransmission or OACK, don't process via callback
	 * and don't change the value of prevblock.  */
	continue;
      
      prevblock = block;
      /* Is it the right place to zero the timer?  */
      retry = 0;
      
      if ((rc = fnc (tr->u.data.download,
		     ++bcounter, len, len < packetsize)) >= 0)
	return rc;

      /* End of data.  */
      if (len < packetsize)           
	return 1;
    }
  
  return 0;
}

/**************************************************************************
RARP - Get my IP address and load information
**************************************************************************/
int 
rarp (void)
{
  int retry;

  /* arp and rarp requests share the same packet structure.  */
  struct arprequest rarpreq;

  /* Make sure that an ethernet is probed.  */
  if (! eth_probe ())
    return 0;

  /* Clear the ready flag.  */
  network_ready = 0;
  
  memset (&rarpreq, 0, sizeof (rarpreq));

  rarpreq.hwtype = htons (1);
  rarpreq.protocol = htons (IP);
  rarpreq.hwlen = ETHER_ADDR_SIZE;
  rarpreq.protolen = 4;
  rarpreq.opcode = htons (RARP_REQUEST);
  grub_memmove ((char *) &rarpreq.shwaddr, arptable[ARP_CLIENT].node,
		ETHER_ADDR_SIZE);
  /* sipaddr is already zeroed out */
  grub_memmove ((char *) &rarpreq.thwaddr, arptable[ARP_CLIENT].node,
		ETHER_ADDR_SIZE);
  /* tipaddr is already zeroed out */

  for (retry = 0; retry < MAX_ARP_RETRIES; rfc951_sleep (++retry))
    {
      eth_transmit (broadcast, RARP, sizeof (rarpreq), &rarpreq);

      if (await_reply (AWAIT_RARP, 0, rarpreq.shwaddr, TIMEOUT))
	break;

      if (ip_abort)
	return 0;
    }

  if (retry < MAX_ARP_RETRIES)
    {
      network_ready = 1;
      return 1;
    }

  return 0;
}

/**************************************************************************
BOOTP - Get my IP address and load information
**************************************************************************/
int 
bootp (void)
{
  int retry;
#ifndef	NO_DHCP_SUPPORT
  int retry1;
#endif /* ! NO_DHCP_SUPPORT */
  struct bootp_t bp;
  unsigned long starttime;
#ifdef T509HACK
  int flag;

  flag = 1;
#endif

  /* Make sure that an ethernet is probed.  */
  if (! eth_probe ())
    return 0;

  /* Clear the ready flag.  */
  network_ready = 0;
  
  grub_memset (&bp, 0, sizeof (struct bootp_t));
  bp.bp_op = BOOTP_REQUEST;
  bp.bp_htype = 1;
  bp.bp_hlen = ETHER_ADDR_SIZE;
  bp.bp_xid = xid = starttime = currticks ();
  grub_memmove (bp.bp_hwaddr, arptable[ARP_CLIENT].node, ETHER_ADDR_SIZE);
#ifdef	NO_DHCP_SUPPORT
  /* Request RFC-style options.  */
  grub_memmove (bp.bp_vend, rfc1533_cookie, 5);
#else
  /* Request RFC-style options.  */
  grub_memmove (bp.bp_vend, rfc1533_cookie, sizeof rfc1533_cookie);
  grub_memmove (bp.bp_vend + sizeof rfc1533_cookie, dhcpdiscover,
		sizeof dhcpdiscover);
  grub_memmove (bp.bp_vend + sizeof rfc1533_cookie + sizeof dhcpdiscover,
		rfc1533_end, sizeof rfc1533_end);
#endif /* ! NO_DHCP_SUPPORT */

  for (retry = 0; retry < MAX_BOOTP_RETRIES;)
    {
      /* Clear out the Rx queue first.  It contains nothing of
       * interest, except possibly ARP requests from the DHCP/TFTP
       * server.  We use polling throughout Etherboot, so some time
       * may have passed since we last polled the receive queue,
       * which may now be filled with broadcast packets.  This will
       * cause the reply to the packets we are about to send to be
       * lost immediately.  Not very clever.  */
      await_reply (AWAIT_QDRAIN, 0, NULL, 0);

      udp_transmit (IP_BROADCAST, BOOTP_CLIENT, BOOTP_SERVER,
		    sizeof (struct bootp_t), &bp);
#ifdef T509HACK
      if (flag)
	{
	  flag--;
	  continue;
	}
#endif /* T509HACK */
      
#ifdef NO_DHCP_SUPPORT
      if (await_reply (AWAIT_BOOTP, 0, NULL, TIMEOUT))
	{
	  network_ready = 1;
	  return 1;
	}
#else /* ! NO_DHCP_SUPPORT */
      if (await_reply (AWAIT_BOOTP, 0, NULL, TIMEOUT))
	{
	  if (dhcp_reply == DHCPOFFER)
	    {
	      dhcp_reply = 0;
	      grub_memmove (bp.bp_vend, rfc1533_cookie, sizeof rfc1533_cookie);
	      grub_memmove (bp.bp_vend + sizeof rfc1533_cookie,
			    dhcprequest, sizeof dhcprequest);
	      grub_memmove (bp.bp_vend + sizeof rfc1533_cookie
			    + sizeof dhcprequest,
			    rfc1533_end, sizeof rfc1533_end);
	      grub_memmove (bp.bp_vend + 9, (char *) &dhcp_server,
			    sizeof (in_addr));
	      grub_memmove (bp.bp_vend + 15, (char *) &dhcp_addr,
			    sizeof (in_addr));
	      for (retry1 = 0; retry1 < MAX_BOOTP_RETRIES;)
		{
		  udp_transmit (IP_BROADCAST, 0, BOOTP_SERVER,
				sizeof (struct bootp_t), &bp);
		  dhcp_reply = 0;
		  if (await_reply (AWAIT_BOOTP, 0, NULL, TIMEOUT))
		    if (dhcp_reply == DHCPACK)
		      {
			network_ready = 1;
			return 1;
		      }

		  if (ip_abort)
		    return 0;
		  
		  rfc951_sleep (++retry1);
		}

	      /* Timeout.  */
	      return 0;
	    }
	  else
	    {
	      network_ready = 1;
	      return 1;
	    }
	}
#endif /* ! NO_DHCP_SUPPORT */
      
      if (ip_abort)
	return 0;
	  
      rfc951_sleep (++retry);
      bp.bp_secs = htons ((currticks () - starttime) / 20);
    }

  /* Timeout.  */
  return 0;
}

/**************************************************************************
AWAIT_REPLY - Wait until we get a response for our request
**************************************************************************/
int 
await_reply (int type, int ival, void *ptr, int timeout)
{
  unsigned long time;
  struct iphdr *ip;
  struct udphdr *udp;
  struct arprequest *arpreply;
  struct bootp_t *bootpreply;
  unsigned short ptype;
  unsigned int protohdrlen = (ETHER_HDR_SIZE + sizeof (struct iphdr)
			      + sizeof (struct udphdr));

  /* Clear the abort flag.  */
  ip_abort = 0;
  
  time = currticks () + TIMEOUT;
  /* The timeout check is done below.  The timeout is only checked if
   * there is no packet in the Rx queue.  This assumes that eth_poll()
   * needs a negligible amount of time.  */
  for (;;)
    {
      if (eth_poll ())
	{
	  /* We have something!  */
	  
	  /* Check for ARP - No IP hdr.  */
	  if (nic.packetlen >= ETHER_HDR_SIZE)
	    {
	      ptype = (((unsigned short) nic.packet[12]) << 8
		       | ((unsigned short) nic.packet[13]));
	    }
	  else
	    /* What else could we do with it?  */
	    continue;
	  
	  if (nic.packetlen >= ETHER_HDR_SIZE + sizeof (struct arprequest)
	      && ptype == ARP)
	    {
	      unsigned long tmp;

	      arpreply = (struct arprequest *) &nic.packet[ETHER_HDR_SIZE];
	      
	      if (arpreply->opcode == ntohs (ARP_REPLY)
		  && ! grub_memcmp (arpreply->sipaddr, ptr, sizeof (in_addr))
		  && type == AWAIT_ARP)
		{
		  grub_memmove ((char *) arptable[ival].node,
				arpreply->shwaddr,
				ETHER_ADDR_SIZE);
		  return 1;
		}
	      
	      grub_memmove ((char *) &tmp, arpreply->tipaddr,
			    sizeof (in_addr));
	      
	      if (arpreply->opcode == ntohs (ARP_REQUEST)
		  && tmp == arptable[ARP_CLIENT].ipaddr.s_addr)
		{
		  arpreply->opcode = htons (ARP_REPLY);
		  grub_memmove (arpreply->tipaddr, arpreply->sipaddr,
				sizeof (in_addr));
		  grub_memmove (arpreply->thwaddr, (char *) arpreply->shwaddr,
				ETHER_ADDR_SIZE);
		  grub_memmove (arpreply->sipaddr,
				(char *) &arptable[ARP_CLIENT].ipaddr,
				sizeof (in_addr));
		  grub_memmove (arpreply->shwaddr,
				arptable[ARP_CLIENT].node,
				ETHER_ADDR_SIZE);
		  eth_transmit (arpreply->thwaddr, ARP,
				sizeof (struct arprequest),
				arpreply);
#ifdef MDEBUG
		  grub_memmove (&tmp, arpreply->tipaddr, sizeof (in_addr));
		  grub_printf ("Sent ARP reply to: %x\n", tmp);
#endif	/* MDEBUG */
		}
	      
	      continue;
	    }

	  if (type == AWAIT_QDRAIN)
	    {
	      continue;
	    }
	  
	  /* Check for RARP - No IP hdr.  */
	  if (type == AWAIT_RARP
	      && nic.packetlen >= ETHER_HDR_SIZE + sizeof (struct arprequest)
	      && ptype == RARP)
	    {
	      arpreply = (struct arprequest *) &nic.packet[ETHER_HDR_SIZE];
	      
	      if (arpreply->opcode == ntohs (RARP_REPLY)
		  && ! grub_memcmp (arpreply->thwaddr, ptr, ETHER_ADDR_SIZE))
		{
		  grub_memmove ((char *) arptable[ARP_SERVER].node,
				arpreply->shwaddr, ETHER_ADDR_SIZE);
		  grub_memmove ((char *) &arptable[ARP_SERVER].ipaddr,
				arpreply->sipaddr, sizeof (in_addr));
		  grub_memmove ((char *) &arptable[ARP_CLIENT].ipaddr,
				arpreply->tipaddr, sizeof (in_addr));
		  return 1;
		}
	      
	      continue;
	    }

	  /* Anything else has IP header.  */
	  if (nic.packetlen < protohdrlen || ptype != IP)
	    continue;
	  
	  ip = (struct iphdr *) &nic.packet[ETHER_HDR_SIZE];
	  if (ip->verhdrlen != 0x45
	      || ipchksum ((unsigned short *) ip, sizeof (struct iphdr))
	      || ip->protocol != IP_UDP)
	    continue;
	  
	  udp = (struct udphdr *)
	    &nic.packet[ETHER_HDR_SIZE + sizeof (struct iphdr)];
	  
	  /* BOOTP ?  */
	  bootpreply = (struct bootp_t *) &nic.packet[ETHER_HDR_SIZE];
	  if (type == AWAIT_BOOTP
#ifdef NO_DHCP_SUPPORT
	      && (nic.packetlen
		  >= (ETHER_HDR_SIZE + sizeof (struct bootp_t)))
#else
	      && (nic.packetlen
		  >= (ETHER_HDR_SIZE + sizeof (struct bootp_t)) - DHCP_OPT_LEN)
#endif /* ! NO_DHCP_SUPPORT */
	      && ntohs (udp->dest) == BOOTP_CLIENT
	      && bootpreply->bp_op == BOOTP_REPLY
	      && bootpreply->bp_xid == xid)
	    {
	      arptable[ARP_CLIENT].ipaddr.s_addr
		= bootpreply->bp_yiaddr.s_addr;
#ifndef	NO_DHCP_SUPPORT
	      dhcp_addr.s_addr = bootpreply->bp_yiaddr.s_addr;
#endif /* ! NO_DHCP_SUPPORT */
	      netmask = default_netmask ();
	      arptable[ARP_SERVER].ipaddr.s_addr
		= bootpreply->bp_siaddr.s_addr;
	      /* Kill arp.  */
	      grub_memset (arptable[ARP_SERVER].node, 0, ETHER_ADDR_SIZE);
	      arptable[ARP_GATEWAY].ipaddr.s_addr
		= bootpreply->bp_giaddr.s_addr;
	      /* Kill arp.  */
	      grub_memset (arptable[ARP_GATEWAY].node, 0, ETHER_ADDR_SIZE);

	      /* GRUB doesn't autoload any kernel image.  */
#ifndef GRUB
	      if (bootpreply->bp_file[0])
		{
		  grub_memmove (kernel_buf, bootpreply->bp_file, 128);
		  kernel = kernel_buf;
		}
#endif /* ! GRUB */
	      
	      grub_memmove ((char *) BOOTP_DATA_ADDR, (char *) bootpreply,
			    sizeof (struct bootpd_t));
#ifdef NO_DHCP_SUPPORT
	      decode_rfc1533 (BOOTP_DATA_ADDR->bootp_reply.bp_vend,
			      0, BOOTP_VENDOR_LEN + MAX_BOOTP_EXTLEN, 1);
#else
	      decode_rfc1533 (BOOTP_DATA_ADDR->bootp_reply.bp_vend,
			      0, DHCP_OPT_LEN, 1);
#endif /* ! NO_DHCP_SUPPORT */
	      
	      return 1;
	    }
	  
	  /* TFTP ? */
	  if (type == AWAIT_TFTP && ntohs (udp->dest) == ival)
	    return 1;
	}
      else
	{
	  /* Check for abort key only if the Rx queue is empty -
	   * as long as we have something to process, don't
	   * assume that something failed.  It is unlikely that
	   * we have no processing time left between packets.  */
	  if (checkkey () != -1 && ASCII_CHAR (getkey ()) == CTRL_C)
	    {
	      ip_abort = 1;
	      return 0;
	    }
	  
	  /* Do the timeout after at least a full queue walk.  */
	  if ((timeout == 0) || (currticks() > time))
	    {
	      break;
	    }
	}
    }
  
  return 0;
}

/**************************************************************************
DECODE_RFC1533 - Decodes RFC1533 header
**************************************************************************/
int
decode_rfc1533 (unsigned char *p, int block, int len, int eof)
{
  static unsigned char *extdata = NULL, *extend = NULL;
  unsigned char *extpath = NULL;
  unsigned char *endp;
  
  if (block == 0)
    {
#ifdef IMAGE_MENU
      memset (imagelist, 0, sizeof (imagelist));
      menudefault = useimagemenu = 0;
      menutmo = -1;
#endif
#ifdef MOTD
      memset (motd, 0, sizeof (motd));
#endif
      
      end_of_rfc1533 = NULL;
      vendorext_isvalid = 0;
      
      if (grub_memcmp (p, rfc1533_cookie, 4))
	/* no RFC 1533 header found */
	return 0;
      
      p += 4;
      endp = p + len;
    }
  else
    {
      if (block == 1)
	{
	  if (grub_memcmp (p, rfc1533_cookie, 4))
	    /* no RFC 1533 header found */
	    return 0;
	  
	  p += 4;
	  len -= 4;
	}
      
      if (extend + len
	  <= ((unsigned char *)
	      &(BOOTP_DATA_ADDR->bootp_extension[MAX_BOOTP_EXTLEN])))
	{
	  grub_memmove (extend, p, len);
	  extend += len;
	}
      else
	{
	  grub_printf ("Overflow in vendor data buffer! Aborting...\n");
	  *extdata = RFC1533_END;
	  return 0;
	}
      
      p = extdata;
      endp = extend;
    }
  
  if (eof)
    {
      while (p < endp)
	{
	  unsigned char c = *p;
	  
	  if (c == RFC1533_PAD)
	    {
	      p++;
	      continue;
	    }
	  else if (c == RFC1533_END)
	    {
	      end_of_rfc1533 = endp = p;
	      continue;
	    }
	  else if (c == RFC1533_NETMASK)
	    {
	      grub_memmove ((char *) &netmask, p + 2, sizeof (in_addr));
	    }
	  else if (c == RFC1533_GATEWAY)
	    {
	      /* This is a little simplistic, but it will
	         usually be sufficient.
	         Take only the first entry.  */
	      if (TAG_LEN (p) >= sizeof (in_addr))
		grub_memmove ((char *) &arptable[ARP_GATEWAY].ipaddr, p + 2,
			      sizeof (in_addr));
	    }
	  else if (c == RFC1533_EXTENSIONPATH)
	    extpath = p;
#ifndef	NO_DHCP_SUPPORT
	  else if (c == RFC2132_MSG_TYPE)
	    {
	      dhcp_reply = *(p + 2);
	    }
	  else if (c == RFC2132_SRV_ID)
	    {
	      grub_memmove ((char *) &dhcp_server, p + 2, sizeof (in_addr));
	    }
#endif /* ! NO_DHCP_SUPPORT */
	  
	  else if (c == RFC1533_VENDOR_MAGIC
# ifndef IMAGE_FREEBSD   /* since FreeBSD uses tag 128 for swap definition */
		   && TAG_LEN(p) >= 6 &&
		   !memcmp(p+2,vendorext_magic,4) &&
		   p[6] == RFC1533_VENDOR_MAJOR
# endif
		   )
	    vendorext_isvalid++;
	  
	  /* GRUB now handles its own tag. Get the name of a configuration
	     file from the network. Cool...  */
#ifdef GRUB
	  else if (c == RFC1533_VENDOR_CONFIGFILE)
	    {
	      int len = TAG_LEN (p);

	      /* Eliminate the trailing NULs according to RFC 2132.  */
	      while (*(p + 2 + len - 1) == '\000' && len > 0)
		len--;

	      /* XXX: Should check if LEN is less than the maximum length
		 of CONFIG_FILE. This kind of robustness will be a goal
		 in GRUB 1.0.  */
	      grub_memmove (config_file, p + 2, len);
	      config_file[len] = 0;
	    }
#else /* ! GRUB */

# ifdef  IMAGE_FREEBSD
	  else if (c == RFC1533_VENDOR_HOWTO) {
	    freebsd_howto = ((p[2]*256+p[3])*256+p[4])*256+p[5];
	  }
# endif
# ifdef IMAGE_MENU
	  else if (c == RFC1533_VENDOR_MNUOPTS)
	    {
	      parse_menuopts (p + 2, TAG_LEN (p));
	    }
	  else if (c >= RFC1533_VENDOR_IMG &&
		   c < RFC1533_VENDOR_IMG + RFC1533_VENDOR_NUMOFIMG)
	    {
	      imagelist[c - RFC1533_VENDOR_IMG] = p;
	      useimagemenu++;
	    }
# endif
# ifdef MOTD
	  else if (c >= RFC1533_VENDOR_MOTD &&
		   c < RFC1533_VENDOR_MOTD +
		   RFC1533_VENDOR_NUMOFMOTD)
	    motd[c - RFC1533_VENDOR_MOTD] = p;
# endif
	  else
	    {
# if 0
	      printf("Unknown RFC1533-tag ");
	      for(q=p;q<p+2+TAG_LEN(p);q++)
		printf("%x ",*q);
	      putchar('\n');
# endif
	    }
#endif /* ! GRUB */
	  
	  p += TAG_LEN (p) + 2;
	}
      
      extdata = extend = endp;

      /* Perhaps we can eliminate this because we doesn't require so
	 much information, but I leave this alone.  */
      if (block == 0 && extpath != NULL)
	{
	  char fname[64];
	  int fnamelen = TAG_LEN (extpath);

	  while (*(extpath + 2 + fnamelen - 1) == '\000' && fnamelen > 0)
	    fnamelen--;

	  if (fnamelen + 1 > sizeof (fname))
	    {
	      grub_printf ("Too long file name for Extensions Path\n");
	      return 0;
	    }
	  else if (! fnamelen)
	    {
	      grub_printf ("Empty file name for Extensions Path\n");
	      return 0;
	    }
	  
	  grub_memmove (fname, extpath + 2, fnamelen);
	  fname[fnamelen] = '\000';
	  grub_printf ("Loading BOOTP-extension file: %s\n", fname);
	  tftp (fname, decode_rfc1533);
	}
    }
  
  /* Proceed with next block.  */
  return -1;
}

/**************************************************************************
IPCHKSUM - Checksum IP Header
**************************************************************************/
unsigned short 
ipchksum (unsigned short *ip, int len)
{
  unsigned long sum = 0;
  len >>= 1;
  while (len--)
    {
      sum += *(ip++);
      if (sum > 0xFFFF)
	sum -= 0xFFFF;
    }
  return (~sum) & 0x0000FFFF;
}

/**************************************************************************
RFC951_SLEEP - sleep for expotentially longer times
**************************************************************************/
void
rfc951_sleep (int exp)
{
  static long seed = 0;
  long q;
  unsigned long tmo;

#ifdef BACKOFF_LIMIT
  if (exp > BACKOFF_LIMIT)
    exp = BACKOFF_LIMIT;
#endif
  
  if (! seed)
    /* Initialize linear congruential generator.  */
    seed = (currticks () + *(long *) &arptable[ARP_CLIENT].node
	    + ((short *) arptable[ARP_CLIENT].node)[2]);
  
  /* Simplified version of the LCG given in Bruce Scheier's
     "Applied Cryptography".  */
  q = seed / 53668;
  if ((seed = 40014 * (seed - 53668 * q) - 12211 * q) < 0)
    seed += 2147483563l;
  
  /* Compute mask.  */
  for (tmo = 63; tmo <= 60 * TICKS_PER_SEC && --exp > 0; tmo = 2 * tmo + 1)
    ;
  
  /* Sleep.  */
  grub_printf ("<sleep>\n");
  
  for (tmo = (tmo & seed) + currticks (); currticks () < tmo;)
    /* If the user interrupts, return immediately.  */ 
    if (checkkey () != -1 && ASCII_CHAR (getkey ()) == CTRL_C)
      break;
}

/**************************************************************************
CLEANUP_NET - shut down networking
**************************************************************************/
void
cleanup_net (void)
{
  if (network_ready)
    {
#ifdef DOWNLOAD_PROTO_NFS
      nfs_umountall (ARP_SERVER);
#endif
      eth_disable ();
      network_ready = 0;
    }
}

#ifndef GRUB
/**************************************************************************
CLEANUP - shut down etherboot so that the OS may be called right away
**************************************************************************/
void
cleanup (void)
{
#if    defined(ANSIESC) && defined(CONSOLE_CRT)
  ansi_reset ();
#endif
}
#endif /* ! GRUB */
