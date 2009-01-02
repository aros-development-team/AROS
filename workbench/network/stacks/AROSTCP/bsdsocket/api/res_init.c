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
#include <kern/amiga_includes.h>
#include <kern/amiga_netdb.h>
#include <api/resolv.h>

#ifndef AMITCP /* AmiTCP has this in the SocketBase */
struct state _res;
#endif

void res_cleanup_db(struct state *state)
{
   char **domain = NULL;

#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_cleanup_db()\n"));
#endif

	if (state->dnsrch) {
		for (domain = state->dnsrch; *domain; domain++)
			bsd_free(*domain, NULL);
		bsd_free(state->dnsrch, NULL);
		state->dnsrch = NULL;
	}
	if (state->nsaddr_list)
		bsd_free(state->nsaddr_list, NULL);
	state->options = NULL;
}

int res_update_db(struct state *state)
{
  struct DomainentNode *domain = NULL;
  struct NameserventNode *ns = NULL;
  int l;
  long opts;
  ULONG n = 1;

#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db()\n"));
#endif

  opts = state->options;
  res_cleanup_db(state);
  LOCK_R_NDB(NDB);
  /* Count number of domain names in the NetDB */
  for (domain = (struct DomainentNode *)NDB->ndb_Domains.mlh_Head;
       domain->dn_Node.mln_Succ;
       domain = (struct DomainentNode *)domain->dn_Node.mln_Succ)
  {
    n++;
  }
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: %d Domains in NetDB\n", n-1));
  ULONG tmp_n = n;
#endif

  ObtainSemaphoreShared(&DynDB.dyn_Lock);
  /* Add domain names from dynamic entries list */
  for (domain = (struct DomainentNode *)DynDB.dyn_Domains.mlh_Head;
       domain->dn_Node.mln_Succ;
       domain = (struct DomainentNode *)domain->dn_Node.mln_Succ)
  {
    n++;
  }

#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: %d Domains in DynDB\n", n-tmp_n-1));
#endif

  /* Allocate space for the array */
  state->dnsrch = bsd_malloc(n*sizeof(char *), NULL, NULL);
  if (!state->dnsrch) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: Failed to allocate array for dnsrch pointers\n"));  
#endif
    UNLOCK_NDB(NDB);
    ReleaseSemaphore(&DynDB.dyn_Lock);
    return -1;
  }
  /* Copy entries themselves, fill in the array */
  n = 0;
  for (domain = (struct DomainentNode *)NDB->ndb_Domains.mlh_Head;
     domain->dn_Node.mln_Succ;
     domain = (struct DomainentNode *)domain->dn_Node.mln_Succ) {
        l = strlen(domain->dn_Ent.d_name)+1;
        state->dnsrch[n] = bsd_malloc(l, NULL, NULL);
        if (!state->dnsrch[n]) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: Failed to allocate space for entry %d from NetDB entry '%s' name\n", n,
 domain->dn_Ent.d_name));
#endif
      UNLOCK_NDB(NDB);
      ReleaseSemaphore(&DynDB.dyn_Lock);
      res_cleanup_db(state);
      return -1;
    }
    strcpy(state->dnsrch[n++], domain->dn_Ent.d_name);
  }
  for (domain = (struct DomainentNode *)DynDB.dyn_Domains.mlh_Head;
     domain->dn_Node.mln_Succ;
     domain = (struct DomainentNode *)domain->dn_Node.mln_Succ) {
    l = strlen(domain->dn_Ent.d_name)+1;
    state->dnsrch[n] = bsd_malloc(l, NULL, NULL);
    if (!state->dnsrch[n]) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: Failed to allocate space for entry %d from DynDB entry '%s' name\n", n, domain->dn_Ent.d_name));
#endif
      UNLOCK_NDB(NDB);
      ReleaseSemaphore(&DynDB.dyn_Lock);

      res_cleanup_db(state);

      return -1;
    }
    strcpy(state->dnsrch[n++], domain->dn_Ent.d_name);
  }  /* Terminate the array */

  state->dnsrch[n] = NULL;
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: Last dnsrch array pointer (%d) marked empty\n", n));
#endif

  /* Count nameservers in the NetDB */
  n = 1;
  for (ns = (struct NameserventNode *)NDB->ndb_NameServers.mlh_Head;
       ns->nsn_Node.mln_Succ;
       ns = (struct NameserventNode *)ns->nsn_Node.mln_Succ)
  {
    n++;
  }

#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: %d Nameservers in NetDB\n", n-1));
   tmp_n = n;
#endif

  for (ns = (struct NameserventNode *)DynDB.dyn_NameServers.mlh_Head;
       ns->nsn_Node.mln_Succ;
       ns = (struct NameserventNode *)ns->nsn_Node.mln_Succ)
  {
    n++;
  }
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: %d Nameservers in DynDB\n", n-tmp_n-1));
#endif

  /* Allocate space for the array */
  state->nsaddr_list = bsd_malloc(n*sizeof(struct in_addr), NULL, NULL);
  if (!state->nsaddr_list) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_init.c) res_update_db: Failed to allocate array for nsaddr_list pointers\n"));  
#endif
    UNLOCK_NDB(NDB);
    ReleaseSemaphore(&DynDB.dyn_Lock);
    return -1;
  }

  /* Copy nameserver entries */
  n = 0;
  for (ns = (struct NameserventNode *)NDB->ndb_NameServers.mlh_Head;
       ns->nsn_Node.mln_Succ;
       ns = (struct NameserventNode *)ns->nsn_Node.mln_Succ)
  {
    state->nsaddr_list[n++].s_addr = ns->nsn_Ent.ns_addr.s_addr;
  }
  UNLOCK_NDB(NDB);

  for (ns = (struct NameserventNode *)DynDB.dyn_NameServers.mlh_Head;
       ns->nsn_Node.mln_Succ;
       ns = (struct NameserventNode *)ns->nsn_Node.mln_Succ)
  {
    state->nsaddr_list[n++].s_addr = ns->nsn_Ent.ns_addr.s_addr;
  }
  ReleaseSemaphore(&DynDB.dyn_Lock);
  /* Terminale the array */
  state->nsaddr_list[n].s_addr = NULL;
  /* Remember NetDB update count */
  state->dbserial = ndb_Serial;
  state->options = opts;
  return 0;
}

int
res_init(struct state *state)
{
  /* Fill in domain names and nameserver addresses */
  if (res_update_db(state)) return -1;

  /* Set up default options */
  state->retrans = RES_TIMEOUT;
  state->retry   = 3;
  state->options = RES_DEFAULT;
  return 0;
}

