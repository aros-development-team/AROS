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
#include <exec/alerts.h>
#include <utility/utility.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>
#include <clib/alib_protos.h>

#include <string.h>

#include "ahi_def.h"
#include "audioctrl.h"
#include "mixer.h"
#include "database.h"
#include "debug.h"
#include "misc.h"
#include "header.h"
#include "gateway.h"


// Makes 'in' fit the given bounds.

#define inbounds(in,min,max) \
    ( (in > max) ? max : ( (in < min) ? min : in ) )


/******************************************************************************
** RecalcBuff *****************************************************************
******************************************************************************/

// Calculates how many samples to mix each mixer pass. The result it 
// both returned and stored in ahiac_BuffSamples.
// ahiac_BuffSizeNow will also be updated (For mixing routine)

static ULONG
RecalcBuff ( Fixed freq, struct AHIPrivAudioCtrl *audioctrl )
{
  int length;

  // If ULONG, convert to Fixed
//  if(freq < 65536) freq <<= 16;

  // Sanity check
  if(freq == 0) return 0;

  audioctrl->ac.ahiac_BuffSamples = (audioctrl->ac.ahiac_MixFreq << 8) /
                                    (freq >> 8);

  // ahiac_BuffSamples *must* fit a WORD according to the driver autodocs!
  if(audioctrl->ac.ahiac_BuffSamples > 65535)
  {
    audioctrl->ac.ahiac_BuffSamples = 65535;
  }

  // Now, calculate the required mixing buffer size.

  length = audioctrl->ac.ahiac_BuffSamples * 
           AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType);

  if(audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC)
  {
    length <<= 1;  // 2 buffers
  }

  // Pad to even 8 and add some more (because of an old Mungwall hit, but I
  // think that bug was fixed a long, long time ago..?)

  length = ((length + 7) & (~7) ) + 80;

  audioctrl->ahiac_BuffSizeNow = length;

  return audioctrl->ac.ahiac_BuffSamples;

}


/******************************************************************************
** CreateAudioCtrl & UpdateAudioCtrl ******************************************
******************************************************************************/

#define DEFPLAYERFREQ (50<<16)

static ULONG
DummyHook( void )
{
  return 0;
}


static const struct Hook DefPlayerHook =
{
  {0, 0},
  (HOOKFUNC) HookEntry,
  (HOOKFUNC) DummyHook,
  0
};


static const struct TagItem boolmap[] =
{
  { AHIDB_Volume,    AHIACF_VOL },
  { AHIDB_Panning,   AHIACF_PAN },
  { AHIDB_Stereo,    AHIACF_STEREO },
  { AHIDB_HiFi,      AHIACF_HIFI },
  { AHIDB_PingPong,  AHIACF_PINGPONG },
  { AHIDB_Record,    AHIACF_RECORD },
  { AHIDB_MultTable, AHIACF_MULTTAB },
  { AHIDB_MultiChannel, AHIACF_MULTICHANNEL },
  { TAG_DONE,        0 }
};


