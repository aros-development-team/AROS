/****************************************************************************
**  File:       pipesched.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added "lockct" check in CheckWaiting().
*/

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <libraries/filehandler.h>
#include   <exec/exec.h>

#include   "pipelists.h"
#include   "pipename.h"
#include   "pipebuf.h"
#include   "pipecreate.h"
#include   "pipesched.h"
#include   "pipe-handler.h"

#if PIPEDIR
# include   "pipedir.h"
#endif /* PIPEDIR */

#ifdef DEBUG
# include   "pipedebug.h"
#endif /* DEBUG */



/*---------------------------------------------------------------------------
** pipesched.c
** -----------
** This module handles pipe I/O scheduling.
**
** Visible Functions
** -----------------
**	void              StartPipeIO    (pkt, iotype)
**	void              CheckWaiting   (pipe)
**	struct DosPacket  *AllocPacket   (ReplyPort)
**	void              FreePacket     (pkt)
**	void              StartTapIO     (pkt, Type, Arg1, Arg2, Arg3, Handler)
**	void              HandleTapReply (pkt)
**
** Macros (in pipesched.h)
** -----------------------
**	- none -
**
** Local Functions
** ---------------
**	void  EndPipeIO (pipe, wd)
*/

static void EndPipeIO (PIPEDATA *pipe, WAITINGDATA *wd);



/*---------------------------------------------------------------------------
** A pipe I/O request is begun.  A WAITINGDATA structure is allocated and
** the request is stored in it.  It is then stored in the appropriate list
** (readerlist or writerlist) of the pipe.  Finally, CheckWaiting is called
** to service requests for that pipe.
**      Notice that CheckWaiting() is only called when a new I/O request
** comes in, or when the pipe is closed.  At no other time will the state of
** the pipe change in such a way that more requests for it can be honored.
*/

void  StartPipeIO (pkt, iotype)

struct DosPacket  *pkt;
IOTYPE            iotype;     /* assumed only PIPEREAD or PIPEWRITE */

{ PIPEKEY      *pipekey;
  PIPEDATA     *pipe;
  WAITINGDATA  *wd;


  if ((iotype != PIPEREAD) && (iotype != PIPEWRITE))
    { pkt->dp_Res2= ERROR_ACTION_NOT_KNOWN;
SPIOEXIT:
      pkt->dp_Res1= -1;
      QuickReplyPkt (pkt);
      return;
    }

      
  pipekey= (PIPEKEY *) pkt->dp_Arg1;
  pipe= pipekey->pipe;

  if ((wd= (WAITINGDATA *) AllocMem (sizeof (WAITINGDATA), ALLOCMEM_FLAGS)) == NULL)
    { pkt->dp_Res2= ERROR_NO_FREE_STORE;
      goto SPIOEXIT;
    }


  pkt->dp_Res2= ERROR_INVALID_LOCK;     /* in case not open for iotype */

  if (iotype == PIPEREAD)
    { if ((pipekey->iotype != PIPEREAD) && (pipekey->iotype != PIPERW))
        goto SPIOEXIT;

      InsertTail (&pipe->readerlist, wd);
    }
  else     /* PIPEWRITE */
    { if ((pipekey->iotype != PIPEWRITE) && (pipekey->iotype != PIPERW))
        goto SPIOEXIT;

      InsertTail (&pipe->writerlist, wd);
    }


  wd->pkt= pkt;
  wd->pktinfo.pipewait.reqtype= iotype;
  wd->pktinfo.pipewait.buf= (BYTE *) pkt->dp_Arg2;     /* buffer */
  wd->pktinfo.pipewait.len= (ULONG)  pkt->dp_Arg3;     /* length */

  CheckWaiting (pipe);
}



/*---------------------------------------------------------------------------
** Read requests for the pipe are satisfied until the pipe is empty or no
** more requests are left.  Then, write requests are satisifed until the pipe
** is full or no more requests are left.  This alternating process is
** repeated until no further changes are possible.
**      Finished requests are sent to EndPipeIO() so that replies may be sent
** to their owners.  Aftereward, if the pipe is empty and is not open for
** either read or write, then it is discarded.  If it is open for read, but
** is empty and has no write requests and is not open for write, then all
** remaining read requests are returned in their current state.  (This
** implements EOF.)  A pipe with a positive "lockct" will not be discarded.
** UnLock() is expected to call here so that a previously locked, empty pipe
** will be discarded.
*/

