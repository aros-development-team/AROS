/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2019 The AROS Dev Team
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

#include <proto/dos.h>
#include <proto/bsdsocket.h>

#include <dos/dos.h>
#include <dos/rdargs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <api/amiga_api.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_includes.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <kern/accesscontrol.h>

#include "net/netdbpaths.h"
#include <net/sana2config.h>
#include <net/route.h>
#include <netinet/in.h>
#include <protos/net/if_protos.h>

extern struct ifnet *iface_make(struct ssconfig *ifc);

LONG read_netdb(struct NetDataBase *ndb, UBYTE *fname,
                UBYTE** errstrp, struct CSource *res,
                int prefixindex, ULONG flags);

/*
 * Global pointer for the NetDataBase
 */
struct NetDataBase *NDB = NULL;

/*
 * Global lock for the NetDatabase
 */
struct SignalSemaphore ndb_Lock;

/*
 * Dynamic items Database (for entries supplied by DHCP)
 */
struct DynDataBase DynDB;

/*
 * NDB Update Counter (used to keep resolver cache up to date)
 */
ULONG ndb_Serial;

/*
 * Default netdatabase name
 */
STRPTR netdbname = netdb_path;

/* 
 * Templates for Arexx commands and DB files
 */
STRPTR NETDBENTRY = 
  "WITH,I=INTERFACE,H=HOST,N=NET,S=SERVICE,P=PROTOCOL,R=ROUTE,NS=NAMESERVER,DO=DOMAIN,RC,ACC=ACCESS";

enum ndbtype { KNDB_WITH, KNDB_IFACE, KNDB_HOST, KNDB_NET, KNDB_SERV, KNDB_PROTO,
	       KNDB_RT, KNDB_DNS, KNDB_DOM, KNDB_RC, KNDB_ACC };

STRPTR NETDBTEMPLATE = 
  "$NAME$/A,$ENTRY$/A,$ALIAS$/M";

STRPTR PROTOCOL_TEMPLATE = 
  "$NAME$/A,$NUMBER$/A/N,$ALIAS$/M";

enum ndbarg { KNDB_NAME, KNDB_DATA, KNDB_ALIAS };

#define NDBARGS 3

STRPTR ROUTE_TEMPLATE =
  "HOST/S,NET/S,DEST/A,GW=GATEWAY/A";

enum rtarg { KRT_HOST, KRT_NET, KRT_DEST, KRT_GATE, RTARGS };

STRPTR ACCESS_TEMPLATE =
  "$PORT$/A,$HOSTMASK$/A,$ACCESS$/A,LOG/S";

enum accarg { KACC_PORT, KACC_HOSTMASK, KACC_ACCESS, KACC_LOG };

#define ACCARGS 4

STRPTR WITH_TEMPLATE = 
  "$FILE$/A,PREFIX/K";
#define WITHARGS 2
enum witharg { WITH_FILE, WITH_PREFIX };

/* prototypes for the netdb parsing functions */