struct AHIPrivAudioCtrl *
CreateAudioCtrl(struct TagItem *tags)
{
  struct AHIPrivAudioCtrl *audioctrl;
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags;
  BOOL   error=TRUE;

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

  audioctrl = AHIAllocVec( sizeof( struct AHIPrivAudioCtrl ),
                           data_flags );

  if( audioctrl != NULL )
  {
    audioctrl->ac.ahiac_AudioCtrl.ahiac_UserData =
      (APTR)GetTagData(AHIA_UserData,0,tags);
    audioctrl->ahiac_AudioID =
      GetTagData(AHIA_AudioID,AHI_DEFAULT_ID,tags);
    audioctrl->ac.ahiac_MixFreq =
      GetTagData(AHIA_MixFreq,AHI_DEFAULT_FREQ,tags);
    audioctrl->ac.ahiac_Channels =
      GetTagData(AHIA_Channels,0,tags);
    audioctrl->ac.ahiac_Sounds =
      GetTagData(AHIA_Sounds,0,tags);
    audioctrl->ac.ahiac_SoundFunc =
      (struct Hook *)GetTagData(AHIA_SoundFunc,0,tags);
    audioctrl->ahiac_RecordFunc =
      (struct Hook *)GetTagData(AHIA_RecordFunc,0,tags);
    audioctrl->ac.ahiac_PlayerFunc =
      (struct Hook *)GetTagData(AHIA_PlayerFunc,0,tags);
    audioctrl->ac.ahiac_PlayerFreq =
      GetTagData(AHIA_PlayerFreq,0,tags);
    audioctrl->ac.ahiac_MinPlayerFreq =
      GetTagData(AHIA_MinPlayerFreq,0,tags);
    audioctrl->ac.ahiac_MaxPlayerFreq =
      GetTagData(AHIA_MaxPlayerFreq,0,tags);
    audioctrl->ac.ahiac_AntiClickSamples = 
      GetTagData(AHIA_AntiClickSamples,~0,tags);

    audioctrl->ahiac_MasterVolume=0x00010000;
    audioctrl->ahiac_SetMasterVolume=0x00010000;
    audioctrl->ahiac_EchoMasterVolume=0x00010000;

    if(audioctrl->ahiac_AudioID == AHI_DEFAULT_ID)
      audioctrl->ahiac_AudioID = AHIBase->ahib_AudioMode;

    if(audioctrl->ac.ahiac_MixFreq == AHI_DEFAULT_FREQ)
      audioctrl->ac.ahiac_MixFreq = AHIBase->ahib_Frequency;

    if(audioctrl->ac.ahiac_PlayerFunc == NULL)
      audioctrl->ac.ahiac_PlayerFunc=&DefPlayerHook;

    if(audioctrl->ac.ahiac_PlayerFreq == 0)
      audioctrl->ac.ahiac_PlayerFreq = DEFPLAYERFREQ;
    if(audioctrl->ac.ahiac_MinPlayerFreq == 0)
      audioctrl->ac.ahiac_MinPlayerFreq = DEFPLAYERFREQ;
    if(audioctrl->ac.ahiac_MaxPlayerFreq == 0)
      audioctrl->ac.ahiac_MaxPlayerFreq = DEFPLAYERFREQ;

    if(audioctrl->ac.ahiac_PlayerFreq < 65536)
      audioctrl->ac.ahiac_PlayerFreq <<= 16;
    if(audioctrl->ac.ahiac_MinPlayerFreq < 65536)
      audioctrl->ac.ahiac_MinPlayerFreq <<= 16;
    if(audioctrl->ac.ahiac_MaxPlayerFreq < 65536)
      audioctrl->ac.ahiac_MaxPlayerFreq <<= 16;

    if(audioctrl->ac.ahiac_AntiClickSamples == ~0)
      audioctrl->ac.ahiac_AntiClickSamples = 
          ( AHIBase->ahib_AntiClickTime * audioctrl->ac.ahiac_MixFreq ) >> 16;

    if((audiodb=LockDatabase()))
    {
      if((dbtags=GetDBTagList(audiodb,audioctrl->ahiac_AudioID)))
      {
        char driver_name[128];

        audioctrl->ac.ahiac_Flags=PackBoolTags(GetTagData(AHIDB_Flags,0,dbtags),dbtags,boolmap);

        if(AHIBase->ahib_Flags & AHIBF_CLIPPING)
        {
          audioctrl->ac.ahiac_Flags |= AHIACF_CLIPPING;
        }

        strcpy( audioctrl->ahiac_DriverName,
                (char *) GetTagData( AHIDB_DriverBaseName, (ULONG) "DEVS:AHI", dbtags) );

        strcpy( driver_name,
                (char *) GetTagData( AHIDB_Driver, (ULONG) "", dbtags ) );
        strcat( driver_name, ".audio" );

        AddPart(audioctrl->ahiac_DriverName, driver_name, sizeof(audioctrl->ahiac_DriverName));

        error=FALSE;
      }
      UnlockDatabase(audiodb);
    }
  }

  if(error)
  {
    AHIFreeVec(audioctrl);
    return NULL;
  }
  else
    return audioctrl;
}

static void
UpdateAudioCtrl(struct AHIPrivAudioCtrl *audioctrl)
{
  ULONG  temp;

  temp=audioctrl->ac.ahiac_MinPlayerFreq;
  if(temp>=65536)
    temp >>=16;
  if(temp)
    audioctrl->ac.ahiac_MaxBuffSamples=audioctrl->ac.ahiac_MixFreq/temp;
  else
    audioctrl->ac.ahiac_MaxBuffSamples=AHIBase->ahib_Frequency/audioctrl->ac.ahiac_PlayerFreq;

  temp=audioctrl->ac.ahiac_MaxPlayerFreq;
  if(temp>=65536)
    temp = (temp + 65535) >> 16;
  if(temp)
    audioctrl->ac.ahiac_MinBuffSamples=audioctrl->ac.ahiac_MixFreq/temp;
  else
    audioctrl->ac.ahiac_MinBuffSamples=AHIBase->ahib_Frequency/audioctrl->ac.ahiac_PlayerFreq;
}



