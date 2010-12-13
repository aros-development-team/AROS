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
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>
#include <stdlib.h>

#include "ahi_def.h"
#include "debug.h"
#include "effectinit.h"
#include "mixer.h"

#ifdef __AMIGAOS4__
#define IAHIsub audioctrl->ahiac_IAHIsub
#endif

/******************************************************************************
** AHI_SetVol *****************************************************************
******************************************************************************/

/***** ahi.device/AHI_SetVol ***********************************************
*
*   NAME
*       AHI_SetVol -- set volume and stereo panning for a channel
*
*   SYNOPSIS
*       AHI_SetVol( channel, volume, pan, audioctrl, flags );
*                   D0:16    D1      D2   A2         D3
*
*       void AHI_SetVol( UWORD, Fixed, sposition, struct AHIAudioCtrl *,
*                        ULONG );
*
*   FUNCTION
*       Changes the volume and stereo panning for a channel.
*
*   INPUTS
*       channel - The channel to set volume for.
*       volume - The desired volume. Fixed is a LONG fixed-point value with
*           16 bits to the left of the point and 16 to the right
*           (typedef LONG Fixed; from IFF-8SVX docs).
*           Maximum volume is 1.0 (0x10000L) and 0.0 (0x0L) will turn off
*           this channel. Note: The sound will continue to play, but you
*           wont hear it. To stop a sound completely, use AHI_SetSound().
*           Starting with V4 volume can also be negative, which tells AHI
*           to invert the samples before playing. Note that all drivers
*           may not be able to handle negative volume. In that case the
*           absolute volume will be used.
*       pan - The desired panning. sposition is the same as Fixed
*           (typedef Fixed sposition; from IFF-8SVX.PAN docs).
*           1.0 (0x10000L) means that the sound is panned all the way to
*           the right, 0.5 (0x8000L) means the sound is centered and 0.0
*           (0x0L) means that the sound is panned all the way to the left.
*           Try to set Pan to the 'correct' value even if you know it has no
*           effect. For example, if you know you use a mono mode, set pan to
*           0.5 even if it does not matter.
*           Starting with V4 pan can also be negative, which tells AHI to
*           use the surround speaker for this channel. Note that all drivers
*           may not be able to handle negative pan. In that case the absolute
*           pan will be used.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidelines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*       Negative volume or negative pan may use more CPU time than positive.
*
*       Using both negative volume and negative pan will play the inverted
*       sound on the surround speaker.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(), AHI_SetFreq(), AHI_SetSound(), AHI_LoadSound()
*       
*
****************************************************************************
*
*/

ULONG
_AHI_SetVol ( UWORD                    channel,
	      Fixed                    volume,
	      sposition                pan,
	      struct AHIPrivAudioCtrl* audioctrl,
	      ULONG                    flags,
	      struct AHIBase*          AHIBase )
{
  struct AHIChannelData *cd;
  struct Library        *AHIsubBase;
  ULONG                  rc;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    Debug_SetVol(channel, volume, pan, audioctrl, flags);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_SetVol(channel, volume, pan, &audioctrl->ac, flags);

  if(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING
     || rc != AHIS_UNKNOWN)
  {
    return 0;   /* We're done! */
  }

  cd = &audioctrl->ahiac_ChannelDatas[channel];

  if((audioctrl->ac.ahiac_Flags & AHIACF_VOL) == 0)
  {
    volume = volume & 0x10000;  /* |volume|=0 or 0x10000 */
  }

  if((audioctrl->ac.ahiac_Flags & AHIACF_PAN) == 0)
  {
    pan = (channel & 1) << 16;  /* pan = 0 or 0x10000 */
  }

  AHIsub_Disable(&audioctrl->ac);

  cd->cd_NextVolumeLeft  = ((volume >> 1) * ((0x10000 - abs(pan)) >> 1)) >> (16 - 2);
  cd->cd_NextVolumeRight = ((volume >> 1) * (pan >> 1)) >> (16 - 2);

  SelectAddRoutine( cd->cd_NextVolumeLeft,
                    cd->cd_NextVolumeRight,
                    cd->cd_NextType,
                    audioctrl,
                   &cd->cd_NextScaleLeft,
                   &cd->cd_NextScaleRight,
       (ADDFUNC**) &cd->cd_NextAddRoutine );

  if(flags & AHISF_IMM)
  {
    cd->cd_DelayedVolumeLeft  = cd->cd_NextVolumeLeft;
    cd->cd_DelayedVolumeRight = cd->cd_NextVolumeRight;

    SelectAddRoutine( cd->cd_DelayedVolumeLeft,
                      cd->cd_DelayedVolumeRight, 
                      cd->cd_DelayedType, 
                      audioctrl,
                     &cd->cd_DelayedScaleLeft,
                     &cd->cd_DelayedScaleRight, 
         (ADDFUNC**) &cd->cd_DelayedAddRoutine );

    /* Enable anti-click routine */
    cd->cd_AntiClickCount = audioctrl->ac.ahiac_AntiClickSamples;

    if( ( flags & AHISF_NODELAY ) || 
          ( cd->cd_AntiClickCount == 0 ) ||
           !cd->cd_FreqOK || !cd->cd_SoundOK )
    {
      cd->cd_VolumeLeft  = cd->cd_DelayedVolumeLeft;
      cd->cd_VolumeRight = cd->cd_DelayedVolumeRight;
      cd->cd_ScaleLeft   = cd->cd_DelayedScaleLeft;
      cd->cd_ScaleRight  = cd->cd_DelayedScaleRight;
      cd->cd_AddRoutine  = cd->cd_DelayedAddRoutine;

      cd->cd_AntiClickCount = 0;
    }
    else
    {
      cd->cd_VolDelayed = TRUE;
    }
  }

