/****************************************************************************
**  File:       pipecreate.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added lock initialization to OpenPipe()
**				 for locks on individual pipes.
**		12-Feb-87	Fixed bug in OpenPipe(): previously ignored
**				 lock passed in packet.  Bug uncovered when
**				 pipes became lockable, and thus assignable.
**		26-Mar-87	Fixed bug in ClosePipe(): not closing r/w
**				 mode properly (extraneous else).
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
** pipecreate.c
** ------------
** This module handles opens and closes for pipes.
**
** Visible Functions
** -----------------
**	void  OpenPipe    (pkt, tapfh)
**	void  ClosePipe   (pkt)
**	void  DiscardPipe (pipe)
**
** Macros (in pipecreate.h)
** ------------------------
**	- none -
**
** Local Functions
** ---------------
**	int   TapFormsLoop (tapfh, pipe)
**	void  OpenTap      (pkt, tapname)
**	void  CloseTap     (tapfh)
*/



/*---------------------------------------------------------------------------
** OpenPipe() handles open requests.  The DosPacket from the client and the
** filehandle of the tap are sent.  If tapfh is 0, but the name sent
** indicates a tap is desired (see ParsePipeName() in pipename.c), then
** an OpenTap() request is initiated and OpenPipe() is immediately exited.
** Later, when the request returns (to HandleTapReply()), OpenPipe() is
** called again with the same client packet and the newly returned tapfh.
**      If tapfh is nonzero, or if it is zero but no tap is desired, then
** the an attempt to open the pipe is made.  If a existent pipe with a tap is
** to be opened and a new tapfh is given, the old tap is closed.
**      If the name's syntax is incorrect, then the request is returned
** unsuccessful.  Otherwise, if the pipe named by the request does not
** already exist, a new pipe is created (if there is enough memory).
** If it does exist, but it is already open for the mode requested, an error
** is returned (a maximum of one reader and one writer is allowed).
**      A successful open returns the client's filehandle with its Arg1 field
** pointing to a PIPEKEY, which in turn identifies the pipe and open mode.
**      Unless an OpenTap() is required, the packet is returned to the cleint
** by this function.  If an OpenTap() is required, it will be returned by the
** the later call to this function when the tap open request is returned.
**      Note: the code which checks if the lock sent in the packet relies on
** the fact that the pipe-handler does not allow subdirectories.  If a lock
** on a pipe is passed in, then that pipe is opened.  Otherwise, the name is
** parsed without reference to the lock.
*/

static int TapFormsLoop(BPTR tapfh, PIPEDATA *pipe);
static void CloseTap(BPTR tapfh);
static void OpenTap(struct DosPacket *pkt,const char *tapname);

void  OpenPipe (pkt, tapfh)

struct DosPacket  *pkt;
BPTR              tapfh;