LONG addwith(struct NetDataBase *ndb,
	     struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addifent(struct NetDataBase *ndb,
		struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addhostent(struct NetDataBase *ndb,
		struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addnetent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addservent(struct NetDataBase *ndb,
		struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addprotoent(struct NetDataBase *ndb,
		 struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addrtent(struct NetDataBase *ndb,
		struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addnameservent(struct NetDataBase *ndb,
		    struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG adddomainent(struct NetDataBase *ndb,
		  struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addrcent(struct NetDataBase *ndb,
		    struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addaccessent(struct NetDataBase *ndb,
		  struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
LONG addndbent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG ifflags);

typedef LONG (*ndb_parse_f)(struct NetDataBase *ndb,
	    struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);

/* Array of parsing functions. Note that the order is same as in the
 * NETDBENTRY.
 */
ndb_parse_f ndb_parse_funs[] = {
  addwith,
  addifent,
  addhostent,
  addnetent,
  addservent,
  addprotoent,
  addrtent,
  addnameservent,
  adddomainent,
  addrcent,
  addaccessent
};

/*
 * Alloc a NetDataBase
 */
struct NetDataBase *
alloc_netdb(struct NetDataBase *ndb)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) alloc_netdb()\n"));
#endif

  if (ndb || 
      (ndb = bsd_malloc(sizeof (*NDB), M_NETDB, M_WAITOK))) {  
    struct MinList *gl;

    for (gl = (struct MinList *)&ndb->ndb_Hosts;
	 gl <= (struct MinList *)&ndb->ndb_Domains;        
	 gl++)
      NewList((struct List *)gl);
    
  }
  
  ndb->ndb_AccessCount = 0;
  if ((ndb->ndb_AccessTable =
       bsd_malloc(TMPACTSIZE, M_NETDB, M_WAITOK)) == NULL) {
    bsd_free(ndb, M_NETDB);
    ndb = NULL;
  }
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) alloc_netdb: Allocated ndb = 0x%p, ndb_AccessTable = 0x%p\n", ndb, ndb->ndb_AccessTable));
#endif
  DNETDB(else __log(LOG_DEBUG,"Allocated ndb = 0x%p, ndb_AccessTable = 0x%p", ndb, ndb->ndb_AccessTable);)
  return ndb;
}

/*
 * Free a NetDataBase
 * Caller must have a write lock on NDB
 */
void
free_netdb(struct NetDataBase *ndb)
{
  struct GenentNode *gn;
  struct MinList *gl;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) free_netdb( 0x%p )\n", ndb));
#endif
  for (gl = (struct MinList *)&ndb->ndb_Hosts;
       gl <= (struct MinList *)&ndb->ndb_Domains;        
       gl++)
    while (gn = (struct GenentNode *)RemHead((struct List *)gl)) 
      bsd_free(gn, M_NETDB);

  if (ndb->ndb_AccessTable != NULL) {
    bsd_free(ndb->ndb_AccessTable, M_NETDB);
    ndb->ndb_AccessTable = NULL;
  }
  bsd_free(ndb, M_NETDB);
}

#ifdef DEBUG
static UBYTE * zap;
static size_t zap_size;
#endif

/* 
 * Copy alias list to ato, alias strings and name to cto
 */
static void
aliascpy(UBYTE *cto, UBYTE *name, UBYTE**ato, UBYTE **afrom)
{ 
#ifdef DEBUG
  UBYTE *logname = name;
#endif
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) aliascopy()\n"));
#endif

  do {
    while(*cto++ = *name++);
  } while (afrom && (name = *afrom++) && (*ato++ = cto));

  *ato = NULL;

#ifdef DEBUG
  if (cto != zap + zap_size) {
    __log(LOG_ERR, "%s: mismatch in size %ld != expected %ld\n",
	logname, cto - (UBYTE *)zap, zap_size);
  }
#endif
}

/*
 * Allocate a netdb node
 *
 * nodesize is the size of the base structure, additional space
 * is allocated for the name and the aliases.
 * alias is NULL terminated array of alias name pointers.
 * Number of aliases is returned via aliasp.
 *
 * size field of the allocated node is set to the total size - size for 
 * the MinNode and the size field itself.
 */
static void *
node_alloc(size_t nodesize, UBYTE *name, UBYTE **alias, int *aliasp)
{
  struct GenentNode *gn;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) node_alloc()\n"));
#endif

  nodesize += strlen(name) + 1;	/* Add space needed for the name */

  *aliasp = 1;
  nodesize += sizeof (char*);	/* Alias list NULL terminator */

  /* Calculate the size of the aliases */
  if (alias) {
    while (*alias) {
      (*aliasp)++;
      nodesize += strlen(*alias++) + 1 + sizeof (char*);
    }
  }
  gn = bsd_malloc(nodesize, M_NETDB, M_WAITOK);
  /*
   * set the ent size
   */
  if (gn)
    gn->gn_EntSize = nodesize - sizeof (struct GenentNode);

#ifdef DEBUG  
  zap_size = nodesize;
  zap = (char *)gn;
#endif
  return gn;
}

/*
 * Parse an include entry.
 */
LONG
addwith(struct NetDataBase *ndb,
	struct RDArgs *rdargs,
	UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  // UBYTE result[REPLYBUFLEN + 1];
//struct CSource res;
  LONG retval = RETURN_OK;
  IPTR Args[WITHARGS] = { 0 };
  int which;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addwith()\n"));
#endif

/*  res.CS_Buffer = result; 
  res.CS_Length = sizeof (result);
  res.CS_CurChr = 0;*/

  if (rdargs = ReadArgs(WITH_TEMPLATE, Args, rdargs)) {
    if (Args[WITH_PREFIX] == 0)	/* no prefix given */
      which = -1;
    else {
      /* match given prefix */
      which = FindArg(NETDBENTRY, (UBYTE *)Args[WITH_PREFIX]);
      if (which < 0) {
        *errstrp = ERR_VALUE;
        retval = RETURN_WARN;
      }
    }
    if (retval == RETURN_OK) {
      retval = read_netdb(ndb, (UBYTE *)Args[WITH_FILE], errstrp, res, 
			  which, flags);
    }
    FreeArgs(rdargs);
  } else {
    *errstrp = ERR_SYNTAX; retval = RETURN_WARN;
  }
  return retval;
}

/* Copy an address */
static int inline setaddr(struct sockaddr_in *sa, char *addr, u_short af)
{
  sa->sin_len = sizeof(struct sockaddr_in);
  sa->sin_family = af;
  return __inet_aton(addr, &sa->sin_addr);
}

/*
 * Parse an interface entry.
 */
LONG
addifent(struct NetDataBase *ndb,
	    struct RDArgs *rdargs,
	    UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  struct ssconfig *ssc;
  struct ifnet *ifp;
  struct ifaliasreq ifr;
  char *cp, *ep;
  LONG retval = RETURN_OK;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addifent()\n"));
#endif
  
  if (flags & (NETDB_IFF_MODIFYOLD|NETDB_IFF_ADDNEW)) {
	ssc = ssconfig_parse(rdargs);
	if (ssc) {
		ifp = ifunit(ssc->args->a_name);
		if ((!ifp) && (flags & NETDB_IFF_ADDNEW)) {
			DIFCONF(Printf("Adding new interface %s\n", ssc->args->a_name);)
			cp = strncpy(ssc->name, ssc->args->a_name, IFNAMSIZ);
			ssc->name[IFNAMSIZ-1] = '\0';

			for (; *cp; cp++)
				if (*cp >= '0' && *cp <= '9')
					break;

			ep = cp;
			for (ssc->unit = 0; *cp >= '0' && *cp <= '9'; )
				ssc->unit = ssc->unit * 10 + *cp++ - '0';

			*ep = 0;
			ifp = iface_make(ssc);
			if (!ifp) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addifent: failed to create interface '%s'\n", ssc->args->a_name));
#endif
				*errstrp = ERR_MEMORY;
				retval = RETURN_FAIL;
			}
			else
			{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addifent: created new interface '%s'\n", ssc->args->a_name));
#endif
				flags |= NETDB_IFF_MODIFYOLD;
			}
		}
		if ((ifp) && (flags & NETDB_IFF_MODIFYOLD))
		{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addifent: configuring interface '%s'\n", ssc->args->a_name));
#endif
			DIFCONF(Printf("Setting up interface %s\n", ssc->args->a_name);)
			memset (&ifr, 0, sizeof (ifr));
			if (ssc->args->a_ip) {
				if (!ssc->args->a_up) {
					DIFCONF(Printf("> No auto-online\n");)
					ifp->if_flags |= IFF_NOUP;
				}
				DIFCONF(Printf("> IP address: %s\n", ssc->args->a_ip);)
				if (stricmp(ssc->args->a_ip, "DHCP")) {
					ifp->if_data.ifi_aros_usedhcp = 0;
					if (setaddr ((struct sockaddr_in *)&ifr.ifra_addr, ssc->args->a_ip, AF_INET)) {
						if (ssc->args->a_netmask) {
							DIFCONF(Printf("> Netmask: %s\n", ssc->args->a_netmask);)
							if (!setaddr ((struct sockaddr_in *)&ifr.ifra_mask, ssc->args->a_netmask, AF_UNSPEC)) {
								*errstrp = ERR_SYNTAX;
								retval = RETURN_WARN;
							}
						}
						in_control(NULL, SIOCAIFADDR, &ifr, ifp);
					} else {
						*errstrp = ERR_SYNTAX;
						retval = RETURN_WARN;
					}
				} else {
					DIFCONF(Printf("> Using DHCP\n");)
					ifp->if_data.ifi_aros_usedhcp = 1;
					if (ssc->args->a_up) {
						if (api_state == API_SHOWN)
							run_dhclient(ifp);
						else
							ifp->if_flags |= IFF_DELAYUP;
					}
				}
			}
		}
		else
		{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addifent: Interface named '%s' already exists..\n", ssc->args->a_name));
#endif
		}
		ssconfig_free(ssc);
	} else {
		*errstrp = ERR_SYNTAX;
		retval = RETURN_WARN;
	}
  }
  return retval;
}


/*
 * Parse a service entry.
 */
LONG
addservent(struct NetDataBase *ndb,
	    struct RDArgs *rdargs,
	    UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  LONG retval;
  IPTR Args[NDBARGS] = { 0 };
  struct ServentNode *sn;     
  int aliases, plen;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addservent()\n"));
#endif

  if (rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
    /* convert port number */
    UBYTE *s_proto = (UBYTE*)Args[KNDB_DATA];
    LONG tmp;
    if ((plen = StrToLong(s_proto, &tmp)) > 0 &&
	s_proto[plen++] == '/') {
      Args[KNDB_DATA] = tmp;
      int protonamelen = strlen(s_proto = s_proto + plen) + 1;
      sn = node_alloc(sizeof (*sn) + protonamelen,
		      (UBYTE*)Args[KNDB_NAME], 
		      (UBYTE **)Args[KNDB_ALIAS], &aliases);
      if (sn) {
	UBYTE **alias = (UBYTE **)(sn+1);
	UBYTE *name = (UBYTE *)(alias + aliases);

	sn->sn_Ent.s_port = Args[KNDB_DATA];
	sn->sn_Ent.s_proto = strcpy(name, s_proto);
	sn->sn_Ent.s_name = name + protonamelen;
	sn->sn_Ent.s_aliases = (char **)alias;

	/* Copy aliases */
	aliascpy(sn->sn_Ent.s_name, (UBYTE*)Args[KNDB_NAME], 
		 alias, (UBYTE **)Args[KNDB_ALIAS]);
	AddTail((struct List*)&ndb->ndb_Services, (struct Node*)sn);
	retval = RETURN_OK;
      } else {
	*errstrp = ERR_MEMORY; retval = RETURN_FAIL;
      }
    } else { 
      *errstrp = ERR_VALUE; retval = RETURN_WARN; 
    } 
    FreeArgs(rdargs);
  } else {
    *errstrp = ERR_SYNTAX; retval = RETURN_WARN;
  }
  return retval;
}

/*
 * Parse a host entry.
 *
 * NOTE: The host entry has the address in the 'name' and the official name
 * in the 'data'.
 */
LONG
addhostent(struct NetDataBase *ndb,
	    struct RDArgs *rdargs,
	    UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  LONG retval;
  IPTR Args[NDBARGS] = { 0 };
  struct HostentNode *hn;
  struct in_addr addr;
  int aliases;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addhostent()\n"));
#endif

  if (rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
    /* convert ip address */
    if (__inet_aton((char*)Args[KNDB_NAME], &addr)) {
      hn = node_alloc(sizeof (*hn) + 2*sizeof (&addr) + sizeof (addr),
		      (UBYTE*)Args[KNDB_DATA], 
		      (UBYTE **)Args[KNDB_ALIAS], &aliases);
      if (hn) {
	struct in_addr **addrtbl = (struct in_addr **)(hn + 1);
	UBYTE **alias = (UBYTE **)((UBYTE*)(addrtbl + 2) + sizeof (addr));
	UBYTE *name = (UBYTE *)(alias + aliases);

	hn->hn_Ent.h_addrtype = AF_INET;
	hn->hn_Ent.h_length = sizeof (addr);
	hn->hn_Ent.h_addr_list = (char **)addrtbl;
	hn->hn_Ent.h_name = name;
	hn->hn_Ent.h_aliases = (char **)alias;

	/* Make address list */
	addrtbl[0] = (struct in_addr *)(addrtbl + 2);
	addrtbl[1] = NULL;
	bcopy(&addr, addrtbl[0], sizeof (addr));

	/* Copy aliases */
	aliascpy(hn->hn_Ent.h_name, (UBYTE*)Args[KNDB_DATA], 
		 alias, (UBYTE **)Args[KNDB_ALIAS]);
	AddTail((struct List*)&ndb->ndb_Hosts, (struct Node*)hn);
	retval = RETURN_OK;
      } else {
	*errstrp = ERR_MEMORY; retval = RETURN_FAIL;
      }
    } else { 
      *errstrp = ERR_VALUE; retval = RETURN_WARN; 
    } 
    FreeArgs(rdargs);
  } else {
    *errstrp = ERR_SYNTAX; retval = RETURN_WARN;
  }
  return retval;
}

/*
 * Parse a net entry.
 */
LONG
addnetent(struct NetDataBase *ndb,
	  struct RDArgs *rdargs,
	  UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  LONG retval;
  IPTR Args[NDBARGS] = { 0 };
  struct NetentNode *nn;
  struct in_addr addr;
  int aliases;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnetent()\n"));
#endif

  if (rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
    /* convert ip address */
    if (__inet_aton((char*)Args[KNDB_DATA], &addr)) {
      nn = node_alloc(sizeof (*nn),
		      (UBYTE*)Args[KNDB_NAME], 
		      (UBYTE **)Args[KNDB_ALIAS], &aliases);
      if (nn) {
	UBYTE **alias = (UBYTE **)(nn + 1);
	UBYTE *name = (UBYTE *)(alias + aliases);

	nn->nn_Ent.n_addrtype = AF_INET;
	nn->nn_Ent.n_name = name;
	nn->nn_Ent.n_aliases = (char **)alias;
	bcopy(&addr, &nn->nn_Ent.n_net, sizeof (addr));

	/* Copy aliases */
	aliascpy(nn->nn_Ent.n_name, (UBYTE*)Args[KNDB_NAME], 
		 alias, (UBYTE **)Args[KNDB_ALIAS]);
	AddTail((struct List*)&ndb->ndb_Networks, (struct Node*)nn);
	retval = RETURN_OK;
      } else {
	*errstrp = ERR_MEMORY; retval = RETURN_FAIL;
      }
    } else { 
      *errstrp = ERR_VALUE; retval = RETURN_WARN;
    } 
    FreeArgs(rdargs);
  } else {
    *errstrp = ERR_SYNTAX; retval = RETURN_WARN;
  }
  return retval;
}

/*
 * Parse a protocol entry.
 */
LONG
addprotoent(struct NetDataBase *ndb,
	    struct RDArgs *rdargs,
	    UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  LONG retval;
  IPTR Args[NDBARGS] = { 0 };
  struct ProtoentNode *pn;     
  int aliases;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addprotoent()\n"));
#endif

  if (rdargs = ReadArgs(PROTOCOL_TEMPLATE, Args, rdargs)) {
    
    if (Args[KNDB_DATA]) {
      pn = node_alloc(sizeof (*pn), (UBYTE*)Args[KNDB_NAME], 
		      (UBYTE **)Args[KNDB_ALIAS], &aliases);
      if (pn) {
	UBYTE **alias = (UBYTE **)(pn+1);
	UBYTE *name = (UBYTE *)(alias + aliases);

	pn->pn_Ent.p_name = name;
	pn->pn_Ent.p_aliases = (char **)alias;
	pn->pn_Ent.p_proto = *(LONG *)Args[KNDB_DATA];

	/* Copy name and aliases */
	aliascpy(name, (UBYTE*)Args[KNDB_NAME], 
		 alias, (UBYTE **)Args[KNDB_ALIAS]);
	AddTail((struct List*)&ndb->ndb_Protocols, (struct Node*)pn);
	retval = RETURN_OK;
      } else {
	*errstrp = ERR_MEMORY; retval = RETURN_FAIL;
      }
    } else { 
      *errstrp = ERR_VALUE; retval = RETURN_WARN; 
    } 
    FreeArgs(rdargs);
  } else {
    *errstrp = ERR_SYNTAX; retval = RETURN_WARN;
  }
  return retval;
}

/*
 * Parse a route entry
 */
LONG
addrtent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs,
	       UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  LONG retval;
  IPTR Args[RTARGS] = { 0 };
  struct ortentry route;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addrtent()\n"));
#endif
  
  if (rdargs = ReadArgs(ROUTE_TEMPLATE, Args, rdargs)) {
    if (!(strcmp(strupr((APTR)Args[KRT_DEST]), "DEFAULT"))){
      struct sockaddr_in *rodst_saddr = (struct sockaddr_in *)&route.rt_dst;
      rodst_saddr->sin_addr.s_addr = 0;
      route.rt_dst.sa_family = AF_INET;
      route.rt_dst.sa_len = sizeof(struct sockaddr_in);
    } else {
      if (!(setaddr((struct sockaddr_in *)&route.rt_dst, (APTR)Args[KRT_DEST], AF_INET)))
        goto bad;
    }
    
    if (setaddr((struct sockaddr_in *)&route.rt_gateway, (APTR)Args[KRT_GATE], AF_INET)) {
      route.rt_flags = RTF_UP | RTF_GATEWAY;
      if (Args[KRT_HOST])
        route.rt_flags |= RTF_HOST;
      rtioctl(SIOCADDRT, (caddr_t)&route);
      retval = RETURN_OK;
    } else {
bad:
      *errstrp = ERR_SYNTAX;
      retval = RETURN_WARN;
    }
    FreeArgs(rdargs);
  }
  return retval;
}

/*
 * Parse a Name Server entry
 */
LONG
addnameservent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs,
	       UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  UBYTE Buffer[KEYWORDLEN];
  LONG  BufLen = sizeof (Buffer);
  struct in_addr ns_addr;
  struct NameserventNode *nsn;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnameservent()\n"));
#endif

/* TODO: NicJA - Where does CHECK_POINTER() Come from? */
#if !defined(__AROS__)
  CHECK_POINTER(ndb);
#endif

  if (ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnameservent: Error in syntax\n"));
#endif
    *errstrp = ERR_SYNTAX; 
    return RETURN_WARN;
  }
  if (!__inet_aton(Buffer, &ns_addr)) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnameservent: error with address\n"));
#endif
    *errstrp = ERR_VALUE;
    return RETURN_WARN; 
  }
  if ((nsn = bsd_malloc(sizeof (*nsn), M_NETDB, M_WAITOK)) == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnameservent: couldnt allocate entry\n"));
#endif
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }
  nsn->nsn_EntSize = sizeof (nsn->nsn_Ent);
  nsn->nsn_Ent.ns_addr.s_addr = ns_addr.s_addr;

