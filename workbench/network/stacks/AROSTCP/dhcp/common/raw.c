/* socket.c

   BSD raw socket interface code... */

/* XXX

   It's not clear how this should work, and that lack of clarity is
   terribly detrimental to the NetBSD 1.1 kernel - it crashes and
   burns.

   Using raw sockets ought to be a big win over using BPF or something
   like it, because you don't need to deal with the complexities of
   the physical layer, but it appears not to be possible with existing
   raw socket implementations.  This may be worth revisiting in the
   future.  For now, this code can probably be considered a curiosity.
   Sigh. */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1995-2003 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id$ Copyright (c) 2004 Internet Systems Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

#if defined (USE_RAW_SEND)
#include <sys/uio.h>

/* Generic interface registration routine... */
void if_register_send (info)
	struct interface_info *info;
{
	struct sockaddr_in name;
	int sock;
	struct socklist *tmp;
	int flag;

	/* Set up the address we're going to connect to. */
	name.sin_family = AF_INET;
	name.sin_port = local_port;
	name.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	memset (name.sin_zero, 0, sizeof (name.sin_zero));

	/* List addresses on which we're listening. */
	if (!quiet_interface_discovery)
		log_info ("Sending on %s, port %d",
		      inet_ntoa (info->primary_address), htons (local_port));
	if ((sock = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		log_fatal ("Can't create dhcp socket: %m");

	/* Set the BROADCAST option so that we can broadcast DHCP responses. */
	flag = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST,
			&flag, sizeof flag) < 0)
		log_fatal ("Can't set SO_BROADCAST option on dhcp socket: %m");

	/* Set the IP_HDRINCL flag so that we can supply our own IP
	   headers... */
	if (setsockopt (sock, IPPROTO_IP, IP_HDRINCL, &flag, sizeof flag) < 0)
		log_fatal ("Can't set IP_HDRINCL flag: %m");

	info -> wfdesc = sock;
        if (!quiet_interface_discovery)
		log_info ("Sending on   Raw/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

void if_deregister_send (info)
	struct interface_info *info;
{
#ifdef SOCKET_IS_NOT_A_FILE
	CloseSocket (info -> wfdesc);
#else
	close (info -> wfdesc);
#endif
	info -> wfdesc = -1;

        if (!quiet_interface_discovery)
		log_info ("Disabling output on Raw/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

ssize_t send_packet (interface, packet, raw, len, from, to, hto)
	struct interface_info *interface;
	struct packet *packet;
	struct dhcp_packet *raw;
	size_t len;
	struct in_addr from;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	unsigned char buf [256];
	int bufp = 0;
	int result;

	/* Assemble the headers... */
	assemble_udp_ip_header (interface, buf, &bufp, from.s_addr,
				to -> sin_addr.s_addr, to -> sin_port,
				(unsigned char *)raw, len);

	/* Fire it off */
	result = sendto(interface -> wfdesc, buf, bufp, 0, (struct sockaddr *)to, sizeof *to);
	if (result >= 0)
		result = sendto(interface -> wfdesc, buf, bufp, 0, (struct sockaddr *)to, sizeof *to);
	if (result < 0)
		log_error ("send_packet: %m");
	return result;
}

int can_unicast_without_arp (ip)
	struct interface_info *ip;
{
	return 0;
}

int supports_multiple_interfaces (ip)
	struct interface_info *ip;
{
	return 0;
}

void maybe_setup_fallback ()
{
}

void if_reinitialize_send (info)
	struct interface_info *info;
{
}

int can_receive_unicast_unconfigured (ip)
	struct interface_info *ip;
{
	return 0;
}

#endif /* USE_SOCKET_SEND */
