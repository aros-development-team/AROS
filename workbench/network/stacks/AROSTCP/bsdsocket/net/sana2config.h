/*
 * Copyright (C) 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
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

#ifndef NET_SANA2CONFIG_H
#define NET_SANA2CONFIG_H

#ifndef DOS_RDARGS_H
#include <dos/rdargs.h>
#endif

#include "net/netdbpaths.h"

#define SSC_TEMPLATE \
  "NAME/A," \
  "DEV=DEVICE/A/K," \
  "UNIT/N/K," \
  "IP/K,NETMASK/K,UP/S," \
  "IPTYPE/N/K," \
  "ARPTYPE=IPARPTYPE/N/K," \
  "IPREQ=IPREQUESTS/N/K," \
  "ARPREQ=ARPREQUESTS/N/K," \
  "WRITEREQ=WRITEREQUESTS/N/K," \
  "NOTRACKING/S," \
  "NOARP/S," \
  "ARPHDR/N/K," \
  "P2P=POINT2POINT/S,NOSIMPLEX/S,LOOPBACK/S"

struct ssc_args {
  UBYTE *a_name;
  UBYTE *a_dev;
  LONG  *a_unit;
  UBYTE *a_ip;
  UBYTE *a_netmask;
  LONG   a_up;
  LONG  *a_iptype;
  LONG  *a_arptype;
  LONG  *a_ipno;
  LONG  *a_arpno;
  LONG  *a_writeno;
  LONG   a_notrack;
  LONG   a_noarp;
  LONG  *a_arphdr;
  LONG   a_point2point;
  LONG   a_nosimplex;
  LONG   a_loopback;
};

struct ssconfig {
  LONG            flags;
  LONG            unit;
  char            name[IFNAMSIZ];
  struct RDArgs   *rdargs;
  struct ssc_args args[1];
};

#define SSCF_RDARGS 1		/* set iff rdargs should be freed */

/*
 * Define how the interface database should be interpreted
 */
#define SSC_ALIAS  0
#define SSC_COMPAT 1

void ssconfig_free(struct ssconfig *config);
struct ssconfig *ssconfig_parse(struct RDArgs *rdargs);
struct ssconfig *ssconfig_make(int how, char *name, long unit);
void ssconfig(struct sana_softc *ifp, struct ssconfig *sscp);

#endif /* !NET_SANA2CONFIG_H */
