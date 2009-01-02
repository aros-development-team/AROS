/*
 * Copyright (C) 1993,1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                         Helsinki University of Technology, Finland.
 *                         All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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

#ifndef ACCESSCONTROL_H
#define ACCESSCONTROL_H

#include <sys/syslog.h>
#ifndef AMIGA_NETDB_H
#include <kern/amiga_netdb.h>
#endif

int controlaccess(struct in_addr shost, unsigned short sport);

static inline void setup_accesscontroltable(struct NetDataBase * ndb)
{
  void * new_AccessTable;
  *((ULONG *)&ndb->ndb_AccessTable[ndb->ndb_AccessCount]) = 0; /*mark default*/

  new_AccessTable =
    bsd_realloc(ndb->ndb_AccessTable,
		ndb->ndb_AccessCount * sizeof (struct AccessItem) +
		sizeof (ULONG), M_NETDB, M_WAITOK);
  if (new_AccessTable)
  {
    ndb->ndb_AccessTable = new_AccessTable;
#ifdef DEBUG_NETDB
    __log(7, "Reallocated accesscontroltable to 0x%08x:", new_AccessTable);
    {
      int i;
#define host ndb->ndb_AccessTable[i].ai_host
#define mask ndb->ndb_AccessTable[i].ai_mask
      for (i = 0; i < ndb->ndb_AccessCount; i++)
        __log(7, "%ld %ld.%ld.%ld.%ld/%ld.%ld.%ld.%ld %lx",
	  ndb->ndb_AccessTable[i].ai_port, 
	  host>>24 & 0xff, host>>16 & 0xff, host>>8 & 0xff, host & 0xff,
	  mask>>24 & 0xff, mask>>16 & 0xff, mask>>8 & 0xff, mask & 0xff,
	  ndb->ndb_AccessTable[i].ai_flags);
    
      __log(7, "%ld %ld", ndb->ndb_AccessTable[i].ai_flags,
	/*                 */ ndb->ndb_AccessTable[i].ai_port);
#undef mask
#undef host    
    }
#endif
  }
  else
    __log(LOG_EMERG, "Memory exhausted while reallocating access control table");
}
#endif /* ACCESSCONTROL_H */

