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

#ifndef NET_A_IF_H
#define NET_A_IF_H

#ifndef NET_IF_H
#include <net/if.h>
#endif

/*
 * Compatibility interface structures
 */
struct	aifreq {
#define	AIFNAMSIZ	64
	char	ifr_name[AIFNAMSIZ]; /* sana-II name, e.g. "slip.device/0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		caddr_t	ifru_data;
	} ifr_ifru;
};

struct aifaliasreq {
	char	ifra_name[AIFNAMSIZ]; /* sana-II name, e.g. "slip.device/0" */
	struct	sockaddr ifra_addr;
	struct	sockaddr ifra_broadaddr;
	struct	sockaddr ifra_mask;
};

#endif /* NET_A_IF_H */
