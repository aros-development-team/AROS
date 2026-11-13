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



extern void  SetPipeDate   ( PIPEDATA *pipe );
extern void  PipeLock      ( struct DosPacket *pkt );
extern void  PipeFHFromLock( struct DosPacket *pkt );
extern void  PipeDupLock   ( struct DosPacket *pkt );
extern void  PipeDupLockFH ( struct DosPacket *pkt );
extern void  PipeUnLock    ( struct DosPacket *pkt );
extern void  PipeExamine   ( struct DosPacket *pkt );
extern void  PipeExNext    ( struct DosPacket *pkt );
extern void  PipeExFH      ( struct DosPacket *pkt );
extern void  PipeParentDir ( struct DosPacket *pkt );
extern void  PipeParentFH  ( struct DosPacket *pkt );
extern void  InitLock      ( struct FileLock *lock, SIPTR key );
