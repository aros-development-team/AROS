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

#include <conf.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <net/if_types.h>
#include <assert.h>
#include <kern/amiga_subr.h>

#if NS
#error NS is not supported
#endif

#include "kern/amiga_includes.h"
#include <proto/dos.h>

#include <netinet/in.h>
#include <devices/sana2.h>
#include <utility/tagitem.h>
#include "net/netdbpaths.h"
#include <net/sana2tags.h>
#include <net/sana2config.h>
#include <net/if_arp.h>
#include <net/if_sana.h>

static const char template[] = SSC_TEMPLATE;

#define CONFIGLINELEN 1024

/*
 * Parse the configuration
 */
struct ssconfig *
ssconfig_parse(struct RDArgs *rdargs)
{
  struct ssconfig *config = AllocVec(sizeof(*config), MEMF_CLEAR|MEMF_PUBLIC);

D(bug("[AROSTCP] ssconfig_parse()\n"));

  if (config != NULL) {
    
    if (ReadArgs(template, (LONG *)config->args, rdargs)) {
      config->rdargs = rdargs;
      config->flags |= SSCF_RDARGS;
      return config;
    } else {
      FreeVec(config);
    }
  }
  return NULL;
}

/*
 * Free the configuration
 */
void
ssconfig_free(struct ssconfig config[])
{
D(bug("[AROSTCP] ssconfig_free()\n"));
  if (config->flags & SSCF_RDARGS)
    FreeArgs(config->rdargs);
  FreeVec(config);
}

static int
getconfs(BPTR iffh, UBYTE *buf)
{
  LONG i, quoted = 0, escaped = 0;

D(bug("[AROSTCP] getconfs()\n"));

  if (FGets(iffh, buf, CONFIGLINELEN - 1)) {
    for (i = 0; buf[i]; i++) {
      UBYTE c = buf[i];
      if (c == '\n') {
	if (quoted) {
	  return ERROR_UNMATCHED_QUOTES;
	}
	if (i > 0 && buf[i - 1] == '+') {
	  i--;
	  if (i < CONFIGLINELEN - 2) {
	    if (FGets(iffh, buf + i, CONFIGLINELEN - 1 - i))
	      continue;
	    return IoErr();
	  }	    
	  return ERROR_LINE_TOO_LONG;
	}
	return 0;
      } else if (quoted) {
	if (escaped) {
	  escaped = 0;
	} else if (c == '*') {
	  escaped = 1;
	  continue;
	} else if (c == '"') {
	  quoted = 0;
	}
      } else if (c == ';' || c == '#') {
	buf[i++] = '\n';
	buf[i] = '\0';
	return 0;
      } else if (escaped) {
	escaped = 0;
      } else if (c == '*') {
	escaped = 1;
      } else if (c == '"') {
	quoted = 1;
      }
    }
    return ERROR_LINE_TOO_LONG;
  }

  buf[0] = '\0';
  return IoErr();
}

/*
 * Default configuration as per hardware type
 */
static const struct wire_defaults {
  LONG  wd_wiretype;
  LONG  wd_iptype;		/* IP packet type */
  WORD  wd_ipno;		/* SANA-II requests reserved for receiving */
  WORD  wd_writeno;		/* SANA-II requests reserved for sending */
  LONG  wd_arptype;		/* ARP packet type */
  WORD  wd_arpno;		/* SANA-II requests reserved for ARP */
  WORD  wd_arphdr;		/* ARP version */
  WORD  wd_ifflags;		/* Interface flags */
  BYTE  wd_pad[2];
} wire_defaults[] = {
  { 
    S2WireType_Ethernet, 
    ETHERTYPE_IP, 16, 16, 
    ETHERTYPE_ARP, 4, 1, 
    IFF_NOTRAILERS|IFF_BROADCAST|IFF_SIMPLEX, 
  },
  { 
    S2WireType_Arcnet, 
    ARCOTYPE_IP, 16, 16, 
    ARCOTYPE_ARP, 4, 7, 
    IFF_NOTRAILERS|IFF_BROADCAST|IFF_SIMPLEX, 
  },
  { 
    S2WireType_SLIP, 
    SLIPTYPE_IP, 8, 8,
    0, 0, 0,
    IFF_NOTRAILERS|IFF_POINTOPOINT|IFF_NOARP, 
  },
  { 
    S2WireType_CSLIP, 
    SLIPTYPE_IP, 8, 8,
    0, 0, 0,
    IFF_NOTRAILERS|IFF_POINTOPOINT|IFF_NOARP, 
  },
  { 
    S2WireType_PPP, 
    PPPTYPE_IP, 8, 8,
    0, 0, 0,
    IFF_NOTRAILERS|IFF_POINTOPOINT|IFF_NOARP, 
  },
  /* Use ethernet as default */
  {
    0,
    ETHERTYPE_IP, 16, 16,
    ETHERTYPE_ARP, 4, 1,
    IFF_NOTRAILERS|IFF_BROADCAST|IFF_SIMPLEX, 
  },
};

/*
 * Initialize sana_softc
 */
void
ssconfig(struct sana_softc *ifp, struct ssconfig *ifc)
{
  const struct ssc_args *args = ifc->args;
  const struct wire_defaults *wd;
  LONG wt = ifp->ss_hwtype;
  LONG reqtotal = 0;

D(bug("[AROSTCP] ssconfig()\n"));

  assert(ifp != NULL);

  for (wd = wire_defaults; wd->wd_wiretype != 0; wd++) {
    if (wt == wd->wd_wiretype)
      break;
  }
  
  ifp->ss_ip.type = args->a_iptype ? *args->a_iptype : wd->wd_iptype;
  reqtotal += ifp->ss_ip.reqno = args->a_ipno ? *args->a_ipno : wd->wd_ipno;

  ifp->ss_arp.type = args->a_arptype ? *args->a_arptype : wd->wd_arptype;
  reqtotal += ifp->ss_arp.reqno = args->a_arpno ? *args->a_arpno : wd->wd_arpno;
  ifp->ss_arp.hrd = args->a_arphdr ? *args->a_arphdr : wd->wd_arphdr;

  reqtotal += args->a_writeno ? *args->a_writeno : wd->wd_writeno;

  if (reqtotal > 65535)
    reqtotal = 65535;
  ifp->ss_reqno = reqtotal;

  {
    UWORD ifflags = wd->wd_ifflags;

    if (args->a_noarp)
      ifflags |= IFF_NOARP;
    if (args->a_point2point) {
      ifflags |= IFF_POINTOPOINT;
      ifflags &= ~IFF_BROADCAST;
    }
    if (args->a_nosimplex)
      ifflags &= ~IFF_SIMPLEX;
    if (args->a_loopback)
      ifflags |= IFF_LOOPBACK;

    ifp->ss_if.if_flags = ifflags;
  }

  /* Flags for soft_sanac */
  ifp->ss_cflags = SS_CFLAGS;

  if (args->a_notrack)
    ifp->ss_cflags &= ~(SSF_TRACK);

  /* Set up name */
  ifp->ss_if.if_name = strcpy(ifp->ss_name, ifc->name);
  ifp->ss_if.if_unit = ifc->unit;
  ifp->ss_execname = strcpy((char *)(ifp + 1), ifc->args->a_dev);
  ifp->ss_execunit = *ifc->args->a_unit;
}

