/*
 * Copyright (C) 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this file; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#ifndef SYSLOG_H
#define SYSLOG_H

#include <sys/syslog.h>
#include <exec/ports.h>

#if defined(__MORPHOS__)
#pragma pack(2)
#endif

struct SysLogPacket
{
   struct Message    Msg;
   ULONG             Level;
   ULONG             Time;
   STRPTR            Tag;
   STRPTR            String;
};

#define LOG_CLOSE    0xff000000

#if defined(__MORPHOS__)
#pragma pack()
#endif

#endif /* !SYSLOG_H */