/******************************************************************************
** SamplerFunc ****************************************************************
******************************************************************************/


static void
SamplerFunc( struct Hook*             hook,
             struct AHIPrivAudioCtrl* actrl,
             struct AHIRecordMessage* recmsg )
{
  if(actrl->ahiac_RecordFunc)
  {
    CallHookPkt(actrl->ahiac_RecordFunc, actrl, recmsg);
  }
}

/******************************************************************************
** AHI_AllocAudioA ************************************************************
******************************************************************************/

/****** ahi.device/AHI_AllocAudioA ******************************************
*
*   NAME
*       AHI_AllocAudioA -- allocates and initializes the audio hardware
*       AHI_AllocAudio -- varargs stub for AHI_AllocAudioA()
*
*   SYNOPSIS
*       audioctrl = AHI_AllocAudioA( tags );
*       D0                           A1
*
*       struct AHIAudioCtrl *AHI_AllocAudioA( struct TagItem * );
*
*       audioctrl = AHI_AllocAudio( tag1, ... );
*
*       struct AHIAudioCtrl *AHI_AllocAudio( Tag, ... );
*
*   FUNCTION
*       Allocates and initializes the audio hardware, selects the best
*       mixing routine (if necessary) according to the supplied tags.
*       To start playing you first need to call AHI_ControlAudioA().
*
*   INPUTS
*       tags - A pointer to a tag list.
*
*   TAGS
*
*       AHIA_AudioID (ULONG) - The audio mode to use. Default is
*           AHI_DEFAULT_ID. (AHI_DEFAULT_ID is the ID the user has selected
*           in the preferences program. It's a good value to use the first
*           time she starts your application.)
*
*       AHIA_MixFreq (ULONG) - Desired mixing frequency. The actual
*           mixing rate may or may not be exactly what you asked for.
*           Default is AHI_DEFAULT_FREQ. (AHI_DEFAULT_FREQ is the user's
*           prefered frequency.)
*
*       AHIA_Channels (UWORD) - Number of channel to use. The actual
*           number of channels used will be equal or grater than the
*           requested. If too many channels were requested, this function
*           will fail. This tag must be supplied.
*
*       AHIA_Sounds (UWORD) - Number of sounds to use. This tag must be
*           supplied.
*
*       AHIA_SoundFunc (struct Hook *) - A function to call each time
*           when a sound has been started. The function receives the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHISoundMessage *)
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*           Default is NULL.
*
*       AHIA_PlayerFunc (struct Hook *) - A function to be called at regular
*           intervals. By using this hook there is no need for music players
*           to use other timing, such as VBLANK or CIA timers. But the real
*           reason it's present is that it makes it possible to do non-
*           realtime mixing to disk.
*
*           Using this interrupt source is currently the only supported way
*           to ensure that no mixing occurs between calls to AHI_SetVol(),
*           AHI_SetFreq() or AHI_SetSound().
*
*           If the sound playback is done without mixing, 'realtime.library'
*           is used to provide timing. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - Undefined.
*           Do not assume A1 contains any particular value!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*           Default is NULL.
*
*       AHIA_PlayerFreq (Fixed) - If non-zero, enables timing and specifies
*           how many times per second PlayerFunc will be called. This must
*           be specified if AHIA_PlayerFunc is! Do not use any extreme
*           frequencies. The result of MixFreq/PlayerFreq must fit an UWORD,
*           ie it must be less or equal to 65535. It is also suggested that
*           you keep the result over 80. For normal use this should not be a
*           problem. Note that the data type is Fixed, not integer (see BUGS
*           below). 50 Hz is 50<<16, which is a reasonable lower limit.
*
*           Default is a reasonable value. Don't depend on it.
*
*       AHIA_MinPlayerFreq (Fixed) - The minimum frequency (AHIA_PlayerFreq)
*           you will use. You MUST supply this if you are using the device's
*           interrupt feature!
*
*       AHIA_MaxPlayerFreq (Fixed) - The maximum frequency (AHIA_PlayerFreq)
*           you will use. You MUST supply this if you are using the device's
*           interrupt feature!
*
*       AHIA_RecordFunc (struct Hook *) - This function will be called
*           regularly when sampling is turned on (see AHI_ControlAudioA())
*           with the following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIRecordMessage *)
*           The message (AHIRecordMessage) is filled as follows:
*               ahirm_Buffer - Pointer to the samples. The buffer is valid
*                   until next time the Hook is called.
*               ahirm_Length - Number of sample FRAMES in buffer.
*                   To get the size in bytes, multiply by 4 if ahiim_Type is
*                   AHIST_S16S.
*               ahirm_Type - Always AHIST_S16S at the moment, but you *must*
*                   check this, since it may change in the future!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply. Signal a process if you wish to save the
*           buffer to disk. The called function should follow normal register
*           conventions, which means that d2-d7 and a2-a6 must be preserved.
*
*           NOTE: The function MUST return NULL (in d0). This was previously
*           not documented. Now you know.
*
*           Default is NULL.
*
*       AHIA_UserData (APTR) - Can be used to initialize the ahiac_UserData
*           field. Default is 0.
*
*   RESULT
*       A pointer to an AHIAudioCtrl structure or NULL if an error occured.
*
*   EXAMPLE
*
*   NOTES
*       SoundFunc will be called in the same manner as Paula interrupts
*       occur; when the device has updated its internal variables and can
*       accept new commands.
*
*   BUGS
*       For compability reasons with some really old applications,
*       AHIA_PlayerFreq, AHIA_MinPlayerFreq and AHIA_MaxPlayerFreq 
*       interpret values lower than 0x10000 as integers, not Fixed.
*       This means that the lowest frequency possible is 1 Hz. However,
*       you should *never* use a value than say 10-20 Hz anyway, because
*       of the high latency and the impact on multitasking. 
*
*       This kludge will be removed some day. Always use Fixed!
*
*
*   SEE ALSO
*       AHI_FreeAudio(), AHI_ControlAudioA()
*
****************************************************************************
*
*/

