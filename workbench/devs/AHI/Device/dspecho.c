/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

#include "ahi_def.h"
#include "dsp.h"
#include "dspecho.h"
#include "dspechofuncs.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

/**************
* Inputs: ahiede_Delay, ahiede_Feedback, ahiede_Mix, ahiede_Cross
*
* Delay      = ahide_Delay
* MixN       = 1-ahide_Mix
* MixD       = ahide_Mix
* FeedbackDO = ahide_Feedback*ahide_Cross
* FeedbackDS = ahide_Feedback*(1-ahide_Cross)
* FeedbackNO = (1-ahide_Feedback)*ahide_Cross
* FeedbackNS = (1-ahide_Feedback)*(1-ahide_Cross)
*
*                                               |\
* left in ->---+----------+---------------------| >---->(+)----> left out
*              |          |                MixN |/       ^
*  FeedbackNO \¯/        \¯/ FeedbackNS                  |
*              v          v                              |
*              |          |                              |
*              |          v    |¯¯¯|            |\       |
*              |    +--->(+)-->| T |----+-------| >------+
*              |    |     ^    |___|    |  MixD |/
*              |    |     |    Delay    |
*              |    |     |             |
*              |    |     |      /|     |
*              |    |     +-----< |-----+
*              |    | FeedbackDS \|     |
*              |    |                   |
*              |    |            /|     |
*             (+)<--(-----------< |-----+
*              |    |            \| FeedbackDO
*              |    |
*              |    |
*              |    |
*              |    |            /| FeedbackDO
*              |   (+)<---------< |-----+
*              |    |            \|     |
*              |    |                   |
*              |    | FeedbackDS /|     |
*              |    |     +-----< |-----+
*              |    |     |      \|     |
*              |    |     |             |
*              |    |     v    |¯¯¯|    |       |\
*              +----(--->(+)-->| T |----+-------| >------+
*                   |     ^    |___|       MixD |/       |
*                   |     |    Delay                     |
*                   ^     ^                              |
*       FeedbackNO /_\   /_\ FeedbackNS                  |
*                   |     |                     |\       v
* right in ->-------+-----+---------------------| >---->(+)----> right out
*                                          MixN |/
*
*
**************
*
* The delay buffer: (BuffSamples = 5, Delay = 8 Total size = 13
*
*  1) Delay times
*
*  +---------+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
*  2) BuffSamples times
*
*  +-Mix-----------+
*  |               |
*  ^               v
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
* Or optimized using a circular buffer:
*
*
* Offset<BuffSamples => BuffSamples-Offset times:
*
*  +-Mix-----------+
*  |               |
*  ^               v
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
* BuffSamples<=Offset<=Delay => BuffSamples times:
*
*  +-Mix-----+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
*           *---------*
*
* Offset>Delay => BuffSamples+Delay-Offset times:
*
*          +-Mix-----+
*          |         |
*          v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* --*               *--------
*
* The delay buffer: (BuffSamples = 5, Delay = 3 Total size = 8
*
* Offset<BuffSamples => BuffSamples-Offset times:
*
*  +-Mix-+
*  |     |
*  ^     v
* |_|_|_|_|_|_|_|_|
* *---------*
*
* Offset>=BuffSamples => BuffSamples+Delay-Offset times:
*
*  +-----Mix-+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|
* ----*     *------
*
*
*
* Algoritm:
*
*   LoopsLeft=BuffSamples
*   Offset=0
*   Src=E
*   Dst=E+Delay
* Loop:
*   If LoopsLeft <= 0 GOTO Exit
*   IF Src >= (E + MaxBuffSamples + Delay) THEN Src = Src - (MaxBuffSamples + Delay)
*   IF Dst >= (E + MaxBuffSamples + Delay) THEN Dst = Dst - (MaxBuffSamples + Delay)
*   IF Offset >= (MaxBuffSamples + Delay) THEN Offset = Offset - (MaxBuffSamples + Delay)
*
*   IF Offset < MaxBuffSamples THEN LoopTimes = MaxBuffSamples-Offset : GOTO Echo
*   IF Offset <= Delay THEN LoopTimes = MaxBuffSamples : GOTO Echo 
*   LoopTimes = MaxBuffSamples+Delay-Offset
* Echo:
*   LoopTimes = min(LoopTimes,LoopsLeft)
*   Echo LoopTimes samples
*
*   Src = Src + LoopTimes
*   Dst = Dst + LoopTimes
*   Offset = Offset + LoopTimes
*   LoopsLeft = LoopsLeft - LoopTimes
*   GOTO Loop
* Exit:
*
*/

static void
do_DSPEcho ( struct Echo *es,
             void *buf,
             struct AHIPrivAudioCtrl *audioctrl,
             void (*echofunc)(LONG, struct Echo *, void **, void **, void **) )
{
  LONG  samples, loops;
  ULONG offset;
  void *srcptr, *dstptr;

  samples = audioctrl->ac.ahiac_BuffSamples;
  offset  = es->ahiecho_Offset;
  srcptr  = es->ahiecho_SrcPtr;
  dstptr  = es->ahiecho_DstPtr;

  while(samples > 0)
  {
    /* Circular buffer stuff */  
    
    if(srcptr >= es->ahiecho_EndPtr)
    {
      srcptr = (char *) srcptr - es->ahiecho_BufferSize;
    }

    if(dstptr >= es->ahiecho_EndPtr)
    {
      dstptr = (char *) dstptr - es->ahiecho_BufferSize;
    }

    if(offset >= es->ahiecho_BufferLength)
    {
      offset -= es->ahiecho_BufferLength;
    }



    if(offset < audioctrl->ac.ahiac_MaxBuffSamples)
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples - offset;
    }
    else if(offset <= es->ahiecho_Delay)
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples;
    }
    else
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples + es->ahiecho_Delay - offset;
    }

    loops = min(loops, samples);
    
    samples -= loops;
    offset  += loops;

    /* Call echo function */

    echofunc(loops, es, &buf, &srcptr, &dstptr);

  } // while(samples > 0)

  es->ahiecho_Offset = offset;
  es->ahiecho_SrcPtr = srcptr;
  es->ahiecho_DstPtr = dstptr;
}




/* Entry points **************************************************************/

void
do_DSPEchoMono16( struct Echo *es,
                  void *buf,
                  struct AHIPrivAudioCtrl *audioctrl )
{
  do_DSPEcho( es, buf, audioctrl, EchoMono16 );
}


void
do_DSPEchoStereo16( struct Echo *es,
                    void *buf,
                    struct AHIPrivAudioCtrl *audioctrl )
{ 
  do_DSPEcho( es, buf, audioctrl, EchoStereo16 );
}


void
do_DSPEchoMono32 ( struct Echo *es,
                   void *buf,
                   struct AHIPrivAudioCtrl *audioctrl )
{
  do_DSPEcho( es, buf, audioctrl, EchoMono32 );
}


void
do_DSPEchoStereo32( struct Echo *es,
                    void *buf,
                    struct AHIPrivAudioCtrl *audioctrl )
{ 
  do_DSPEcho( es, buf, audioctrl, EchoStereo32 );
}


void
do_DSPEchoMulti32( struct Echo *es,
		   void *buf,
		   struct AHIPrivAudioCtrl *audioctrl )
{ 
  do_DSPEcho( es, buf, audioctrl, EchoMulti32 );
}

