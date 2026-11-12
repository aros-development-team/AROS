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

/*
 * The hosts table's half of gethostbyname() and gethostbyaddr():
 * _gethtbyname(), _gethtbyname2(), _gethtbyaddr() and their makehostent().
 *
 * Moved here unchanged from api/getxbyy.c, so that the resolver can become a
 * module of its own (Stage 7, package R). copyGenent() and matchAlias() are
 * copies of the ones that stay there for the services, protocols and
 * networks: a module library has its own (two short functions, against two
 * entries in the core's table and a call through each).
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
#include <api/resolver_ops.h>

#include <net/netcore_map.h>        /* always the last include */

/*
 * Names compared without regard to case, in ASCII as names in the DNS are
 * (RFC 4343): strcasecmp() of the C library would bring its character
 * tables (about 1 KB) into the module for this. Also used by
 * api/gethostnamadr.c.
 */
int
res_strcasecmp(const char *a, const char *b)
{
    unsigned char ca, cb;

    do {
        ca = *a++;
        cb = *b++;
        if(ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if(cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
    } while(ca == cb && ca != '\0');
    return (int)ca - (int)cb;
}


static long copyGenent(struct SocketBase *libPtr,
                       struct DataBuffer *DB,
                       struct GenentNode *ent)
{
    long diff;

    if(res_allocdatabuffer(DB, ent->gn_EntSize) == FALSE) {
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

static BOOL matchAlias(char **aliases, const char *name)
{
    for(; *aliases != 0; aliases++)
        if(res_strcasecmp(*aliases, name) == 0)
            return TRUE;

    return FALSE;
}


/*
 * Makehostent() must be called when NDB_Semaphore is obtained.
 */
static struct hostent *makehostent(struct SocketBase *libPtr,
                                   struct HostentNode *ent)
{
    long diff;
    short i;

    if((diff = copyGenent(libPtr, &libPtr->hostents,
                          (struct GenentNode *)ent)) == 0)
        return NULL; /* failed to allocate memory */

    /*
     * patch pointers
     */
#define HOSTENT ((struct hostent *)libPtr->hostents.db_Addr)
    HOSTENT->h_name += diff;

    HOSTENT->h_aliases = (char **)((caddr_t)HOSTENT->h_aliases + diff);
    for(i = 0; HOSTENT->h_aliases[i]; i++)
        HOSTENT->h_aliases[i] += diff;
    /* NULL remains null */

    HOSTENT->h_addr_list = (char **)((caddr_t)HOSTENT->h_addr_list + diff);
    for(i = 0; HOSTENT->h_addr_list[i]; i++)
        HOSTENT->h_addr_list[i] += diff;
    /* NULL remains null */

    return HOSTENT;
#undef HOSTENT
}


/*
 * The host table holds entries of both address families (a HOST line may
 * give an IPv4 or an IPv6 address); a lookup by name is for one of them.
 */
struct hostent *_gethtbyname2(struct SocketBase *libPtr,
                              const char *name, int af)
{
    struct HostentNode *entNode;
    struct hostent *host;

    LOCK_R_NDB(NDB);

    for(entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
            entNode->hn_Node.mln_Succ;
            entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ)
        if(entNode->hn_Ent.h_addrtype == af &&
                (res_strcasecmp(entNode->hn_Ent.h_name, name) == 0 ||
                 matchAlias(entNode->hn_Ent.h_aliases, name))) {
            host = makehostent(libPtr, entNode);
            UNLOCK_NDB(NDB);
            return host;
        }
    UNLOCK_NDB(NDB);
    writeErrnoValue(libPtr, 0);
    return NULL;
}

struct hostent *_gethtbyname(struct SocketBase *libPtr,
                             const char *name)
{
    struct HostentNode *entNode;
    struct hostent *host;

    LOCK_R_NDB(NDB);

    for(entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
            entNode->hn_Node.mln_Succ;
            entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ)
        if(entNode->hn_Ent.h_addrtype == AF_INET &&
                (res_strcasecmp(entNode->hn_Ent.h_name, name) == 0 ||
                 matchAlias(entNode->hn_Ent.h_aliases, name))) {
            host = makehostent(libPtr, entNode);
            UNLOCK_NDB(NDB);
            return host;
        }
    UNLOCK_NDB(NDB);
    writeErrnoValue(libPtr, 0);
    return NULL;
}


struct hostent *_gethtbyaddr(struct SocketBase *libPtr,
                             const char *addr, int len, int type)
{
    struct HostentNode *entNode;
    struct hostent *host;

    LOCK_R_NDB(NDB);
    for(entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
            entNode->hn_Node.mln_Succ;
            entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ)
        if(entNode->hn_Ent.h_addrtype == type &&
                ! bcmp(entNode->hn_Ent.h_addr, addr, len)) {
            host = makehostent(libPtr, entNode);
            UNLOCK_NDB(NDB);
            return host;
        }
    UNLOCK_NDB(NDB);
    writeErrnoValue(libPtr, 0);
    return NULL;
}
