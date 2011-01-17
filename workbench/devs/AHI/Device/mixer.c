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

#include <string.h>
#include <stddef.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__

#include "ahi_def.h"

#include "addroutines.h"
#include "debug.h"
#include "dsp.h"
#include "header.h"
#include "misc.h"
#include "mixer.h"
#include "sound.h"

#if defined( ENABLE_WARPUP )
# include "warpup.h"
#endif

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

/******************************************************************************
** Prototypes *****************************************************************
******************************************************************************/

void
CallSoundHook( struct AHIPrivAudioCtrl *audioctrl,
               void* arg )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      CallHookPkt( audioctrl->ac.ahiac_SoundFunc,
                   audioctrl,
                   arg );
      break;

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      WarpUpCallSoundHook( audioctrl, arg );
      break;
#endif
  }
}


void
MixerFunc( struct Hook*             hook,
           struct AHIPrivAudioCtrl* audioctrl,
           void*                    dst )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      Mix( hook, audioctrl, dst );
      DoMasterVolume( dst, audioctrl );
      DoOutputBuffer( dst, audioctrl );
      DoChannelInfo( audioctrl );
      break;
      
#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      WarpUpCallMixer( audioctrl, dst );
      DoOutputBuffer( dst, audioctrl );
      DoChannelInfo( audioctrl );
      break;
#endif
  }
}


/******************************************************************************
** InitMixroutine *************************************************************
******************************************************************************/

// This function is used to initialize the mixer routine (called from 
// AHI_AllocAudio()).

BOOL
InitMixroutine( struct AHIPrivAudioCtrl *audioctrl )
{
  BOOL rc = FALSE;

  // Allocate and initialize the AHIChannelData structures
  // This structure could be accessed from from interrupts!

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

  audioctrl->ahiac_ChannelDatas = AHIAllocVec(
      audioctrl->ac.ahiac_Channels * sizeof( struct AHIChannelData ),
      data_flags );

  // Allocate and initialize the AHISoundData structures
  // This structure could be accessed from from interrupts!

  audioctrl->ahiac_SoundDatas = AHIAllocVec(
      audioctrl->ac.ahiac_Sounds * sizeof( struct AHISoundData ),
      data_flags );

  // Now link the list and fill in the channel number for each structure.

  if( audioctrl->ahiac_ChannelDatas != NULL
      && audioctrl->ahiac_SoundDatas != NULL )
  {
    struct AHIChannelData *cd;
    struct AHISoundData   *sd;
    int                    i;

    cd = audioctrl->ahiac_ChannelDatas;

    audioctrl->ahiac_WetList = cd;
    audioctrl->ahiac_DryList = NULL;

    for(i = 0; i < audioctrl->ac.ahiac_Channels - 1; i++)
    {
      // Set Channel No
      cd->cd_ChannelNo = i;

      // Set link to next channel
      cd->cd_Succ = cd + 1;
      cd++;
    }

    // Set the last No
    cd->cd_ChannelNo = i;

    // Clear the last link;
    cd->cd_Succ = NULL;


    sd = audioctrl->ahiac_SoundDatas;

    for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
    {
      sd->sd_Type = AHIST_NOTYPE;
      sd++;
    }
  }


  switch( MixBackend )
  {
    case MB_NATIVE:
      rc = TRUE;
      break;
      
#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      rc = WarpUpInit( audioctrl );
      break;
#endif
  }

  return rc;
}


/******************************************************************************
** CleanUpMixroutine **********************************************************
******************************************************************************/

// This function is used to clean up after the mixer routine (called from 
// AHI_FreeAudio()).

void
CleanUpMixroutine( struct AHIPrivAudioCtrl *audioctrl )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      break;
      
#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      WarpUpCleanUp( audioctrl );
      break;
#endif
  }

  AHIFreeVec( audioctrl->ahiac_SoundDatas );
  audioctrl->ahiac_SoundDatas = NULL;

  AHIFreeVec( audioctrl->ahiac_ChannelDatas );
  audioctrl->ahiac_ChannelDatas = NULL;
}


/******************************************************************************
** SelectAddRoutine ***********************************************************
******************************************************************************/

// This routine gets called each time there is reason to believe that a new
// add-routine should be used (new sound selected, volume changed,
// mastervolume changed)

