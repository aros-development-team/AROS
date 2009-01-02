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

#include <conf.h>

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <kern/amiga_netdb.h>
#include <api/allocdatabuffer.h>

#include <api/gethtbynamadr.h> /* prototypes (NO MORE BUGS HERE) */

int strcasecmp(const char *, const char *);

#if __SASC
#define strcasecmp stricmp
#endif


static long copyGenent(struct SocketBase * libPtr, 
		       struct DataBuffer * DB,
		       struct GenentNode *ent)
{
  long diff;

  if (allocDataBuffer(DB, ent->gn_EntSize) == FALSE) {
    writeErrnoValue(libPtr, ENOMEM);
    return 0;
  }

  /*
   * how much to add to old pointers
   */
  diff = (caddr_t)DB->db_Addr - (caddr_t)&ent->gn_Ent;

  /*
   * copy given ent verbatim
   */
  bcopy((caddr_t)&ent->gn_Ent, DB->db_Addr, ent->gn_EntSize);

  return diff;
}

 
/*
 * Host queries if Nameserver is not in use *****************************
 */

/*
 * Match name in aliaslist, used by `...byname()' -queries
 */

static BOOL matchAlias(char ** aliases, const char * name)
{
  for ( ; *aliases != 0; aliases++)
    if (strcasecmp(*aliases, name) == 0)
      return TRUE;

  return FALSE;
}
  

/*
 * Makehostent() must be called when NDB_Semaphore is obtained.
 */
static struct hostent * makehostent(struct SocketBase * libPtr,
				    struct HostentNode * ent)
{
  long diff;
  short i;

  if ((diff = copyGenent(libPtr, &libPtr->hostents,
			 (struct GenentNode *)ent)) == 0)
    return NULL; /* failed to allocate memory */

  /*
   * patch pointers
   */
#define HOSTENT ((struct hostent *)libPtr->hostents.db_Addr)
  HOSTENT->h_name += diff;

  HOSTENT->h_aliases = (char **)((caddr_t)HOSTENT->h_aliases + diff);
  for (i = 0; HOSTENT->h_aliases[i]; i++)
    HOSTENT->h_aliases[i] += diff;
  /* NULL remains null */

  HOSTENT->h_addr_list = (char **)((caddr_t)HOSTENT->h_addr_list + diff);
  for (i = 0; HOSTENT->h_addr_list[i]; i++)
    HOSTENT->h_addr_list[i] += diff;
  /* NULL remains null */

  return HOSTENT;
#undef HOSTENT
}