void  CheckWaiting (pipe)

PIPEDATA  *pipe;

{ BYTE         change;
  WAITINGDATA  *wd;
  ULONG        amt;
  void         EndPipeIO();


#if PIPEDIR
  SetPipeDate (pipe);
#endif /* PIPEDIR */

  for (change= TRUE; change; )
    { change= FALSE;

      while ( (! (PipebufEmpty (pipe->buf))) &&
              ((wd= (WAITINGDATA *) FirstItem (&pipe->readerlist)) != NULL) )
        { amt= MoveFromPipebuf (pipe->buf, wd->pktinfo.pipewait.buf, wd->pktinfo.pipewait.len);

          if (amt)
            { wd->pktinfo.pipewait.buf += amt;
              wd->pktinfo.pipewait.len -= amt;
              change= TRUE;
            }

          if (wd->pktinfo.pipewait.len == 0L)     /* then finished with request */
            EndPipeIO (pipe, wd);
        }     /* end of readerlist loop */


      while ( (! (PipebufFull (pipe->buf))) &&
              ((wd= (WAITINGDATA *) FirstItem (&pipe->writerlist)) != NULL) )
        { amt= MoveToPipebuf (pipe->buf, wd->pktinfo.pipewait.buf, wd->pktinfo.pipewait.len);

          if (amt)
            { wd->pktinfo.pipewait.buf += amt;
              wd->pktinfo.pipewait.len -= amt;
              change= TRUE;
            }

          if (wd->pktinfo.pipewait.len == 0L)     /* then finished with request */
            EndPipeIO (pipe, wd);
        }     /* end of writerlist loop */
    }


  if ( PipebufEmpty (pipe->buf)                &&
       (! (pipe->flags & OPEN_FOR_WRITE))      &&
       (FirstItem (&pipe->writerlist) == NULL)    )     /* then EOF */
    { while ((wd= (WAITINGDATA *) FirstItem (&pipe->readerlist)) != NULL)
        EndPipeIO (pipe, wd);

      if (! (pipe->flags & OPEN_FOR_READ))     /* readerlist is now empty */
#if PIPEDIR
        if (pipe->lockct == 0)
#endif /* PIPEDIR */
          DiscardPipe (pipe);
    }
}



/*---------------------------------------------------------------------------
** This routine returns a finished pipe I/O request.  If it is a write
** request to a pipe with a tap, then the same write request is sent to the
** tap, and the reply is deferred until HandleTapReply() gets the tap request
** reply.  (This lets the user stop a pipe by typing a character into a
** tap window.)
*/

static void  EndPipeIO (pipe, wd)

PIPEDATA     *pipe;
WAITINGDATA  *wd;

{ struct DosPacket   *pkt, *tappkt;
  struct FileHandle  *taphandle;


  pkt= wd->pkt;

  pkt->dp_Res1= pkt->dp_Arg3 - wd->pktinfo.pipewait.len;
  pkt->dp_Res2= 0;

  if (wd->pktinfo.pipewait.reqtype == PIPEREAD)
    { Delete (&pipe->readerlist, wd);

      QuickReplyPkt (pkt);
      FreeMem (wd, sizeof (WAITINGDATA));
    }
  else     /* must be PIPEWRITE -- reqtype is new PIPERW */
    { Delete (&pipe->writerlist, wd);

      if (pipe->tapfh != 0)     /* then write to the pipe tap */
        { if ((tappkt= AllocPacket (TapReplyPort)) == NULL)
            { QuickReplyPkt (pkt);
              FreeMem (wd, sizeof (WAITINGDATA));
#ifdef DEBUG
              OS ("!!! ERROR - Could not allocate packet for tap write\n");
#endif /* DEBUG */
            }
          else
            { wd->pkt= tappkt;     /* reuse wd for tap write request */
              wd->pktinfo.tapwait.clientpkt= pkt;
              /* don't need ...tapwait.handle */

              taphandle= (struct FileHandle *) BPTRtoCptr (pipe->tapfh);

              StartTapIO ( tappkt, ACTION_WRITE,
                           taphandle->fh_Arg1, pkt->dp_Arg2, pkt->dp_Arg3,
                           taphandle->fh_Type );

              InsertHead (&tapwaitlist, wd);     /* for HandleTapReply() */
            }
        }
      else     /* otherwise, return finished packet */
        { QuickReplyPkt (pkt);
          FreeMem (wd, sizeof (WAITINGDATA));
        }
    }
}



