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
#include "dspecho.h"
#include "dspechofuncs.h"

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
*/


void
EchoMono16( LONG          loops,
            struct Echo  *es,
            void        **buffer,
            void        **srcptr,
            void        **dstptr)
{
  WORD *buf;
  WORD *src, *dst;
  LONG sample, delaysample;

  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sample      = *buf;
    delaysample = *src++;

    *buf++ = ( es->ahiecho_MixN * sample + es->ahiecho_MixD * delaysample ) >> 16;

    sample = es->ahiecho_FeedbackNS * sample + 
             es->ahiecho_FeedbackDS * (delaysample + 1);
    
    *dst++ = sample >> 16;

    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}


void
EchoStereo16( LONG          loops,
              struct Echo  *es,
              void        **buffer,
              void        **srcptr,
              void        **dstptr)
{
  WORD *buf;
  WORD *src, *dst;
  LONG sample, sampleL, sampleR, delaysampleL, delaysampleR;
  
  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sampleL      = *buf;
    delaysampleL = *src++;

    *buf++ = ( es->ahiecho_MixN * sampleL + es->ahiecho_MixD * delaysampleL ) >> 16;

    sampleR      = *buf;
    delaysampleR = *src++;

    *buf++ = ( es->ahiecho_MixN * sampleR + es->ahiecho_MixD * delaysampleR ) >> 16;

    sample = es->ahiecho_FeedbackDS * (delaysampleL + 1) +
             es->ahiecho_FeedbackDO * (delaysampleR + 1) +
             es->ahiecho_FeedbackNS * sampleL +
             es->ahiecho_FeedbackNO * sampleR;

    *dst++ = sample >> 16;

    sample = es->ahiecho_FeedbackDO * (delaysampleL + 1) +
             es->ahiecho_FeedbackDS * (delaysampleR + 1) +
             es->ahiecho_FeedbackNO * sampleL +
             es->ahiecho_FeedbackNS * sampleR;

    *dst++ = sample >> 16;

    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}


void
EchoMono32 ( LONG          loops,
             struct Echo  *es,
             void        **buffer,
             void        **srcptr,
             void        **dstptr)
{
  LONG *buf;
  WORD *src, *dst;
  LONG sample, delaysample;

  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sample      = *buf >> 16;
    delaysample = *src++;

    *buf++ = es->ahiecho_MixN * sample + es->ahiecho_MixD * delaysample;

    sample = es->ahiecho_FeedbackNS * sample + 
             es->ahiecho_FeedbackDS * (delaysample + 1);
    
    *dst++ = sample >> 16;

    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}


void
EchoStereo32 ( LONG          loops,
               struct Echo  *es,
               void        **buffer,
               void        **srcptr,
               void        **dstptr)
{
  LONG *buf;
  WORD *src, *dst;
  LONG sample, sampleL, sampleR, delaysampleL, delaysampleR;
  
  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sampleL      = *buf >> 16;
    delaysampleL = *src++;

    *buf++ = es->ahiecho_MixN * sampleL + es->ahiecho_MixD * delaysampleL;

    sampleR      = *buf >> 16;
    delaysampleR = *src++;

    *buf++ = es->ahiecho_MixN * sampleR + es->ahiecho_MixD * delaysampleR;

    sample = es->ahiecho_FeedbackDS * (delaysampleL + 1) +
             es->ahiecho_FeedbackDO * (delaysampleR + 1) +
             es->ahiecho_FeedbackNS * sampleL +
             es->ahiecho_FeedbackNO * sampleR;

    *dst++ = sample >> 16;

    sample = es->ahiecho_FeedbackDO * (delaysampleL + 1) +
             es->ahiecho_FeedbackDS * (delaysampleR + 1) +
             es->ahiecho_FeedbackNO * sampleL +
             es->ahiecho_FeedbackNS * sampleR;

    *dst++ = sample >> 16;

    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}


void
EchoMulti32 ( LONG          loops,
	      struct Echo  *es,
	      void        **buffer,
	      void        **srcptr,
	      void        **dstptr)
{
  LONG *buf;
  WORD *src, *dst;
  LONG sample, sampleL, sampleR, delaysampleL, delaysampleR;
  
  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sampleL      = *buf >> 16;
    delaysampleL = *src++;

    *buf++ = es->ahiecho_MixN * sampleL + es->ahiecho_MixD * delaysampleL;

    sampleR      = *buf >> 16;
    delaysampleR = *src++;

    *buf++ = es->ahiecho_MixN * sampleR + es->ahiecho_MixD * delaysampleR;

    sample = es->ahiecho_FeedbackDS * (delaysampleL + 1) +
             es->ahiecho_FeedbackDO * (delaysampleR + 1) +
             es->ahiecho_FeedbackNS * sampleL +
             es->ahiecho_FeedbackNO * sampleR;

    *dst++ = sample >> 16;

    sample = es->ahiecho_FeedbackDO * (delaysampleL + 1) +
             es->ahiecho_FeedbackDS * (delaysampleR + 1) +
             es->ahiecho_FeedbackNO * sampleL +
             es->ahiecho_FeedbackNS * sampleR;

    *dst++ = sample >> 16;

    // Skip unused channels
    buf += 6;
    src += 6;
    dst += 6;
    
    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}