// Based on VolumeLeft, VolumeRight and SampleType, fill in ScaleLeft,
// ScaleRight and AddRoutine.

void
SelectAddRoutine ( Fixed     VolumeLeft,
                   Fixed     VolumeRight,
                   ULONG     SampleType,
                   struct    AHIPrivAudioCtrl *audioctrl,
                   LONG     *ScaleLeft,
                   LONG     *ScaleRight,
                   ADDFUNC **AddRoutine )

{
  // This version only cares about the sample format and does not use any
  // optimized add-routines.

  // Scale the volume

  VolumeLeft  = VolumeLeft  * (audioctrl->ahiac_MasterVolume >> 8) / 
                              (audioctrl->ahiac_Channels2 << 8);

  VolumeRight = VolumeRight * (audioctrl->ahiac_MasterVolume >> 8) / 
                              (audioctrl->ahiac_Channels2 << 8);

  // First, select HiFi or LoFi...

  if( audioctrl->ac.ahiac_Flags & AHIACF_HIFI )
  {

    // Then, check the output format...

    switch(audioctrl->ac.ahiac_BuffType)
    {

      case AHIST_M32S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddByteMonoBPtr;
            else
              *AddRoutine = AddByteMonoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddBytesMonoBPtr;
            else
              *AddRoutine = AddBytesMonoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordMonoBPtr;
            else
              *AddRoutine = AddWordMonoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordsMonoBPtr;
            else
              *AddRoutine = AddWordsMonoPtr;
            break;

          case AHIST_M32S:
          case AHIST_BW|AHIST_M32S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLongMonoBPtr;
            else
              *AddRoutine = AddLongMonoPtr;
            break;

          case AHIST_S32S:
          case AHIST_BW|AHIST_S32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLongsMonoBPtr;
            else
              *AddRoutine = AddLongsMonoPtr;
            break;

          case AHIST_L7_1:
          case AHIST_BW|AHIST_L7_1:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = Add71MonoBPtr;
            else
              *AddRoutine = Add71MonoPtr;
            break;

          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;

      case AHIST_S32S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddByteStereoBPtr;
            else
              *AddRoutine = AddByteStereoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddBytesStereoBPtr;
            else
              *AddRoutine = AddBytesStereoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordStereoBPtr;
            else
              *AddRoutine = AddWordStereoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordsStereoBPtr;
            else
              *AddRoutine = AddWordsStereoPtr;
            break;

          case AHIST_M32S:
          case AHIST_BW|AHIST_M32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLongStereoBPtr;
            else
              *AddRoutine = AddLongStereoPtr;
            break;

          case AHIST_S32S:
          case AHIST_BW|AHIST_S32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLongsStereoBPtr;
            else
              *AddRoutine = AddLongsStereoPtr;
            break;

          case AHIST_L7_1:
          case AHIST_BW|AHIST_L7_1:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = Add71StereoBPtr;
            else
              *AddRoutine = Add71StereoPtr;
            break;

          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;

      case AHIST_L7_1:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddByte71BPtr;
	    else
	      *AddRoutine = AddByte71Ptr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddBytes71BPtr;
	    else
	      *AddRoutine = AddBytes71Ptr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddWord71BPtr;
	    else
	      *AddRoutine = AddWord71Ptr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddWords71BPtr;
	    else
	      *AddRoutine = AddWords71Ptr;
            break;

          case AHIST_M32S:
          case AHIST_BW|AHIST_M32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddLong71BPtr;
	    else
	      *AddRoutine = AddLong71Ptr;
            break;

          case AHIST_S32S:
          case AHIST_BW|AHIST_S32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = AddLongs71BPtr;
	    else
	      *AddRoutine = AddLongs71Ptr;
            break;

          case AHIST_L7_1:
          case AHIST_BW|AHIST_L7_1:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
	      *AddRoutine = Add7171BPtr;
	    else
	      *AddRoutine = Add7171Ptr;
            break;
	    
          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;
	
      default:
        *ScaleLeft  = 0;
        *ScaleRight = 0;
        *AddRoutine = NULL;
        break;
    }
  }
  else
  {

    // Then, check the output format...

    switch(audioctrl->ac.ahiac_BuffType)
    {

      case AHIST_M16S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiByteMonoBPtr;
            else
              *AddRoutine = AddLofiByteMonoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiBytesMonoBPtr;
            else
              *AddRoutine = AddLofiBytesMonoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordMonoBPtr;
            else
              *AddRoutine = AddLofiWordMonoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordsMonoBPtr;
            else
              *AddRoutine = AddLofiWordsMonoPtr;
            break;

          case AHIST_M32S:
          case AHIST_BW|AHIST_M32S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiLongMonoBPtr;
            else
              *AddRoutine = AddLofiLongMonoPtr;
            break;

          case AHIST_S32S:
          case AHIST_BW|AHIST_S32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiLongsMonoBPtr;
            else
              *AddRoutine = AddLofiLongsMonoPtr;
            break;

          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;

      case AHIST_S16S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiByteStereoBPtr;
            else
              *AddRoutine = AddLofiByteStereoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiBytesStereoBPtr;
            else
              *AddRoutine = AddLofiBytesStereoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordStereoBPtr;
            else
              *AddRoutine = AddLofiWordStereoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordsStereoBPtr;
            else
              *AddRoutine = AddLofiWordsStereoPtr;
            break;

          case AHIST_M32S:
          case AHIST_BW|AHIST_M32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiLongStereoBPtr;
            else
              *AddRoutine = AddLofiLongStereoPtr;
            break;

          case AHIST_S32S:
          case AHIST_BW|AHIST_S32S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiLongsStereoBPtr;
            else
              *AddRoutine = AddLofiLongsStereoPtr;
            break;

          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;

      default:
        *ScaleLeft  = 0;
        *ScaleRight = 0;
        *AddRoutine = NULL;
        break;
    }
  }
}