struct hostent * _gethtbyname(struct SocketBase * libPtr,
			      const char * name)
{
  struct HostentNode * entNode;
  struct hostent * host;

  LOCK_R_NDB(NDB);

  for (entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
       entNode->hn_Node.mln_Succ;
       entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ)
    if (strcasecmp(entNode->hn_Ent.h_name, (char *)name) == 0 ||
	matchAlias(entNode->hn_Ent.h_aliases, name)) {
      host = makehostent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return host;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
}


struct hostent * _gethtbyaddr(struct SocketBase * libPtr,
			      const char * addr, int len, int type)
{
  struct HostentNode * entNode;
  struct hostent * host;

  LOCK_R_NDB(NDB);
  for (entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
       entNode->hn_Node.mln_Succ;
       entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ)
    if (entNode->hn_Ent.h_addrtype == type &&
	! bcmp(entNode->hn_Ent.h_addr, addr, len)) {
      host = makehostent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return host;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
}


/*
 * Network queries ****************************************************
 */


/*
 * Makenetent() must be called when NDB_Semaphore is obtained.
 */
static struct netent * makenetent(struct SocketBase * libPtr,
				  struct NetentNode * ent)
{
  long diff;
  short i;

  if ((diff = copyGenent(libPtr, &libPtr->netents,
			 (struct GenentNode *)ent)) == 0)
    return NULL;

  /*
   * patch pointers
   */
#define NETENT ((struct netent *)libPtr->netents.db_Addr)
  NETENT->n_name += diff;

  NETENT->n_aliases = (char **)((caddr_t)NETENT->n_aliases + diff);
  for (i = 0; NETENT->n_aliases[i]; i++)
    NETENT->n_aliases[i] += diff;
  /* NULL remains null */

  return NETENT;
#undef NETENT
}  


/*struct netent * SAVEDS getnetbyname(
   REG(a0, const char * name),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH1(struct netent *, getnetbyname,
   AROS_LHA(const char *, name, A0),
   struct SocketBase *, libPtr, 34, UL)
{
  AROS_LIBFUNC_INIT
  struct NetentNode * entNode;
  struct netent * net;
  
  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  for (entNode = (struct NetentNode *)NDB->ndb_Networks.mlh_Head;
       entNode->nn_Node.mln_Succ;
       entNode = (struct NetentNode *)entNode->nn_Node.mln_Succ)
    if (strcmp(entNode->nn_Ent.n_name, name) == 0 ||
	matchAlias(entNode->nn_Ent.n_aliases, name)) {
      net = makenetent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return net;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}

/*struct netent * SAVEDS getnetbyaddr(
   REG(d0, long netw),
   REG(d1, long type),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH2(struct netent *, getnetbyaddr,
   AROS_LHA(long, netw, D0),
   AROS_LHA(long, type, D1),
   struct SocketBase *, libPtr, 35, UL)
{
  AROS_LIBFUNC_INIT
  struct NetentNode * entNode;
  struct netent * net;

  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  for (entNode = (struct NetentNode *)NDB->ndb_Networks.mlh_Head;
       entNode->nn_Node.mln_Succ;
       entNode = (struct NetentNode *)entNode->nn_Node.mln_Succ)
    if (entNode->nn_Ent.n_addrtype == type && entNode->nn_Ent.n_net == netw) {
      net = makenetent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return net;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}


/*
 * Service queries ****************************************************
 */

/*
 * Makeservent() must be called when NDB_Semaphore is obtained.
 */
static struct servent * makeservent(struct SocketBase * libPtr,
				    struct ServentNode * ent)
{
  long diff;
  short i;

  if ((diff = copyGenent(libPtr, &libPtr->servents,
			 (struct GenentNode *)ent)) == 0)
    return NULL;

  /*
   * patch pointers
   */
#define SERVENT ((struct servent *)libPtr->servents.db_Addr)
  SERVENT->s_name += diff;

  SERVENT->s_aliases = (char **)((caddr_t)SERVENT->s_aliases + diff);
  for (i = 0; SERVENT->s_aliases[i]; i++)
    SERVENT->s_aliases[i] += diff;
  /* NULL remains null */

  SERVENT->s_proto += diff;

  return SERVENT;
#undef SERVENT
}  


/*
 * findservent is needed for external call.
 */
struct ServentNode * findServentNode(struct NetDataBase * ndb,
				     const char * name, const char * proto)
{
  struct ServentNode * entNode;
  
  for (entNode = (struct ServentNode *)ndb->ndb_Services.mlh_Head;
       entNode->sn_Node.mln_Succ;
       entNode = (struct ServentNode *)entNode->sn_Node.mln_Succ)
    if ((strcmp(entNode->sn_Ent.s_name, name) == 0 ||
	 matchAlias(entNode->sn_Ent.s_aliases, name)) &&
	(proto == NULL || strcmp(entNode->sn_Ent.s_proto, proto) == 0))
      return entNode;

  return NULL;
}
  


/*struct servent * SAVEDS getservbyname(
   REG(a0, const char * name),
   REG(a1, const char * proto),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH2(struct servent *, getservbyname,
   AROS_LHA(const char *, name, A0),
   AROS_LHA(const char *, proto, A1),
   struct SocketBase *, libPtr, 36, UL)
{
  AROS_LIBFUNC_INIT
  struct ServentNode * entNode;
  struct servent * serv;
  
  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  if ((entNode = findServentNode(NDB, name, proto)) != NULL) {
    serv = makeservent(libPtr, entNode);
    UNLOCK_NDB(NDB);
    return serv;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}

/*struct servent * SAVEDS getservbyport(
   REG(d0, LONG port),
   REG(a0, const char * proto),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH2(struct servent *, getservbyport,
   AROS_LHA(LONG, port, D0),
   AROS_LHA(const char *, proto, A0),
   struct SocketBase *, libPtr, 37, UL)
{
  AROS_LIBFUNC_INIT
  struct ServentNode * entNode;
  struct servent * serv;
  
  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  for (entNode = (struct ServentNode *)NDB->ndb_Services.mlh_Head;
       entNode->sn_Node.mln_Succ;
       entNode = (struct ServentNode *)entNode->sn_Node.mln_Succ)
    if (entNode->sn_Ent.s_port == port &&
	(proto == NULL || strcmp(entNode->sn_Ent.s_proto, proto) == 0)) {
      serv = makeservent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return serv;
    }

  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}


/*
 * Protocol queries ****************************************************
 */

/*
 * Makeprotoent() must be called when NDB_Semaphore is obtained.
 */
static struct protoent * makeprotoent(struct SocketBase * libPtr,
				      struct ProtoentNode * ent)
{
  long diff;
  short i;

  if ((diff = copyGenent(libPtr, &libPtr->protoents,
			 (struct GenentNode *)ent)) == 0)
    return NULL;

  /*
   * patch pointers
   */
#define PROTOENT ((struct protoent *)libPtr->protoents.db_Addr)
  PROTOENT->p_name += diff;

  PROTOENT->p_aliases = (char **)((caddr_t)PROTOENT->p_aliases + diff);
  for (i = 0; PROTOENT->p_aliases[i]; i++)
    PROTOENT->p_aliases[i] += diff;
  /* NULL remains null */

  return PROTOENT;
#undef PROTOENT
}  

/*struct protoent * SAVEDS getprotobyname(
   REG(a0, const char * name),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH1(struct protoent *, getprotobyname,
   AROS_LHA(const char *, name, A0),
   struct SocketBase *, libPtr, 38, UL)
{
  AROS_LIBFUNC_INIT
  struct ProtoentNode * entNode;
  struct protoent * proto;
  
  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  for (entNode = (struct ProtoentNode *)NDB->ndb_Protocols.mlh_Head;
       entNode->pn_Node.mln_Succ;
       entNode = (struct ProtoentNode *)entNode->pn_Node.mln_Succ)
    if (strcmp(entNode->pn_Ent.p_name, name) == 0 ||
	matchAlias(entNode->pn_Ent.p_aliases, name)) {
      proto = makeprotoent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return proto;
    }
  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}

/*struct protoent * SAVEDS getprotobynumber(
   REG(a0, long protoc),
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH1(struct protoent *, getprotobynumber,
   AROS_LHA(long, protoc, A0),
   struct SocketBase *, libPtr, 39, UL)
{
  AROS_LIBFUNC_INIT
  struct ProtoentNode * entNode;
  struct protoent * proto;

  CHECK_TASK2();

  LOCK_R_NDB(NDB);
  for (entNode = (struct ProtoentNode *)NDB->ndb_Protocols.mlh_Head;
       entNode->pn_Node.mln_Succ;
       entNode = (struct ProtoentNode *)entNode->pn_Node.mln_Succ)
    if (entNode->pn_Ent.p_proto == protoc) {
      proto = makeprotoent(libPtr, entNode);
      UNLOCK_NDB(NDB);
      return proto;
    }

  UNLOCK_NDB(NDB);
  writeErrnoValue(libPtr, 0);
  return NULL;
  AROS_LIBFUNC_EXIT
}
