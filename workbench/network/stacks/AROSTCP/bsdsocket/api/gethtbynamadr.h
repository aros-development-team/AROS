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

#ifndef API_GETHTBYNAMADR_H
#define API_GETHTBYNAMADR_H

struct hostent *__gethostbyname(const char *name, struct SocketBase *libPtr);

struct hostent * _gethtbyname(struct SocketBase * libPtr,
			      const char * name);
struct hostent * _gethtbyaddr(struct SocketBase * libPtr,
			      const char * addr, int len, int type);

#endif /* API_GETHTBYNAMADR_H */