  AHIsub_Enable(&audioctrl->ac);
  return 0;
}



/******************************************************************************
** AHI_SetFreq ****************************************************************
******************************************************************************/

/***** ahi.device/AHI_SetFreq **********************************************
*
*   NAME
*       AHI_SetFreq -- set frequency for a channel
*
*   SYNOPSIS
*       AHI_SetFreq( channel, freq, audioctrl, flags );
*                    D0:16    D1    A2         D2
*
*       void AHI_SetFreq( UWORD, ULONG, struct AHIAudioCtrl *, ULONG );
*
*   FUNCTION
*       Sets the playback frequency for a channel.
*
*   INPUTS
*       channel - The channel to set playback frequency for.
*       freq - The playback frequency in Hertz. Can also be AHI_MIXFREQ,
*           which is the current mixing frequency, or 0 to temporary stop
*           the sound (it will restart at the same point when its frequency
*           changed).
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidelines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(),  AHI_SetSound(), AHI_SetVol(), AHI_LoadSound()
*
****************************************************************************
*
*/

ULONG
_AHI_SetFreq ( UWORD                    channel,
	       ULONG                    freq,
	       struct AHIPrivAudioCtrl* audioctrl,
	       ULONG                    flags,
	       struct AHIBase*          AHIBase )
{
  struct AHIChannelData *cd;
  struct Library        *AHIsubBase;
  ULONG                  rc;
  ULONG                  add;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    Debug_SetFreq(channel, freq, audioctrl, flags);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_SetFreq(channel, freq, &audioctrl->ac, flags);

  if(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING
     || rc != AHIS_UNKNOWN)
  {
    return 0;   /* We're done! */
  }

  cd = &audioctrl->ahiac_ChannelDatas[channel];

  if(freq == 0)
  {
    cd->cd_NextFreqOK = FALSE;

    add = 0;
  }
  else
  {
    cd->cd_NextFreqOK = TRUE;

    if(freq == AHI_MIXFREQ)
    {
      add = 0x10000;
    }
    else
    {
      int shift = 0;
      
      while(freq >= 65536)
      {
        shift++;
        freq >>= 1;
      }

      add = ((freq << 16) / audioctrl->ac.ahiac_MixFreq) << shift;
    }
  }
  
  AHIsub_Disable(&audioctrl->ac);

  cd->cd_NextAdd = (Fixed64) add << 16;

  if(flags & AHISF_IMM)
  {
    cd->cd_DelayedAdd     = cd->cd_NextAdd;
    cd->cd_DelayedFreqOK  = cd->cd_NextFreqOK;

    // cd->cd_Samples is also calculated when it actually happens¹...

    cd->cd_DelayedSamples = CalcSamples( cd->cd_DelayedAdd,
                                         cd->cd_DelayedType,
                                         cd->cd_DelayedLastOffset,
                                         cd->cd_DelayedOffset );

    /* Enable anti-click routine */
    cd->cd_AntiClickCount = audioctrl->ac.ahiac_AntiClickSamples;

    if( ( flags & AHISF_NODELAY ) || 
        ( cd->cd_AntiClickCount == 0 ) ||
        !cd->cd_FreqOK || !cd->cd_SoundOK )
    {
      cd->cd_Add     = cd->cd_DelayedAdd;
      cd->cd_FreqOK  = cd->cd_DelayedFreqOK;

      // ¹) Unless we're not using any delay, in which case it's recalculated
      //    here instead.

      cd->cd_Samples = CalcSamples( cd->cd_Add,
                                    cd->cd_Type,
                                    cd->cd_LastOffset,
                                    cd->cd_Offset );

      cd->cd_AntiClickCount = 0;
    }
    else
    {
      cd->cd_FreqDelayed = TRUE;
    }
  }

  AHIsub_Enable(&audioctrl->ac);

  return 0;
}


/******************************************************************************
** AHI_SetSound ***************************************************************
******************************************************************************/

