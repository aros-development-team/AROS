/*
 * $Id$
 *
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SYSTEM_HEADERS_H
#define _SYSTEM_HEADERS_H 1

/*****************************************************************************/

#define __USE_INLINE__
#define __NOGLOBALIFACE__
#define __NOLIBBASE__

/****************************************************************************/

#if defined(__SASC)
#define USE_BUILTIN_MATH
#endif /* __SASC */

/*****************************************************************************/

#define NULL ((APTR)0L)
#include <exec/types.h>

/*****************************************************************************/

#define byte IGNORE_THIS

/*****************************************************************************/

#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>
#include <dos/exall.h>

#include <exec/memory.h>

#include <devices/timer.h>
#include <devices/inputevent.h>
#include <devices/input.h>

#include <libraries/locale.h>

#if defined(__AROS__)
#include <bsdsocket/socketbasetags.h>
#endif

#include <utility/date.h>
#include <utility/tagitem.h>

/*****************************************************************************/

#include <clib/alib_protos.h>

/*****************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/bsdsocket.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/timer.h>
#include <proto/icon.h>

/*****************************************************************************/

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#if defined(__AROS__)
#define	EPROCLIM	67		/* Too many processes */
#endif
/*#include <sys/param.h>*/
/*#include <sys/ioctl.h>*/
#include <sys/stat.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <net/if.h>
#include <unistd.h>
#include <netdb.h>

/*****************************************************************************/

#undef byte

/*****************************************************************************/

#endif /* _SYSTEM_HEADERS_H */
