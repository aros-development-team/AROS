/****************************************************************************
**  File:       pipe-handler.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.2
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added shared locks for individual pipes.
**				PIPEDATA structure modified to include
**				 a FileLock structure.
**		07-Feb-87	Added #if's forautomatic pipe naming "feature"
**				 for pipes specified with empty names.
**		12-Feb-87	Added ParentDir packet handling.
**		12-Feb-87	Fixed bug in OpenPipe() and PipeLock():
**				 they previously ignored the lock passed in
**				 packet.  Bug uncovered when pipes became
**				 lockable, and thus assignable.
**		27-Mar-87	Added the case for PipeDupLock().  This was
**				 missing in the original version!
**		28-Mar-87	Added code to handler() to remove ':' from
**				 end of handler name.  This caused problems
**				 with Examine(); it expects no ending  ':'.
*/

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <libraries/filehandler.h>
#include   <exec/exec.h>

#include   <proto/exec.h>
#include   <proto/dos.h>
#include   <proto/alib.h>

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
** pipe-handler.c
** --------------
** This is the main module for the handler.  Handlers are started with
** register D1 containing a BPTR to a startup packet, which in turn contains
** (BCPL) pointers to the name and DeviceNode.  Since the entry, handler(),
** expects a byte address of the startup packet, an assembly language startup
** must be used to convert the BCPL pointer, and pass it on the stack.
**
** Problems arise if a handler tries to do I/O via the DOS functions Open(),
** Close(), Read() and Write().  DOS sends request packets to the handler
** via its DOS port (the one whose address forms the process ID).  This is
** also the port used by the I/O functions.  Therefore, if a request comes,
** and then an Open() call is performed, DOS will send a request packet for
** the open and erroneously pick up the request packet meant for the handler
** as its reply.  A crash ensues.
**
** This is the reason for the I/O functions in pipedebug.c.  They implement
** the regular I/O calls, but use a different ReplyPort.  With no debugging,
** these functions are unneeded, since all of the handler's normal I/O is
** performed asynchronously, using PutMsg().
**
** An alternate solution is to patch the handler's Task field with a new port
** instead of the handler's DOS port.  This works, except that DOS always
** sends the initial request packets to the DOS port (when the handler is
** first started).  This is probably because DeviceProc(), upon seeing that
** the handler has not yet been loaded, returns the result from its call to
** CreateProc() for the handler process.  Only on subsequent calls to
** DeviceProc() will the patched field be returned.  The upshot of this is
** that an alternate port can be used for handler requests, but there are
** always an unspecified number that may come over the DOS port regardless.
** Note that since not all handlers patch their Task field (because they want
** to be restarted each time), DOS is doing the "right" thing, or at least
** the best it can.
**
** Visible Functions
** -----------------
**	void      handler   (StartPkt)
**	PIPEDATA  *FindPipe (name)
**
** Macros (in pipe-handler.h)
** --------------------------
**	BPTRtoCptr (Bp)
**	CptrtoBPTR (Cp)
**	QuickReplyPkt   (pkt)
**
** Local Functions
** ---------------
**	struct DosPacket  *QuickGetPkt (port)
*/



/*---------------------------------------------------------------------------
** HandlerName  : passed as a BSTR in startup packet Arg1, our device name.
**		Everything from the ':' and beyond is removed.
**		Used by PipeExamine() for the handler's "directory" name.
**
** DevNode	: passed as a BPTR in startup packet Arg3.  This is a pointer
**		to our DeviceNode entry in the system device list (DevInfo).
**
** PipePort	: our DOS MsgPort, as well as our process ID.  See above for
**		notes about why we can't let DOS use this.
**
** pipelist	: the list of currently existing pipes.  PIPEDATA nodes are
**		linked into this list.
**
** tapwaitlist	: the list of requests waiting on tap opens/closes/writes.
**		WAITINGDATA nodes are linked into this list.  See pipesched.c
**		and pipecreate.c.
**
** TapReplyPort	: this is the MsgPort to which tap I/O replys are returned.
**
** SysBase,
** DOSBase	: Standard system library pointers.  Since we don't have the
**		usual startup code, we must initialize these ourselves.
**
** PipeDate	: If compiled with PIPEDIR true, the handler responds to some
**		directory-like actions.  This is the date for the entire
**		handler, i.e., the directory date.  The flag UPDATE_PIPEDATE
**		controls whether this date is updated with each pipe access
**		(true) or not (false).  See SetPipeDate() and PipeExamine().
*/

