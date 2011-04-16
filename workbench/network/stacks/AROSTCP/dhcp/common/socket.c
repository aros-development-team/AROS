/* socket.c

   BSD socket interface code... */

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

/* SO_BINDTODEVICE support added by Elliot Poger (poger@leland.stanford.edu).
 * This sockopt allows a socket to be bound to a particular interface,
 * thus enabling the use of DHCPD on a multihomed host.
 * If SO_BINDTODEVICE is defined in your system header files, the use of
 * this sockopt will be automatically enabled. 
 * I have implemented it under Linux; other systems should be doable also.
 */

#if 0
static char copyright[] =
"$Id$ Copyright (c) 2004 Internet Systems Consortium.  All rights reserved.\n";
#endif

#include "dhcpd.h"

#ifdef USE_SOCKET_FALLBACK
# if !defined (USE_SOCKET_SEND)
#  define if_register_send if_register_fallback
#  define send_packet send_fallback
#  define if_reinitialize_send if_reinitialize_fallback
# endif
#endif

static int once = 0;

/* Reinitializes the specified interface after an address change.   This
   is not required for packet-filter APIs. */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
void if_reinitialize_send (info)
	struct interface_info *info;
{
#if 0
#ifndef USE_SOCKET_RECEIVE	/* ***CHECK HERE*** */
	once = 0;
#ifdef SOCKET_IS_NOT_A_FILE
	CloseSocket (info -> wfdesc);
#else
	close (info -> wfdesc);
#endif
#endif
	if_register_send (info);
#endif
}
#endif

#ifdef USE_SOCKET_RECEIVE
void if_reinitialize_receive (info)
	struct interface_info *info;
{
#if 0
	once = 0;
#ifdef SOCKET_IS_NOT_A_FILE
	CloseSocket (info -> rfdesc);
#else
	close (info -> rfdesc);
#endif
	if_register_receive (info);
#endif
}
#endif

#if defined (USE_SOCKET_SEND) || \
	defined (USE_SOCKET_RECEIVE) || \
		defined (USE_SOCKET_FALLBACK)
/* Generic interface registration routine... */
int if_register_socket (info)
	struct interface_info *info;
{
	struct sockaddr_in name;
	int sock;
	int flag;

#if !defined (HAVE_SO_BINDTODEVICE) && !defined (USE_FALLBACK)
	/* Make sure only one interface is registered. */
	if (once)
		log_fatal ("The standard socket API can only support %s",
		       "hosts with a single network interface.");
	once = 1;
#endif

	memset (&name, 0, sizeof (name));
	/* Set up the address we're going to bind to. */
	name.sin_family = AF_INET;
	name.sin_port = local_port;
	name.sin_addr = local_address;

	/* Make a socket... */
	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		log_fatal ("Can't create dhcp socket: %m");

	/* Set the REUSEADDR option so that we don't fail to start if
	   we're being restarted. */
	flag = 1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&flag, sizeof flag) < 0)
		log_fatal ("Can't set SO_REUSEADDR option on dhcp socket: %m");

	/* Set the BROADCAST option so that we can broadcast DHCP responses.
	   We shouldn't do this for fallback devices, and we can detect that
	   a device is a fallback because it has no ifp structure. */
	if (info -> ifp &&
	    (setsockopt (sock, SOL_SOCKET, SO_BROADCAST,
			 (char *)&flag, sizeof flag) < 0))
		log_fatal ("Can't set SO_BROADCAST option on dhcp socket: %m");

	/* Bind the socket to this interface's IP address. */
	if (bind (sock, (struct sockaddr *)&name, sizeof name) < 0) {
		log_error ("Can't bind to dhcp address: %m");
		log_error ("Please make sure there is no other dhcp server");
		log_error ("running and that there's no entry for dhcp or");
		log_error ("bootp in /etc/inetd.conf.   Also make sure you");
		log_error ("are not running HP JetAdmin software, which");
		log_fatal ("includes a bootp server.");
	}

#if defined (HAVE_SO_BINDTODEVICE)
	/* Bind this socket to this interface. */
	if (info -> ifp &&
	    setsockopt (sock, SOL_SOCKET, SO_BINDTODEVICE,
			(char *)(info -> ifp), sizeof *(info -> ifp)) < 0) {
		log_fatal ("setsockopt: SO_BINDTODEVICE: %m");
	}