{
  LONG               openmode;
  struct FileHandle  *handle;
  struct FileLock    *lock;
  char               *pipename = NULL, *tapname = NULL;
  ULONG              pipesize;
  PIPEKEY            *pipekey = NULL;
  PIPEDATA           *pipe = NULL;


  pkt->dp_Res1= 0;     /* error, for now */

  if (! ParsePipeName (BPTRtoCptr (pkt->dp_Arg3), &pipename, &pipesize, &tapname))
    { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
      goto OPENREPLY;
    }

  if ( (tapfh == 0) && (tapname != NULL) && (tapname[0] != '\0') )
    { OpenTap (pkt, tapname);     /* start tap open request */
      return;                     /* HandleTapReply() re-calls when request returns */
    }

  openmode= pkt->dp_Type;
  lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg2);
  pipe= NULL;

  if ( (lock == NULL) || ((pipe= (PIPEDATA *) lock->fl_Key) == NULL) )
    { if (pipename[0] == '\0')
#if AUTONAME
        pipename= get_autoname ((openmode == MODE_NEWFILE) || (openmode == MODE_READWRITE));
#else /* !AUTONAME */
        { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
          goto OPENREPLY;
        }
#endif /* AUTONAME */
      if (AUTONAME_STAR && pipename[0] == '*' && pipename[1] == 0) {
          pipename= get_autoname ((openmode == MODE_NEWFILE) || (openmode == MODE_READWRITE));
      }

      pipe= FindPipe (pipename);
    }
  else     /* packet's lock was on the pipe */
    { if (pipename[0] != '\0')
        { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
          goto OPENREPLY;
        }

      pipename= pipe->name;
    }


  handle= (struct FileHandle *) BPTRtoCptr (pkt->dp_Arg1);

  if ((pipekey= (PIPEKEY *) AllocMem (sizeof (PIPEKEY), ALLOCMEM_FLAGS)) == NULL)
    { pkt->dp_Res2= ERROR_NO_FREE_STORE;
      goto OPENREPLY;
    }


  if (pipe == NULL)     /* then PIPE NOT FOUND */
    { if (openmode == MODE_READONLY)
        { pkt->dp_Res2= ERROR_OBJECT_NOT_FOUND;
          goto OPENREPLY;
        }

      pkt->dp_Res2= ERROR_NO_FREE_STORE;     /* in case of AllocMem error */

      if ((pipe= (PIPEDATA *) AllocMem (sizeof (PIPEDATA), ALLOCMEM_FLAGS)) == NULL)
        goto OPENMEMERR1;

      if ((pipe->buf= AllocPipebuf (pipesize)) == NULL)
        goto OPENMEMERR2;

      if ((pipe->lock= (struct FileLock *) AllocMem (sizeof (struct FileLock), ALLOCMEM_FLAGS)) == NULL)
        { FreePipebuf (pipe->buf);
OPENMEMERR2:
          FreeMem (pipe, sizeof (PIPEDATA));
OPENMEMERR1:
          goto OPENREPLY;
        }

      l_strcpy (pipe->name, pipename);

      pipekey->pipe=     pipe;
      pipekey->openmode= openmode;

      if (openmode == MODE_READONLY)
        { pipekey->iotype= PIPEREAD;
          pipe->flags |=   OPEN_FOR_READ;
        }
      else if (openmode == MODE_NEWFILE)
        { pipekey->iotype= PIPEWRITE;
          pipe->flags=     OPEN_FOR_WRITE;
        }
      else     /* MODE_READWRITE */
        { pipekey->iotype= PIPERW;
          pipe->flags=     (OPEN_FOR_READ | OPEN_FOR_WRITE);
        }

      InitList (&pipe->readerlist);
      InitList (&pipe->writerlist);

      pipe->tapfh= tapfh;

#if PIPEDIR
      pipe->lockct= 0;
      InitLock (pipe->lock, pipe);
#endif /* PIPEDIR */

      InsertTail (&pipelist, pipe);     /* at tail for directory's sake */

#ifdef DEBUG
       OS ("*** created pipe '"); OS (pipe->name);
       OS ("'   [buflen "); OL (pipe->buf->len); OS ("]\n");
#endif /* DEBUG */
    }
  else     /* PIPE WAS FOUND */
    { if (TapFormsLoop (tapfh, pipe))
        { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
          goto OPENREPLY;
        }

      pipekey->pipe=     pipe;
      pipekey->openmode= openmode;

      pkt->dp_Res2= ERROR_OBJECT_IN_USE;     /* in case of openmode error */

      if (openmode == MODE_READONLY)
        { if (pipe->flags & OPEN_FOR_READ)
            goto OPENREPLY;

          pipekey->iotype= PIPEREAD;
          pipe->flags  |=  OPEN_FOR_READ;
        }
      else if (openmode == MODE_NEWFILE)
        { if (pipe->flags & OPEN_FOR_WRITE)
            goto OPENREPLY;

          pipekey->iotype= PIPEWRITE;
          pipe->flags  |=  OPEN_FOR_WRITE;
        }
      else     /* MODE_READWRITE */
        { if (pipe->flags & (OPEN_FOR_READ | OPEN_FOR_WRITE))
            goto OPENREPLY;

          pipekey->iotype= PIPERW;
          pipe->flags=     (OPEN_FOR_READ | OPEN_FOR_WRITE);
        }

      if (tapfh != 0)
        { if (pipe->tapfh != 0)
            CloseTap (pipe->tapfh);     /* close old tap first */

          pipe->tapfh= tapfh;
        }
    }


  handle->fh_Arg1= (IPTR) pipekey;     /* for identification on Read, Write, Close */
  pkt->dp_Res1= 1;
  pkt->dp_Res2= 0;     /* for successful open */