struct AHIAudioCtrl*
_AHI_AllocAudioA( struct TagItem* tags,
		  struct AHIBase* AHIBase )
{
  struct AHIPrivAudioCtrl* audioctrl;
  struct Library *AHIsubBase;
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags;
#ifdef __AMIGAOS4__
  struct AHIsubIFace* IAHIsub;
#endif

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_AllocAudioA(tags);
  }

  audioctrl = CreateAudioCtrl( tags );
  if(!audioctrl)
    goto error;

  AHIBase->ahib_AudioCtrl=audioctrl;                      // Save latest (for KillAudio)

  if(!audioctrl->ac.ahiac_Channels || !audioctrl->ac.ahiac_Sounds)
    goto error;

  audioctrl->ahiac_SubAllocRC = AHISF_ERROR;
  audioctrl->ahiac_SubLib=
  AHIsubBase = OpenLibrary(audioctrl->ahiac_DriverName,DriverVersion);
//KPrintF("Opened AHIsubBase()\n");

#ifdef __AMIGAOS4__
  audioctrl->ahiac_IAHIsub = NULL;
#endif

  if(!AHIsubBase)
    goto error;

#ifdef __AMIGAOS4__
  if ((audioctrl->ahiac_IAHIsub = (struct AHIsubIFace *) GetInterface((struct Library *) AHIsubBase, "main", 1, NULL)) == NULL)
  {    
       goto error;
  }
  IAHIsub = audioctrl->ahiac_IAHIsub;
#endif

  // Never allow drivers that are newer than ahi.device.
  if(AHIsubBase->lib_Version > AHIBase->ahib_Library.lib_Version)
    goto error;

  audiodb=LockDatabase();
  if(!audiodb)
    goto error;

  dbtags=GetDBTagList(audiodb,audioctrl->ahiac_AudioID);
  if(dbtags)
    audioctrl->ahiac_SubAllocRC=AHIsub_AllocAudio(dbtags,(struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_AllocAudio()\n");
  UnlockDatabase(audiodb);

  if(!dbtags)
    goto error;

  UpdateAudioCtrl(audioctrl);

  if(audioctrl->ahiac_SubAllocRC & AHISF_ERROR)
    goto error;

// Mixing
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_MIXING))
    audioctrl->ac.ahiac_Flags |= AHIACF_NOMIXING;

// Timing    
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_TIMING))
    audioctrl->ac.ahiac_Flags |= AHIACF_NOTIMING;

