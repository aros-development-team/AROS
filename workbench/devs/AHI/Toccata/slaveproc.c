/*
     toccata.library - AHI-based Toccata emulation library
     Copyright (C) 1997-2005 Martin Blom <martin@blom.org> and Teemu Suikki.
     
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


#include <devices/ahi.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/exec.h>
#include <libraries/iffparse.h>
#include <libraries/maud.h>

#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <clib/toccata_protos.h>
#include <pragmas/toccata_pragmas.h>

#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "toccata.h"

//#define DEBUG

#ifdef DEBUG
BPTR dfh;
#define DBG(text) {Forbid();FPuts(dfh,text);Flush(dfh);Permit();}
#define DVAL(text,val) {Forbid();VFPrintf(dfh,(STRPTR)text,&val);Flush(dfh);Permit();}
#else
#define DBG(text)
#define DVAL(text,val)
#endif

/* Prototypes */

static ASM ULONG SoundFunc(REG(a0) struct Hook *hook,
                           REG(a2) struct AHIAudioCtrl *audioctrl,
                           REG(a1) struct AHISoundMessage *msg);
static ASM ULONG RecordFunc(REG(a0) struct Hook *hook,
                            REG(a2) struct AHIAudioCtrl *audioctrl,
                            REG(a1) struct AHIRecordMessage *msg);

ASM LONG ReadMAUD(REG(a0) UBYTE *buffer, REG(d0) ULONG length, REG(a1) ULONG unused);

static BOOL AllocAudio(void);
static void FreeAudio(void);
static BOOL TuneAudio(void);
static BOOL ControlAudio(void);

static void Pause(ULONG pause);
static void Stop(ULONG flags);
static BOOL RawPlayback(struct TagItem *tags);
static BOOL Playback(struct TagItem *tags);


struct Process *IOProcess = NULL;
BOOL IOInitialized = FALSE;



/* Some flags */

BOOL SlaveInitialized = FALSE;
BOOL AudioInitialized = FALSE;
BOOL Playing          = FALSE;
BOOL Recording        = FALSE;
BOOL Leveling         = FALSE;
BOOL Pausing          = FALSE;
BOOL Sound0Loaded     = FALSE;
BOOL Sound1Loaded     = FALSE;
LONG SoundFlag        = 0;
BOOL FirstBuf		  = FALSE;

/* RawPlayback variables */

struct Task      *ErrorTask, *RawTask, *SigTask;
ULONG             ErrorMask, RawMask, SigMask;
struct Interrupt *RawInt;
BYTE             *RawBuffer1, *RawBuffer2;
ULONG             RawBufferSize, RawIrqSize=512;
ULONG            *ByteCount;
ULONG             ByteSkip;

ULONG             RawBufferLength;  /* RawBufferSize / samplesize */
LONG              NextBufferOK;
LONG              ByteSkipCounter;

/* Playback variables */

struct Task      *EndTask;
ULONG             EndMask;
struct Window    *Window;
BYTE              IoPri;
ULONG             BufferSize;
ASM LONG          (*Load)(REG(a0) UBYTE *, REG(d0) ULONG, REG(a1) ULONG);
ULONG             LoadParam;
STRPTR            FileName;
LONG              Length;
BOOL              SmartPlay;
UWORD             PreBlocks;
UWORD             PlayBlocks;
ULONG             Flags;

struct IFFHandle *iff;
BOOL              iffopened;
BYTE             *Buffer1, *Buffer2;
UWORD             BuffersLoaded;
struct List       BufferList;
WORD              BufferFlag;

struct Buffer {
  struct Node     Node;
  LONG            Size;
  BYTE           *Buffer;
};

/* Audio stuff */

struct AHIAudioCtrl *audioctrl  = NULL;

Fixed MinMonVol, MaxMonVol, MinOutVol, MaxOutVol, MinGain, MaxGain;

struct Hook SoundHook  = {
  NULL, NULL, (ULONG (*)()) HookLoad, SoundFunc, NULL
};

struct Hook RecordHook = {
  NULL, NULL, (ULONG (*)()) HookLoad, RecordFunc, NULL
};

/* dB<->Fixed conversion */