OPENREPLY:
  if (pkt->dp_Res1 == 0)     /* then there was an error */
    { if (pipekey != NULL)
        FreeMem (pipekey, sizeof (PIPEKEY));

      if (tapfh != 0)
        CloseTap (tapfh);
    }
#if PIPEDIR
   else
     SetPipeDate (pipe);
#endif /* PIPEDIR */

  QuickReplyPkt(pkt);
}



/*---------------------------------------------------------------------------
** This routine checks for "the old loop in the pipe trick" (86).  If the
** handler has a loop through its tap, the handler will endlessly pass
** packets to itself.  This would be disastrous if the handler is running at
** high priority.
*/

static int  TapFormsLoop (tapfh, pipe)

BPTR      tapfh;
PIPEDATA  *pipe;

{ struct FileHandle  *handle;
  PIPEKEY            *tapkey;
  PIPEDATA           *tappipe;
  int                numchecks;     /* protection */


  for (numchecks= 0; ((tapfh != 0) && (numchecks < 10000)); ++numchecks)
    { handle= (struct FileHandle *) BPTRtoCptr (tapfh);

      if (handle->fh_Type == PipePort)     /* then the tap is a pipe, too */
        { if ( ((tapkey= (PIPEKEY *) handle->fh_Arg1) == NULL) ||
               ((tappipe= tapkey->pipe) == NULL)                  )
            return FALSE;

          if (tappipe == pipe)
            return TRUE;

          tapfh= tappipe->tapfh;
        }
      else
        return FALSE;
    }

  return FALSE;
}



/*---------------------------------------------------------------------------
** The previous open performed on a pipe is terminated.  The PIPEKEY
** allocated for the client when the pipe was opened is freed.  Then,
** CheckWaiting() is called -- it will discard the pipe if it becomes empty
** and is not opened for read or write.
*/

void  ClosePipe (pkt)

struct DosPacket  *pkt;

{ PIPEKEY   *pipekey;
  PIPEDATA  *pipe;
  void      DeletePipe();


  pipekey= (PIPEKEY *) pkt->dp_Arg1;
  pipe= pipekey->pipe;

  if ((pipekey->iotype == PIPEREAD) || (pipekey->iotype == PIPERW))
    pipe->flags &= ~OPEN_FOR_READ;

  if ((pipekey->iotype == PIPEWRITE) || (pipekey->iotype == PIPERW))
    pipe->flags &= ~OPEN_FOR_WRITE;

  FreeMem (pipekey, sizeof (PIPEKEY));

  CheckWaiting (pipe);     /* will discard if empty */