/***** ahi.device/AHI_SetSound *********************************************
*
*   NAME
*       AHI_SetSound -- set what sound to play for a channel
*
*   SYNOPSIS
*       AHI_SetSound( channel, sound, offset, length, audioctrl, flags );
*                      D0:16   D1:16   D2      D3      A2         D4
*
*       void AHI_SetSound( UWORD, UWORD, ULONG, LONG,
*                          struct AHIAudioCtrl *, ULONG );
*
*   FUNCTION
*       Sets a sound to be played on a channel.
*
*   INPUTS
*       channel - The channel to set sound for.
*       sound - Sound to be played, or AHI_NOSOUND to turn the channel off.
*       offset - Only available if the sound type is AHIST_SAMPLE or
*           AHIST_DYNAMICSAMPLE. Must be 0 otherwise.
*           Specifies an offset (in samples) where the playback will begin.
*           If you wish to play the whole sound, set offset to 0.
*       length - Only available if the sound type is AHIST_SAMPLE or
*           AHIST_DYNAMICSAMPLE. Must be 0 otherwise.
*           Specifies how many samples that should be played. If you
*           wish to play the whole sound forwards, set offset to 0 and length
*           to either 0 or the length of the sample array. You may not set
*           length to 0 if offset is not 0! To play a sound backwards, just
*           set length to a negative number.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidelines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*       If offset or length is not zero, make sure you do not exceed the
*       sample limits.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(),  AHI_SetFreq(), AHI_SetVol(), AHI_LoadSound()
*
****************************************************************************
*
*/

ULONG
_AHI_SetSound ( UWORD                    channel,
		UWORD                    sound,
		ULONG                    offset,
		LONG                     length,
		struct AHIPrivAudioCtrl* audioctrl,
		ULONG                    flags,
		struct AHIBase*          AHIBase )
{
  struct AHIChannelData *cd;
  struct AHISoundData   *sd;
  struct Library        *AHIsubBase;
  ULONG                  rc;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    Debug_SetSound(channel, sound, offset, length, audioctrl, flags);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_SetSound(channel, sound, offset, length, &audioctrl->ac, flags);

  if(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING
     || rc != AHIS_UNKNOWN)
  {
    return 0;   /* We're done! */
  }

  cd = &audioctrl->ahiac_ChannelDatas[channel];
  sd = &audioctrl->ahiac_SoundDatas[sound];

  AHIsub_Disable(&audioctrl->ac);

  if(sound == AHI_NOSOUND)
  {
    cd->cd_NextSoundOK    = FALSE;

    if(flags & AHISF_IMM)
    {
      cd->cd_EOS          = TRUE;  /* Signal End-Of-Sample */
      cd->cd_SoundOK      = FALSE;
    }
  }
  else if(sd->sd_Type != AHIST_NOTYPE) /* This is a user error, shouldn't happen! */
  {
    if(length == 0) length = sd->sd_Length;

    cd->cd_NextDataStart = sd->sd_Addr;
    cd->cd_NextType      = sd->sd_Type;
    cd->cd_NextOffset    = (Fixed64) offset << 32 ;

    cd->cd_NextSoundOK   = TRUE;

    if(length < 0)
    {
      cd->cd_NextType         |= AHIST_BW;
      cd->cd_NextLastOffset    = (Fixed64) ( offset + length + 1 ) << 32;
//      cd->cd_NextOffset       |= 0xffffffffLL;
      *(ULONG*) ((char*) (&cd->cd_NextOffset)+4) = 0xffffffffUL;
    }
    else
    {
#if !defined( __mc68000__ )
      cd->cd_NextLastOffset    = ( (Fixed64) ( offset + length - 1 ) << 32 )
                                 | (Fixed64) 0xffffffffLL;
#else
      // Fix for m68k compiler bug! :-(
      *(ULONG*) &cd->cd_NextLastOffset = offset + length - 1;
      *(ULONG*) ((char*) (&cd->cd_NextLastOffset)+4) = 0xffffffffUL;
#endif

      /* Low cd->cd_NextOffset already 0 */
    }

    SelectAddRoutine( cd->cd_NextVolumeLeft,
                      cd->cd_NextVolumeRight,
                      cd->cd_NextType,
                      audioctrl,
                     &cd->cd_NextScaleLeft,
                     &cd->cd_NextScaleRight,
         (ADDFUNC**) &cd->cd_NextAddRoutine );

    if(flags & AHISF_IMM)
    {
      cd->cd_DelayedOffset        = cd->cd_NextOffset;
      cd->cd_DelayedFirstOffsetI  = cd->cd_NextOffset >> 32; /* for linear interpol. */
      cd->cd_DelayedLastOffset    = cd->cd_NextLastOffset;
      cd->cd_DelayedDataStart     = cd->cd_NextDataStart;
      cd->cd_DelayedType          = cd->cd_NextType;
      cd->cd_DelayedSoundOK       = cd->cd_NextSoundOK;

      SelectAddRoutine( cd->cd_DelayedVolumeLeft,
                        cd->cd_DelayedVolumeRight,
                        cd->cd_DelayedType,
                        audioctrl,
                       &cd->cd_DelayedScaleLeft,
                       &cd->cd_DelayedScaleRight,
           (ADDFUNC**) &cd->cd_DelayedAddRoutine );

      cd->cd_DelayedSamples = CalcSamples( cd->cd_DelayedAdd,
                                           cd->cd_DelayedType,
                                           cd->cd_DelayedLastOffset,
                                           cd->cd_DelayedOffset );

      /* Enable anti-click routine */
      cd->cd_AntiClickCount = audioctrl->ac.ahiac_AntiClickSamples;

      if( ( flags & AHISF_NODELAY ) || 
          ( cd->cd_AntiClickCount == 0 ) ||
           !cd->cd_FreqOK || !cd->cd_SoundOK )
      {
        cd->cd_StartPointL   = 0;
        cd->cd_StartPointR   = 0;

        cd->cd_Offset        = cd->cd_DelayedOffset;
        cd->cd_FirstOffsetI  = cd->cd_DelayedFirstOffsetI;
        cd->cd_LastOffset    = cd->cd_DelayedLastOffset;
        cd->cd_DataStart     = cd->cd_DelayedDataStart;
        cd->cd_Type          = cd->cd_DelayedType;
        cd->cd_SoundOK       = cd->cd_DelayedSoundOK;
        cd->cd_ScaleLeft     = cd->cd_DelayedScaleLeft;
        cd->cd_ScaleRight    = cd->cd_DelayedScaleRight;
        cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;
        cd->cd_Samples       = cd->cd_DelayedSamples;
        cd->cd_AntiClickCount = 0;
      }
      else
      {
        cd->cd_SoundDelayed = TRUE;
      }

      cd->cd_EOS = TRUE;  /* Signal End-Of-Sample */
    }
  }