/******************************************************************************
** Mix ************************************************************************
******************************************************************************/

// This is the function that the driver calls each time it want more data
// to play. 

void
Mix( struct Hook*             unused_Hook, 
     struct AHIPrivAudioCtrl* audioctrl,
     void*                    dst )
{
  struct AHIChannelData	*cd;
  void                  *dstptr;
  LONG                   samplesleft;

  /* Clear the buffer */

  memset( dst, 0, audioctrl->ahiac_BuffSizeNow );
  /* Mix the samples */

  audioctrl->ahiac_WetOrDry = AHIEDM_WET;

  cd = audioctrl->ahiac_WetList;

  while(TRUE)
  {
    while(cd != NULL) // .nextchannel
    {
      samplesleft = audioctrl->ac.ahiac_BuffSamples;
      dstptr      = dst;

      while(TRUE) // .contchannel
      {
        LONG samples;
        LONG processed;

        /* Call Sound Hook */

        if(cd->cd_EOS)
        {
          cd->cd_EOS = FALSE;
          if(audioctrl->ac.ahiac_SoundFunc != NULL)
          {
            CallSoundHook( audioctrl, &cd->cd_ChannelNo );
          }
        }

        processed = 0;

        if( cd->cd_AntiClickCount > 0 && cd->cd_FreqOK && cd->cd_SoundOK )
        {
          // Sound is ok and we're looking for a zero-crossing.

          LONG try_samples;

          samples     = min( samplesleft, cd->cd_Samples );
          try_samples = min( samples, cd->cd_AntiClickCount );

          if( try_samples > 0 )
          {
            cd->cd_TempStartPointL   = cd->cd_StartPointL;
            cd->cd_TempStartPointR   = cd->cd_StartPointR;
            cd->cd_TempStartPointRL  = cd->cd_StartPointRL;
            cd->cd_TempStartPointRR  = cd->cd_StartPointRR;
            cd->cd_TempStartPointSL  = cd->cd_StartPointSL;
            cd->cd_TempStartPointSR  = cd->cd_StartPointSR;
            cd->cd_TempStartPointC   = cd->cd_StartPointC;
            cd->cd_TempStartPointLFE = cd->cd_StartPointLFE;

            processed = ((ADDFUNC *) cd->cd_AddRoutine)( try_samples,
                                                         cd->cd_ScaleLeft,
                                                         cd->cd_ScaleRight,
                                                        &cd->cd_TempStartPointL,
                                                        &cd->cd_TempStartPointR,
                                                         cd->cd_DataStart,
                                                        &dstptr,
                                                         cd->cd_FirstOffsetI,
                                                         cd->cd_Add,
                                                        &cd->cd_Offset, 
                                                         TRUE );

            cd->cd_Samples -= processed;
            samplesleft    -= processed;
          }
          else
          {
            processed = 0;
          }

          if( processed != try_samples ||             // Found zero-crossing
              try_samples <= cd->cd_AntiClickCount )  // End-of-sound or
                                                      // max delay reached
          {
            // We either found a zero-crossing, looked as far as
            // we were allowed to or reached the end of sound.
            
            // To be perfect, the we should not run this code if the 
            // end-of-sound was reached. Instead, since cd_Samples is zero
            // and the second cd_AddRoutine call below will have no effect,
            // we should just go onand the cd_Next#? variables will be copied
            // instead. However, that requires two sets of delay variables:
            // The one we have now and one delayed set of "next" variables.
            // I might do that another time, but not today...

            // Now start the delayed sound.

            if( cd->cd_VolDelayed )
            {
              cd->cd_VolDelayed = FALSE;
              cd->cd_VolumeLeft  = cd->cd_DelayedVolumeLeft;
              cd->cd_VolumeRight = cd->cd_DelayedVolumeRight;
              cd->cd_ScaleLeft   = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight  = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine  = cd->cd_DelayedAddRoutine;
            }

            if( cd->cd_FreqDelayed )
            {
              cd->cd_FreqDelayed = FALSE;
              cd->cd_FreqOK      = cd->cd_DelayedFreqOK;
              cd->cd_Add         = cd->cd_DelayedAdd;
              
              // Since we have advanced, cd_Samples must be recalculated!
              cd->cd_Samples     = CalcSamples( cd->cd_Add,
                                                cd->cd_Type,
                                                cd->cd_LastOffset,
                                                cd->cd_Offset );
            }

            if( cd->cd_SoundDelayed )
            {
              cd->cd_SoundDelayed = FALSE;

              cd->cd_SoundOK       = cd->cd_DelayedSoundOK;

              cd->cd_StartPointL   = 0;
              cd->cd_StartPointR   = 0;

              cd->cd_Offset        = cd->cd_DelayedOffset;
              cd->cd_FirstOffsetI  = cd->cd_DelayedFirstOffsetI;
              cd->cd_LastOffset    = cd->cd_DelayedLastOffset;
              cd->cd_DataStart     = cd->cd_DelayedDataStart;

              cd->cd_Type          = cd->cd_DelayedType;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;

              cd->cd_Samples       = cd->cd_DelayedSamples;

              cd->cd_ScaleLeft     = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight    = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;
            }
          }

          if( cd->cd_VolDelayed || cd->cd_FreqDelayed || cd->cd_SoundDelayed )
          {
            cd->cd_AntiClickCount -= processed;
          }
          else
          {
            cd->cd_AntiClickCount = 0;
          }
        }

        if( cd->cd_FreqOK && cd->cd_SoundOK )
        {
          // Sound is still ok, let's rock'n roll.

          samples = min( samplesleft, cd->cd_Samples );

          if( samples > 0 )
          {
            cd->cd_TempStartPointL   = cd->cd_StartPointL;
            cd->cd_TempStartPointR   = cd->cd_StartPointR;
            cd->cd_TempStartPointRL  = cd->cd_StartPointRL;
            cd->cd_TempStartPointRR  = cd->cd_StartPointRR;
            cd->cd_TempStartPointSL  = cd->cd_StartPointSL;
            cd->cd_TempStartPointSR  = cd->cd_StartPointSR;
            cd->cd_TempStartPointC   = cd->cd_StartPointC;
            cd->cd_TempStartPointLFE = cd->cd_StartPointLFE;
	    
	    processed = ((ADDFUNC *) cd->cd_AddRoutine)( samples,
                                                         cd->cd_ScaleLeft,
                                                         cd->cd_ScaleRight,
                                                        &cd->cd_TempStartPointL,
                                                        &cd->cd_TempStartPointR,
                                                         cd->cd_DataStart,
                                                        &dstptr,
                                                         cd->cd_FirstOffsetI,
                                                         cd->cd_Add,
                                                        &cd->cd_Offset,
                                                         FALSE );
            cd->cd_Samples -= processed;
            samplesleft    -= processed;
          }

          if( cd->cd_Samples == 0 )
          {
            /* Linear interpol. stuff */

	    ULONG lo = (ULONG) (cd->cd_LastOffset >> 32);

	    switch( cd->cd_Type ) {

	      case AHIST_M8S:
		cd->cd_StartPointL = ((BYTE*) cd->cd_DataStart)[ lo ] << 8;
		break;

	      case AHIST_S8S:
		cd->cd_StartPointL = ((BYTE*) cd->cd_DataStart)[ lo*2+0 ] << 8;
		cd->cd_StartPointR = ((BYTE*) cd->cd_DataStart)[ lo*2+1 ] << 8;
		break;

	      case AHIST_M16S:
		cd->cd_StartPointL = ((WORD*) cd->cd_DataStart)[ lo ];
		break;

	      case AHIST_S16S:
		cd->cd_StartPointL = ((WORD*) cd->cd_DataStart)[ lo*2+0 ];
		cd->cd_StartPointR = ((WORD*) cd->cd_DataStart)[ lo*2+1 ];
		break;

	      case AHIST_M32S:
		cd->cd_StartPointL = ((LONG*) cd->cd_DataStart)[ lo ] >> 16;
		break;

	      case AHIST_S32S:
		cd->cd_StartPointL = ((LONG*) cd->cd_DataStart)[ lo*2+0 ] >> 16;
		cd->cd_StartPointR = ((LONG*) cd->cd_DataStart)[ lo*2+1 ] >> 16;
		break;
	    }

//	    This old code is totally fucked up ... Why didn't anybody
//	    complain?!
//            cd->cd_StartPointL = cd->cd_TempStartPointL;
//            cd->cd_StartPointR = cd->cd_TempStartPointR;

/* 	    KPrintF( "cd->cd_StartPointL=%08lx, cd->cd_StartPointR=%08lx\n", */
/* 		     cd->cd_StartPointL, cd->cd_StartPointR ); */
            /*
            ** Offset always points OUTSIDE the sample after this
            ** call.  Ie, if we read a sample at offset (Offset.I)
            ** now, it does not belong to the sample just played.
            ** This is true for both backward and forward mixing.
            */

/* 	    KPrintF( "cd->cd_Add=0x%08lx:%08lx\n", */
/* 		     (ULONG) (cd->cd_Add >> 32), (ULONG) cd->cd_Add ); */
/* 	    KPrintF( "cd->cd_Offset=0x%08lx:%08lx, " */
/* 		     "cd->cd_LastOffset=0x%08lx:%08lx," */
/* 		     "cd->cd_FirstOffset=%08lx\n", */
/* 		     (ULONG) (cd->cd_Offset >> 32), (ULONG) cd->cd_Offset, */
/* 		     (ULONG) (cd->cd_LastOffset >> 32), (ULONG) cd->cd_LastOffset, */
/* 		     cd->cd_FirstOffsetI ); */
		     
            /* What we do now is to calculate how much futher we have
               advanced. */

              cd->cd_Offset -= cd->cd_LastOffset + 1;

/* 	    KPrintF( "cd->cd_Offset=0x%08lx:%08lx, " */
/* 		     "cd->cd_LastOffset=0x%08lx:%08lx," */
/* 		     "cd->cd_FirstOffset=%08lx\n", */
/* 		     (ULONG) (cd->cd_Offset >> 32), (ULONG) cd->cd_Offset, */
/* 		     (ULONG) (cd->cd_LastOffset >> 32), (ULONG) cd->cd_LastOffset, */
/* 		     cd->cd_FirstOffsetI ); */

            /*
            ** Offset should now be added to the NEXT Offset. Offset
            ** is positive of the sample was mixed forwards, and
            ** negative if the sample was mixed backwards.  There is
            ** one catch, however.  If the direction is about to
            ** change now, Offset should instead be SUBTRACTED.
            ** Let's check:
            */

            if( (cd->cd_Type ^ cd->cd_NextType) & AHIST_BW )
            {
              cd->cd_Offset = -cd->cd_Offset;
            }

            cd->cd_Offset += cd->cd_NextOffset;

//            cd->cd_FirstOffsetI = cd->cd_Offset >> 32;
            cd->cd_FirstOffsetI = cd->cd_NextOffset >> 32;

/* 	    KPrintF( "cd->cd_Offset=0x%08lx:%08lx, " */
/* 		     "cd->cd_LastOffset=0x%08lx:%08lx," */
/* 		     "cd->cd_FirstOffset=%08lx\n", */
/* 		     (ULONG) (cd->cd_Offset >> 32), (ULONG) cd->cd_Offset, */
/* 		     (ULONG) (cd->cd_LastOffset >> 32), (ULONG) cd->cd_LastOffset, */
/* 		     cd->cd_FirstOffsetI ); */
            /*
            ** But what if the next sample is so short that we just
            ** passed it!?  Here is the nice part.  CalcSamples
            ** checks this, and sets cd_Samples to 0 in that case.
            ** And the add routines doesn't do anything when asked to
            ** mix 0 samples.  Assume we have passed a sample with 4
            ** samples, and the next one is only 3.  CalcSamples
            ** returns 0.  The (ADDFUNC) call above does not do
            ** anything at all, OffsetI is still 4.  Now we subtract
            ** LastOffsetI, which is 3.  Result:  We have passed the
            ** sample with 1.  And guess what?  That's in range.
            */

            /* Now, let's copy the rest of the cd_Next#? stuff... */

            cd->cd_FreqOK        = cd->cd_NextFreqOK;
            cd->cd_SoundOK       = cd->cd_NextSoundOK;
            cd->cd_Add           = cd->cd_NextAdd;
            cd->cd_DataStart     = cd->cd_NextDataStart;
            cd->cd_LastOffset    = cd->cd_NextLastOffset;
            cd->cd_ScaleLeft     = cd->cd_NextScaleLeft;
            cd->cd_ScaleRight    = cd->cd_NextScaleRight;
            cd->cd_AddRoutine    = cd->cd_NextAddRoutine;
            cd->cd_VolumeLeft    = cd->cd_NextVolumeLeft;
            cd->cd_VolumeRight   = cd->cd_NextVolumeRight;
            cd->cd_Type          = cd->cd_NextType;

            cd->cd_Samples = CalcSamples( cd->cd_Add,
                                          cd->cd_Type,
                                          cd->cd_LastOffset,
                                          cd->cd_Offset );

            /* Also update all cd_Delayed#? stuff */

            if( !cd->cd_VolDelayed )
            {
              cd->cd_DelayedVolumeLeft    = cd->cd_NextVolumeLeft;
              cd->cd_DelayedVolumeRight   = cd->cd_NextVolumeRight;
            }

            if( !cd->cd_FreqDelayed )
            {
              cd->cd_DelayedFreqOK        = cd->cd_NextFreqOK;
              cd->cd_DelayedAdd           = cd->cd_NextAdd;
            }

            if( !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedSoundOK       = cd->cd_NextSoundOK;
              cd->cd_DelayedOffset        = cd->cd_NextOffset;
              cd->cd_DelayedFirstOffsetI  = cd->cd_FirstOffsetI;  // See above
              cd->cd_DelayedLastOffset    = cd->cd_NextLastOffset;
              cd->cd_DelayedType          = cd->cd_NextType;
              cd->cd_DelayedDataStart     = cd->cd_NextDataStart;
            }

            if( !cd->cd_VolDelayed && !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedScaleLeft     = cd->cd_NextScaleLeft;
              cd->cd_DelayedScaleRight    = cd->cd_NextScaleRight;
              cd->cd_DelayedAddRoutine    = cd->cd_NextAddRoutine;
            }

            if( !cd->cd_FreqDelayed && !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedSamples       = cd->cd_Samples;
            }

            cd->cd_EOS = TRUE;      // signal End-Of-Sample
            continue;               // .contchannel (same channel, new sound)
          }
        } // FreqOK && SoundOK
        break; // .contchannel

      } // while(TRUE)

      cd = cd->cd_Succ;
    } // while(cd)

    if(audioctrl->ahiac_WetOrDry == AHIEDM_WET)
    {
      audioctrl->ahiac_WetOrDry = AHIEDM_DRY;

      /*** AHIET_DSPECHO ***/
      if(audioctrl->ahiac_EffDSPEchoStruct != NULL)
      {
        audioctrl->ahiac_EffDSPEchoStruct->ahiecho_Code(
            audioctrl->ahiac_EffDSPEchoStruct, dst, audioctrl);
      }

      cd = audioctrl->ahiac_DryList;

      if(audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC)
      {
        /*** AHIET_MASTERVOLUME ***/

        DoMasterVolume(dst, audioctrl);

        /*
        ** When AHIACB_POSTPROC is set, the dry data shall be placed
        ** immediate after the wet data. This is done by modifying the
        ** dst pointer
        */

        dst = (char *) dst + audioctrl->ac.ahiac_BuffSamples * 
	  _AHI_SampleFrameSize( audioctrl->ac.ahiac_BuffType, AHIBase );
      }

      continue; /* while(TRUE) */
    }
    else
    {
      break; /* while(TRUE) */
    }
  } // while(TRUE)

  return;
}


