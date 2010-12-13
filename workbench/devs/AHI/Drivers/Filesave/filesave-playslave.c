
#include <config.h>

#include <exec/execbase.h>
#include <libraries/ahi_sub.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <string.h>

#include "DriverData.h"
#include "FileFormats.h"
#include "library.h"

#define dd ((struct FilesaveData*) AudioCtrl->ahiac_DriverData)

#define abs(x) ((x)<0?(-x):(x))

void ulong2extended (ULONG in, extended *ex);

/******************************************************************************
** Endian conversion **********************************************************
******************************************************************************/

#ifdef WORDS_BIGENDIAN

#define __htole_short(x) \
            ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))

#define __htole_long(x) \
            ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned short
htole_short( unsigned short x )
{
  return (unsigned short) __htole_short( x );
}

unsigned long
htole_long( unsigned long x )
{
  return __htole_long( x );
}

#define __htobe_short(x) (x)
#define __htobe_long(x)  (x)
#define htobe_short(x)   (x)
#define htobe_long(x)    (x)

#else

#define __htole_short(x) (x)
#define __htole_long(x)  (x)
#define htole_short(x)   (x)
#define htole_long(x)    (x)

#define __htobe_short(x) \
            ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))

#define __htobe_long(x) \
            ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned short
htobe_short( unsigned short x )
{
  return (unsigned short) __htobe_short( x );
}

unsigned long
htobe_long( unsigned long x )
{
  return __htobe_long( x );
}

#endif


/******************************************************************************
** The slave process **********************************************************
******************************************************************************/


#if defined( __MORPHOS__ )

/* For f*cks sake!!! Don't use SysBase in memcpy()! */

void *
memcpy(void *dst, const void *src, size_t len)
{
  bcopy(src, dst, len);
  return dst;
}

#endif


#undef SysBase

static void PlaySlave( struct ExecBase* SysBase );

#if defined( __AROS__ )

#include <aros/asmcall.h>

AROS_UFH3(void, PlaySlaveEntry,
	  AROS_UFHA(STRPTR, argPtr, A0),
	  AROS_UFHA(ULONG, argSize, D0),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
   AROS_USERFUNC_INIT
   PlaySlave( SysBase );
   AROS_USERFUNC_EXIT
}

#else

void PlaySlaveEntry(void)
{
  struct ExecBase* SysBase = *((struct ExecBase**) 4);

  PlaySlave( SysBase );
}

#endif

static void PlaySlave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct FilesaveBase*    FilesaveBase;

  struct EIGHTSVXheader EIGHTSVXheader = // All 0s will be filled later.
  { 
    __htobe_long(ID_FORM), 0, __htobe_long(ID_8SVX),
    __htobe_long(ID_VHDR), __htobe_long(sizeof(Voice8Header)),
    {
      0,
      0,
      0,
      0,
      1,
      sCmpNone,
      __htobe_long(0x10000)
    },
    __htobe_long(ID_BODY), 0
  };

  struct AIFFheader AIFFheader = // All 0s will be filled later.
  { 
    __htobe_long(ID_FORM), 0, __htobe_long(ID_AIFF),
    __htobe_long(ID_COMM), __htobe_long(sizeof(CommonChunk)),
    {
      0,
      0,
      __htobe_short(16),
      {
        0, { 0, 0 }
      }
    },
    __htobe_long(ID_SSND), 0,
    {
      0,
      0
    }
  };

  struct AIFCheader AIFCheader = // All 0s will be filled later.
  { 
    __htobe_long(ID_FORM), 0, __htobe_long(ID_AIFC),
    __htobe_long(ID_FVER), __htobe_long(sizeof(FormatVersionHeader)),
    {
      __htobe_long(AIFCVersion1)
    },
    __htobe_long(ID_COMM), __htobe_long(sizeof(ExtCommonChunk)),
    {
      0,
      0,
      __htobe_short(16),
      {
        0, { 0, 0 }
      },
      __htobe_long(NO_COMPRESSION),
      { sizeof("not compressed") - 1,
	'n','o','t',' ','c','o','m','p','r','e','s','s','e','d' }
    },
    __htobe_long(ID_SSND), 0,
    {
      0,
      0
    }
  };

  struct STUDIO16FILE S16header = // All 0s will be filled later.
  {
    __htobe_long(S16FID),
    0,
    __htobe_long(S16FINIT),
    __htobe_short(S16_VOL_0),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    {
      0
    }
  };

  struct WAVEheader WAVEheader = // All 0s will be filled later.
  {
    __htobe_long(ID_RIFF), 0, __htobe_long(ID_WAVE),
    __htobe_long(ID_fmt), __htole_long( sizeof(FormatChunk) ),
    {
      __htole_short( WAVE_PCM ),
      0,
      0,
      0,
      0,
      __htole_short( 16 )
    },
    __htobe_long(ID_data), 0
  };

  BPTR lock = 0,cd = 0,file = 0, file2 = 0;
  LONG maxVolume = 0;
  ULONG signals, i, samplesAdd =0, samples = 0, length = 0;
  ULONG offset = 0, bytesInBuffer = 0, samplesWritten = 0, bytesWritten = 0;

  /* Note that in OS4, we cannot call FindTask(NULL) here, since IExec
   * is inside AHIsubBase! */
  AudioCtrl    = (struct AHIAudioCtrlDrv*) SysBase->ThisTask->tc_UserData;
  AHIsubBase   = (struct DriverBase*) dd->fs_AHIsubBase;
  FilesaveBase = (struct FilesaveBase*) AHIsubBase;