  pkt->dp_Res1= 1;
  pkt->dp_Res2= 0;

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** Remove a pipe from the pipe list and release its memory.  The pipe is
** assumed empty and having no clients.
*/

void  DiscardPipe (pipe)

PIPEDATA  *pipe;

{
#ifdef DEBUG
  OS ("*** discarding pipe '"); OS (pipe->name); OS ("'\n");
#endif /* DEBUG */

  Delete (&pipelist, pipe);

  FreePipebuf (pipe->buf);
  FreeMem (pipe->lock, sizeof (struct FileLock));

  if (pipe->tapfh != 0)
    CloseTap (pipe->tapfh);

  FreeMem (pipe, sizeof (PIPEDATA));
}



/*---------------------------------------------------------------------------
** An open request for a tap is performed.  A WAITINGDATA structure is
** allocated to hold the client packet until later.  HandleTapReply() will
** deal with the reply and, if successful, re-call OpenPipe().
*/

static void  OpenTap (pkt, tapname)

struct DosPacket  *pkt;
const char        *tapname;

{ char               *Bname;
  struct FileHandle  *handle;
  WAITINGDATA        *wd;
  struct DosPacket   *tappkt;
  struct MsgPort     *Handler;
  struct FileLock    *Lock;
  void               StartTapIO();


  if ( (tapname == NULL) ||
       ((Bname= (char *) AllocMem (OPENTAP_STRSIZE, ALLOCMEM_FLAGS)) == NULL) )
    goto OPENTAPERR;

  if ((handle= (struct FileHandle *) AllocMem (sizeof (struct FileHandle), (ALLOCMEM_FLAGS | MEMF_CLEAR))) == NULL)
    goto OPENTAPERR1;

  if ((wd= (WAITINGDATA *) AllocMem (sizeof (WAITINGDATA), ALLOCMEM_FLAGS)) == NULL)
    goto OPENTAPERR2;

  if ((tappkt= AllocPacket (TapReplyPort)) == NULL)
    goto OPENTAPERR3;

  if ((Handler= DeviceProc (tapname)) == NULL)
    { FreePacket (tappkt);
OPENTAPERR3:
      FreeMem (wd, sizeof (WAITINGDATA));
OPENTAPERR2:
      FreeMem (handle, sizeof (struct FileHandle));
OPENTAPERR1:
      FreeMem (Bname, OPENTAP_STRSIZE);
OPENTAPERR:
      pkt->dp_Res1= 0;
      pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
      QuickReplyPkt (pkt);
      return;
    }

  Lock= (struct FileLock *) IoErr ();
  CstrtoBSTR (tapname, Bname, OPENTAP_STRSIZE);

  handle->fh_Pos= -1;
  handle->fh_End= -1;
  handle->fh_Type= Handler;     /* initialize file handle */

  wd->pkt= tappkt;
  wd->pktinfo.tapwait.clientpkt= pkt;
  wd->pktinfo.tapwait.handle= handle;     /* for HandleTapReply() */

  StartTapIO ( tappkt, ACTION_FINDOUTPUT,
               CptrtoBPTR (handle), CptrtoBPTR (Lock), CptrtoBPTR (Bname),
               Handler );

  InsertHead (&tapwaitlist, wd);
}



/*---------------------------------------------------------------------------
** A close request for a tap filehandle is initiated.  When HandleTapReply()
** gets the reply, it merely discards it.
*/

static void  CloseTap (tapfh)

BPTR  tapfh;

{ struct FileHandle  *taphandle;
  struct DosPacket   *tappkt;
  WAITINGDATA        *wd;
  void               StartTapIO();


  taphandle= (struct FileHandle *) BPTRtoCptr (tapfh);

  if ((tappkt= AllocPacket (TapReplyPort)) == NULL)
    goto CLOSETAPERR;

  if ((wd= (WAITINGDATA *) AllocMem (sizeof (WAITINGDATA), ALLOCMEM_FLAGS)) == NULL)
    { FreePacket (tappkt);
CLOSETAPERR:
      FreeMem (taphandle, sizeof (struct FileHandle));
#ifdef DEBUG
      OS ("!!! ERROR - CloseTap() failed\n");
#endif /* DEBUG */
      return;
    }

  wd->pkt= tappkt;
  /* don't need ...tapwait.clientpkt */
  wd->pktinfo.tapwait.handle= taphandle;     /* for HandleTapReply() */

  StartTapIO ( tappkt, ACTION_END,
               taphandle->fh_Arg1, 0, 0,
               taphandle->fh_Type );

  InsertHead (&tapwaitlist, wd);
}