/* TODO: NicJA - Where does CHECK_POINTER() Come from? */
#if !defined(__AROS__)
  CHECK_POINTER(nsn);
#endif

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addnameservent: Added nameserver %s (0x%08lx) to netdb = 0x%08lx\n",Buffer, nsn->nsn_Ent.ns_addr.s_addr, ndb));
#endif
  DNETDB(__log(LOG_DEBUG,"Added nameserver %s (0x%08lx) to netdb = 0x%08lx\n",Buffer, nsn->nsn_Ent.ns_addr.s_addr, ndb);)

  AddTail((struct List*)&ndb->ndb_NameServers, (struct Node*)nsn);
  return RETURN_OK;
}

/*
 * Parse a Domain Name entry
 */
LONG
adddomainent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs,
	       UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  UBYTE Buffer[REPLYBUFLEN];
  LONG  BufLen = sizeof (Buffer);
  struct DomainentNode *dn;
  short  nodesize;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) adddomainent()\n"));
#endif

  if (ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
    *errstrp = ERR_SYNTAX; 
    return RETURN_WARN;
  }
  nodesize = sizeof (*dn) + strlen(Buffer) + 1;
  if ((dn = bsd_malloc(nodesize, M_NETDB, M_WAITOK)) == NULL) {
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }
  dn->dn_EntSize = nodesize - sizeof (struct GenentNode);
  dn->dn_Ent.d_name = (char *)(dn + 1);
  
  strcpy((char *)(dn + 1), Buffer);

  AddTail((struct List*)&ndb->ndb_Domains, (struct Node*)dn);
  return RETURN_OK;
}

