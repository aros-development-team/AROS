/*
 * Copyright (C) 1993,1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                         Helsinki University of Technology, Finland.
 *                         All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#ifndef NET_SANA2TAGS_H
#define NET_SANA2TAGS_H

#define	ETHERTYPE_IP	0x0800		/* IP protocol */
#define ETHERTYPE_ARP	0x0806		/* Addr. resolution protocol */

#define SLIPTYPE_IP ETHERTYPE_IP
#define CSLIPTYPE_IP ETHERTYPE_IP

#define ARCOTYPE_IP     240	/* RFC 1051 framing */
#define ARCOTYPE_ARP    241	/* RFC 1051 framing */

#define ARCNTYPE_IP     212	/* RFC 1201 framing */
#define ARCNTYPE_ARP    213	/* RFC 1201 framing */
#define ARCNTYPE_RARP   214	/* RFC 1201 framing */

#define PPPTYPE_IP      0x21	/* RFC 1334 IP protocol */

#endif