// We cannot handle stereo 8SVXs!
  if( (dd->fs_Format == FORMAT_8SVX) &&
      (AudioCtrl->ahiac_Flags & AHIACF_STEREO) )
  {
    goto quit;
  }

  if((dd->fs_SlaveSignal = AllocSignal(-1)) == -1)
  {
    goto quit;
  }

  if(!(lock = Lock(dd->fs_FileReq->fr_Drawer, ACCESS_READ)))
  {
    goto quit;
  }

  cd = CurrentDir(lock);

  switch(dd->fs_Format)
  {
    case FORMAT_8SVX:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &EIGHTSVXheader, sizeof EIGHTSVXheader);
      break;

    case FORMAT_AIFF:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &AIFFheader, sizeof AIFFheader);
      break;

    case FORMAT_AIFC:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &AIFCheader, sizeof AIFCheader);
      break;

    case FORMAT_S16:
      if (AudioCtrl->ahiac_Flags & AHIACF_STEREO)
      {
        char filename[256];
        int len;

        strncpy (filename, dd->fs_FileReq->fr_File, sizeof(filename) - 3);
        len = strlen(filename);

        if(len >= 2 && filename[len - 2] == '_'
           && (filename[len - 1] == 'L' || filename[len - 1] == 'R'))
        {
          filename[len - 1] = 'L';
        }
        else
        {
          strcat (filename, "_L");
        }

        if(!(file = Open(filename, MODE_NEWFILE))) goto quit;

        filename[strlen(filename) - 1] = 'R';
        if(!(file2 = Open(filename, MODE_NEWFILE))) goto quit;

        Write(file, &S16header, sizeof S16header);
        Write(file2, &S16header, sizeof S16header);
      }
      else
      {
        if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
        Write(file, &S16header, sizeof S16header);
      }
      break;

    case FORMAT_WAVE:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &WAVEheader, sizeof WAVEheader);
      break;
  }

  // Everything set up. Tell Master we're alive and healthy.
  Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_MasterSignal);

  for(;;)
  {
    signals = SetSignal(0L,0L);
    if(signals & (SIGBREAKF_CTRL_C | 1L<<dd->fs_SlaveSignal))
    {
      break;
    }

    CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
    CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->fs_MixBuffer);

    samplesAdd = AudioCtrl->ahiac_BuffSamples;
    samples    = samplesAdd;

    if(AudioCtrl->ahiac_Flags & AHIACF_STEREO)
    {
      samples <<= 1;
    }

