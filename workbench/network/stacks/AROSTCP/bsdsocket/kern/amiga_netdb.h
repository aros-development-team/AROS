/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 Pavel Fedin
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

#ifndef AMIGA_NETDB_H
#define AMIGA_NETDB_H

#ifndef NETDB_H
#include <netdb.h>
#endif

#include "net/netdbpaths.h"

#ifndef IN_H
#include <netinet/in.h>
#endif

/* Access control table item */
struct AccessItem {
  UWORD	ai_flags;
  UWORD	ai_port;
  ULONG	ai_host;
  ULONG ai_mask;
};

/* Access control flags */
#define ACF_ALLOW	0x01
#define ACF_LOG		0x02
#define ACF_PRIVONLY	0x04 /* 0 as port matches for privileged ports only */
#define ACF_CONTINUE	(1 << 15)

/* AC table temporary buffer size */
#define TMPACTSIZE	0x4000 

/* NetDataBase */
struct NetDataBase {
  struct MinList         ndb_Hosts;
  struct MinList         ndb_Networks;
  struct MinList         ndb_Services;
  struct MinList         ndb_Protocols;
  struct MinList         ndb_NameServers;
  struct MinList	 ndb_Rc;
  struct MinList         ndb_Domains;
  LONG			 ndb_AccessCount; /* tmp var, but reduces code size */
  struct AccessItem *	 ndb_AccessTable;
};

extern struct NetDataBase *NDB;
extern struct SignalSemaphore ndb_Lock;
extern struct DynDataBase DynDB;
extern ULONG ndb_Serial;

/* Locking macros for NDB */
#define LOCK_W_NDB(ndb) (ObtainSemaphore(&ndb_Lock))
#define LOCK_R_NDB(ndb) (ObtainSemaphoreShared(&ndb_Lock))
#define UNLOCK_NDB(ndb) (ReleaseSemaphore(&ndb_Lock))
#define ATTEMPT_NDB(ndb) (AttemptSemaphore(&ndb_Lock))

/* Dynamic part of the NetDB */
struct DynDataBase {
  struct SignalSemaphore dyn_Lock;
  struct MinList	 dyn_NameServers;
  struct MinList	 dyn_Domains;
};

/*
 * GenEnt has the common part of all NetDataBase Nodes
 */
struct GenentNode {
  struct MinNode gn_Node;
  short          gn_EntSize;
  struct {
    char *dummy[0];
  }              gn_Ent;
};

/* NetDatabase nodes */
struct NameserventNode {
  struct MinNode  nsn_Node;
  short           nsn_EntSize;
  struct nameservent {
    struct in_addr ns_addr;
  } nsn_Ent;
};

struct DomainentNode {
  struct MinNode  dn_Node;
  short           dn_EntSize;
  struct domainent {
    char *d_name;
  } dn_Ent;
};

/* NetDataBase Nodes */
struct HostentNode {
  struct MinNode hn_Node;
  short          hn_EntSize;
  struct hostent hn_Ent;
};

struct NetentNode {
  struct MinNode nn_Node;
  short          nn_EntSize;
  struct netent  nn_Ent;
};

struct ServentNode {
  struct MinNode  sn_Node;
  short           sn_EntSize;
  struct servent  sn_Ent;
};

struct ProtoentNode {
  struct MinNode  pn_Node;
  short           pn_EntSize;
  struct protoent pn_Ent;
};

struct RcentNode {
  struct MinNode  rn_Node;
  short		  rn_EntSize;
  char *	  rn_Ent;
};

/*
 * netdatabase calls for some AmiTCP/IP functions
 */
struct ServentNode * findServentNode(struct NetDataBase * ndb,
				     const char * name, const char * proto);

/*
 * Read NetDB...
 */
LONG do_netdb(struct CSource *cs, UBYTE **errstrp, struct CSource *res);
LONG reset_netdb(struct CSource *cs, UBYTE **errstrp, struct CSource *res);
LONG init_netdb(void);
void netdb_deinit(void);
     
#endif /* AMIGA_NETDB_H */
