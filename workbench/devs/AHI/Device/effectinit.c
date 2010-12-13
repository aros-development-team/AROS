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

#include <exec/memory.h>
#include <proto/exec.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include "ahi_def.h"
#include "dsp.h"
#include "dspecho.h"
#include "effectinit.h"
#include "header.h"
#include "misc.h"
#include "mixer.h"

#ifdef __AMIGAOS4__
#define IAHIsub audioctrl->ahiac_IAHIsub
#endif

/***********************************************
***** NOTE: The mixing routine might execute while we are inside these
***** functions!
***********************************************/


/******************************************************************************
** MASTERVOLUME ***************************************************************
******************************************************************************/

BOOL 
update_MasterVolume ( struct AHIPrivAudioCtrl *audioctrl )
{
  struct Library        *AHIsubBase;
  struct AHIChannelData *cd;
  Fixed                  volume;
  int                    i;

  /* In V5, clipping is always used:
   *
   * if(audioctrl->ac.ahiac_Flags & AHIACF_CLIPPING)
   * {
   *   volume = 0x10000;
   * }
   * else
   * {
   *  volume = audioctrl->ahiac_SetMasterVolume;
   * }
   */

  volume = 0x10000;

  /* Scale to what the echo effect think is best... */
  volume = (volume * (audioctrl->ahiac_EchoMasterVolume >> 8)) >> 8;

  /* This is the actual master volume in use */
  audioctrl->ahiac_MasterVolume = volume;

  /* Update volume for channels */

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = audioctrl->ac.ahiac_Channels, cd = audioctrl->ahiac_ChannelDatas;
      i > 0;
      i--, cd++)
  {
    SelectAddRoutine(cd->cd_VolumeLeft, cd->cd_VolumeRight, cd->cd_Type, audioctrl,
                     &cd->cd_ScaleLeft, &cd->cd_ScaleRight, (ADDFUNC**) &cd->cd_AddRoutine);
    SelectAddRoutine(cd->cd_NextVolumeLeft, cd->cd_NextVolumeRight, cd->cd_NextType, audioctrl,
                     &cd->cd_NextScaleLeft, &cd->cd_NextScaleRight, (ADDFUNC**) &cd->cd_NextAddRoutine);
  }

  AHIsub_Enable(&audioctrl->ac);

  return TRUE;
}


/******************************************************************************
** DSPECHO ********************************************************************
******************************************************************************/

#define mode_stereo 1
#define mode_32bit  2
#define mode_multi  4

BOOL
update_DSPEcho ( struct AHIEffDSPEcho *echo,
                 struct AHIPrivAudioCtrl *audioctrl )
{
  ULONG size, samplesize;
  struct Echo *es;

  ULONG data_flags = MEMF_ANY;
  
  switch( MixBackend )
  {
    case MB_NATIVE:
      data_flags = MEMF_PUBLIC | MEMF_CLEAR;
      break;
      
#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      // Non-cached from both the PPC and m68k side
      data_flags = MEMF_PUBLIC | MEMF_CLEAR | MEMF_CHIP;
      break;
#endif
  }

  /* Set up the delay buffer format */

  switch(audioctrl->ac.ahiac_BuffType)
  {
    case AHIST_M16S:
    case AHIST_M32S:
      samplesize = 2;
      break;

    case AHIST_S16S:
    case AHIST_S32S:
      samplesize = 4;
      break;

    case AHIST_L7_1:
      samplesize = 16;
      break;
      
    default:
      return FALSE; // Panic
  }

  size = samplesize * (echo->ahiede_Delay + audioctrl->ac.ahiac_MaxBuffSamples);

  es = AHIAllocVec( sizeof(struct Echo) + size,
                    data_flags );
  