// Search for loudest part in sample
    if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    {
      for(i = 0; i < samples; i++)
        if(abs(((LONG *)dd->fs_MixBuffer)[i]) > maxVolume)
          maxVolume = abs(((LONG *)dd->fs_MixBuffer)[i]);
    }
    else
    {
      for(i = 0; i< samples; i++)
        if(abs(((WORD *)dd->fs_MixBuffer)[i]) > maxVolume)
          maxVolume = abs(((WORD *)dd->fs_MixBuffer)[i]);
    }

    if((AudioCtrl->ahiac_Flags & AHIACF_STEREO) && dd->fs_Format == FORMAT_S16)
    {
      samples >>= 1;  // Two buffers instead
    }

    if(offset+samples >= dd->fs_SaveBufferSize)
    {
      if((ULONG) Write(file, dd->fs_SaveBuffer, bytesInBuffer) != bytesInBuffer)
      {
        break;
      }
      if(file2 != 0) {
        if((ULONG) Write(file2, dd->fs_SaveBuffer2, bytesInBuffer) != bytesInBuffer)
        {
          break;
        }
      }
      offset = 0;
      bytesInBuffer = 0;
    }

    switch(dd->fs_Format)
    {
      case FORMAT_8SVX:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          BYTE *dest = &((BYTE *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
            *dest++ = *source++ >> 24;
        }
        else
        {
          BYTE *dest = &((BYTE *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
            *dest++ = *source++ >> 8;
        }
        length = samples;
        break;

      case FORMAT_AIFF:
      case FORMAT_AIFC:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htobe_short( *source++ >> 16 );
          }
        }
        else
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htobe_short( *source++ );
          }
        }
        length = samples*2;
        break;

      case FORMAT_S16:
        switch(AudioCtrl->ahiac_Flags & (AHIACF_HIFI | AHIACF_STEREO))
        {
          case 0:
          {
            WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest++ = htobe_short( *source++ );
            }

            break;
          }

          case AHIACF_STEREO:
          {
            WORD *dest1 = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *dest2 = &((WORD *) dd->fs_SaveBuffer2)[offset];
            WORD *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest1++ = htobe_short( *source++ );
              *dest2++ = htobe_short( *source++ );
            }

            break;
          }

          case AHIACF_HIFI:
          {
            WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
            LONG *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest++ = htobe_short( *source++ >> 16 );
            }

            break;
          }

          case (AHIACF_HIFI | AHIACF_STEREO):
          {
            WORD *dest1 = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *dest2 = &((WORD *) dd->fs_SaveBuffer2)[offset];
            LONG *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest1++ = htobe_short( *source++ >> 16 );
              *dest2++ = htobe_short( *source++ >> 16 );
            }

            break;
          }
        }

        length = samples*2;
        break;

      case FORMAT_WAVE:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htole_short( *source++ >> 16 );
          }
        }
        else
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htole_short( *source++ );
          }
        }
        length = samples*2;
        break;
    }

    offset          += samples;
    samplesWritten  += samplesAdd;
    bytesWritten    += length;
    bytesInBuffer   += length;
  }

  Write(file, dd->fs_SaveBuffer, bytesInBuffer);
  if(file2 != 0)
  {
    Write(file2, dd->fs_SaveBuffer2, bytesInBuffer);
  }

  switch(dd->fs_Format)
  {
    case FORMAT_8SVX:
      EIGHTSVXheader.FORMsize = htobe_long(sizeof(EIGHTSVXheader)-8+bytesWritten);
      EIGHTSVXheader.VHDRchunk.oneShotHiSamples = htobe_long(samplesWritten);
      EIGHTSVXheader.VHDRchunk.samplesPerSec = htobe_short(AudioCtrl->ahiac_MixFreq);
      EIGHTSVXheader.BODYsize = htobe_long(bytesWritten);
      if(bytesWritten & 1)
        FPutC(file,'\0');   // Pad to even
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&EIGHTSVXheader,sizeof EIGHTSVXheader);
      break;

    case FORMAT_AIFF:
      AIFFheader.FORMsize = htobe_long(sizeof(AIFFheader)-8+bytesWritten);
      AIFFheader.COMMchunk.numChannels = htobe_short((AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1));
      AIFFheader.COMMchunk.numSampleFrames = htobe_long(samplesWritten);
      ulong2extended(AudioCtrl->ahiac_MixFreq,&AIFFheader.COMMchunk.sampleRate);
      AIFFheader.SSNDsize = htobe_long(sizeof(SampledSoundHeader)+bytesWritten);
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&AIFFheader,sizeof AIFFheader);
      break;

    case FORMAT_AIFC:
      AIFCheader.FORMsize = htobe_long(sizeof(AIFCheader)-8+bytesWritten);
      AIFCheader.COMMchunk.numChannels = htobe_short((AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1));
      AIFCheader.COMMchunk.numSampleFrames = htobe_long(samplesWritten);
      ulong2extended(AudioCtrl->ahiac_MixFreq,&AIFCheader.COMMchunk.sampleRate);
      AIFCheader.SSNDsize = htobe_long(sizeof(SampledSoundHeader)+bytesWritten);
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&AIFCheader,sizeof AIFCheader);
      break;

    case FORMAT_S16:
      S16header.S16F_RATE = htobe_long(AudioCtrl->ahiac_MixFreq);
      S16header.S16F_SAMPLES0 =
      S16header.S16F_SAMPLES1 = htobe_long(samplesWritten);
      S16header.S16F_SAMPLES2 = htobe_long(samplesWritten - 1);
      if (file2 == 0)
      {
        S16header.S16F_PAN = htobe_long(S16_PAN_MID);
      }
      else
      {
        S16header.S16F_PAN = htobe_long(S16_PAN_LEFT);
      }

      Seek(file, 0, OFFSET_BEGINNING);
      Write(file, &S16header, sizeof S16header);
      if(file2 != 0)
      {
        S16header.S16F_PAN = htobe_long(S16_PAN_RIGHT);
        Seek(file2,0,OFFSET_BEGINNING);
        Write(file2, &S16header, sizeof S16header);
      }
      break;   

    case FORMAT_WAVE:
    {
      short num_channels;
      short block_align;
      
      num_channels = AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1;
      block_align  = num_channels * 16 / 8;
      
      WAVEheader.FORMsize                   = htole_long( sizeof(WAVEheader)-8+bytesWritten );
      WAVEheader.FORMATchunk.numChannels    = htole_short( num_channels );
      WAVEheader.FORMATchunk.samplesPerSec  = htole_long( AudioCtrl->ahiac_MixFreq );
      WAVEheader.FORMATchunk.avgBytesPerSec = htole_long( AudioCtrl->ahiac_MixFreq * block_align );
      WAVEheader.FORMATchunk.blockAlign     = htole_short( block_align );
      WAVEheader.DATAsize                   = htole_long( bytesWritten );
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&WAVEheader,sizeof WAVEheader);
      break;
    }
  }

  if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    maxVolume >>=16;

  if(maxVolume != 0)
  {
    Req("Rendering finished.\nTo futher improve the quality of the sample,\n"
	"you can raise the volume to %ld%% and render again.",
	3276800/maxVolume );
  }

