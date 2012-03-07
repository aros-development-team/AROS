/****************************************************************************
**  File:       pipedir.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.2
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added modifications for allowing shared locks
**				 on individual pipes.
**		12-Feb-87	Added PipeParentDir.
**		12-Feb-87	Fixed bug in PipeLock(): previously ignored
**				 lock passed in packet.  Bug uncovered when
**				 pipes became lockable, and thus assignable.
**		27-Mar-87	Added PipeDupLock().  This was missing
**				 in the original version!
*/

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <libraries/filehandler.h>
#include   <exec/exec.h>

#include   <proto/exec.h>
#include   <proto/dos.h>

#include   "pipelists.h"
#include   "pipename.h"
#include   "pipebuf.h"
#include   "pipecreate.h"
#include   "pipesched.h"
#include   "pipe-handler.h"
#include   "pipedir.h"



/*---------------------------------------------------------------------------
** pipedir.c
** ---------
** This module handles the directory-related requests to the handler.
** The functions contained here are not needed if the compile-time flag
** PIPEDIR is false.
**
** Visible Functions
** -----------------
**	void  SetPipeDate (pipe)
**	void  PipeLock    (pkt)
**	void  PipeDupLock (pkt)
**	void  PipeUnLock  (pkt)
**	void  PipeExamine (pkt)
**	void  PipeExNext  (pkt)
**	void  InitLock    (lock, key)
**
** Macros (in pipedir.h)
** ---------------------
**	- none -
**
** Local Functions
** ---------------
**	void  InitPipedirLock ()
**	void  FillFIB (fib, DiskKey, FileName, Protection, Type, Size, NumBlocks, Datep)
*/

static void InitPipedirLock (void);
static void FillFIB (
		struct FileInfoBlock  *fib,
		SIPTR                 DiskKey,
		char                  *FileName,     /* null-terminated */
		LONG                  Protection,
		LONG                  Type,
		LONG                  Size,
		LONG                  NumBlocks,
		struct DateStamp      *Datep);




/*---------------------------------------------------------------------------
** "PipedirLock" is the lock returned by PipeLock() to clients requesting a
** shared lock on the handler.  "LockBytes" is used for the storage of the
** lock.  InitLock() sets "PipedirLock" to point to the first longword within
** "LockBytes" to ensure longword alignment for BCPL's sake.
*/

static BYTE             LockBytes[sizeof (struct FileLock) + 3];
static struct FileLock  *PipedirLock  =  NULL;


/*---------------------------------------------------------------------------
** SetPipeDate() modifies the date field for the pipe sent.  If the compile-
** time flag UPDATE_PIPEDATE is true (see pipe-handler.h), the handler's date
** is modified as well.
*/

void  SetPipeDate (pipe)

PIPEDATA  *pipe;

{ (void) DateStamp (&pipe->accessdate);

#if UPDATE_PIPEDATE
  (void) DateStamp (&PipeDate);
#endif /* UPDATE_PIPEDATE */
}



/*---------------------------------------------------------------------------
** PipeLock() responds to Lock requests.  Only multiple access locks are
** granted.  The same lock is returned to all clients for a given entity.
**      Note: the code which checks if the lock sent in the packet relies on
** the fact that the pipe-handler does not allow subdirectories.  If a lock
** on a pipe is passed in, then that pipe is opened.  Otherwise, the name is
** parsed without reference to the lock.
*/

void  PipeLock (pkt)

struct DosPacket  *pkt;

{ char             *name, *tapname;
  ULONG            size;
  struct FileLock  *lock;
  PIPEDATA         *pipe;
  void             InitPipedirLock();


  InitPipedirLock ();

  pkt->dp_Res1= 0;     /* error, for now */
  pkt->dp_Res2= 0;     /* clear for case of no error */

  lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1);

  if (pkt->dp_Arg3 != SHARED_LOCK)
    { pkt->dp_Res2= ERROR_OBJECT_WRONG_TYPE;
      goto PLOCKEXIT;
    }

  if (! ParsePipeName (BPTRtoCptr (pkt->dp_Arg2), &name, &size, &tapname))
    { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
      goto PLOCKEXIT;
    }

  if ( (lock == NULL) || ((pipe= (PIPEDATA *) lock->fl_Key) == NULL) )
    { if (name[0] == '\0')
        pkt->dp_Res1= (SIPTR)CptrtoBPTR (PipedirLock);
      else
        { if ((pipe= FindPipe (name)) == NULL)
            { pkt->dp_Res2= ERROR_OBJECT_NOT_FOUND;
              goto PLOCKEXIT;
            }

          pkt->dp_Res1= (SIPTR)CptrtoBPTR (pipe->lock);
          ++pipe->lockct;
        }
    }
  else     /* lock sent in packet was on the pipe */
    { if (name[0] != '\0')
        { pkt->dp_Res2= ERROR_INVALID_COMPONENT_NAME;
          goto PLOCKEXIT;
        }

      pkt->dp_Res1= (SIPTR)CptrtoBPTR (pipe->lock);
      ++pipe->lockct;
    }

PLOCKEXIT:
  QuickReplyPkt (pkt);
}


/*---------------------------------------------------------------------------
** PipeFHFromLock() responds to FHFromLock requests.  It is assumed that the
** lock sent is valid.
*/

void  PipeFHFromLock (pkt)

struct DosPacket  *pkt;

{ struct FileLock  *lock;
  PIPEDATA         *pipe;
  PIPEKEY          *pipekey;
  struct FileHandle *fh;


  pkt->dp_Res1= DOSFALSE;
  pkt->dp_Res2= ERROR_OBJECT_NOT_FOUND;

  if ((fh= (struct FileHandle *) BPTRtoCptr (pkt->dp_Arg1)) != NULL) {
      if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg2)) != NULL) {
          if ((pipe= (PIPEDATA *) lock->fl_Key) != NULL) {
              if ((pipekey= (PIPEKEY *) AllocMem (sizeof (PIPEKEY), ALLOCMEM_FLAGS)) != NULL) {
                  pipekey->pipe = pipe;
                  pipekey->openmode = (lock->fl_Access == ACCESS_READ) ? OPEN_FOR_READ : OPEN_FOR_WRITE;
                  pipekey->iotype = (lock->fl_Access == ACCESS_READ) ? PIPEREAD : PIPEWRITE;
                  fh->fh_Arg1 = (SIPTR)pipekey;
                  pkt->dp_Res1= DOSTRUE;
                  pkt->dp_Res2= 0;
              }
          }
      }
  }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeDupLock() responds to DupLock requests.  It is assumed that the lock
** sent is valid.  The same lock is returned; the only action taken is to
** increment the lock count if the lock is on an individual pipe.  If the
** zero lock is sent, the zero lock is (properly) returned, even though this
** handler should never receive that request.  Notice that this routine never
** returns an error.
*/

void  PipeDupLock (pkt)

struct DosPacket  *pkt;

{ struct FileLock  *lock;
  PIPEDATA         *pipe;


  pkt->dp_Res1= pkt->dp_Arg1;     /* reuse same structure */
  pkt->dp_Res2= 0;

  if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1)) != NULL)
    { if ((pipe= (PIPEDATA *) lock->fl_Key) != NULL)
        ++pipe->lockct;     /* lock is on an individual pipe */
    }

  QuickReplyPkt (pkt);
}


/*---------------------------------------------------------------------------
** PipeDupLockFH() responds to DupLockFH requests.  It is assumed that the fh
** sent is valid.  The lock for the fh is returned; the only action taken is to
** increment the lock count if the lock is on an individual pipe.
*/

void  PipeDupLockFH (pkt)

struct DosPacket  *pkt;

{ PIPEKEY          *pipekey;
  PIPEDATA         *pipe;

  pkt->dp_Res1= 0;
  pkt->dp_Res2= ERROR_OBJECT_NOT_FOUND;


  if ((pipekey= (PIPEKEY *) (pkt->dp_Arg1)) != NULL)
    { if ((pipe= pipekey->pipe) != NULL) {
        ++pipe->lockct;     /* lock is on an individual pipe */
        pkt->dp_Res1= (SIPTR)MKBADDR(pipe->lock);
        pkt->dp_Res2= 0;
    }
  }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeUnLock() responds to UnLock requests.
*/

void  PipeUnLock (pkt)

struct DosPacket  *pkt;

{ struct FileLock  *lock;
  PIPEDATA         *pipe;