/*
 * Parse an RC script entry
 */
LONG
addrcent(struct NetDataBase *ndb,
	       struct RDArgs *rdargs,
	       UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  UBYTE Buffer[REPLYBUFLEN];
  LONG  BufLen = sizeof (Buffer);
  struct RcentNode *dn;
  short  nodesize;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addrcent()\n"));
#endif

  if (ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_WARN;
  }
  nodesize = sizeof (*dn) + strlen(Buffer) + 1;
  if ((dn = bsd_malloc(nodesize, M_NETDB, M_WAITOK)) == NULL) {
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }
  dn->rn_EntSize = nodesize - sizeof (struct GenentNode);
  dn->rn_Ent = (char *)(dn + 1);

  strcpy((char *)(dn + 1), Buffer);

  AddTail((struct List*)&ndb->ndb_Rc, (struct Node*)dn);
  return RETURN_OK;
}


/*
 * Parse a access control entry.. after reading the whole netdatabase
 * access list must be reorganized;
 */
LONG
addaccessent(struct NetDataBase *ndb,
	     struct RDArgs *rdargs,
	     UBYTE **errstrp, struct CSource *res, ULONG ifflags)
{
  LONG retval = RETURN_WARN;
  IPTR Args[ACCARGS] = { 0 };

  ULONG host, mask;
  UWORD port, flags = ACF_CONTINUE;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addaccessent()\n"));
#endif

  if (ndb->ndb_AccessCount >= TMPACTSIZE / sizeof (struct AccessItem)) {
    *errstrp = "Too many access control items\n";
    return retval; /* copy propagation expected */
  }
  
  if ((rdargs = ReadArgs(ACCESS_TEMPLATE, Args, rdargs)) != NULL) {
    
    if (strcmp((char *)Args[KACC_PORT], "*") == 0)
      port = 0;
    else if (strcmp((char *)Args[KACC_PORT], "@") == 0) {
      port = 0; flags |= ACF_PRIVONLY;
    }
    else if (StrToLong((char *)Args[KACC_PORT], (LONG *)&host) > 0
	     && host != 0) {
      if (host > 0xffff) {
	*errstrp = "Illegal port value\n";
	goto exit;
      }
      port = host;
    }
    else {
      struct ServentNode * entNode;

      if ((entNode =
	   findServentNode(ndb, (char *)Args[KACC_PORT], "tcp")) != NULL)
	  port = entNode->sn_Ent.s_port;
      else {
	*errstrp = "Illegal port value\n";
	goto exit;
      }
    }
    {
      int zmask = 0xFFFFFFFF;
      int i = 0, ls = 0, dots = 0;

#define hm ((char *)Args[KACC_HOSTMASK])
      
      while ((hm[i] >= '0' && hm[i] <= '9') || hm[i] == '.' || hm[i] == '*') {
	if (hm[i] == '.') {
	  ls = 0;
	  dots++;
	}
	else if (hm[i] == '*') {
	  hm[i] = '0';
	  ls = 1;
	  zmask ^= (0xFF000000 >> 8 * dots);
	}
	i++;
      }
      if (ls == 1)
	while (dots++ < 3)
	  zmask ^= (0xFF000000 >> 8 * dots);

      if (hm[i] == '/') {
	hm[i++] = '\0';
	if (__inet_aton(&hm[i], (struct in_addr *)&mask) == 0) {
	  *errstrp = "Illegal mask value\n";
	  goto exit;
	}
      }
      else
	mask = 0xffffffff;

      mask &= zmask;
      
      if (__inet_aton(hm, (struct in_addr *)&host) == 0) {
	*errstrp = "Illegal host value\n";
	goto exit;
      }
#undef hm      
    }
    if (strcmp((char *)Args[KACC_ACCESS], "allow") == 0)
      flags |= ACF_ALLOW;
    else if (strcmp((char *)Args[KACC_ACCESS], "deny") != 0) {
      *errstrp = "Illegal access value\n";
      goto exit;
    }

    if (Args[KACC_LOG])
      flags |= ACF_LOG;

    ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_port = port;
    ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_host = host;
    ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_mask = mask;
    ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_flags = flags;
    ndb->ndb_AccessCount++;

    retval = 0;
  exit:
    FreeArgs(rdargs);
  }
  else
    *errstrp = ERR_SYNTAX;

  return retval;
}

