/****************************************************************************
**  File:       pipe-handler.h
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
**		26-Mar-87	Fixed bug in ClosePipe() in pipecreate.c: not
**				 closing r/w mode properly (extraneous else).
**		27-Mar-87	Added PipeDupLock() to pipedir.c and the case
**				 for it in pipe-handler.c.  This was missing
**				 in the original version!
**		28-Mar-87	Added code to handler() to remove ':' from
**				 end of handler name.  This caused problems
**				 with Examine(); it expects no ending  ':'.
*/



/*---------------------------------------------------------------------------
** Compilation Flags
** -----------------
** DEBUG	: add code to open a window for debugging information.
**		Messages are output as requests come in, etc.  DEBUG is
**		active if defined at all.
**
** (The following are active only if #defined nonzero)
**
** CON_TAP_ONLY	: only CON: pipe taps are allowed.  The full CON:
**		 specification must be given, though, like CON:0/0/100/100/z.
**
** PIPEDIR	: include code so that the handler looks like a directory.
**		This allows "Dir" and "List" to work, as well as "CD".
**		The functions in pipedir.c are unnecessary if false.
**
** UPDATE_PIPEDATE : if PIPEDIR is true, then this controls whether or not
**		the handler's date is updated with each access to a pipe,
**		or is just left at its startup time.
**
** AUTONAME	: include code so that specifying a null pipe name causes
**		the handler to select a new, as yet unused, name.
**		Unfortunately, this causes inconsistent behaviour for Lock(),
**		since a null name indicates a lock is desired on the handler.
**		Thus locking PIPE: and opening PIPE: reference different
**		objects.
**
** AUTONAME_STAR: Like AUTONAME, but use the special name 'PIPE:*' for the
**		automatic pipe name. This works around the Lock() issues
**		that AUTONAME has.
*/

#define   CON_TAP_ONLY      0
#define   PIPEDIR           1
#define   UPDATE_PIPEDATE   1
#define   AUTONAME          0           /* Use PIPE: for auto names */
#define   AUTONAME_STAR     1           /* Use PIPE:* for automatic names */



#define   ALLOCMEM_FLAGS    MEMF_PUBLIC



/*---------------------------------------------------------------------------
*/

typedef struct pipedata
  { PIPELISTNODE      link;                  /* for list handling */
    char              name[PIPENAMELEN];     /* the pipe's name */
    PIPEBUF           *buf;                  /* see pipebuf.c */
    BYTE              flags;                 /* see values below */
    PIPELISTHEADER    readerlist;            /* list of waiting read requests */
    PIPELISTHEADER    writerlist;            /* list of waiting write requests */
    BPTR              tapfh;                 /* file handle of tap, 0 if none */
#if    PIPEDIR
    ULONG             lockct;                /* number of extant locks */
    struct FileLock   *lock;                 /* this pipe's lock - see note above */
    struct DateStamp  accessdate;            /* date last accessed */
#endif /* PIPEDIR */
  }
PIPEDATA;

#define   OPEN_FOR_READ    (1 << 0)
#define   OPEN_FOR_WRITE   (1 << 1)     /* flags for pipedata struct */



/*---------------------------------------------------------------------------
** PIPEKEYs are similar to file handles.  Each successful pipe open request
** has a PIPEKEY associated with it, which in turn refers to the pipe.
** The filehandle returned to the client has the address of the PIPEKEY
** stored in its Arg1 field, so that read, write and close packets will
** identify the pipe and its mode of opening.
*/

typedef struct pipekey
  { PIPEDATA  *pipe;
    int       openmode;     /* Type field of original open request */
    IOTYPE    iotype;       /* (somewhat redundant) see pipesched.h */
  }
PIPEKEY;



extern struct DeviceNode  *DevNode;
extern struct MsgPort     *PipePort;
extern char               HandlerName[];

extern PIPELISTHEADER     pipelist;

extern PIPELISTHEADER     tapwaitlist;
extern struct MsgPort     *TapReplyPort;

#if PIPEDIR
  extern struct DateStamp  PipeDate;
#endif /* PIPEDIR */

#ifdef __AROS__
#define   BPTRtoCptr(Bp)      BADDR(Bp)
#define   CptrtoBPTR(Cp)      MKBADDR(Cp)
#else
#define   BPTRtoCptr(Bp)      ((char *) ((ULONG) (Bp) << 2))
#define   CptrtoBPTR(Cp)      ((BPTR)   ((ULONG) (Cp) >> 2))
#endif

#define   QuickReplyPkt(pkt)       PutMsg ((pkt)->dp_Port, (pkt)->dp_Link)

extern void      handler   ( /* StartPkt */ );
extern PIPEDATA  *FindPipe ( /* name */ );



/*---------------------
** references to system
*/

#ifdef __AROS__
#include <proto/exec.h>
#include <proto/dos.h>
#else
extern struct Library     *OpenLibrary ();
extern void               CloseLibrary ();
extern struct Task        *FindTask ();
extern struct MsgPort     *CreatePort ();
extern ULONG              Wait ();
extern struct Message     *GetMsg ();
extern void               PutMsg ();
extern BYTE               *AllocMem ();
extern void               FreeMem ();
extern void               CopyMem ();

extern struct MsgPort     *DeviceProc ();
extern int                IoErr ();

#if PIPEDIR
  extern struct DateStamp   *DateStamp ();
#endif /* PIPEDIR */

extern struct Library     *AbsExecBase;

/*---------------------------------
** these are new to the 1.2 release
*/

#ifndef MODE_READWRITE
# define   MODE_READWRITE   1004
#endif /*  MODE_READWRITE */

#ifndef ACTION_END
# define   ACTION_END       1007     /* not really new, just missing */
#endif /*  ACTION_END */

#endif /* !__AROS__ */

#ifndef MODE_READONLY
# define   MODE_READONLY    MODE_OLDFILE
#endif /*  MODE_READONLY */


