/****************************************************************************
**  File:       pipedir.h
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



extern void  SetPipeDate   ( /* pipe */ );
extern void  PipeLock      ( /* pkt */ );
extern void  PipeFHFromLock( /* pkt */ );
extern void  PipeDupLock   ( /* pkt */ );
extern void  PipeDupLockFH ( /* pkt */ );
extern void  PipeUnLock    ( /* pkt */ );
extern void  PipeExamine   ( /* pkt */ );
extern void  PipeExNext    ( /* pkt */ );
extern void  PipeExFH      ( /* pkt */ );
extern void  PipeParentDir ( /* pkt */ );
extern void  PipeParentFH  ( /* pkt */ );
extern void  InitLock      ( /* lock, key */ );