  if(es)
  {
    ULONG mode = 0;

    es->ahiecho_Offset       = 0;
    es->ahiecho_SrcPtr       = es->ahiecho_Buffer;
    es->ahiecho_DstPtr       = es->ahiecho_Buffer + (samplesize * echo->ahiede_Delay);
    es->ahiecho_EndPtr       = es->ahiecho_Buffer + size;
    es->ahiecho_BufferLength = echo->ahiede_Delay + audioctrl->ac.ahiac_MaxBuffSamples;
    es->ahiecho_BufferSize   = size;

    switch(audioctrl->ac.ahiac_BuffType)
    {
      case AHIST_M16S:
        echo->ahiede_Cross = 0;
        break;
      case AHIST_S16S:
        mode |= mode_stereo;
        break;
      case AHIST_M32S:
        echo->ahiede_Cross = 0;
        mode |= mode_32bit;
        break;
      case AHIST_S32S:
        mode |= (mode_32bit | mode_stereo);
        break;
      case AHIST_L7_1:
	mode |= (mode_32bit | mode_multi);
	break;
    }

    es->ahiecho_Delay      = echo->ahiede_Delay;
    es->ahiecho_MixD       = echo->ahiede_Mix;
    es->ahiecho_MixN       = 0x10000 - echo->ahiede_Mix;
    es->ahiecho_FeedbackDO = (echo->ahiede_Feedback >> 8) *
                             (echo->ahiede_Cross >> 8);
    es->ahiecho_FeedbackDS = (echo->ahiede_Feedback >> 8) *
                             ((0x10000 - echo->ahiede_Cross) >> 8);
    es->ahiecho_FeedbackNO = ((0x10000 - echo->ahiede_Feedback) >> 8) *
                             (echo->ahiede_Cross >> 8);
    es->ahiecho_FeedbackNS = ((0x10000 - echo->ahiede_Feedback) >> 8) *
                             ((0x10000 - echo->ahiede_Cross) >> 8);

    audioctrl->ahiac_EchoMasterVolume = 0x10000;

    update_MasterVolume( audioctrl );

    switch(mode)
    {
      // 16 bit
      case 0:
        es->ahiecho_Code   = do_DSPEchoMono16;
        break;

      // stereo 16 bit
      case 1:
        es->ahiecho_Code   = do_DSPEchoStereo16;
        break;

      // 32 bit
      case 2:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32 bit
      case 3:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // multichannel 32 bit
      case 6:
	es->ahiecho_Code   = do_DSPEchoMulti32;
	break;

      // Should not happen!
      default:
        AHIFreeVec(es);
        return FALSE;
    }

    // Structure filled, make it available to the mixing routine

    audioctrl->ahiac_EffDSPEchoStruct = es;

    return TRUE;
  }

  return FALSE;
}


void
free_DSPEcho ( struct AHIPrivAudioCtrl *audioctrl )
{
  void *p = audioctrl->ahiac_EffDSPEchoStruct;

  // Hide it from mixing routine before freeing it!
  audioctrl->ahiac_EffDSPEchoStruct = NULL;
  AHIFreeVec(p);

  audioctrl->ahiac_EchoMasterVolume = 0x10000;
  update_MasterVolume( audioctrl );
}




/******************************************************************************
** DSPMASK ********************************************************************
******************************************************************************/

static void
addchannel ( struct AHIChannelData **list, struct AHIChannelData *cd )
{
  struct AHIChannelData *ptr;

  if(*list == NULL)
  {
    *list = cd;
  }
  else
  {
    ptr = *list;
    while(ptr->cd_Succ != NULL)
    {
      ptr = ptr->cd_Succ;
    }
    ptr->cd_Succ = cd;
  }

  cd->cd_Succ = NULL;
}

BOOL 
update_DSPMask ( struct AHIEffDSPMask *mask,
                 struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIChannelData *cd, *wet = NULL, *dry = NULL;
  struct Library        *AHIsubBase;
  int                    i;
  UBYTE                 *flag;

  if(mask->ahiedm_Channels != audioctrl->ac.ahiac_Channels)
  {
    return FALSE;
  }

  cd = audioctrl->ahiac_ChannelDatas;

  flag = mask->ahiedm_Mask;

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = 0; i < audioctrl->ac.ahiac_Channels; i++)
  {
    if(*flag == AHIEDM_WET)
    {
      addchannel(&wet, cd);
    }
    else
    {
      addchannel(&dry, cd);
    }
    
    flag++;
    cd++;
  }

  audioctrl->ahiac_WetList = wet;
  audioctrl->ahiac_DryList = dry;

  AHIsub_Enable(&audioctrl->ac);

  return TRUE;
}


void
clear_DSPMask ( struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIChannelData *cd;
  struct Library        *AHIsubBase;
  int                    i;

  // Make all channels wet

  cd = audioctrl->ahiac_ChannelDatas;

  audioctrl->ahiac_WetList = cd;
  audioctrl->ahiac_DryList = NULL;

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = 0; i < audioctrl->ac.ahiac_Channels - 1; i++)
  {
    // Set link to next channel
    cd->cd_Succ = cd + 1;
    cd++;
  }

  // Clear the last link;
  cd->cd_Succ = NULL;

  AHIsub_Enable(&audioctrl->ac);
}
