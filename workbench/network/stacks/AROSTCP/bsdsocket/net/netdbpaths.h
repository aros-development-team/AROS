/*
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

#ifndef NET_NETDBPATHS_H
#define NET_NETDBPATHS_H

#include <exec/types.h>


#define _PATH_DB            "db"
#define _PATH_SANA2CONFIG   "db/interfaces"
#define _FILE_SANA2CONFIG   "interfaces"
#define _PATH_CONFIG        "db/general.config"
#define _FILE_CONFIG        "general.config"
#define _PATH_NETDB         "db/netdb"
#define _FILE_NETDB         "netdb"
#define _PATH_DHCLIENT	    "C/dhclient"
/*
#define	_PATH_HEQUIV        "db/hosts.equiv"
#define	_PATH_INETDCONF	    "db/inetd.conf"
*/
extern TEXT db_path[];
extern TEXT interfaces_path[];
extern TEXT config_path[];
extern TEXT netdb_path[];
/*
extern TEXT hequiv_path[];
extern TEXT inetdconf_path[];
*/

#endif /* !NET_NETDBPATHS_H */