const Fixed negboundaries[] = {
  65536,55141,46395,39037,32845,27636,23253,19565,16461,13850,11654,9805,8250,
  6941,5840,4914,4135,3479,2927,2463,2072,1743,1467,1234,1038,873,735,618,520,
  438,368,310,260,219,184,155,130,110,92,77,65,55,46,39,32,27,23,19,16,13,11,9,
  8,6,5,4,4,3,2,2,2,1,1,1,0
};

const Fixed posboundaries[] = {
  65536,77889,92572,110022,130761,155410,184705,219522,260903,
  310084,368536,438005,520570,618699,735326,873936
};



/*
 *  SoundFunc(): Called when a sample has just started
 */

static ASM ULONG SoundFunc(REG(a0) struct Hook *hook,
                           REG(a2) struct AHIAudioCtrl *audioctrl,
                           REG(a1) struct AHISoundMessage *msg) {

    if(SoundFlag == 0) {
      SoundFlag = 1;
    }
    else {
      SoundFlag = 0;
    }

	if (!FirstBuf) {
	  if(!NextBufferOK && (ErrorTask != NULL)) {
	      Signal(ErrorTask, ErrorMask);
	      if(IOProcess != NULL) {
	        Signal((struct Task *) IOProcess, SIGBREAKF_CTRL_E);
	      }
	      else {
	        Signal((struct Task *) SlaveProcess, SIGBREAKF_CTRL_E);
	      }
	  }
	
	  if(RawInt != NULL) {
	      Cause(RawInt);
	  }
	  else if(RawTask != NULL) {
		  NextBufferOK=FALSE;
	      Signal(RawTask, RawMask);
	  }
  } else FirstBuf=FALSE;

  AHI_SetSound(0, SoundFlag, 0,RawBufferLength, audioctrl,0);

  if(ByteCount != NULL) {
    *ByteCount += RawBufferSize;
  }


  if(SigTask != NULL) {
    ByteSkipCounter -= RawBufferSize;
    if(ByteSkipCounter <= 0) {
      ByteSkipCounter += ByteSkip;
      Signal(SigTask, SigMask);
    }
  }

  return 0;
}


/*
 *  RecordFunc(): Called when a new block of recorded data is available
 */

static ASM ULONG RecordFunc(REG(a0) struct Hook *hook,
                            REG(a2) struct AHIAudioCtrl *audioctrl,
                            REG(a1) struct AHIRecordMessage *msg) {

  return 0;
}


/*
 *  SlaveTask(): The slave process
 *  CTRL_C terminates, CTRL_E stops playing/recording (error signal)
 */

void ASM SlaveTask(void) {
  struct MsgPort    *AHImp = NULL;
  struct AHIRequest *AHIio = NULL;
  BYTE AHIDevice = -1;
  struct Process    *me;
  ToccataBase->tb_HardInfo = NULL;

#ifdef DEBUG
  dfh=Open("con:10/300/400/300/Output",MODE_OLDFILE);
  DBG("Slave started!\n");
#endif    

  me = (struct Process *) FindTask(NULL);

  if(AHImp=CreateMsgPort()) {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)AHIio, NULL);
    }
  }

  if(AHIDevice == 0) {
    AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;

	fillhardinfo();

    SlaveInitialized = TRUE;

    while(TRUE) {
      ULONG signals;
      struct slavemessage *msg;

      signals = Wait((1L << me->pr_MsgPort.mp_SigBit) |
                     SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E);

      if(signals & SIGBREAKF_CTRL_C) {
        break;
      }

      if(signals & SIGBREAKF_CTRL_E) {
        Stop(0);
      }

      if(signals & (1L << me->pr_MsgPort.mp_SigBit)) {
        while(msg = (struct slavemessage *) GetMsg(&me->pr_MsgPort)) {
          DVAL("gotmessage! %ld: ",msg->ID);
		  DVAL("0x%08lx\n",msg->Data);
          switch(msg->ID) {
            case MSG_MODE:
              FreeAudio();
              AllocAudio();
              break;
            case MSG_HWPROP:
              TuneAudio();
              break;
            case MSG_RAWPLAY:
              msg->Data = (APTR) RawPlayback(msg->Data);
              break;
            case MSG_PLAY:
              msg->Data = (APTR) Playback(msg->Data);
              break;
            case MSG_RECORD:
              DBG("record\n");
              break;
            case MSG_STOP:
              Stop(*((ULONG *) msg->Data));
              break;
            case MSG_PAUSE:
              Pause(*((ULONG *) msg->Data));
              break;
            case MSG_LEVELON:
              DBG("levelon\n");
              break;
            case MSG_LEVELOFF:
              DBG("leveloff\n");
              break;
            default:
              DBG("unknown\n");
              break;
          }
          ReplyMsg((struct Message *) msg);
        }
      }
    } /* while */
  }

  SlaveInitialized = FALSE;

  Stop(0);
  FreeAudio();

  AHIBase = NULL;
  if(!AHIDevice) {
    CloseDevice((struct IORequest *)AHIio);
  }
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  DBG("(slave closed down)\n");