  if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1)) == NULL)
    { pkt->dp_Res1= 0;
      pkt->dp_Res2= ERROR_INVALID_LOCK;
    }
  else
    { if ((pipe= (PIPEDATA *) lock->fl_Key) != NULL)
        { --pipe->lockct;
          CheckWaiting (pipe);     /* will discard if totally unused */
        }

      pkt->dp_Res1= 1;     /* no error */
      pkt->dp_Res2= 0;
    }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeExamine() responds to Examine requests.  For locks on the handler, the
** address first item of the pipelist is stored in the DiskKey field for
** PipeExNext()'s reference.
*/

void  PipeExamine (pkt)

struct DosPacket  *pkt;

{ struct FileInfoBlock  *fib;
  struct FileLock       *lock;
  PIPEDATA              *pipe;


  pkt->dp_Res1= 1;     /* no error, for now */
  pkt->dp_Res2= 0;

  fib= (struct FileInfoBlock *) BPTRtoCptr (pkt->dp_Arg2);

  if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1)) == NULL)
    { pkt->dp_Res1= 0;
      pkt->dp_Res2= ERROR_OBJECT_NOT_FOUND;
    }
  else
    { if ((pipe= (PIPEDATA *) lock->fl_Key) == NULL)     /* then this is a lock on the handler */
        { FillFIB ( fib, (SIPTR)FirstItem (&pipelist), HandlerName,
                    (FIBF_EXECUTE | FIBF_DELETE), ST_ROOT,
                    0, 0, &PipeDate );
        }
      else
        { FillFIB ( fib, (SIPTR)NULL, pipe->name,
                    (FIBF_EXECUTE | FIBF_DELETE), ST_PIPEFILE,
                    pipe->buf->len, 1, &pipe->accessdate );
        }
    }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeExNext() responds to ExNext requests.  The DiskKey field of the
** FileInfoBlock is assumed to be a pointer to the next pipe in the pipelist
** which is to  be listed in the directory.  We then scan pipelist for this
** pointer, and upon finding it, store its information in the FileInfoBlock
** and store the address of the next pipe in pipelist in the DiskKey field.
** If the pipe is not found in the list, or if DiskKey is NULL, then
** ERROR_NO_MORE_ENTRIES is returned.
**      By rescanning the list each time, deletion of a pipe cannot hurt us
** by causing a dangling pointer in DiskKey -- we just end the directory
** listing there.  This can cause incomplete directory information for the
** cleint, however, if the last listed pipe is deleted before the client's
** next ExNext() call.
*/

void  PipeExNext (pkt)

struct DosPacket  *pkt;

{ struct FileLock       *lock;
  struct FileInfoBlock  *fib;
  PIPEDATA              *listitem, *pipe;


  pkt->dp_Res1= 0;     /* error, for now */

  if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1)) == NULL)
    { pkt->dp_Res2= ERROR_INVALID_LOCK;
      goto EXNEXTREPLY;
    }

  if (lock->fl_Key != (IPTR)NULL)     /* then an individual pipe */
    { pkt->dp_Res2= ERROR_OBJECT_WRONG_TYPE;
      goto EXNEXTREPLY;
    }

  pkt->dp_Res2= ERROR_NO_MORE_ENTRIES;     /* until found otherwise */

  fib= (struct FileInfoBlock *) BPTRtoCptr (pkt->dp_Arg2);

  if ((listitem= (PIPEDATA *) fib->fib_DiskKey) == NULL)
    goto EXNEXTREPLY;


  for (pipe= (PIPEDATA *) FirstItem (&pipelist); pipe != NULL; pipe= (PIPEDATA *) NextItem (pipe))
    if (listitem == pipe)
      break;

  if (listitem == pipe)     /* then found next entry */
    { FillFIB ( fib, (SIPTR)NextItem (listitem), listitem->name,
                (FIBF_EXECUTE | FIBF_DELETE), ST_PIPEFILE,
                listitem->buf->len, 1, &listitem->accessdate );

      pkt->dp_Res1= 1;
      pkt->dp_Res2= 0;
    }

EXNEXTREPLY:
  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeExFH() responds to ExFH requests.
*/

void  PipeExFH (pkt)

struct DosPacket  *pkt;

{ struct FileInfoBlock  *fib;
  PIPEKEY               *pipekey;
  PIPEDATA              *pipe;

