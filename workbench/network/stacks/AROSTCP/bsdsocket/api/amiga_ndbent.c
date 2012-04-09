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

#include <aros/asmcall.h>
#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/synch.h>

#include <sys/time.h>
#include <sys/errno.h>

#include <sys/socket.h>
#include <net/route.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/allocdatabuffer.h>

#include <api/apicalls.h>

#include <net/if_protos.h>

#include <bsdsocket/socketbasetags.h>

#include <kern/uipc_domain_protos.h>
#include <kern/uipc_socket_protos.h>
#include <kern/uipc_socket2_protos.h>

#include <exec/semaphores.h>
#include <proto/exec.h>
#include <errno.h>
#include <api/miami_api.h>
#include <kern/amiga_netdb.h>

#include <syslog.h>

#include <proto/bsdsocket.h>

void sethtent(struct SocketBase *libPtr)
{
	ObtainSemaphoreShared (&ndb_Lock);
	libPtr->HostentNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
}

struct hostent *gethtent(struct SocketBase *libPtr)
{
	struct HostentNode *hn;

	hn = libPtr->HostentNode;
	if (!hn) {
		ObtainSemaphoreShared (&ndb_Lock);
		hn = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
	}
	if (hn->hn_Node.mln_Succ) {
		libPtr->HostentNode = (struct HostentNode *)hn->hn_Node.mln_Succ;
		return &hn->hn_Ent;
	} else
		return NULL;
}

void endhostent(struct SocketBase *libPtr)
{
	libPtr->HostentNode = NULL;
	ReleaseSemaphore (&ndb_Lock);
	return;
}

/* **** GETPROTOENT */

struct protoent *__getprotoent(struct SocketBase *libPtr)
{
	struct ProtoentNode *pn;

	DSYSCALLS(log(LOG_DEBUG,"getprotoent() called");)

	pn = libPtr->ProtoentNode;
	if (!pn) {
		ObtainSemaphoreShared (&ndb_Lock);
		pn = (struct ProtoentNode *)NDB->ndb_Protocols.mlh_Head;
	}
	if (pn->pn_Node.mln_Succ) {
		libPtr->ProtoentNode = (struct ProtoentNode *)pn->pn_Node.mln_Succ;
		return &pn->pn_Ent;
	} else
		return NULL;
}

#if defined(__CONFIG_ROADSHOW__)
AROS_LH0(struct protoent *,  getprotoent,
                struct SocketBase *, libPtr, 95, UL)
{
	AROS_LIBFUNC_INIT

	return __getprotoent(libPtr);

	AROS_LIBFUNC_EXIT
}
#endif

AROS_LH0(struct protoent *, Miami_getprotoent,
         struct MiamiBase *, MiamiBase, 14, Miami
)
{
	AROS_LIBFUNC_INIT

	return __getprotoent(MiamiBase->_SocketBase);

	AROS_LIBFUNC_EXIT
}

/* **** ENDPROTOENT */

void __endprotoent(struct SocketBase *libPtr)
{
	DSYSCALLS(log(LOG_DEBUG,"endprotoent() called");)
	libPtr->ProtoentNode = NULL;
	ReleaseSemaphore (&ndb_Lock);

	return;
}

#if defined(__CONFIG_ROADSHOW__)
AROS_LH0(void, endprotoent,
                 struct SocketBase *, libPtr, 94, UL)
{
	AROS_LIBFUNC_INIT

	__endprotoent(libPtr);

	AROS_LIBFUNC_EXIT
}
#endif

AROS_LH0(void, Miami_endprotoent,
         struct MiamiBase *, MiamiBase, 15, Miami
)
{
	AROS_LIBFUNC_INIT

	DSYSCALLS(log(LOG_DEBUG,"endprotoent() called");)
	__endprotoent(MiamiBase->_SocketBase);

	AROS_LIBFUNC_EXIT
}

/* **** */