  AHIsub_Enable(&audioctrl->ac);

  return 0;
}


/******************************************************************************
** AHI_SetEffect **************************************************************
******************************************************************************/

/***** ahi.device/AHI_SetEffect ********************************************
*
*   NAME
*       AHI_SetEffect -- set effect
*
*   SYNOPSIS
*       error = AHI_SetEffect( effect, audioctrl );
*       d0                     A0      A2
*
*       ULONG AHI_SetEffect( APTR, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Selects an effect to be used, described by a structure.
*
*   INPUTS
*       effect - A pointer to an effect data structure, as defined in
*           <devices/ahi.h>. The following effects are defined:
*           AHIET_MASTERVOLUME - Changes the volume for all channels. Can
*               also be used to boost volume over 100%.
*           AHIET_OUTPUTBUFFER - Gives READ-ONLY access to the mixed output.
*               Can be used to show nice scopes and VU-meters.
*           AHIET_DSPMASK - Select which channels will be affected by the
*               DSP effects. (V4)
*           AHIET_DSPECHO - A DSP effects that adds (cross-)echo and delay.
*               (V4)
*           AHIET_CHANNELINFO - Get info about all channels. (V4)
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   EFFECTS
*       AHIET_MASTERVOLUME - Effect is a struct AHIEffMasterVolume, with
*           ahiemv_Volume set to the desired volume. The range is 0 to
*           (channels/hardware channel). Assume you have 4 channels in
*           mono mode. The range is then 0.0 to 4.0. The range is the same
*           if the mode is stereo with panning. However, assume you have 4
*           channels with a stereo mode *without* panning. Then you have two
*           channels to the left and two to the right => range is 0.0 - 2.0.
*           Setting the volume outside the range will give an unpredictable
*           result!
*
*       AHIET_OUTPUTBUFFER - Effect is a struct AHIEffOutputBuffer, with
*           ahieob_Func pointing to a hook that will be called with the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIEffOutputBuffer *)
*           The information you are looking for then is in ahieob_Type,
*           ahieob_Buffer and ahieob_Length. Always check ahieob_Type!
*           ahieob_Length is neither in bytes nor samples, but sample frames.
*
*       AHIET_DSPMASK - Effect is a struct AHIEffDSPMask, where ahiedm_Mask
*           is an array with ahiedm_Channels elements. Each UBYTE in the
*           array can either make the channel 'wet' (affected by the DSP
*           effects), by using the AHIEDM_WET constant or 'dry' (not
*           affected by the DSP effects) by using the AHIEDM_DRY constant.
*           The default is all channels wet. If ahiedm_Channels does not
*           equal the current number of channels allocated, the result of
*           this call is undefined (crash warning!). (V4)
*
*       AHIET_DSPECHO - Effect is a struct AHIEffDSPEcho.
*           ahiede_Delay is the delay in samples (and thus depends on the
*           mixing rate).
*
*           ahiede_Feedback is a Fixed value between 0 and 1.0, and defines
*           how much of the delayed signal should be feed back to the delay
*           stage. Setting this to 0 gives a delay effect, otherwise echo.
*
*           ahiede_Mix tells how much of the delayed signal should be mixed
*           with the normal signal. Setting this to 0 disables delay/echo,
*           and setting it to 1.0 outputs only the delay/echo signal.
*
*           ahiede_Cross only has effect of the current playback mode is
*           stereo. It tells how the delayed signal should be panned to
*           the other channel. 0 means no cross echo, 1.0 means full
*           cross echo.
*
*           If the user has enabled "Fast Echo", AHI may take several short-
*           cuts to increase the performance. This could include rounding the
*           parameters to a power of two, or even to the extremes. 
*
*           If you set ahiede_Mix to 0x10000 and ahiede_Cross to 0x0, much
*           faster mixing routines will be used, and "Fast Echo" will improve
*           that even more.
*
*           Otherwise, even with "Fast Echo" turned on, this effect will 
*           probably suck some major CPU cycles on most sound hardware. (V4)
*
*       AHIET_CHANNELINFO - Effect is a struct AHIEffChannelInfo, where
*           ahieci_Func is pointing to a hook that will be called with the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIEffChannelInfo *)
*           ahieci_Channels must equal the current number of channels used.
*           ahieci_Offset is an array of ULONGs, which will be filled by
*           AHI before the hook is called (the offset is specified in sample
*           frames). The array must have at least ahieci_Channels elements.
*
*           This "effect" can be used to find out how far each channel has
*           played. You must probably keep track of the other parameters
*           yourself (like which sound is playing, it's volume, balance and
*           frequency etc) in order have meaningful usage of the information.
*           (V4)
*
*
*       NOTE! To turn off an effect, call again with ahie_Effect OR:ed
*       with AHIET_CANCEL. For example, it is NOT correct to disable
*       the AHIET_MASTERVOLUME effect by setting ahiemv_Volume to 1.0!
*
*       It is important that you always turn off effects before you
*       deallocate the audio hardware. Otherwise memory may be lost.
*       It is safe to turn off an effect that has never been turned on
*       in the first place.
*
*       Never count on that an effect will be available. For example,
*       AHIET_OUTPUTBUFFER is impossible to implement with some sound
*       cards.
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       Unlike the other functions whose names begin with "AHI_Set", this
*       function may NOT be called from an interrupt (or AHI Hook).
*
*       Previous to V4, this call always returned AHIE_OK.
*
*   BUGS
*       The idea of updating the source structure instead of allocating
*       a new one that is passed the hook it pretty flawed. The reason is
*       that AHI_SetEffect() originally could be called from interrupts,
*       and memory allocation is not allowed from within interrupts.
*
*   SEE ALSO
*       AHI_SetFreq(), AHI_SetSound(), AHI_SetVol(), AHI_LoadSound(),
*       <devices/ahi.h>
*
****************************************************************************
*
*/