// Stereo
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_KNOWSTEREO))
    audioctrl->ac.ahiac_Flags &= ~AHIACF_STEREO;

// Multichannel 7.1
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_KNOWMULTICHANNEL))
    audioctrl->ac.ahiac_Flags &= ~AHIACF_MULTICHANNEL;
  
// HiFi

  if(!(audioctrl->ahiac_SubAllocRC & AHISF_KNOWHIFI))
    audioctrl->ac.ahiac_Flags &= ~AHIACF_HIFI;

// Post-processing
  if(audioctrl->ahiac_SubAllocRC & AHISF_CANPOSTPROCESS)
    audioctrl->ac.ahiac_Flags |= AHIACF_POSTPROC;

  if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING))
  {
    switch(audioctrl->ac.ahiac_Flags & (AHIACF_STEREO | AHIACF_HIFI | AHIACF_MULTICHANNEL))
    {
      case 0:
        audioctrl->ac.ahiac_BuffType=AHIST_M16S;
        break;
      case AHIACF_STEREO:
        audioctrl->ac.ahiac_BuffType=AHIST_S16S;
        break;
      case AHIACF_HIFI:
        audioctrl->ac.ahiac_Flags |= AHIACF_CLIPPING;
        audioctrl->ac.ahiac_BuffType=AHIST_M32S;
        break;
      case (AHIACF_STEREO | AHIACF_HIFI):
        audioctrl->ac.ahiac_Flags |= AHIACF_CLIPPING;
        audioctrl->ac.ahiac_BuffType=AHIST_S32S;
        break;
      case (AHIACF_STEREO | AHIACF_HIFI | AHIACF_MULTICHANNEL):
        audioctrl->ac.ahiac_Flags |= AHIACF_CLIPPING;
        audioctrl->ac.ahiac_BuffType=AHIST_L7_1;
        break;
      default:
        Alert(AT_Recovery|AG_BadParm);
        goto error;
    }

/* Max channels/2 channels per hardware channel if stereo w/o pan */
    if((audioctrl->ac.ahiac_Flags & (AHIACF_STEREO | AHIACF_PAN)) == AHIACF_STEREO)
      audioctrl->ahiac_Channels2=(audioctrl->ac.ahiac_Channels+1)/2;
    else
      audioctrl->ahiac_Channels2=audioctrl->ac.ahiac_Channels;

    if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOTIMING))
    {
      RecalcBuff(audioctrl->ac.ahiac_MinPlayerFreq,audioctrl);
      audioctrl->ac.ahiac_BuffSize=audioctrl->ahiac_BuffSizeNow;
      RecalcBuff(audioctrl->ac.ahiac_PlayerFreq,audioctrl);
    }
    else // No timing
    {
      ULONG size;

      size=audioctrl->ac.ahiac_BuffSamples*\
           AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType)*\
           (audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC ? 2 : 1);

      size +=7;
      size &= ~7;  // byte align

      audioctrl->ahiac_BuffSizeNow=size;
    }

    audioctrl->ac.ahiac_MixerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC|MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_MixerFunc)
      goto error;


    audioctrl->ac.ahiac_MixerFunc->h_Entry    = (HOOKFUNC) HookEntryPreserveAllRegs;
    audioctrl->ac.ahiac_MixerFunc->h_SubEntry = (HOOKFUNC) MixerFunc;

    if((AHIBase->ahib_MaxCPU >= 0x10000) || (AHIBase->ahib_MaxCPU <= 0x0))
    {
      audioctrl->ahiac_MaxCPU = 0x100;
    }
    else
    {
      audioctrl->ahiac_MaxCPU = AHIBase->ahib_MaxCPU >> 8;
    }

    audioctrl->ac.ahiac_PreTimer  = (BOOL (*)(void)) PreTimerPreserveAllRegs;
    audioctrl->ac.ahiac_PostTimer = (void (*)(void)) PostTimerPreserveAllRegs;

    audioctrl->ac.ahiac_PreTimerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC|MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_PreTimerFunc)
      goto error;

    audioctrl->ac.ahiac_PostTimerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC|MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_PostTimerFunc)
      goto error;

    audioctrl->ac.ahiac_PreTimerFunc->h_Entry    = (HOOKFUNC) HookEntry;
    audioctrl->ac.ahiac_PreTimerFunc->h_SubEntry = (HOOKFUNC) PreTimerFunc;
    
    audioctrl->ac.ahiac_PostTimerFunc->h_Entry    = (HOOKFUNC) HookEntry;
    audioctrl->ac.ahiac_PostTimerFunc->h_SubEntry = (HOOKFUNC) PostTimerFunc;
    
    if( !InitMixroutine( audioctrl ) ) goto error;
  }

  audioctrl->ac.ahiac_SamplerFunc = AllocVec(sizeof(struct Hook),
      MEMF_PUBLIC|MEMF_CLEAR);
  if(!audioctrl->ac.ahiac_SamplerFunc)
    goto error;

  audioctrl->ac.ahiac_SamplerFunc->h_Entry    = (HOOKFUNC) HookEntry;
  audioctrl->ac.ahiac_SamplerFunc->h_SubEntry = (HOOKFUNC) SamplerFunc;

  /* Set default hardware properties, only if AHI_DEFAULT_ID was used!*/
  if(GetTagData(AHIA_AudioID, AHI_DEFAULT_ID, tags) == AHI_DEFAULT_ID)
  {
    AHI_ControlAudio((struct AHIAudioCtrl *)audioctrl,
        AHIC_MonitorVolume,   AHIBase->ahib_MonitorVolume,
        AHIC_InputGain,       AHIBase->ahib_InputGain,
        AHIC_OutputVolume,    AHIBase->ahib_OutputVolume,
        AHIC_Input,           AHIBase->ahib_Input,
        AHIC_Output,          AHIBase->ahib_Output,
        TAG_DONE);
  }