AROS_LH0(void, ClearDynDomain,
         struct MiamiBase *, MiamiBase, 24, Miami
)
{
	AROS_LIBFUNC_INIT

	struct MinNode *node, *nnode;

	if (MiamiBase->DynDomain_Locked) {
		ObtainSemaphore (&DynDB.dyn_Lock);
		MiamiBase->DynDomain_Locked = 1;
	}
	for (node = DynDB.dyn_Domains.mlh_Head; node->mln_Succ;) {
		nnode = node->mln_Succ;
		bsd_free(node, NULL);
		node = nnode;
	}
	NewList((struct List *)&DynDB.dyn_Domains);

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void, ClearDynNameServ,
         struct MiamiBase *, MiamiBase, 9, Miami
)
{
	AROS_LIBFUNC_INIT

	struct MinNode *node, *nnode;

	if (MiamiBase->DynNameServ_Locked) {
		ObtainSemaphore (&DynDB.dyn_Lock);
		MiamiBase->DynNameServ_Locked = 1;
	}
	for (node = DynDB.dyn_NameServers.mlh_Head; node->mln_Succ;) {
		nnode = node->mln_Succ;
		bsd_free(node, NULL);
		node = nnode;
	}
	NewList((struct List *)&DynDB.dyn_NameServers);

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void, EndDynDomain,
         struct MiamiBase *, MiamiBase, 19, Miami
)
{
	AROS_LIBFUNC_INIT

	if (MiamiBase->DynDomain_Locked) {
		ReleaseSemaphore (&DynDB.dyn_Lock);
		MiamiBase->DynDomain_Locked = 0;
		if (!MiamiBase->DynNameServ_Locked)
			ndb_Serial++;
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH0(void, EndDynNameServ,
         struct MiamiBase *, MiamiBase, 20, Miami
)
{
	AROS_LIBFUNC_INIT

	if (MiamiBase->DynNameServ_Locked) {
		ReleaseSemaphore (&DynDB.dyn_Lock);
		MiamiBase->DynNameServ_Locked = 0;
		if (!MiamiBase->DynDomain_Locked)
			ndb_Serial++;
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AddDynNameServ,
         AROS_LHA(struct sockaddr_in *, entry, A0),
         struct MiamiBase *, MiamiBase, 21, Miami
)
{
	AROS_LIBFUNC_INIT

  struct NameserventNode *nsn;

  if (entry->sin_family != AF_INET)
    return EAFNOSUPPORT;
  if ((nsn = bsd_malloc(sizeof(struct NameserventNode), NULL, NULL)) == NULL) {
    writeErrnoValue(MiamiBase->_SocketBase, ENOMEM);
    return ENOMEM;
  }

  nsn->nsn_EntSize = sizeof (nsn->nsn_Ent);
  nsn->nsn_Ent.ns_addr.s_addr = entry->sin_addr.s_addr;

  if (MiamiBase->DynNameServ_Locked) {
	ObtainSemaphore (&DynDB.dyn_Lock);
	MiamiBase->DynNameServ_Locked = 1;
  }

  AddTail((struct List *)&DynDB.dyn_NameServers, (struct Node*)nsn);
  return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AddDynDomain,
         AROS_LHA(STRPTR, entry, A0),
         struct MiamiBase *, MiamiBase, 22, Miami
)
{
	AROS_LIBFUNC_INIT

  struct DomainentNode *dn;
  short  nodesize;

  nodesize = sizeof (*dn) + strlen(entry) + 1;
  if ((dn = bsd_malloc(nodesize, NULL, NULL)) == NULL) {
    writeErrnoValue(MiamiBase->_SocketBase, ENOMEM);
    return ENOMEM;
  }
  dn->dn_EntSize = nodesize - sizeof (struct GenentNode);
  dn->dn_Ent.d_name = (char *)(dn + 1);

  strcpy((char *)(dn + 1), entry);

  if (MiamiBase->DynDomain_Locked) {
	ObtainSemaphore (&DynDB.dyn_Lock);
	MiamiBase->DynDomain_Locked = 1;
  }

  AddTail((struct List *)&DynDB.dyn_Domains, (struct Node*)dn);
  return 0;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(struct hostent *, Miami_gethostent,
         struct MiamiBase *, MiamiBase, 10, Miami)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] amiga_ndbent.c: Miami_gethostent()\n"));
#endif

	return gethtent(MiamiBase->_SocketBase);

	AROS_LIBFUNC_EXIT
}


AROS_LH0(void, Miami_endhostent,
         struct MiamiBase *, MiamiBase, 12, Miami)
{
	AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP.MIAMI] amiga_ndbent.c: Miami_endhostent()\n"));
#endif

	endhostent(MiamiBase->_SocketBase);

	AROS_LIBFUNC_EXIT
}