/*
 * Add an entry into NetDB. 
 * Caller must have a write lock on ndb 
 */
LONG 
addndbent(struct NetDataBase *ndb,
	  struct RDArgs *rdargs, 
	  UBYTE **errstrp, struct CSource *res, ULONG flags)
{
  if (NDB) {
    LONG item;
    enum ndbtype which;
    UBYTE Buffer[KEYWORDLEN];

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) addndbent()\n"));
#endif

    /* Get entry type */
    item = ReadItem(Buffer, sizeof (Buffer), &rdargs->RDA_Source);

    if (item == 0)
      return RETURN_OK;		/* empty line */
    if (item < 0) {
      DNETDB(Printf("addndbent: syntax error\n");)
      *errstrp = ERR_SYNTAX;
      return RETURN_WARN;
    }
    if ((which = FindArg(NETDBENTRY, Buffer)) < 0) {
      DNETDB(Printf("Unknown keyword\n");)
      *errstrp = ERR_UNKNOWN;
      return RETURN_WARN;
    } 

    DNETDB(Printf("Keyword number: %ld\n", which);)
    which =  ndb_parse_funs[which](ndb, rdargs, errstrp, res, flags);
    return which;

  } else {
    *errstrp = ERR_NONETDB;
    return RETURN_FAIL;
  }
}