#ifdef DEBUG
	Close(dfh);
	dfh=NULL;
#endif

  Forbid();
  SlaveProcess = NULL;
}


/*
 *  IOTask(): The play/record process
 *  CTRL_C terminates, CTRL_D rawsignal, CTRL_E error signal
 */

void ASM IOTask(void) {
  struct Process    *me;
  BOOL rc = FALSE;

  me = (struct Process *) FindTask(NULL);

  BuffersLoaded = 0;
  NewList(&BufferList);

  iff = NULL;
  iffopened = FALSE;


  /* Open MAUD file for reading */

  if(FileName != NULL) {
    iff = AllocIFF();
    if(iff) {
      iff->iff_Stream = Open(FileName, MODE_OLDFILE);
      if(iff->iff_Stream) {
        InitIFFasDOS(iff);
        if(!OpenIFF(iff, IFFF_READ)) {
          iffopened = TRUE;
          if(!StopChunk(iff, ID_MAUD, ID_MDAT)) {
            while(TRUE) {
              LONG error = ParseIFF(iff,IFFPARSE_SCAN);
              struct ContextNode *cn;

              if(error == IFFERR_EOC) continue;
              if(error) break;
              
              cn = CurrentChunk(iff);
              
              if(cn && (cn->cn_Type == ID_MAUD) && (cn->cn_ID == ID_MDAT)) {
                rc = TRUE;
                break;
              }
            } /* while */
          }
        }
      }
    }
  }
  else rc = TRUE; /* Not an error! */


  /* Fill the two play buffers with data */

  memset(Buffer1, 0, BufferSize);
  Load(Buffer1, BufferSize, LoadParam);

  memset(Buffer2, 0, BufferSize);
  Load(Buffer2, BufferSize, LoadParam);

  BufferFlag = 0;

  /* Now prefill some more */

  if(rc) {
    while(BuffersLoaded < (PreBlocks - 2)) {
      struct Buffer *buffer;
    
      buffer = (struct Buffer *) AllocVec(sizeof (struct Buffer), MEMF_CLEAR);
      if(buffer == NULL) {
        break;
      }

      buffer->Buffer = AllocVec(BufferSize, MEMF_PUBLIC | MEMF_CLEAR);
      if(buffer->Buffer) {
        buffer->Size = Load(buffer->Buffer, BufferSize, LoadParam);
        AddTail(&BufferList, (struct Node *) buffer);
        BuffersLoaded++;
      }
      else {
        FreeVec(buffer);
        break;
      }
    }

    IOInitialized = TRUE;

    DBG("iotask initialized\n");
  }

  while(rc) {
    ULONG signals;

    signals = SetSignal(0,0);
    
    if(signals & SIGBREAKF_CTRL_C) {
      DBG("iotask got break\n");
      break;
    }

    if(SIGBREAKF_CTRL_D) {
      struct Buffer *buffer;
      LONG size = 0;

      DBG("iotask swapped buffer!\n");

      buffer = (struct Buffer *) RemHead(&BufferList);
      if(buffer) {
        BuffersLoaded--;
        size = buffer->Size;
        memcpy((BufferFlag == 0 ? Buffer1 : Buffer2) , buffer->Buffer,
               buffer->Size);
      }

      /* Clear the rest of the block */
      if((BufferSize - size) > 0) {
        memset((BufferFlag == 0 ? Buffer1 : Buffer2) + size, 0,
               BufferSize - size);
      }

      if(BufferFlag == 0) {
        BufferFlag = 1;
      }
      else {
        BufferFlag = 0;
      }

      RawReply();

    }

    if(SIGBREAKF_CTRL_E) {
      DBG("iotask got error\n");
      if(!SmartPlay) {
        /* Ask him to stop and kill us */
        Signal((struct Task *) SlaveProcess, SIGBREAKF_CTRL_E);
      }
    }


    /* If there are no signals pending, lets load some data! */

    while((signals & ((1L << me->pr_MsgPort.mp_SigBit) |
                   SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E)) == 0) {

      if(BuffersLoaded < PlayBlocks) {
        struct Buffer *buffer;
    
        buffer = AllocVec(sizeof (struct Buffer), MEMF_CLEAR);
        if(buffer == NULL) {
          break;
        }

        buffer->Buffer = AllocVec(BufferSize, MEMF_PUBLIC | MEMF_CLEAR);
        if(buffer->Buffer) {
          buffer->Size = Load(buffer->Buffer, BufferSize, LoadParam);
          AddTail(&BufferList, (struct Node *) buffer);
          BuffersLoaded++;
        }
        else {
          FreeVec(buffer);
          break;
        }
      }
      else {
        /* Wait */
        signals = Wait((1L << me->pr_MsgPort.mp_SigBit) |
                       SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);
      }
    }




  }

  DBG("iotask terminating\n");

  if(iff) {

    if(iffopened) {
      CloseIFF(iff);
    }

    if(iff->iff_Stream) {
      Close(iff->iff_Stream);
    }

    FreeIFF(iff);
  }

  iff = NULL;


  /* Free buffers if any left */
  {
    struct Buffer *buffer;

    while(buffer = (struct Buffer *) RemHead(&BufferList)) {
      FreeVec(buffer->Buffer);
      FreeVec(buffer);
    }
  }

  FreeVec(Buffer1);
  Buffer1 = NULL;
  FreeVec(Buffer2);
  Buffer2 = NULL;

  Forbid();
  IOProcess = NULL;
}