quit:
  if(file)
  {
    Close(file);
  }
  if(file2)
  {
    Close(file2);
  }
  if(lock)
  {
    CurrentDir(cd);
    UnLock(lock);
  }

  Forbid();
  dd->fs_SlaveTask = NULL;
  FreeSignal(dd->fs_SlaveSignal);
  dd->fs_SlaveSignal    = -1;
  // Tell the Master we're dying
  Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_MasterSignal);
  // Multitaking will resume when we are dead.
}

/*
** Apple's 80-bit SANE extended has the following format:

 1       15      1            63
+-+-------------+-+-----------------------------+
|s|       e     |i|            f                |
+-+-------------+-+-----------------------------+
  msb        lsb   msb                       lsb

The value v of the number is determined by these fields as follows:
If 0 <= e < 32767,              then v = (-1)^s * 2^(e-16383) * (i.f).
If e == 32767 and f == 0,       then v = (-1)^s * (infinity), regardless of i.
If e == 32767 and f != 0,       then v is a NaN, regardless of i.
*/

void ulong2extended (ULONG in, extended *ex)
{
  ex->exponent = 31+16383;
  ex->mantissa[1] = 0;
  while(!(in & 0x80000000))
  {
    ex->exponent--;
    in <<= 1;
  }
  ex->mantissa[0] = in;

  ex->exponent    = htobe_short( ex->exponent );
  ex->mantissa[0] = htobe_long( ex->mantissa[0] );
  ex->mantissa[1] = htobe_long( ex->mantissa[1] );
}