/* 
 * Read in a NetDataBase file
 */
LONG 
read_netdb(struct NetDataBase *ndb, UBYTE *fname, 
	   UBYTE** errstrp, struct CSource *res, int prefixindex, ULONG flags)
{
  LONG warnval = RETURN_OK;
  LONG retval = RETURN_OK, ioerr = 0;
  UBYTE *p, *buf = AllocMem(CONFIGLINELEN, MEMF_PUBLIC);
  struct RDArgs *rdargs;
  BPTR fh;
  short line = 0;
  ndb_parse_f parser;
  BPTR lock, oldcd;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) read_netdb('%s')\n", fname));
#endif

  DNETDB(Printf("Reading netdb file: %s\n", fname);)
  /* Get an exclusive lock on the database.
   * Multiple locks are OK (when this function is called recursively)
   */
  LOCK_W_NDB(ndb);		
  if (ndb_Lock.ss_NestCount > 10) {
    UNLOCK_NDB(ndb);
    *errstrp = "Too many files included";
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) read_netdb: Too many files included\n"));
#endif
    return RETURN_ERROR;
  }
  if (buf) {

    /* CD to netdb directory */
    lock = Lock(db_path, ACCESS_READ);
    if (lock)
      oldcd = CurrentDir(lock);
    
    if (fh = Open(fname, MODE_OLDFILE)) {
      if (rdargs = AllocDosObject(DOS_RDARGS, NULL)) {
	/* initialize CSource of the rdargs */
	rdargs->RDA_Source.CS_Buffer = buf;
	/* initialize rest fields (see dos/rdargs.h) */
	rdargs->RDA_DAList = 0;
	rdargs->RDA_ExtHelp = NULL;
	rdargs->RDA_Flags = 0;
	
	if (prefixindex < 0)
	  parser = addndbent;	/* no prefix */
	else
	  parser = ndb_parse_funs[prefixindex];
	
	while (FGets(fh, buf, CONFIGLINELEN - 1)) {
	  line++;		/* maintain line number */
	  /* pass by white space */
	  for (p = buf; *p == ' ' || *p == '\t' || *p == '\r'; p++)
	    ;
	  rdargs->RDA_Source.CS_CurChr = p - buf;
	  if (*p == '#' || *p == ';' || *p == '\n') /* only a comment line */
	    continue;
	  /* remove comments & calc length */
	  for (; *p; p++) { 
	    if (*p == '#' || *p == ';') {
	      *p++ = '\n';
	      *p   = '\0';	/* terminate line */
	      break;
	    }
	  }
	  /* ensure that line ends with '\n' (ReadArgs() depends on it) */
	  if (*(p - 1) != '\n') {
	    *p++ = '\n';
	    *p   = '\0';
	  }
	  rdargs->RDA_Source.CS_Length = p - buf;
	  rdargs->RDA_Buffer = NULL;
	  rdargs->RDA_BufSiz = 0;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) read_netdb: parsing line: %s, prefixindex=%ld\n", buf, prefixindex));
#endif
	  DNETDB(Printf("Processing line: %s, prefixindex=%ld\n", buf, prefixindex);)
	  retval = parser(ndb, rdargs, errstrp, res, flags);
	  if (retval == RETURN_OK)
	    continue;
	  if (retval != RETURN_WARN) { /* severe error */
	    error_request("Fatal error in NetDB file %s at line %ld, col %ld\n%s\nAROSTCP will quit",
		(IPTR)fname, (IPTR)line, (IPTR)rdargs->RDA_Source.CS_CurChr, (IPTR)*errstrp);
	    break;
	  }
	  /* Log the error */
	  error_request("Error in NetDB file %s at line %ld, col %ld\n%s",
		(IPTR)fname, (IPTR)line, (IPTR)rdargs->RDA_Source.CS_CurChr, (IPTR)*errstrp);
	  __log(LOG_WARNING, "NetDB(%s) line %ld: %s before col %ld\n",
	      fname, line, *errstrp, rdargs->RDA_Source.CS_CurChr);

	  warnval = retval;
	}
	/* Check file error */ 
	ioerr = IoErr();
	
	FreeDosObject(DOS_RDARGS, rdargs);
      }
      Close(fh);
    } else {
      ioerr = IoErr();
    }
    
    if (ioerr) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) read_netdb: ioerror\n"));