exit:
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%08lx\n", (ULONG) audioctrl);
  }
  return (struct AHIAudioCtrl *) audioctrl;


error:
  AHI_FreeAudio((struct AHIAudioCtrl *)audioctrl);
  audioctrl=NULL;
  goto exit;
}


/******************************************************************************
** AHI_FreeAudio **************************************************************
******************************************************************************/

/****** ahi.device/AHI_FreeAudio *******************************************
*
*   NAME
*       AHI_FreeAudio -- deallocates the audio hardware
*
*   SYNOPSIS
*       AHI_FreeAudio( audioctrl );
*                      A2
*
*       void AHI_FreeAudio( struct AHIAudioCtrl * );
*
*   FUNCTION
*       Deallocates the AHIAudioCtrl structure and any other resources
*       allocated by AHI_AllocAudioA(). After this call it must not be used
*       by any other functions anymore. AHI_UnloadSound() is automatically
*       called for every sound.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure obtained from
*           AHI_AllocAudioA(). If NULL, this function does nothing.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_UnloadSound()
*
****************************************************************************
*
*/

ULONG
_AHI_FreeAudio( struct AHIPrivAudioCtrl* audioctrl,
		struct AHIBase*          AHIBase )
{
  struct Library *AHIsubBase;
  int i;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_FreeAudio(audioctrl);
  }

  if(audioctrl)
  {
    if((AHIsubBase=audioctrl->ahiac_SubLib))
    {
#ifdef __AMIGAOS4__
      struct AHIsubIFace* IAHIsub = audioctrl->ahiac_IAHIsub;
#endif

      if(!(audioctrl->ahiac_SubAllocRC & AHISF_ERROR))
      {
//KPrintF("Called AHIsub_Stop(play|record)\n");
        AHIsub_Stop(AHISF_PLAY|AHISF_RECORD,(struct AHIAudioCtrlDrv *)audioctrl);

        for(i=audioctrl->ac.ahiac_Sounds-1;i>=0;i--)
        {
          AHI_UnloadSound(i,(struct AHIAudioCtrl *)audioctrl);
        }
      }
//KPrintF("Called AHIsub_FreeAudio()\n");
      AHIsub_FreeAudio((struct AHIAudioCtrlDrv *) audioctrl);
//KPrintF("Closed AHIsubbase\n");
#ifdef __AMIGAOS4__
      DropInterface((struct Interface *) IAHIsub);
      IAHIsub = NULL;
#endif
      
      CloseLibrary(AHIsubBase);
    }

    if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING))
    {
      CleanUpMixroutine( audioctrl );
    }

    FreeVec( audioctrl->ac.ahiac_SamplerFunc );
    FreeVec( audioctrl->ac.ahiac_MixerFunc );
    FreeVec( audioctrl->ac.ahiac_PreTimerFunc );
    FreeVec( audioctrl->ac.ahiac_PostTimerFunc );

    AHIFreeVec( audioctrl );
  }
  return 0;
}