/*---------------------------------------------------------------------------
** An exec Message and a DosPacket are allocated, and they are initialized.
** A pointer to the packet is returned, or NULL if it could not be allocated.
*/

struct DosPacket  *AllocPacket (ReplyPort)

struct MsgPort  *ReplyPort;

{ struct Message    *msg;
  struct DosPacket  *pkt;


  if ((msg = (struct Message *) AllocMem (sizeof (struct Message), (ALLOCMEM_FLAGS | MEMF_CLEAR))) == NULL)
    return NULL;

  if ((pkt = (struct DosPacket *) AllocMem (sizeof (struct DosPacket), (ALLOCMEM_FLAGS | MEMF_CLEAR))) == NULL)
    { FreeMem (msg, sizeof (struct Message));
      return NULL;
    }

  msg->mn_Node.ln_Type= NT_MESSAGE;
  msg->mn_Node.ln_Name= (char *) pkt;

  msg->mn_ReplyPort= ReplyPort;

  pkt->dp_Link= msg;
  pkt->dp_Port= ReplyPort;

  return pkt;
}



/*---------------------------------------------------------------------------
** A DosPacket/exec Message pair is freed.
*/

void  FreePacket (pkt)

struct DosPacket  *pkt;

{ if (pkt != NULL)
    { if (pkt->dp_Link != NULL)
        FreeMem (pkt->dp_Link, sizeof (struct Message));

      FreeMem (pkt, sizeof (struct DosPacket));
    }
}



/*---------------------------------------------------------------------------
** The indicated fields are filled into the packet and it is sent.
*/

void  StartTapIO (pkt, Type, Arg1, Arg2, Arg3, Handler)

struct DosPacket  *pkt;
LONG              Type;
LONG              Arg1;
LONG              Arg2;
LONG              Arg3;
struct MsgPort    *Handler;

{ pkt->dp_Type= Type;
  pkt->dp_Arg1= Arg1;
  pkt->dp_Arg2= Arg2;
  pkt->dp_Arg3= Arg3;

  PutMsg (Handler, pkt->dp_Link);
}



/*---------------------------------------------------------------------------
** Handle replies from tap I/O requests.  These were initiated by OpenTap(),
** CloseTap() and EndPipeIO().
*/

void  HandleTapReply (pkt)

struct DosPacket  *pkt;

{ WAITINGDATA  *wd;


  for (wd= (WAITINGDATA *) FirstItem (&tapwaitlist); wd != NULL; wd= (WAITINGDATA *) NextItem (wd))
    if (wd->pkt == pkt)
      { Delete (&tapwaitlist, wd);
        break;
      }

  if (wd == NULL)
    {
#ifdef DEBUG
      OS ("!!! ERROR - WAITINGDATA not found in HandleTapReply()\n");
#endif /* DEBUG */
      FreePacket (pkt);
      return;     /* not found - this should never happen */
    }

  switch (pkt->dp_Type)
    { case MODE_READWRITE:
      case MODE_READONLY:
      case MODE_NEWFILE:     /* for a tap open request */
             if (pkt->dp_Res1)     /* then successful */
               OpenPipe (wd->pktinfo.tapwait.clientpkt, pkt->dp_Arg1);
             else     /* couldn't open tap */
               { FreeMem (wd->pktinfo.tapwait.handle, sizeof (struct FileHandle));
                 pkt->dp_Res1= 0;
                 pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
                 QuickReplyPkt (wd->pktinfo.tapwait.clientpkt);
               }

             FreeMem (BPTRtoCptr (pkt->dp_Arg3), OPENTAP_STRSIZE);
             break;

      case ACTION_END:     /* for a tap close request */
             FreeMem (wd->pktinfo.tapwait.handle, sizeof (struct FileHandle));
             break;

      case ACTION_WRITE:     /* for a tap write request */
             QuickReplyPkt (wd->pktinfo.tapwait.clientpkt);     /* return to client */
             break;

#ifdef DEBUG
      default:     /* should never happen */
             OS ("!!! ERROR - bad packet type in HandleTapReply(), type ="); OL (pkt->dp_Type); NL;
#endif /* DEBUG */
    }

  FreePacket (pkt);
  FreeMem (wd, sizeof (WAITINGDATA));
}
