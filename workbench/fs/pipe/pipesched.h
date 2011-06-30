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



extern void              StartPipeIO    ( /* pipe, pkt, iotype */ );
extern void              CheckWaiting   ( /* pipe */ );
extern struct DosPacket  *AllocPacket   ( /* ReplyPort */ );
extern void              FreePacket     ( /* pkt */ );
extern void              StartTapIO     ( /* pkt, Type, Arg1, Arg2, Arg3, Handler */ );
extern void              HandleTapReply ( /* pkt */ );