  pkt->dp_Res1= 0;     /* error, for now */

  if ((pipekey= (PIPEKEY *) pkt->dp_Arg1) == NULL)
    { pkt->dp_Res2= ERROR_OBJECT_WRONG_TYPE;
      goto EXFHREPLY;
    }

  fib= (struct FileInfoBlock *) BPTRtoCptr (pkt->dp_Arg2);

  pipe=pipekey->pipe;
  FillFIB ( fib, (SIPTR)NextItem (pipe), pipe->name,
            (FIBF_EXECUTE | FIBF_DELETE), ST_PIPEFILE,
            pipe->buf->len, 1, &pipe->accessdate );

  pkt->dp_Res1= DOSTRUE;
  pkt->dp_Res2= 0;

EXFHREPLY:
  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeParentDir() responds to ParentDir requests.
*/

void  PipeParentDir (pkt)

struct DosPacket  *pkt;

{ struct FileLock  *lock;
  void             InitPipedirLock();


  InitPipedirLock ();

  pkt->dp_Res2= 0;

  if ((lock= (struct FileLock *) BPTRtoCptr (pkt->dp_Arg1)) == NULL)
    { pkt->dp_Res1= 0;
      pkt->dp_Res2= ERROR_INVALID_LOCK;
    }
  else
    { if (lock->fl_Key == (IPTR)NULL)     /* then lock is on handler */
        pkt->dp_Res1= 0;     /* root of current filing system */
      else
        pkt->dp_Res1= (SIPTR)CptrtoBPTR (PipedirLock);
    }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
** PipeParentFH() responds to ParentFH requests.
*/

void  PipeParentFH (pkt)

struct DosPacket  *pkt;

{ PIPEKEY         *pipekey;
  void             InitPipedirLock();


  InitPipedirLock ();

  pkt->dp_Res2= 0;

  if ((pipekey= (PIPEKEY *) (pkt->dp_Arg1)) == NULL)
    { pkt->dp_Res1= 0;
      pkt->dp_Res2= ERROR_INVALID_LOCK;
    }
  else
    {
      pkt->dp_Res1= (SIPTR)CptrtoBPTR (PipedirLock);
    }

  QuickReplyPkt (pkt);
}



/*---------------------------------------------------------------------------
*/

static void  InitPipedirLock ()

{ if (PipedirLock == NULL)
    { PipedirLock= (struct FileLock *) (((SIPTR) LockBytes + 3) & ((~0)<<2));
      InitLock (PipedirLock, NULL);
    }
}



/*---------------------------------------------------------------------------
** InitLock() initializes locks returned to clients by PipeLock().  For locks
** on individual pipes, the "fl_Key" field points to the associated pipe's
** PIPEDATA structure.  For the handler, the "fl_Key" field is NULL.
*/

void  InitLock (lock, key)

struct FileLock  *lock;
LONG             key;

{ lock->fl_Link=   0;
  lock->fl_Key=    key;
  lock->fl_Access= SHARED_LOCK;              /* only mode allowed */
  lock->fl_Task=   PipePort;                 /* set during handler init */
  lock->fl_Volume= CptrtoBPTR (DevNode);     /* also set during init */
}



/*---------------------------------------------------------------------------
** FillFIB() fills a FileInfoBlock with the specified information.  Note
** that handlers must store BSTR's in the FileInfoBlock.
*/

static void  FillFIB (fib, DiskKey, FileName, Protection, Type, Size, NumBlocks, Datep)

struct FileInfoBlock  *fib;
SIPTR                 DiskKey;
char                  *FileName;     /* null-terminated */
LONG                  Protection;
LONG                  Type;
LONG                  Size;
LONG                  NumBlocks;
struct DateStamp      *Datep;

{ fib->fib_DiskKey=      DiskKey;
  fib->fib_DirEntryType= Type;

  CstrtoFIB (FileName, fib->fib_FileName, sizeof (fib->fib_FileName));

  fib->fib_Protection=   Protection;
  fib->fib_EntryType=    Type;     /* ??? */
  fib->fib_Size=         Size;
  fib->fib_NumBlocks=    NumBlocks;

  CopyMem (Datep, &fib->fib_Date, sizeof (struct DateStamp));

  fib->fib_Comment[0]= '\0';     /* empty BSTR */
}