ULONG
_AHI_SetEffect( ULONG*                   effect,
		struct AHIPrivAudioCtrl* audioctrl,
		struct AHIBase*          AHIBase )
{
  struct Library        *AHIsubBase;
  ULONG                  rc;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    Debug_SetEffect(effect, audioctrl);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_SetEffect(effect, &audioctrl->ac);

  if(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING
     || rc != AHIS_UNKNOWN)
  {
    return rc;   /* We're done! */
  }


  switch(*effect)
  {

  /*
   *  MASTERVOLUME
   */

    case AHIET_MASTERVOLUME:
    {
      struct AHIEffMasterVolume *emv = (struct AHIEffMasterVolume *) effect;
      
      if(audioctrl->ahiac_SetMasterVolume != emv->ahiemv_Volume)
      {
        audioctrl->ahiac_SetMasterVolume = emv->ahiemv_Volume;
        rc = update_MasterVolume( audioctrl );
      }
      break;
    }

    case AHIET_MASTERVOLUME|AHIET_CANCEL:

      if(audioctrl->ahiac_SetMasterVolume != 0x10000)
      {
        audioctrl->ahiac_SetMasterVolume = 0x10000;
        rc = update_MasterVolume( audioctrl );
      }
      break;    

  /*
   *  OUTPUTBUFFER
   */

    case AHIET_OUTPUTBUFFER:
      audioctrl->ahiac_EffOutputBufferStruct = (struct AHIEffOutputBuffer *) effect;
      rc = AHIE_OK;
      break;

    case AHIET_OUTPUTBUFFER|AHIET_CANCEL:
      audioctrl->ahiac_EffOutputBufferStruct = NULL;
      rc = AHIE_OK;
      break;


  /*
   *  DSPMASK
   */

    case AHIET_DSPMASK:
      rc = update_DSPMask( (struct AHIEffDSPMask *) effect, audioctrl );
      break;
      
    case AHIET_DSPMASK|AHIET_CANCEL:
      clear_DSPMask( audioctrl );
      rc = AHIE_OK;
      break;


  /*
   *  DSPECHO
   */

    case AHIET_DSPECHO:
      rc = update_DSPEcho( (struct AHIEffDSPEcho *) effect, audioctrl );
      break;
      
    case AHIET_DSPECHO|AHIET_CANCEL:
      free_DSPEcho( audioctrl );
      rc = AHIE_OK;
      break;


  /*
   *  CHANNELINFO
   */

    case AHIET_CHANNELINFO:
      audioctrl->ahiac_EffChannelInfoStruct = (struct AHIEffChannelInfo *) effect;
      rc = AHIE_OK;
      break;
      
    case AHIET_CHANNELINFO|AHIET_CANCEL:
      audioctrl->ahiac_EffChannelInfoStruct = NULL;
      rc = AHIE_OK;
      break;
  }

  return rc;
}


/******************************************************************************
** AHI_LoadSound **************************************************************
******************************************************************************/