/******************************************************************************
** DoMasterVolume *************************************************************
******************************************************************************/

/*
** This function would be better if it was written in assembler,
** since overflow could then be detected. Instead we reduce the
** number of bits to 20 and then scale and compare.
*/

void
DoMasterVolume( void *buffer,
                struct AHIPrivAudioCtrl *audioctrl )
{
  int   cnt;
  LONG  vol;
  LONG  sample;

  cnt = audioctrl->ac.ahiac_BuffSamples;

  switch(audioctrl->ac.ahiac_BuffType)
  {

    case AHIST_M16S:
    case AHIST_M32S:
      break;

    case AHIST_S16S:
    case AHIST_S32S:
      cnt *= 2;
      break;

    case AHIST_L7_1:
      cnt *= 8;
      break;
      
    default:
      return; // Panic
  }

  if( audioctrl->ac.ahiac_BuffType == AHIST_M32S
      || audioctrl->ac.ahiac_BuffType == AHIST_S32S
      || audioctrl->ac.ahiac_BuffType == AHIST_L7_1 )
  {
    LONG *dst = buffer;

    vol = audioctrl->ahiac_SetMasterVolume >> 8;

    while(cnt > 0)
    {
      cnt--;
    
      sample = (*dst >> 12) * vol;

      if(sample > (LONG) 0x07ffffff)
        sample = 0x07ffffff;
      else if(sample < (LONG) 0xf8000000)
        sample = 0xf8000000;

      *dst++ = sample << 4;
    }
  }
  else
  {
    WORD *dst = buffer;

    vol = audioctrl->ahiac_SetMasterVolume >> 4;

    while(cnt > 0)
    {
      cnt--;
    
      sample = *dst * vol;

      if(sample > (LONG) 0x07ffffff)
        sample = 0x07ffffff;
      else if(sample < (LONG) 0xf8000000)
        sample = 0xf8000000;

      *dst++ = sample >> 12;
    }
  
  }
}


