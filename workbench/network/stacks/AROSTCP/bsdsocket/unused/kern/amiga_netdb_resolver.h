/* The Definitions in this file are PRIVATE to the stack.
   No user applications should access them since
              they may and WILL change!!                   */

#ifndef __private_amiga_netdb_resolver_h
#define __private_amiga_netdb_resolver_h

#include <netinet/in.h>
#include <netdb.h>
#include <exec/lists.h>
#include <exec/nodes.h>

/* PRIVATE SocketBaseTags */

#define SBTC_PRIVATE_RES_STATE            0x3FFF /* Access to resolver state structure
                                                    for many funcs */

#define PRIVATE_STACK_RESOLVER_DISABLED   0x8000 /* Used to Implement MiamiDisallowDNS() */

/* Resolver state */
struct state
{
   int      retrans;                /* retransmition time interval */
   int      retry;                  /* number of times to retransmit */
   long     options;                /* option flags - see below. */
   u_short  id;                     /* current packet id */
   ULONG    dbserial;
   char     **dnsrch;
   struct in_addr *nsaddr_list;
};

/* Access control table item */
struct AccessItem {
  UWORD	ai_flags;
  UWORD	ai_port;
  ULONG	ai_host;
  ULONG ai_mask;
};

/* NetDataBase */
struct NetDataBase {
  struct MinList         ndb_Hosts;
  struct MinList         ndb_Networks;
  struct MinList         ndb_Services;
  struct MinList         ndb_Protocols;
  struct MinList         ndb_NameServers;
  struct MinList         ndb_Domains;
  LONG			 ndb_AccessCount; /* tmp var, but reduces code size */
  struct AccessItem *	 ndb_AccessTable;
};

struct DynDataBase
{
   struct SignalSemaphore dyn_Lock;
   struct MinList         dyn_NameServers;
   struct MinList         dyn_Domains;
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

#endif