/****** ahi.device/AHI_LoadSound ********************************************
*
*   NAME
*       AHI_LoadSound -- prepare a sound for playback
*
*   SYNOPSIS
*       error = AHI_LoadSound( sound, type, info, audioctrl );
*       D0                     D0:16  D1    A0    A2
*
*       ULONG AHI_LoadSound( UWORD, ULONG, APTR, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Defines an ID number for the sound and prepares it for playback.
*
*   INPUTS
*       sound - The numeric ID to be used as a reference to this sound.
*           The ID is a number greater or equal to 0 and less than what you
*           specified with AHIA_Sounds when you called AHI_AllocAudioA().
*       type - The type of the sound. Currently four types are supported:
*           AHIST_SAMPLE - array of 8 or 16 bit samples. Note that the
*               portion of memory where the sample is stored must NOT be
*               altered until AHI_UnloadSound() has been called! This is
*               because some audio drivers may wish to upload the samples
*               to local RAM. It is OK to read, though.
*
*           AHIST_DYNAMICSAMPLE - array of 8 or 16 bit samples, which can be
*               updated dynamically. Typically used to play data that is
*               loaded from disk or calculated realtime.
*               Avoid using this sound type as much as possible; it will
*               use much more CPU power than AHIST_SAMPLE on a DMA/DSP
*               sound card.
*
*       info - Depends on type:
*           AHIST_SAMPLE - A pointer to a struct AHISampleInfo, filled with:
*               ahisi_Type - Format of samples (four formats are supported).
*                   AHIST_M8S: Mono, 8 bit signed (BYTEs).
*                   AHIST_S8S: Stereo, 8 bit signed (2×BYTEs) (V4). 
*                   AHIST_M16S: Mono, 16 bit signed (WORDs).
*                   AHIST_S16S: Stereo, 16 bit signed (2×WORDs) (V4).
*                   AHIST_M32S: Mono, 32 bit signed (LONGs). (V6)
*                   AHIST_S32S: Stereo, 32 bit signed (2×LONGs) (V6).
*                   AHIST_L7_1: 7.1, 32 bit signed (8×LONGs) (V6).
*               ahisi_Address - Address to the sample array.
*               ahisi_Length - The size of the array, in samples.
*               Don't even think of setting ahisi_Address to 0 and
*               ahisi_Length to 0xffffffff as you can do with
*               AHIST_DYNAMICSAMPLE! Very few DMA/DSP cards have 4 GB onboard
*               RAM...
*
*           AHIST_DYNAMICSAMPLE A pointer to a struct AHISampleInfo, filled
*               as described above (AHIST_SAMPLE).
*               If ahisi_Address is 0 and ahisi_Length is 0xffffffff
*               AHI_SetSound() can take the real address of an 8 bit sample
*               to be played as offset argument. Unfortunately, this does not
*               work for 16 bit samples.
*
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       There is no need to place a sample array in Chip memory, but it
*       MUST NOT be swapped out! Allocate your sample memory with the
*       MEMF_PUBLIC flag set. 
*
*   BUGS
*       AHIST_L7_1 can only be played using 7.1 audio modes -- it will NOT
*       be downmixed! It can't be played backwards either.
*
*   SEE ALSO
*       AHI_UnloadSound(), AHI_SetEffect(), AHI_SetFreq(), AHI_SetSound(),
*       AHI_SetVol(), <devices/ahi.h>
*
****************************************************************************
*
*/

ULONG
_AHI_LoadSound( UWORD                    sound,
		ULONG                    type,
		APTR                     info,
		struct AHIPrivAudioCtrl* audioctrl,
		struct AHIBase*          AHIBase )
{

  struct Library *AHIsubBase;
  ULONG rc;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_LoadSound(sound, type, info, audioctrl);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_LoadSound(sound, type, info, (struct AHIAudioCtrlDrv *) audioctrl);

  if((audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING) || (rc != AHIS_UNKNOWN))
  {
    return rc;
  }

  rc = AHIE_OK;

  switch(type)
  {
    case AHIST_DYNAMICSAMPLE:
    case AHIST_SAMPLE:
    {
      struct AHISampleInfo *si = (struct AHISampleInfo *) info;

      switch(si->ahisi_Type)
      {
        case AHIST_L7_1:
	  if( (audioctrl->ac.ahiac_Flags & AHIACF_HIFI) == 0 )
	  {
	    rc = AHIE_BADSAMPLETYPE;
	    break;
	  }
	  else
	  {
	    // Fall through ...
	  }
	  
        case AHIST_M8S:
        case AHIST_M16S:
        case AHIST_S8S:
        case AHIST_S16S:
        case AHIST_M32S:
        case AHIST_S32S:
          /* AHI_FreeAudio() will deallocate...  */

          audioctrl->ahiac_SoundDatas[sound].sd_Type   = si->ahisi_Type;
          audioctrl->ahiac_SoundDatas[sound].sd_Addr   = si->ahisi_Address;
          audioctrl->ahiac_SoundDatas[sound].sd_Length = si->ahisi_Length;
          break;

        default:
          rc = AHIE_BADSAMPLETYPE;
	  break;
      }
      
      break;
    }
 
    default:
      rc = AHIE_BADSOUNDTYPE;
      break;
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n", rc);
  }
  return rc;
}