#endif

	return sock;
}
#endif /* USE_SOCKET_SEND || USE_SOCKET_RECEIVE || USE_SOCKET_FALLBACK */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
void if_register_send (info)
	struct interface_info *info;
{
#ifndef USE_SOCKET_RECEIVE
	info -> wfdesc = if_register_socket (info);
#if defined (USE_SOCKET_FALLBACK)
	/* Fallback only registers for send, but may need to receive as
	   well. */
	info -> rfdesc = info -> wfdesc;
#endif
#else
	info -> wfdesc = info -> rfdesc;
#endif
	if (!quiet_interface_discovery)
		log_info ("Sending on   Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

#if defined (USE_SOCKET_SEND)
void if_deregister_send (info)
	struct interface_info *info;
{
#ifndef USE_SOCKET_RECEIVE
#ifdef SOCKET_IS_NOT_A_FILE		/* ***CHECK HERE*** */
	CloseSocket(info -> wfdesc);
#else
	close (info -> wfdesc);
#endif
#endif
	info -> wfdesc = -1;

	if (!quiet_interface_discovery)
		log_info ("Disabling output on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_SOCKET_SEND */
#endif /* USE_SOCKET_SEND || USE_SOCKET_FALLBACK */

#ifdef USE_SOCKET_RECEIVE
void if_register_receive (info)
	struct interface_info *info;
{
	/* If we're using the socket API for sending and receiving,
	   we don't need to register this interface twice. */
	info -> rfdesc = if_register_socket (info);
	if (!quiet_interface_discovery)
		log_info ("Listening on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}

void if_deregister_receive (info)
	struct interface_info *info;
{
#ifdef SOCKET_IS_NOT_A_FILE
	CloseSocket (info -> rfdesc);
#else
	close (info -> rfdesc);
#endif
	info -> rfdesc = -1;

	if (!quiet_interface_discovery)
		log_info ("Disabling input on Socket/%s%s%s",
		      info -> name,
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));
}
#endif /* USE_SOCKET_RECEIVE */

#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_FALLBACK)
ssize_t send_packet (interface, packet, raw, len, from, to, hto)
	struct interface_info *interface;
	struct packet *packet;
	struct dhcp_packet *raw;
	size_t len;
	struct in_addr from;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	int result;
#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
	do {
#endif
		result = sendto (interface -> wfdesc, (char *)raw, len, 0,
				 (struct sockaddr *)to, sizeof *to);
#ifdef IGNORE_HOSTUNREACH
	} while (to -> sin_addr.s_addr == htonl (INADDR_BROADCAST) &&
		 result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	if (result < 0) {
		log_error ("send_packet: %m");
		if (errno == ENETUNREACH)
			log_error ("send_packet: please consult README file%s",
				   " regarding broadcast address.");
	}
	return result;
}
#endif /* USE_SOCKET_SEND || USE_SOCKET_FALLBACK */

#ifdef USE_SOCKET_RECEIVE
ssize_t receive_packet (interface, buf, len, from, hfrom)
	struct interface_info *interface;
	unsigned char *buf;
	size_t len;
	struct sockaddr_in *from;
	struct hardware *hfrom;
{
	SOCKLEN_T flen = sizeof *from;
	int result;

#ifdef IGNORE_HOSTUNREACH
	int retry = 0;
	do {
#endif
		result = recvfrom (interface -> rfdesc, (char *)buf, len, 0,
				   (struct sockaddr *)from, &flen);
#ifdef IGNORE_HOSTUNREACH
	} while (result < 0 &&
		 (errno == EHOSTUNREACH ||
		  errno == ECONNREFUSED) &&
		 retry++ < 10);
#endif
	return result;
}
#endif /* USE_SOCKET_RECEIVE */

#if defined (USE_SOCKET_FALLBACK)
/* This just reads in a packet and silently discards it. */

isc_result_t fallback_discard (object)
	omapi_object_t *object;
{
	char buf [1540];
	struct sockaddr_in from;
	SOCKLEN_T flen = sizeof from;
	int status;
	struct interface_info *interface;

	if (object -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	interface = (struct interface_info *)object;

	status = recvfrom (interface -> wfdesc, buf, sizeof buf, 0,
			   (struct sockaddr *)&from, &flen);
#if defined (DEBUG)
	/* Only report fallback discard errors if we're debugging. */
	if (status < 0) {
		log_error ("fallback_discard: %m");
		return ISC_R_UNEXPECTED;
	}
#endif
	return ISC_R_SUCCESS;
}
#endif /* USE_SOCKET_FALLBACK */

#if defined (USE_SOCKET_SEND)
int can_unicast_without_arp (ip)
	struct interface_info *ip;
{
	return 0;
}

int can_receive_unicast_unconfigured (ip)
	struct interface_info *ip;
{
#if defined (SOCKET_CAN_RECEIVE_UNICAST_UNCONFIGURED)
	return 1;
#else
	return 0;
#endif
}

int supports_multiple_interfaces (ip)
	struct interface_info *ip;
{
#if defined (SO_BINDTODEVICE)
	return 1;
#else
	return 0;
#endif
}

/* If we have SO_BINDTODEVICE, set up a fallback interface; otherwise,
   do not. */

void maybe_setup_fallback ()
{
#if defined (USE_SOCKET_FALLBACK)
	isc_result_t status;
	struct interface_info *fbi = (struct interface_info *)0;
	if (setup_fallback (&fbi, MDL)) {
		fbi -> wfdesc = if_register_socket (fbi);
		fbi -> rfdesc = fbi -> wfdesc;
		log_info ("Sending on   Socket/%s%s%s",
		      fbi -> name,
		      (fbi -> shared_network ? "/" : ""),
		      (fbi -> shared_network ?
		       fbi -> shared_network -> name : ""));
	
		status = omapi_register_io_object ((omapi_object_t *)fbi,
						   if_readsocket, 0,
						   fallback_discard, 0, 0);
		if (status != ISC_R_SUCCESS)
			log_fatal ("Can't register I/O handle for %s: %s",
				   fbi -> name, isc_result_totext (status));
		interface_dereference (&fbi, MDL);
	}
#endif
}
#endif /* USE_SOCKET_SEND */