/*
 *  ReadMAUD(): Used as callback function when playing
 */

ASM LONG ReadMAUD(REG(a0) UBYTE *buffer, REG(d0) ULONG length,
                  REG(a1) ULONG unused) {

  return ReadChunkBytes(iff, buffer, length);
}

/*
 *  AllocAudio(): Allocate the audio hardware
 */

static BOOL AllocAudio(void) {

  DBG("(AllocAudio())...\n");

  /* Set up for HookFunc */
  SoundHook.h_Data = ToccataBase;
  RecordHook.h_Data = ToccataBase;

  MinMonVol = MaxMonVol = 0;
  MinOutVol = MaxOutVol = 0x10000;
  MinGain   = MaxGain   = 0x10000;

  audioctrl = AHI_AllocAudio(
      AHIA_AudioID,       (tprefs.Mode & TMODEF_STEREO ?
                           tprefs.StereoMode : tprefs.MonoMode),
      AHIA_MixFreq,       tprefs.Frequency,
      AHIA_Channels,      1,
      AHIA_Sounds,        2,
      AHIA_SoundFunc,     &SoundHook,

      AHIA_PlayerFreq,    (tprefs.Frequency / 512) << 16,
      AHIA_MinPlayerFreq, (tprefs.Frequency / 512) << 16,
      AHIA_MaxPlayerFreq, (tprefs.Frequency / 512) << 16,
      AHIA_RecordFunc,    &RecordHook,
      TAG_DONE);

  if(audioctrl != NULL) {
    AHI_GetAudioAttrs(AHI_INVALID_ID, audioctrl,
        AHIDB_MinMonitorVolume, &MinMonVol,
        AHIDB_MaxMonitorVolume, &MaxMonVol,
        AHIDB_MinOutputVolume,  &MinOutVol,
        AHIDB_MaxOutputVolume,  &MaxOutVol,
        AHIDB_MinInputGain,     &MinGain,
        AHIDB_MaxInputGain,     &MaxGain,
        TAG_DONE);

    fillhardinfo();
    TuneAudio();

    AudioInitialized = TRUE;
    DBG("ok!\n");
    return TRUE;
  }
  DBG("Nope!\n");
  return FALSE;
}