/******************************************************************************
** AHI_UnloadSound ************************************************************
******************************************************************************/

/****** ahi.device/AHI_UnloadSound *****************************************
*
*   NAME
*       AHI_UnloadSound -- discard a sound
*
*   SYNOPSIS
*       AHI_UnloadSound( sound, audioctrl );
*                        D0:16  A2
*
*       void AHI_UnloadSound( UWORD, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Tells 'ahi.device' that this sound will not be used anymore.
*
*   INPUTS
*       sound - The ID of the sound to unload.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       This call will not break a Forbid() state.
*
*   BUGS
*
*   SEE ALSO
*       AHI_LoadSound()
*
****************************************************************************
*
*/

ULONG
_AHI_UnloadSound( UWORD                    sound,
		  struct AHIPrivAudioCtrl* audioctrl,
		  struct AHIBase*          AHIBase )
{
  struct Library *AHIsubBase;
  ULONG rc;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_UnloadSound(sound, audioctrl);
  }

  AHIsubBase = audioctrl->ahiac_SubLib;

  rc = AHIsub_UnloadSound(sound, (struct AHIAudioCtrlDrv *) audioctrl);

  if((audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING) || (rc != AHIS_UNKNOWN))
  {
    return 0;
  }
  
  audioctrl->ahiac_SoundDatas[sound].sd_Type = AHIST_NOTYPE;
  
  return 0;
}


/******************************************************************************
** AHI_PlayA ******************************************************************
******************************************************************************/

/****** ahi.device/AHI_PlayA ************************************************
*
*   NAME
*       AHI_PlayA -- Start multiple sounds in one call (V4)
*       AHI_Play -- varargs stub for AHI_PlayA()
*
*   SYNOPSIS
*       AHI_PlayA( audioctrl, tags );
*                  A2         A1
*
*       void AHI_PlayA( struct AHIAudioCtrl *, struct TagItem * );
*
*       AHI_Play( AudioCtrl, tag1, ...);
*
*       void AHI_Play( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function performs the same actions as multiple calls to
*       AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). The advantages
*       of using only one call is that simple loops can be set without
*       using a SoundFunc (see AHI_AllocAudioA(), tag AHIA_SoundFunc) and
*       that sounds on different channels can be synchronized even when the
*       sounds are not started from a PlayerFunc (see AHI_AllocAudioA(), tag
*       AHIA_PlayerFunc). The disadvantage is that this call has more
*       overhead than AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). It is
*       therefore recommended that you only use this call if you are not
*       calling from a SoundFunc or PlayerFunc.
*
*       The supplied tag list works like a 'program'. This means that
*       the order of tags matter.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIP_BeginChannel (UWORD) - Before you start setting attributes
*           for a sound to play, you have to use this tag to chose a
*           channel to operate on. If AHIP_BeginChannel is omitted, the
*           result is undefined.
*
*       AHIP_EndChannel (ULONG) - Signals the end of attributes for
*           the current channel. If AHIP_EndChannel is omitted, the result
*           is undefined. ti_Data MUST BE NULL!
*
*       AHIP_Freq (ULONG) - The playback frequency in Hertz or AHI_MIXFREQ.
*
*       AHIP_Vol (Fixed) - The desired volume. If omitted, but AHIP_Pan is
*           present, AHIP_Vol defaults to 0.
*
*       AHIP_Pan (sposition) - The desired panning. If omitted, but AHIP_Vol
*           is present, AHIP_Pan defaults to 0 (extreme left).
*
*       AHIP_Sound (UWORD) - Sound to be played, or AHI_NOSOUND.
*
*       AHIP_Offset (ULONG) - Specifies an offset (in samples) into the
*           sound. If this tag is present, AHIP_Length MUST be present too!
*
*       AHIP_Length (LONG) - Specifies how many samples that should be
*           played.
*
*       AHIP_LoopFreq (ULONG)
*       AHIP_LoopVol (Fixed)
*       AHIP_LoopPan (sposition)
*       AHIP_LoopSound (UWORD)
*       AHIP_LoopOffset (ULONG)
*       AHIP_LoopLength (LONG) - These tags can be used to set simple loop
*          attributes. They default to their sisters. These tags must be
*          after the other tags.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetFreq(), AHI_SetSound(), AHI_SetVol()
*
****************************************************************************
*
*/