char               HandlerName[30];
struct DeviceNode  *DevNode   =  NULL;
struct MsgPort     *PipePort  =  NULL;

PIPELISTHEADER     pipelist;

PIPELISTHEADER     tapwaitlist;
struct MsgPort     *TapReplyPort  =  NULL;

#ifndef __AROS__
struct Library     *SysBase  =  NULL;
#endif
struct DosLibrary  *DOSBase  =  NULL;

#if PIPEDIR
  struct DateStamp   PipeDate;
#endif /* PIPEDIR */

static struct DosPacket *QuickGetPkt (register struct MsgPort  *port);


/*---------------------------------------------------------------------------
** Performs initialization, replies to startup packet, and dispatches
** incoming request packets to the apropriate functions.  The TapReplyPort is
** also monitored for returning requests which were sent out by the handler.
** These returned requests are routed to HandleTapReply().
**      Our DeviceNode Task field is patched with our process ID so that this
** process is used for subsequent handler requests.  The function exits only
** if there is some initialization error.
*/

void  handler (StartPkt)

struct DosPacket  *StartPkt;

{ char              *cp;
  struct Task       *Task;
  ULONG             PipeMask, TapReplyMask, WakeupMask, SigMask;
  struct DosPacket  *pkt;

#ifndef __AROS__
  SysBase= AbsExecBase;
#endif

  if ((DOSBase= (APTR)OpenLibrary (DOSNAME, 0)) == NULL)
    goto QUIT;

  BSTRtoCstr (BPTRtoCptr (StartPkt->dp_Arg1), HandlerName, sizeof (HandlerName));
  for (cp= HandlerName; *cp != '\0'; ++cp)
    if (*cp == ':')      /* remainder of handler's first refernece follows */
      { *cp= '\0';
        break;
      }

  Task= FindTask (NULL);
  PipePort= &((struct Process *)Task)->pr_MsgPort;
  ((struct Process *) Task)->pr_CurrentDir= 0;     /* initial file system root */

  if ((TapReplyPort= CreatePort (NULL, PipePort->mp_Node.ln_Pri)) == NULL)
    goto QUIT;

#ifdef DEBUG
  if (! InitDebugIO (PipePort->mp_Node.ln_Pri))
    goto QUIT;
#endif /* DEBUG */


  PipeMask=     (1L << PipePort->mp_SigBit);
  TapReplyMask= (1L << TapReplyPort->mp_SigBit);
  WakeupMask=   (PipeMask | TapReplyMask);

  DevNode= (struct DeviceNode *) BPTRtoCptr (StartPkt->dp_Arg3);
  DevNode->dn_Task= PipePort;

  InitList (&pipelist);
  InitList (&tapwaitlist);

#if PIPEDIR
  (void) DateStamp (&PipeDate);
#endif /* PIPEDIR */

  StartPkt->dp_Res1 = DOSTRUE;
  QuickReplyPkt (StartPkt);


LOOP:
  SigMask= Wait (WakeupMask);

  if (SigMask & TapReplyMask)
    while ((pkt= QuickGetPkt (TapReplyPort)) != NULL)
      HandleTapReply (pkt);