/*
 *  FreeAudio(): Release the audio hardware
 */

static void FreeAudio(void) {

  DBG("(FreeAudio())...\n");

  Playing = FALSE;
  Recording = FALSE;

  ControlAudio();

  if(audioctrl != NULL) {
    AHI_FreeAudio(audioctrl);
  }

  audioctrl = NULL;
  AudioInitialized = FALSE;
}


/*
 *  TuneAudio(): Change (hardware) properties of the allocated audio mode
 */

static BOOL TuneAudio() {
  Fixed MonVol, OutVol, Gain;
  ULONG Input;
  BOOL rc = FALSE;

  DBG("(TuneAudio())\n");

  if(audioctrl != NULL) {

    MonVol = negboundaries[tprefs.LoopbackVolume];
    OutVol = negboundaries[tprefs.OutputVolumeLeft];
    Gain   = posboundaries[tprefs.InputVolumeLeft];

    MonVol = min( max(MonVol, MinMonVol), MaxMonVol);
    OutVol = min( max(OutVol, MinOutVol), MaxOutVol);
    Gain   = min( max(Gain,   MinGain),   MaxGain);

    switch(tprefs.Input) {

      case TINPUT_Line:
        Input = tprefs.LineInput;
        break;

      case TINPUT_Aux1:
        Input = tprefs.Aux1Input;
        break;

      case TINPUT_Mic:
        if(tprefs.MicGain) {
          Input = tprefs.MicGainInput;
        }
        else {
          Input = tprefs.MicInput;
        }
        break;

      case TINPUT_Mix:
        Input = tprefs.MixInput;
        break;
    }

    rc = AHI_ControlAudio(audioctrl,
        AHIC_MonitorVolume, MonVol,
        AHIC_OutputVolume,  OutVol,
        AHIC_InputGain,     Gain,
        AHIC_Input,         Input,
        TAG_DONE);
    rc = (rc == AHIE_OK ? TRUE : FALSE);
  }

  return rc;
}


/*
 *  ControlAudio(): Start/Stop/Pause playing and recording
 */

static BOOL ControlAudio(void) {
  BOOL rc;

  DBG("(ControlAudio())\n");

  if(audioctrl) {
    rc = AHI_ControlAudio(audioctrl,
        AHIC_Play,        (Playing && !Pausing),
        AHIC_Record,      (Recording && !Pausing),
        AHIA_PlayerFreq,  (tprefs.Frequency / 512),
        TAG_DONE);
  }

  rc = (rc == AHIE_OK ? TRUE : FALSE);
  return rc;
}


/*
 *  Pause(): Take care of the T_Pause() function
 */

static void Pause(ULONG pause) {

  DVAL("(Pause %ld)\n", pause);
  Pausing = pause;
  ControlAudio();
}


/*
 *  Stop(): Take care of the T_Stop() function
 */

static void Stop(ULONG flags) {

  DVAL("Stop(%lx)...\n", flags);

  Playing = FALSE;
  Recording = FALSE;

  ControlAudio();

  if(Sound0Loaded) {
    AHI_UnloadSound(0, audioctrl);
  }

  if(Sound1Loaded) {
    AHI_UnloadSound(1, audioctrl);
  }

  Sound0Loaded = Sound1Loaded = FALSE;

  /* Fill defaults */

  ErrorTask       =
  RawTask         =
  SigTask         = NULL;
  RawInt          = NULL;
  RawBuffer1      =
  RawBuffer2      = NULL;
  RawBufferSize   = 32768;
  RawIrqSize      = 512;
  ByteCount       = NULL;
  ByteSkip        =
  ByteSkipCounter = 2048;

  if(IOProcess) {
    Signal((struct Task *)IOProcess, SIGBREAKF_CTRL_C);
    while(IOProcess) {
      Delay(1);
    }
  }

  if((flags & TSF_DONTSAVECACHE) == 0) {
    /* Save cache here... */
  }

  /* Check if a record/play file is open, and close them if so */
  DBG("ok\n");
}


/*
 *  RawPlayback(): Take care of the T_RawPlayback() function
 */