ULONG
_AHI_PlayA( struct AHIPrivAudioCtrl* audioctrl,
	    struct TagItem*          tags,
	    struct AHIBase*          AHIBase )
{
  struct TagItem *tag,*tstate=tags;
  struct Library *AHIsubBase;
  BOOL  setfreq = FALSE, setvol = FALSE, setsound = FALSE,
        loopsetfreq = FALSE, loopsetvol = FALSE, loopsetsound = FALSE;
  ULONG channel = 0,
        freq = 0, vol = 0, pan = 0, sound = 0, offset = 0, length = 0;
  ULONG loopfreq = 0, loopvol = 0, looppan = 0, loopsound = 0,
        loopoffset = 0, looplength = 0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    Debug_PlayA(audioctrl,tags);
  }

  AHIsubBase = ((struct AHIPrivAudioCtrl *)audioctrl)->ahiac_SubLib;

  AHIsub_Disable((struct AHIAudioCtrlDrv *)audioctrl);

  for( tag = NextTagItem( &tstate );
       tag != NULL;
       tag = NextTagItem( &tstate ) )
  {
    switch(tag->ti_Tag)
    {
    case AHIP_BeginChannel:
      channel=tag->ti_Data;
      setfreq=setvol=setsound=loopsetfreq=loopsetvol=loopsetsound= \
      vol=pan=offset=length=loopvol=looppan=loopoffset=looplength=0;
      break;
    case AHIP_Freq:
      loopfreq=
      freq=tag->ti_Data;
      setfreq=TRUE;
      break;
    case AHIP_Vol:
      loopvol=
      vol=tag->ti_Data;
      setvol=TRUE;
      break;
    case AHIP_Pan:
      looppan=
      pan=tag->ti_Data;
      setvol=TRUE;
      break;
    case AHIP_Sound:
      loopsound=
      sound=tag->ti_Data;
      setsound=TRUE;
      break;
    case AHIP_Offset:
      loopoffset=
      offset=tag->ti_Data;
      break;
    case AHIP_Length:
      looplength=
      length=tag->ti_Data;
      break;
    case AHIP_LoopFreq:
      loopfreq=tag->ti_Data;
      loopsetfreq=TRUE;
      break;
    case AHIP_LoopVol:
      loopvol=tag->ti_Data;
      loopsetvol=TRUE;
      break;
    case AHIP_LoopPan:
      looppan=tag->ti_Data;
      loopsetvol=TRUE;
      break;
    case AHIP_LoopSound:
      loopsound=tag->ti_Data;
      loopsetsound=TRUE;
      break;
    case AHIP_LoopOffset:
      loopoffset=tag->ti_Data;
      loopsetsound=TRUE;           // AHIP_LoopSound: doesn't have to be present
      break;
    case AHIP_LoopLength:
      looplength=tag->ti_Data;
      break;
    case AHIP_EndChannel:
      if(setfreq)
        AHI_SetFreq( channel, freq, (struct AHIAudioCtrl*) audioctrl, AHISF_IMM );
      if(loopsetfreq)
        AHI_SetFreq( channel, loopfreq, (struct AHIAudioCtrl*) audioctrl, AHISF_NONE );
      if(setvol)
        AHI_SetVol( channel, vol, pan, (struct AHIAudioCtrl*) audioctrl, AHISF_IMM );
      if(loopsetvol)
        AHI_SetVol( channel, loopvol, looppan, (struct AHIAudioCtrl*) audioctrl, AHISF_NONE );
      if(setsound)
        AHI_SetSound( channel, sound, offset, length, (struct AHIAudioCtrl*) audioctrl, AHISF_IMM );
      if(loopsetsound)
        AHI_SetSound( channel, loopsound, loopoffset, looplength, (struct AHIAudioCtrl*) audioctrl, AHISF_NONE);
      break;
    }
  }

  AHIsub_Enable((struct AHIAudioCtrlDrv *)audioctrl);
  return 0;
}


/******************************************************************************
** AHI_SampleFrameSize ********************************************************
******************************************************************************/

/****** ahi.device/AHI_SampleFrameSize **************************************
*
*   NAME
*       AHI_SampleFrameSize -- get the size of a sample frame (V4)
*
*   SYNOPSIS
*       size = AHI_SampleFrameSize( sampletype );
*       D0                          D0
*
*       ULONG AHI_SampleFrameSize( ULONG );
*
*   FUNCTION
*       Returns the size in bytes of a sample frame for a given sample type.
*
*   INPUTS
*       sampletype - The sample type to examine. See <devices/ahi.h> for
*           possible types.
*
*   RESULT
*       The number of bytes, or 0 for invalid types.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       This function returned trash for invalid sample types
*       before V6.
*
*   SEE ALSO
*      <devices/ahi.h>
*
****************************************************************************
*
*/

static const UBYTE type2bytes[]=
{
  1,    // AHIST_M8S  (0)
  2,    // AHIST_M16S (1)
  2,    // AHIST_S8S  (2)
  4,    // AHIST_S16S (3)
  1,    // AHIST_M8U  (4)
  0,
  0,
  0,
  4,    // AHIST_M32S (8)
  0,
  8     // AHIST_S32S (10)
};

ULONG
_AHI_SampleFrameSize( ULONG           sampletype,
		      struct AHIBase* AHIBase )
{
  ULONG result = 0;
  
  if(sampletype <= AHIST_S32S )
  {
    result = type2bytes[sampletype];
  }
  else if(sampletype == AHIST_L7_1)
  {
    result = 32;
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_SampleFrameSize(sampletype);
    KPrintF("=>%ld\n",type2bytes[sampletype]);
  }

  return result;
}
