/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
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

#ifndef NET_SANA2ERRNO_H
#define NET_SANA2ERRNO_H

extern const short io_nerr;
extern const char * const io_errlist[]; 
extern const short sana2io_nerr;
extern const char * const sana2io_errlist[];
extern const short sana2wire_nerr;
extern const char * const sana2wire_errlist[];

void sana2perror(const char *banner, struct IOSana2Req *ios2);
void Sana2PrintFault(const char *banner, struct IOSana2Req *ios2);

#endif