static BOOL RawPlayback(struct TagItem *tags) {
  BOOL rc = TRUE;
  BOOL newmode = FALSE;
  struct TagItem *tstate;
  struct TagItem *tag;

  DBG("RawPlayback()...\n");

  /* Is this correct?? */

  if(Playing || Recording) {
    return FALSE;
  }

  /* Check arguments */

  tstate = tags;

  while (tag = NextTagItem(&tstate)) {
    DVAL("%ld ", tag->ti_Tag - TT_Min);
	DVAL("0x%08lx,\n", tag->ti_Data);
    switch (tag->ti_Tag) {

      case TT_IrqPri:
        break;

      case TT_Mode:
        if(tag->ti_Data != tprefs.Mode) {
          tprefs.Mode = tag->ti_Data;
          newmode = TRUE;
        }
        break;

      case TT_Frequency:
        if(tag->ti_Data != tprefs.Frequency) {
          tprefs.Frequency = tag->ti_Data;
          newmode = TRUE;
        }
        break;

      case TT_ErrorTask:
        ErrorTask = (struct Task *) tag->ti_Data;
        break;
        
      case TT_ErrorMask:
        ErrorMask = tag->ti_Data;
        break;

      case TT_RawTask:
        RawTask = (struct Task *) tag->ti_Data;
        break;

      case TT_RawMask:
        RawMask = tag->ti_Data;
        break;

      case TT_RawReply:
      {
        ULONG *p = (ULONG *) tag->ti_Data;
        
        *p = GetRawReply(ToccataBase);
        DVAL("Rawreply is 0x%08lx\n", *p);
        break;
      }

      case TT_RawInt:
        RawInt = (struct Interrupt *) tag->ti_Data;
        break;

      case TT_RawBuffer1:
        RawBuffer1 = (BYTE *) tag->ti_Data;
        break;

      case TT_RawBuffer2:
        RawBuffer2 = (BYTE *) tag->ti_Data;
        break;

      case TT_BufferSize:
        RawBufferSize = tag->ti_Data;
        break;

      case TT_RawIrqSize:
        RawIrqSize = tag->ti_Data;
        break;

      case TT_ByteCount:
        ByteCount = (ULONG *) tag->ti_Data;
        break;

      case TT_ByteSkip:
        ByteSkip = tag->ti_Data;
        break;

      case TT_SigTask:
        SigTask = (struct Task *) tag->ti_Data;
        break;

      case TT_SigMask:
        SigMask = tag->ti_Data;
        break;

    } /* switch */
  } /* while */

  DVAL("<<%ld\n", rc);

  if((ErrorTask == NULL) ||
     ((RawTask == NULL) && (RawInt == NULL)) ||
     (RawBuffer1 == NULL) ||
     (RawBuffer2 == NULL)) {

    rc = FALSE;
  }

  DVAL("<<%ld\n", rc);
  if(rc && (newmode || !AudioInitialized)) {
    FreeAudio();
    rc = AllocAudio();
  }

  DVAL("<<%ld\n", rc);

  if(rc) {
    ULONG sampletype = AHIST_NOTYPE;
    struct AHISampleInfo s0, s1;

    switch(tprefs.Mode) {
      case TMODE_LINEAR_8:
          sampletype = AHIST_M8S;
          break;
      case TMODE_LINEAR_16:
          sampletype = AHIST_M16S;
          break;
      case TMODE_ALAW:
      case TMODE_ULAW:
      case TMODE_RAW_16:
          rc = FALSE;
          break;
      case TMODE_LINEAR_8_S:
          sampletype = AHIST_S8S;
          break;
      case TMODE_LINEAR_16_S:
          sampletype = AHIST_S16S;
          break;
      case TMODE_ALAW_S:
      case TMODE_ULAW_S:
      case TMODE_RAW_16_S:
          rc = FALSE;
          break;
      default:
          rc = FALSE;
    }

  DVAL("<<%ld\n", rc);
    if(sampletype != AHIST_NOTYPE) {

      s0.ahisi_Type    =
      s1.ahisi_Type    = sampletype;
      s0.ahisi_Address = RawBuffer1;
      s1.ahisi_Address = RawBuffer2;
      s0.ahisi_Length  =
      s1.ahisi_Length  = RawBufferSize / AHI_SampleFrameSize(sampletype);

      if(AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &s0, audioctrl) == AHIE_OK) {
        Sound0Loaded = TRUE;
      }

      if(AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &s1, audioctrl) == AHIE_OK) {
        Sound1Loaded = TRUE;
      }
      
      if(!(Sound0Loaded && Sound1Loaded)) {
        rc = FALSE;
      }

      RawBufferLength = RawBufferSize / AHI_SampleFrameSize(sampletype);

    }
  }

  DVAL("<<%ld\n", rc);
  if(rc) {
    Playing         = TRUE;
    SoundFlag       = 0;
    NextBufferOK    = TRUE;
	FirstBuf		= TRUE;

    ControlAudio();
    AHI_Play(audioctrl,
      AHIP_BeginChannel,  0,
      AHIP_Freq,          tprefs.Frequency,
      AHIP_Vol,           0x10000,
      AHIP_Pan,           0x8000,
      AHIP_Sound,         0,
      AHIP_Offset,        0,
      AHIP_Length,        RawBufferLength,
      AHIP_EndChannel,    NULL,
      TAG_DONE);
  }

  DVAL("ok %ld\n", rc);
  return rc;
}