#endif
      Fault(ioerr, "readnetdb", res->CS_Buffer, res->CS_Length);
      *errstrp = res->CS_Buffer;
      retval = RETURN_ERROR;
    }
    if ((!fh) && (ndb_Lock.ss_NestCount == 1))
	error_request("Unable to open NetDB file %s\n%s", (IPTR)fname, (IPTR)*errstrp);
    
    /* return old current directory */
    if (lock) {
      CurrentDir(oldcd);
      UnLock(lock);
    }

    FreeMem(buf, CONFIGLINELEN);
  } else {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) read_netdb: Failed to allocate buffer\n"));
#endif
    *errstrp = ERR_MEMORY;
    retval = RETURN_FAIL;
  }

  UNLOCK_NDB(ndb);

  return retval > warnval? retval: warnval;
}

/*
 * Parse the 'ADD' command
 */
LONG
do_netdb(struct CSource *csarg, UBYTE **errstrp, struct CSource *res)
{
  struct RDArgs *rdargs;
  LONG retval;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) do_netdb()\n"));
#endif

  if (rdargs = AllocDosObject(DOS_RDARGS, NULL)) {
    /* initialize CSource of the rdargs */
    rdargs->RDA_Source = *csarg;
    /* initialize rest fields (see <dos/rdargs.h>) */
    rdargs->RDA_DAList = 0;
    rdargs->RDA_Buffer = NULL;
    rdargs->RDA_BufSiz = 0;
    rdargs->RDA_ExtHelp = NULL;
    rdargs->RDA_Flags = 0;

    LOCK_W_NDB(NDB);

    retval = addndbent(NDB, rdargs, errstrp, res, 0);
/* TODO: set flags here */

    UNLOCK_NDB(NDB);
    
    FreeDosObject(DOS_RDARGS, rdargs);
  }
  else 
    retval = RETURN_FAIL;
	
  return retval;
}

