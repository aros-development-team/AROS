/*
 * $Id$
 *
 * Simplified doublebuffered AmigaDOS IO.
 * 
 * Author: Tomi Ollila <Tomi.Ollila@nsdi.fi>
 *
 * 	Copyright (c) 1994 Network Solutions Development Inc. OY
 * 		    	All rights reserved
 *
 * Created: Thu Sep 15 06:11:55 1994 too
 * Last modified: Thu Sep 15 08:33:18 1994 too
 *
 * HISTORY 
 * $Log: pkts.c,v $
 * Revision 1.1.1.2  2005/12/07 10:50:34  sonic_amiga
 * First full import into the CVS
 *
 * Revision 1.1.1.1  2005/11/08 13:51:05  sonic_amiga
 * Initial partial import upon request
 *
 * Revision 1.1  1996/01/21 01:26:36  jraja
 * Initial revision
 *
 */

#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <string.h>		/* memset() */
#include <errno.h>

#if __GNUC__
#include <inline/exec.h>
#include <inline/dos.h>
#error ios1 is SAS/C specific
#elif __SASC
#include <proto/exec.h>
#include <proto/dos.h>
#include <ios1.h>		/* SAS/C, chkufb() */
#else
#error :foo
#endif

#include "pkts.h"

extern int __io2errno(long);

int
initPktInfo(struct pktinfo * pkti, int fd, char * buf1, char * buf2, int size)
{
  struct DosPacket * pkt;
  struct MsgPort * port;
  struct UFB * ufb;
  struct FileHandle * fh;

  memset(pkti, 0, sizeof(*pkti));
 
  /*
   * get AmigaDOS file handle from SAS/C file handle
   */
  ufb = chkufb(fd);
  if (ufb == NULL || ufb->ufbfh == NULL) {
    errno = EBADF;
    return -1;
  }
  /* convert DOS file handle to struct FileHandle * */
  fh = BADDR((BPTR)ufb->ufbfh);
  pkti->fhType = fh->fh_Type;

  port = CreateMsgPort();
  if (port != NULL) {
    pkti->port = port;

    pkt = (struct DosPacket *)AllocDosObject(DOS_STDPKT, 0);
    if (pkt != NULL) {
      pkti->pkt = pkt;

      pkti->bufs[0] = buf1;
      pkti->bufs[1] = buf2;
      pkti->whichbuf = 0;

      pkt->dp_Arg1 = fh->fh_Arg1;

      /*
       * Do the initial action. Do nothing if file is NIL:
       */
      if (pkti->fhType == NULL) {
	pkt->dp_Res1 = 0; /* keep everyone happy */
	pkt->dp_Arg3 = 0;
	AddTail(&port->mp_MsgList, (struct Node *)pkt->dp_Link);
      }
      else {
	if (size == 0) {
	  pkt->dp_Type = ACTION_WRITE;
	  AddTail(&port->mp_MsgList, (struct Node *)pkt->dp_Link);
	}
	else {
	  pkt->dp_Arg2 = (LONG)buf1;
	  pkt->dp_Arg3 = size;
	  pkt->dp_Type = ACTION_READ;
	  SendPkt(pkt, pkti->fhType, port);
	}
      }
      return 0;
    }

    DeleteMsgPort(port);
  }

  errno = ENOMEM;
  return -1;
}

int
deInitPktInfo(struct pktinfo * pkti)
{
  int rv;
  /*
   * Take the packet out of the port, save the return code
   */
  WaitPort(pkti->port);
  rv = pkti->pkt->dp_Res1;
  /*
   * Set errno if error
   */
  if (rv < 0 || 
      (pkti->pkt->dp_Type == ACTION_WRITE && rv < pkti->pkt->dp_Arg3)) {
    errno = __io2errno(_OSERR = pkti->pkt->dp_Res2);
    rv = -1;
  }
  
  pkti->fhType = NULL;
  
  FreeDosObject(DOS_STDPKT, pkti->pkt);
  DeleteMsgPort(pkti->port);

  memset(pkti, 0, sizeof(*pkti));

  return rv;
}

#define msgToPkt(msg) ((struct DosPacket *)msg->mn_Node.ln_Name)

int
readPkt(struct pktinfo * pkti, char ** buf)
{
  struct DosPacket * pkt;
  int rv;
  
  /*
   * If file is NIL:, return 0 to end the reading
   */
  if (pkti->fhType == NULL)
    return 0;
  
  WaitPort(pkti->port);
  pkt = msgToPkt(GetMsg(pkti->port));

  rv = pkt->dp_Res1;

  if (rv > 0) {
    *buf = pkti->bufs[pkti->whichbuf];
    pkti->whichbuf ^= 1;

    pkt->dp_Arg2 = (LONG)pkti->bufs[pkti->whichbuf];
    SendPkt(pkt, pkti->fhType, pkti->port);
  }
  else {
    if (rv < 0)
      errno = __io2errno(_OSERR = pkt->dp_Res2);
    pkt->dp_Res1 = 0; /* do not receive same error again */
    AddTail(&pkti->port->mp_MsgList, (struct Node *)pkt->dp_Link);
  }
  
  return rv;
}

int
writePkt(struct pktinfo * pkti, char ** buf, int len)
{
  struct DosPacket * pkt;
  
  WaitPort(pkti->port);
  pkt = msgToPkt(GetMsg(pkti->port));

  if (pkt->dp_Res1 < pkt->dp_Arg3) {
    errno = __io2errno(_OSERR = pkt->dp_Res2);
    pkt->dp_Res1 = pkt->dp_Arg3 = 0; /* do not receive same error again */
    AddTail(&pkti->port->mp_MsgList, (struct Node *)pkt->dp_Link);
    return -1;
  }

/* Assert (*buf ==  pkti->whichbuf); */

  pkt->dp_Arg2 = (LONG)*buf;
  pkt->dp_Arg3 = len;

  if (pkti->fhType != NULL)
    SendPkt(pkt, pkti->fhType, pkti->port);
  else {
    pkt->dp_Res1 = len;		/* writing to NIL: always succeeds */
    AddTail(&pkti->port->mp_MsgList, (struct Node *)pkt->dp_Link);
  }

  pkti->whichbuf ^= 1;
  *buf = pkti->bufs[pkti->whichbuf];

  return len;
}