static BOOL Playback(struct TagItem *tags) {
  BOOL rc = TRUE;
  struct TagItem *tstate;
  struct TagItem *tag;

  DBG("Playback()...\n");

  Stop(0);

  /* Fill defaults */

  EndTask     = NULL;
  EndMask     = 0;
  Window      = NULL;
  IoPri       = tprefs.PlaybackIoPri;
  BufferSize  = tprefs.PlaybackBlockSize;
  Load        = NULL;
  LoadParam   = 0;
  FileName    = NULL;
  Length      = LONG_MAX;
  SmartPlay   = FALSE;
  PreBlocks   = tprefs.PlaybackStartBlocks;
  PlayBlocks  = tprefs.PlaybackBlocks;
  Flags       = NULL;

  Buffer1 = Buffer2 = NULL;

  /* Check arguments */

  FileName = (STRPTR) GetTagData(TT_FileName, NULL, tags);
  
  if(FileName != NULL) {
    struct IFFHandle *iff;
    struct StoredProperty *sp;
    BOOL gotheader = FALSE;

    Load = ReadMAUD;

    iff = AllocIFF();

    if(iff) {

      iff->iff_Stream = Open(FileName, MODE_OLDFILE);

      if(iff->iff_Stream) {

        InitIFFasDOS(iff);

        if(!OpenIFF(iff, IFFF_READ)) {

          if(!(PropChunk(iff, ID_MAUD, ID_MHDR)
            || StopOnExit(iff,ID_MAUD, ID_FORM))) {

            while(ParseIFF(iff,IFFPARSE_SCAN) == IFFERR_EOC) {

              sp = FindProp(iff, ID_MAUD, ID_MHDR);

              if(sp) {
                struct MaudHeader *mhdr = (struct MaudHeader *) sp->sp_Data;

                gotheader = TRUE;

                Length = mhdr->mhdr_Samples * mhdr->mhdr_SampleSizeU / 8;

                tprefs.Frequency = mhdr->mhdr_RateSource / mhdr->mhdr_RateDevide;

                switch(mhdr->mhdr_Compression) {

                  case MCOMP_NONE:
                    if(mhdr->mhdr_SampleSizeU == 8) {
                      tprefs.Mode = TMODE_LINEAR_8;
                    }
                    else if(mhdr->mhdr_SampleSizeU == 16) {
                      tprefs.Mode = TMODE_LINEAR_16;
                    }
                    else rc = FALSE;
                    break;

                  case MCOMP_ALAW:
                    tprefs.Mode = TMODE_ALAW;
                    break;

                  case MCOMP_ULAW:
                    tprefs.Mode = TMODE_ULAW;
                    break;
                } /* swicth */

                if(mhdr->mhdr_ChannelInfo == MCI_STEREO) {
                  tprefs.Mode |= TMODEF_STEREO;
                }
                else if(mhdr->mhdr_ChannelInfo != MCI_MONO) {
                  rc = FALSE;
                } 

                FreeAudio();
                AllocAudio();
                break; /* We have what we want, no need to loop futher */
              }
            }
          }
          CloseIFF(iff);
        }
        Close(iff->iff_Stream);
      }
      FreeIFF(iff);
    }

    if(!gotheader) {
      rc = FALSE;
    }
  }

  tstate = tags;

  while (rc && (tag = NextTagItem(&tstate))) {
    //kprintf("%ld, 0x%08lx,\n", tag->ti_Tag - TT_Min, tag->ti_Data);
    switch (tag->ti_Tag) {

      case TT_Window:
        Window = (struct Window *) tag->ti_Data;
        break;

      case TT_IoPri:
        IoPri = tag->ti_Data;
        break;

      case TT_IrqPri:
        break;

      case TT_BufferSize:
        BufferSize = tag->ti_Data;
        break;

      case TT_Load:
        Load = (ASM LONG (*)(REG(a0) UBYTE *, REG(d0) ULONG, REG(a1) ULONG)) tag->ti_Data;
        break;

      case TT_CBParamA1:
        LoadParam = tag->ti_Data;
        break;

      case TT_Mode:
      case TT_Frequency:
        break;

/* Already handled above!
      case TT_FileName:
        FileName = (STRPTR) tag->ti_Data;
        break;
*/

      case TT_Length:
        Length = tag->ti_Data;
        break;

      case TT_ErrorTask:
      case TT_ErrorMask:
        break;

      case TT_SmartPlay:
        SmartPlay = tag->ti_Data;
        break;

      case TT_PreBlocks:
        PreBlocks = tag->ti_Data;
        break;

      case TT_PlayBlocks:
        PlayBlocks = tag->ti_Data;
        break;

      case TT_Flags:
        Flags = tag->ti_Data;
        break;

      case TT_EndTask:
        EndTask = (struct Task *) tag->ti_Data;

      case TT_EndMask:
        EndMask = tag->ti_Data;
        break;

      case TT_MSList:
      case TT_StartOffset:
      case TT_FieldsPerSecond:
        rc = FALSE;
        break;

      /* RawPlayback takes care of these */
      case TT_ByteCount:
      case TT_ByteSkip:
      case TT_SigTask:
      case TT_SigMask:
        break;

    } /* switch */
  } /* while */
  
  if((Load == 0) && (FileName == NULL)) {
    rc = FALSE;
  }

  if(rc) {
    IOProcess = CreateNewProcTags(
        NP_Entry,     IOTaskEntry,
        NP_Name,      _LibName,
        NP_Priority,  IoPri,
        NP_WindowPtr, Window,
        TAG_DONE);

    if(IOProcess == NULL) {
      rc = FALSE;
    }
  }

  if(rc) {
    struct TagItem rawtags[] = {
      TT_RawTask,     0, /* IOProcess */
      TT_RawMask,     SIGBREAKF_CTRL_D,
      TT_RawBuffer1,  0, /* Buffer1 */
      TT_RawBuffer2,  0, /* Buffer2 */
      TT_BufferSize,  0, /* BufferSize */
      TT_ErrorTask,   0, /* IOProcess -- Can be overridded by users errortask */
      TT_ErrorMask,   SIGBREAKF_CTRL_E,
      TAG_MORE,       0, /* tags */
      TAG_DONE
    };

    Buffer1 = AllocVec(BufferSize, MEMF_PUBLIC);
    Buffer2 = AllocVec(BufferSize, MEMF_PUBLIC);

    rawtags[0].ti_Data = (ULONG) IOProcess;
    rawtags[2].ti_Data = (ULONG) Buffer1;
    rawtags[3].ti_Data = (ULONG) Buffer2;
    rawtags[4].ti_Data = (ULONG) BufferSize;
    rawtags[5].ti_Data = (ULONG) IOProcess;
    rawtags[7].ti_Data = (ULONG) tags;

    /* Make sure all buffers are preloaded */
    while((IOProcess != NULL) && !IOInitialized) {
      Delay(1);
    }

    if(IOProcess == NULL) {
      rc = FALSE;
    }

    if(rc) {
      rc = RawPlayback((struct TagItem *) &rawtags);
    }

  }


  return rc;
}
