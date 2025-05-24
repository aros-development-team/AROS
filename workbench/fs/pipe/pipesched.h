/****************************************************************************
**  File:       pipesched.h
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added "lockct" check in CheckWaiting().
*/

struct pipedata;
typedef struct pipedata PIPEDATA;

typedef enum iotype
  { PIPEREAD,
    PIPEWRITE,
    PIPERW
  }
IOTYPE;


struct pipewait
  { BYTE    *buf;        /* the next position for read/write */
    ULONG   len;         /* the remaining length t read/write */
    IOTYPE  reqtype;     /* PIPEREAD or PIPEWRITE only */
  };


struct tapwait
  { struct DosPacket   *clientpkt;     /* the client's packet */
    struct FileHandle  *handle;        /* the associated filehandle */
  };


union pktinfo
  { struct pipewait  pipewait;     /* for packet waiting on pipe */
    struct tapwait   tapwait;      /* for packet waiting on tap */
  };


typedef struct waitingdata
  { PIPELISTNODE      link;        /* for list use */
    struct DosPacket  *pkt;        /* the packet we are waiting on */
    union pktinfo     pktinfo;     /* data pertaining to the waiting request */
  }
WAITINGDATA;

extern void              StartPipeIO    ( struct DosPacket *pkt, IOTYPE iotype );
extern void              CheckWaiting   ( PIPEDATA *pipe );
extern struct DosPacket  *AllocPacket   ( struct MsgPort *ReplyPort );
extern void              FreePacket     ( struct DosPacket *pkt );
extern void              StartTapIO     ( struct DosPacket *pkt, SIPTR Type, SIPTR Arg1, SIPTR Arg2, SIPTR Arg3, struct MsgPort *Handler );
extern void              HandleTapReply ( struct DosPacket *pkt );