/*
 * Initialize the Network Data Base
 */
LONG 
init_netdb(void)
{
  UBYTE result[REPLYBUFLEN + 1]; /* for error returns */
  struct CSource res;
  UBYTE *errstr;
  LONG   retval;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) init_netdb()\n"));
#endif

  res.CS_Buffer = result;      
  res.CS_Length = sizeof (result); 
  res.CS_CurChr = 0;

  InitSemaphore (&ndb_Lock);
  InitSemaphore (&DynDB.dyn_Lock);
  NewList((struct List *)&DynDB.dyn_NameServers);
  NewList((struct List *)&DynDB.dyn_Domains);
  ndb_Serial = 0;

  /* Allocate the NetDataBase */
  if (!(NDB = alloc_netdb(NULL))) {
    return RETURN_FAIL;
  }

  /* Read in the default data base file */
  retval = read_netdb(NDB, netdbname, &errstr, &res, -1, NETDB_IFF_ADDNEW);
/*  if (retval) {
    Printf("init_netdb: file %s: %s\n", netdbname, errstr);
    __log(LOG_WARNING, "init_netdb: file %s: %s", netdbname, errstr);
  } else */
  if (retval == RETURN_WARN)
    retval = RETURN_OK;

  if (!retval)
    setup_accesscontroltable(NDB);

  return retval;
}


void netdb_deinit(void)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) netdb_deinit()\n"));
#endif
  /* A Placeholder for possible future deinitializations */
}
  
/*
 * Reset the NetDataBase
 */
LONG reset_netdb(struct CSource *cs,
		 UBYTE **errstrp,
		 struct CSource *res)
{
  LONG retval;
  struct NetDataBase *newnetdb;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_netdb.c) reset_netdb()\n"));
#endif

  /* Allocate a temporary NetDataBase */
  if (!(newnetdb = alloc_netdb(NULL))) {
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  retval = read_netdb(newnetdb, netdbname, errstrp, res, -1, 0);
/* TODO: set flags here */

  if (retval == RETURN_OK) {
    setup_accesscontroltable(newnetdb);

    /* Now clear the old lists of the NDB */
    LOCK_W_NDB(NDB);

    free_netdb(NDB);
    NDB = newnetdb;

    UNLOCK_NDB(NDB);
    ndb_Serial++;
  } else {
    free_netdb(newnetdb);
  }

  return retval;
}

