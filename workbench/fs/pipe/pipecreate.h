/****************************************************************************
**  File:       pipecreate.h
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
*/



#define   OPENTAP_STRSIZE   108

struct pipedata;
typedef struct pipedata PIPEDATA;

extern void  OpenPipe    ( struct DosPacket *pkt, BPTR tapfh );
extern void  ClosePipe   ( struct DosPacket *pkt );
extern void  DiscardPipe ( PIPEDATA *pipe );
