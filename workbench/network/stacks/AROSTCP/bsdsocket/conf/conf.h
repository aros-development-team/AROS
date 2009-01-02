/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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

/*
 * This file contains several definitions which affect the compilation of
 * the TCP/IP code. Normally they are boolean switches and the comments tell
 * what happens if the value is TRUE (eg. non zero)
 *
 */

/*
 * Do diagnostic tests which are not necessary in production version
 */
#define DIAGNOSTIC 0

/*
 * Be compatible with BSD 4.2. Affects only checksumming of UDP data. If true
 * the checksum is NOT calculated by default.
 */
#define COMPAT_42 0

/*
 * Make TCP compatible with BSD 4.2
 */
#define TCP_COMPAT_42 0

/*
 * protocol families
 */
#define INET 1
#define CCITT 0
#define NHY 0			/* HYPERchannel */
#define NIMP 0
#define ISO 0
#define NS 0
#define RMP 0

/*
 * optional protocols over IP
 */
#define NSIP 0
#define EON 0
#define TPIP 0

/*
 * default values for IP configurable flags
 */
#define IPFORWARDING    0
#define IPSENDREDIRECTS 1
#define IPPRINTFS       0

/*
 * Network level
 */
#define NETHER 1		/* Call ARP ioctl */