/******************************************************************************
** AHI_KillAudio **************************************************************
******************************************************************************/

/****i* ahi.device/AHI_KillAudio *******************************************
*
*   NAME
*      AHI_KillAudio -- clean up
*
*   SYNOPSIS
*      AHI_KillAudio();
*
*      void AHI_KillAudio( void );
*
*   FUNCTION
*      'ahi.device' keeps track of most of what the user does. This call is
*      used to clean up as much as possible. It must never, ever, be used
*      in an application. It is included for development use only, and can
*      be used to avoid rebooting the computer if your program has allocated
*      the audio hardware and crashed. This call can lead to a system crash,
*      so don't use it if you don't have to.
*
*   INPUTS
*
*   RESULT
*      This function returns nothing. In fact, it may never return.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_FreeAudio()
*
****************************************************************************
*
*/

ULONG
_AHI_KillAudio( struct AHIBase* AHIBase )
{
  UWORD i;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_KillAudio();
  }

  for(i=0xffff;i != 0; i--)
  {
    *((UWORD *) 0xdff102)=i;
  }

  AHI_FreeAudio(AHIBase->ahib_AudioCtrl);
  AHIBase->ahib_AudioCtrl=NULL;
  return 0;
}


/******************************************************************************
** AHI_ControlAudioA **********************************************************
******************************************************************************/

/****** ahi.device/AHI_ControlAudioA ***************************************
*
*   NAME
*       AHI_ControlAudioA -- change audio attributes
*       AHI_ControlAudio -- varargs stub for AHI_ControlAudioA()
*
*   SYNOPSIS
*       error = AHI_ControlAudioA( audioctrl, tags );
*       D0                         A2         A1
*
*       ULONG AHI_ControlAudioA( struct AHIAudioCtrl *, struct TagItem * );
*
*       error = AHI_ControlAudio( AudioCtrl, tag1, ...);
*
*       ULONG AHI_ControlAudio( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function should be used to change attributes for a given
*       AHIAudioCtrl structure. It is also used to start and stop playback,
*       and to control special hardware found on some sound cards.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIC_Play (BOOL) - Starts (TRUE) and stops (FALSE) playback and
*           PlayerFunc. NOTE: If the audio hardware cannot play at the same
*           time as recording samples, the recording will be stopped.
*
*       AHIC_Record (BOOL) - Starts (TRUE) and stops (FALSE) sampling and
*           RecordFunc. NOTE: If the audio hardware cannot record at the same
*           time as playing samples, the playback will be stopped.
*
*       AHIC_MonitorVolume (Fixed) - Sets the input monitor volume, i.e. how
*           much of the input signal is mixed with the output signal while
*           recording. Use AHI_GetAudioAttrsA() to find the available range.
*
*       AHIC_MonitorVolume_Query (Fixed *) - Get the current input monitor
*           volume. ti_Data is a pointer to a Fixed variable, where the result
*           will be stored.
*
*       AHIC_MixFreq_Query (ULONG *) - Get the current mixing frequency.
*           ti_Data is a pointer to an ULONG variable, where the result will
*           be stored.
*
*       AHIC_InputGain (Fixed) - Set the input gain. Use AHI_GetAudioAttrsA()
*           to find the available range. (V2)
*
*       AHIC_InputGain_Query (Fixed *) - Get current input gain. (V2)
*
*       AHIC_OutputVolume (Fixed) - Set the output volume. Use
*           AHI_GetAudioAttrsA() to find the available range. (V2)
*
*       AHIC_OutputVolume_Query (Fixed *) - Get current output volume. (V2)
*
*       AHIC_Input (ULONG) - Select input source. See AHI_GetAudioAttrsA().
*           (V2)
*
*       AHIC_Input_Query (ULONG *) - Get current input source. (V2)
*
*       AHIC_Output (ULONG) - Select destination for output. See
*           AHI_GetAudioAttrsA(). (V2)
*
*       AHIC_Output_Query (ULONG *) - Get destination for output. (V2)
*
*       The following tags are also recognized by AHI_ControlAudioA(). See
*       AHI_AllocAudioA() for what they do. They may be used from interrupts.
*
*       AHIA_SoundFunc (struct Hook *)
*       AHIA_PlayerFunc (struct Hook *)
*       AHIA_PlayerFreq (Fixed)
*       AHIA_RecordFunc (struct Hook *)
*       AHIA_UserData (APTR)
*
*       Note that AHIA_PlayerFreq must never be outside the limits specified
*       with AHIA_MinPlayerFreq and AHIA_MaxPlayerFreq! Also note that the
*       timing feature is designed to be used for music. When you change the
*       frequency, be reasonable. Using 50 Hz one moment and 5 the other is
*       to ask for trouble.
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       The AHIC_Play and AHIC_Record tags *must not* be used from
*       interrupts.
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_GetAudioAttrsA(), <devices/ahi.h>
*
****************************************************************************
*
*/

