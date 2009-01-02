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

#ifndef API_APICALLS_H
#define API_APICALLS_H

typedef VOID (* f_void)(APTR args, ...);



#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdarg.h>

#ifndef IN_H
#include <netinet/in.h>
#endif
/*#endif*/

#include <api/amiga_api.h>


extern LONG __CloseSocket(LONG, struct SocketBase *);
extern LONG __connect(LONG, struct sockaddr *, LONG, struct SocketBase *);
extern LONG __recv(LONG, char *, LONG, LONG, struct SocketBase *);
extern LONG __send(LONG, char *, LONG, LONG, struct SocketBase *);
extern LONG __sendto(LONG, char *, LONG, LONG, struct sockaddr *, LONG, struct SocketBase *);
extern LONG __socket(LONG, LONG, LONG, struct SocketBase *);
extern LONG __IoctlSocket(LONG fdes, ULONG cmd, caddr_t data, struct SocketBase *libPtr);
extern LONG __WaitSelect(ULONG, fd_set *, fd_set *, fd_set *, struct timeval *, ULONG *, struct SocketBase *);
extern struct hostent * __gethostbyaddr(UBYTE *, int, int, struct SocketBase *);
extern LONG __gethostname(STRPTR, LONG, struct SocketBase *);
extern LONG __SetErrnoPtr(VOID *, UBYTE, struct SocketBase *);
extern LONG __inet_aton(STRPTR cp,  struct in_addr * addr);
extern char * __Inet_NtoA(ULONG, struct SocketBase *);
#define __inet_ntoa __Inet_NtoA
extern REGARGFUN void SetSysLogPort(void);
extern REGARGFUN void endndbent(struct SocketBase *);

#endif /* API_APICALLS_H */