  if (SigMask & PipeMask)
    while ((pkt= QuickGetPkt (PipePort)) != NULL)
      switch (pkt->dp_Type)
        { case MODE_READWRITE:
#ifdef DEBUG
  OS ("Open READWRITE packet received\n");
#endif /* DEBUG */
            OpenPipe (pkt, 0);
            break;

          case MODE_READONLY:     /* syn: MODE_OLDFILE, ACTION_FINDINPUT */
#ifdef DEBUG
  OS ("Open READONLY packet received\n");
#endif /* DEBUG */
            OpenPipe (pkt, 0);
            break;

          case MODE_NEWFILE:     /* syn: ACTION_FINDOUTPUT */
#ifdef DEBUG
  OS ("Open NEWFILE packet received\n");
#endif /* DEBUG */
            OpenPipe (pkt, 0);
            break;

          case ACTION_END:
#ifdef DEBUG
  OS ("Close packet received\n");
#endif /* DEBUG */
            ClosePipe (pkt);
            break;

          case ACTION_READ:
#ifdef DEBUG
  OS ("<<< Read packet received\n");
#endif /* DEBUG */
            StartPipeIO (pkt, PIPEREAD);
            break;

          case ACTION_WRITE:
#ifdef DEBUG
  OS (">>> Write packet received\n");
#endif /* DEBUG */
            StartPipeIO (pkt, PIPEWRITE);
            break;

#if PIPEDIR
          case ACTION_LOCATE_OBJECT:
#  ifdef DEBUG
     OS (  "Lock packet received\n");
#  endif /* DEBUG */
            PipeLock (pkt);
            break;

          case ACTION_FH_FROM_LOCK:
#  ifdef DEBUG
     OS (  "FHFromLock packet received\n");
#  endif /* DEBUG */
            PipeFHFromLock (pkt);
            break;

          case ACTION_COPY_DIR:
#  ifdef DEBUG
     OS (  "DupLock packet received\n");
#  endif /* DEBUG */
            PipeDupLock (pkt);
            break;

          case ACTION_COPY_DIR_FH:
#  ifdef DEBUG
     OS (  "DupLockFH packet received\n");
#  endif /* DEBUG */
            PipeDupLockFH (pkt);
            break;

          case ACTION_FREE_LOCK:
#  ifdef DEBUG
     OS (  "UnLock packet received\n");
#  endif /* DEBUG */
            PipeUnLock (pkt);
            break;

          case ACTION_EXAMINE_OBJECT:
#  ifdef DEBUG
     OS (  "Examine packet received\n");
#  endif /* DEBUG */
            PipeExamine (pkt);
            break;

          case ACTION_EXAMINE_NEXT:
#  ifdef DEBUG
     OS (  "ExNext packet received\n");
#  endif /* DEBUG */
            PipeExNext (pkt);
            break;

          case ACTION_EXAMINE_FH:
#  ifdef DEBUG
     OS (  "ExFH packet received\n");
#  endif /* DEBUG */
            PipeExFH (pkt);
            break;

          case ACTION_PARENT:
#  ifdef DEBUG
     OS (  "ParentDir packet received\n");
#  endif /* DEBUG */
            PipeParentDir (pkt);
            break;

          case ACTION_PARENT_FH:
#  ifdef DEBUG
     OS (  "ParentFH packet received\n");
#  endif /* DEBUG */
            PipeParentFH (pkt);
            break;
#endif /* PIPEDIR */

          default:
#ifdef DEBUG
  OS ("BAD packet received, type = "); OL (pkt->dp_Type); NL;
#endif /* DEBUG */
            pkt->dp_Res1= 0;
            pkt->dp_Res2= ERROR_ACTION_NOT_KNOWN;
            QuickReplyPkt (pkt);
        }

  goto LOOP;


QUIT:
  DevNode->dn_Task= NULL;     /* bad if someone in process of accessing us . . . */

  if (TapReplyPort != NULL)
    FreeMem (TapReplyPort, sizeof (struct MsgPort));     /* signal bit won't matter */

#ifdef DEBUG
  CleanupDebugIO ();
#endif /* DEBUG */

  if (DOSBase != NULL)
    CloseLibrary ((APTR)DOSBase);
}



/*---------------------------------------------------------------------------
** Returns the DosPacket associated with the next message on "port", or NULL
** if the port is empty.  The message is removed from the port.
** A related macro, QuickReplyPkt() is provided in pipe-handler.h.
*/
static struct DosPacket  *QuickGetPkt (port)

register struct MsgPort  *port;

{ register struct Message  *msg;

  return  ((msg= GetMsg (port)) == NULL)
            ? NULL
            : (struct DosPacket *) msg->mn_Node.ln_Name;
}



/*---------------------------------------------------------------------------
** Searches "pipelist" for a pipe whose name is "name".  If found, a pointer
** to the pipe returns.  Otherwise, NULL returns.
*/

PIPEDATA  *FindPipe (name)

char  *name;

{ PIPEDATA  *p;
  char      *cp;


  for (p= (PIPEDATA *) FirstItem (&pipelist); p != NULL; p= (PIPEDATA *) NextItem (p))
    { cp= strdiff (name, p->name);

      if ((*cp == '\0') && (p->name[(const UBYTE *)cp - (const UBYTE *)name] == '\0'))
        return p;     /* same name */
    }

  return NULL;     /* no match found */
}