/******************************************************************************
** DoOutputBuffer *************************************************************
******************************************************************************/

void
DoOutputBuffer( void *buffer,
                struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIEffOutputBuffer *ob;

  ob = audioctrl->ahiac_EffOutputBufferStruct;

  if(ob != NULL)
  {
    ob->ahieob_Buffer = buffer;
    ob->ahieob_Length = audioctrl->ac.ahiac_BuffSamples;
    ob->ahieob_Type   = audioctrl->ac.ahiac_BuffType;

    CallHookPkt( ob->ahieob_Func,
                 audioctrl,
                 ob);
  }
}


/******************************************************************************
** DoChannelInfo **************************************************************
******************************************************************************/

void
DoChannelInfo( struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIEffChannelInfo *ci;
  struct AHIChannelData    *cd;
  ULONG                    *offsets;

  ci = audioctrl->ahiac_EffChannelInfoStruct;

  if(ci != NULL)
  {
    int i;
    
    cd      = audioctrl->ahiac_ChannelDatas;
    offsets = ci->ahieci_Offset;

    for(i = ci->ahieci_Channels; i > 0; i--)
    {
      *offsets++ = cd->cd_Offset >> 32;
      cd++;
    }
    
    CallHookPkt( ci->ahieci_Func,
                 audioctrl,
                 ci );
  }
}


/******************************************************************************
** CalcSamples ****************************************************************
******************************************************************************/

LONG
CalcSamples ( Fixed64 Add,
              ULONG   Type,
              Fixed64 LastOffset,
              Fixed64 Offset )

{
  Fixed64 len;

  if( Type & AHIST_BW )
  {
    len = Offset - LastOffset; 
  }
  else
  {
    len = LastOffset - Offset;
  }

  if(len < 0 || Add == 0) return 0; // Error!

  return (LONG) ( len / Add ) + 1;
}