ULONG
_AHI_ControlAudioA( struct AHIPrivAudioCtrl* audioctrl,
		    struct TagItem*          tags,
		    struct AHIBase*          AHIBase )
{
  ULONG *ptr, playflags=0, stopflags=0, rc=AHIE_OK;
  UBYTE update=FALSE;
  struct TagItem *tag,*tstate=tags;
  struct Library *AHIsubBase=audioctrl->ahiac_SubLib;
#ifdef __AMIGAOS4__
  struct AHIsubIFace* IAHIsub = audioctrl->ahiac_IAHIsub;
#endif

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_ControlAudioA(audioctrl,tags);
  }

  while((tag=NextTagItem(&tstate)))
  {
    ptr=(ULONG *)tag->ti_Data;  // For ..._Query tags
    switch(tag->ti_Tag)
    {
    case AHIA_SoundFunc:
      audioctrl->ac.ahiac_SoundFunc=(struct Hook *) tag->ti_Data;
      update=TRUE;
      break;
    case AHIA_RecordFunc:
      audioctrl->ahiac_RecordFunc=(struct Hook *) tag->ti_Data;
      update=TRUE;
      break;
    case AHIA_PlayerFunc:
      audioctrl->ac.ahiac_PlayerFunc=(struct Hook *) tag->ti_Data;
      update=TRUE;
      break;
    case AHIA_PlayerFreq:
      audioctrl->ac.ahiac_PlayerFreq=tag->ti_Data;

      if(audioctrl->ac.ahiac_PlayerFreq < 65536)
        audioctrl->ac.ahiac_PlayerFreq <<= 16;

      if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOTIMING)) // Dont call unless timing is used.
        RecalcBuff(audioctrl->ac.ahiac_PlayerFreq,audioctrl);
      update=TRUE;
      break;
    case AHIA_UserData:
      audioctrl->ac.ahiac_AudioCtrl.ahiac_UserData=(void *)tag->ti_Data;
      break;
    case AHIC_Play:
      if(tag->ti_Data)
      {
        playflags |= AHISF_PLAY;
        stopflags &= ~AHISF_PLAY;
      }
      else
      {
        playflags &= ~AHISF_PLAY;
        stopflags |= AHISF_PLAY;
      }
      update=FALSE;
      break;
    case AHIC_Record:
      if(tag->ti_Data)
      {
        playflags |= AHISF_RECORD;
        stopflags &= ~AHISF_RECORD;
      }
      else
      {
        playflags &= ~AHISF_RECORD;
        stopflags |= AHISF_RECORD;
      }
      update=FALSE;
      break;
    case AHIC_MixFreq_Query:
      *ptr=audioctrl->ac.ahiac_MixFreq;
      break;
    case AHIC_MonitorVolume:
    case AHIC_InputGain:
    case AHIC_OutputVolume:
    case AHIC_Input:
    case AHIC_Output:
      AHIsub_HardwareControl(tag->ti_Tag, tag->ti_Data, (struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_HardwareControl(%08lx:%08lx)\n",tag->ti_Tag, tag->ti_Data);
      break;
    case AHIC_MonitorVolume_Query:
    case AHIC_InputGain_Query:
    case AHIC_OutputVolume_Query:
    case AHIC_Input_Query:
    case AHIC_Output_Query:
      *ptr=AHIsub_HardwareControl(tag->ti_Tag, 0, (struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_HardwareControl(%08lx:NULL)\n",tag->ti_Tag);
      break;
    }
  }

// Let's act!
  if(update)
  {
    AHIsub_Update(0,(struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_Update()\n");
  }
  
  if(playflags)
  {
    rc=AHIsub_Start(playflags,(struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_Start(%08lx)=>%ld\n",playflags,rc);
  }
  
  if(stopflags)
  {
    AHIsub_Stop(stopflags,(struct AHIAudioCtrlDrv *)audioctrl);
//KPrintF("Called AHIsub_Stop(%08lx)\n",stopflags);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}
