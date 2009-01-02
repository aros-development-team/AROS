/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * Includes changes (c) 2005 - 2006 Neil Cafferkey and Pavel Fedin
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

extern struct TagItem buffermanagement[];

#ifndef SYS_PARAM_H
#include <sys/param.h>
#endif

BOOL
ioip_alloc_mbuf(struct IOIPReq *s2rp, ULONG MTU);

/*
 * Allocate a new Sana-II IORequest for this task
 */
static inline struct IOSana2Req *
CreateIOSana2Req(register struct sana_softc *ssc)
{
  register struct IOSana2Req *req;
  register struct MsgPort *port;

  port = CreateMsgPort();
  if (!port) return NULL;

  req = CreateIORequest(port, sizeof(*req));
  if (!req) {
    DeleteMsgPort(port);
    return NULL;
  }

  if (ssc) {
    req->ios2_Req.io_Device    = ssc->ss_dev;
    req->ios2_Req.io_Unit      = ssc->ss_unit;
    req->ios2_BufferManagement = ssc->ss_bufmgnt;

    aligned_bcopy(ssc->ss_hwaddr, req->ios2_SrcAddr, ssc->ss_if.if_addrlen);
  }
  return req;
}

/*
 * Delete a Sana-II IORequest 
 */
static inline VOID 
DeleteIOSana2Req(register struct IOSana2Req *open)
{
  register struct MsgPort *port = open->ios2_Req.io_Message.mn_ReplyPort;

  DeleteIORequest((struct IORequest*) open);
  if (port)
    DeleteMsgPort(port);
}

