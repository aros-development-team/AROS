/* Simple sample player for AHI using the low-level API.
   Hardcoded sample names.

   This software is Public Domain. */

#include <devices/ahi.h>
#include <exec/exec.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define USE_AHI_V4 TRUE

#define CHANNELS   2
#define MAXSAMPLES 16

#define INT_FREQ   50

char *ID = "$VER: SimplePlay 1.3 (23.4.97)\r\n";

APTR samples[MAXSAMPLES] = { 0 };

struct {
  BOOL      FadeVolume;
  Fixed     Volume;
  sposition Position;
} channelstate[CHANNELS];

struct {
  struct AHIEffDSPMask mask;
  UBYTE                mode[CHANNELS];
} maskeffect = {0};
struct AHIEffDSPEcho   echoeffect = {0};

struct Library      *AHIBase;
struct MsgPort      *AHImp     = NULL;
struct AHIRequest   *AHIio     = NULL;
BYTE                 AHIDevice = -1;
struct AHIAudioCtrl *actrl     = NULL;

LONG mixfreq = 0;

/* Prototypes */

BOOL  OpenAHI(void);
void  CloseAHI(void);
BOOL  AllocAudio(void);
void  FreeAudio(void);
UWORD LoadSample(unsigned char * , ULONG );
void  UnloadSample(UWORD );
int   main(void);

/******************************************************************************
** PlayerFunc *****************************************************************
******************************************************************************/

__asm __interrupt __saveds static void PlayerFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 APTR ignored) {

  int i;
  
  for(i = 0; i < CHANNELS; i++) {

    if(channelstate[i].FadeVolume) {

      channelstate[i].Volume = (channelstate[i].Volume * 90) / 100; // Fade volume

      if(channelstate[i].Volume == 0) {
        channelstate[i].FadeVolume = FALSE;
      }

      AHI_SetVol(i, channelstate[i].Volume, channelstate[i].Position,
          actrl, AHISF_IMM);
    }
  }
  return;
}

struct Hook PlayerHook = {
  0,0,
  (ULONG (* )()) PlayerFunc,
  NULL,
  NULL,
};

/******************************************************************************
**** OpenAHI ******************************************************************
******************************************************************************/

/* Open the device for low-level usage */

BOOL OpenAHI(void) {

  if(AHImp = CreateMsgPort()) {
    if(AHIio = (struct AHIRequest *)CreateIORequest(
        AHImp,sizeof(struct AHIRequest))) {

#if USE_AHI_V4
      AHIio->ahir_Version = 4;
#else
      AHIio->ahir_Version = 2;
#endif

      if(!(AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT,
          (struct IORequest *) AHIio,NULL))) {
        AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;
        return TRUE;
      }
    }
  }
  FreeAudio();
  return FALSE;
}


/******************************************************************************
**** CloseAHI *****************************************************************
******************************************************************************/

/* Close the device */

void CloseAHI(void) {

  if(! AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  AHIDevice=-1;
  DeleteIORequest((struct IORequest *)AHIio);
  AHIio=NULL;
  DeleteMsgPort(AHImp);
  AHImp=NULL;
}


/******************************************************************************
**** AllocAudio ***************************************************************
******************************************************************************/

/* Ask user for an audio mode and allocate it */

BOOL AllocAudio(void) {
  struct AHIAudioModeRequester *req;
  BOOL   rc = FALSE;

  req = AHI_AllocAudioRequest(
      AHIR_PubScreenName, NULL,
      AHIR_TitleText,     "Select a mode and rate",
      AHIR_DoMixFreq,     TRUE,
      TAG_DONE);

  if(req) {
    if(AHI_AudioRequest(req, TAG_DONE)) {
      actrl = AHI_AllocAudio(
          AHIA_AudioID,         req->ahiam_AudioID,
          AHIA_MixFreq,         req->ahiam_MixFreq,
          AHIA_Channels,        CHANNELS,
          AHIA_Sounds,          MAXSAMPLES,
          AHIA_PlayerFunc,      &PlayerHook,
          AHIA_PlayerFreq,      INT_FREQ<<16,
          AHIA_MinPlayerFreq,   INT_FREQ<<16,
          AHIA_MaxPlayerFreq,   INT_FREQ<<16,
          TAG_DONE);
      if(actrl) {
        // Get real mixing frequency
        AHI_ControlAudio(actrl, AHIC_MixFreq_Query, &mixfreq, TAG_DONE);
        rc = TRUE;
      }
    }
    AHI_FreeAudioRequest(req);
  }
  return rc;
}


/******************************************************************************
**** FreeAudio ****************************************************************
******************************************************************************/

/* Release the audio hardware */

void FreeAudio() {

  AHI_FreeAudio(actrl);
  actrl = NULL;
}

/******************************************************************************
**** LoadSample ***************************************************************
******************************************************************************/

/* Load a (raw) 8 or 16 bit sample from disk. The sample ID is returned
   (or AHI_NOSOUND on error). */

UWORD LoadSample(char *filename, ULONG type) {
  struct AHISampleInfo sample;
  APTR *samplearray = samples;
  UWORD id = 0, rc = AHI_NOSOUND;
  BPTR file;

  // Find a free sample slot

  while(*samplearray) {
    id++;
    samplearray++;
    if(id >= MAXSAMPLES) {
      return AHI_NOSOUND;
    }
  }

  file = Open(filename, MODE_OLDFILE);
  
  if(file) {
    int length;

    Seek(file, 0, OFFSET_END);
    length = Seek(file, 0, OFFSET_BEGINNING);
    *samplearray = AllocVec(length, MEMF_PUBLIC);
    if(*samplearray) {
      Read(file, *samplearray, length);

      sample.ahisi_Type = type;
      sample.ahisi_Address = *samplearray;
#if USE_AHI_V4
      sample.ahisi_Length = length / AHI_SampleFrameSize(type);
#else
      sample.ahisi_Length = length / (type == AHIST_M16S ? 2 : 1);
#endif
      if(! AHI_LoadSound(id, AHIST_SAMPLE, &sample, actrl)) {
        rc = id;
      }
    }
    Close(file);
  }
  return rc;
}


/******************************************************************************
**** UnloadSample *************************************************************
******************************************************************************/

void UnloadSample(UWORD id) {

  AHI_UnloadSound(id, actrl);
  FreeVec(samples[id]);
  samples[id] = NULL;
}

/******************************************************************************
**** main *********************************************************************
******************************************************************************/

int main() {
  UWORD sample, ahem, louise;
  int i;

  for(i = 0; i < CHANNELS; i++) {
    channelstate[i].FadeVolume = FALSE;
  }

  if(OpenAHI()) {
    if(AllocAudio()) {
      ahem   = LoadSample("Projekt:ahi/samples/ASS-14.sb", AHIST_M8S);
      louise = LoadSample("Projekt:ahi/samples/LouiseR.sw", AHIST_M16S);
      if((ahem != AHI_NOSOUND) && (louise != AHI_NOSOUND)) {

        // Start feeding samples to sound hardware
        if(!(AHI_ControlAudio(actrl,
            AHIC_Play, TRUE,
            TAG_DONE)))
        {

#if USE_AHI_V4
          Printf("Playing two samples, one with echo and one without.\n");

          // Turn on echo on channel 1
          // No error checking here, but you should do that.
          maskeffect.mask.ahie_Effect = AHIET_DSPMASK;
          maskeffect.mask.ahiedm_Channels = CHANNELS;
          maskeffect.mode[0] = AHIEDM_DRY;
          maskeffect.mode[1] = AHIEDM_WET;
          AHI_SetEffect( &maskeffect, actrl );
          
          echoeffect.ahie_Effect     = AHIET_DSPECHO;
          echoeffect.ahiede_Delay    = mixfreq / 4; // 250 ms
          echoeffect.ahiede_Feedback = 0x8000;      // 50 %
          echoeffect.ahiede_Mix      = 0x10000;     // 100% and...
          echoeffect.ahiede_Cross    = 0;           // ...0% gives faster echo code.
          AHI_SetEffect( &echoeffect, actrl );

          AHI_Play(actrl,
            // A forever-looping sample on channel 0
            AHIP_BeginChannel,  0,
            AHIP_Freq,          17640,
            AHIP_Vol,           0x10000,
            AHIP_Pan,           0xc000,
            AHIP_Sound,         louise,
            AHIP_EndChannel,    NULL,

            // A oneshot sample on channel 1
            AHIP_BeginChannel,  1,
            AHIP_Freq,          22254,
            AHIP_Vol,           0x10000,
            AHIP_Pan,           0x4000,
            AHIP_Sound,         ahem,
            AHIP_LoopSound,     AHI_NOSOUND,
            AHIP_EndChannel,    NULL,

            TAG_DONE);
#else
          Printf("Playing two samples.\n");

          // A forever-looping sample on channel 0
          AHI_SetFreq(0, 17640, actrl, AHISF_IMM);
          AHI_SetVol(0, 0x10000, 0xc000, actrl, AHISF_IMM);
          AHI_SetSound(0, louise, 0, 0, actrl, AHISF_IMM);

          // A oneshot sample on channel 1
          AHI_SetFreq(1, 22254, actrl, AHISF_IMM);
          AHI_SetVol(1, 0x10000, 0x4000, actrl, AHISF_IMM);
          AHI_SetSound(1, ahem, 0, 0, actrl, AHISF_IMM);
          AHI_SetSound(1, AHI_NOSOUND, 0, 0, actrl, 0);
#endif
          channelstate[0].Volume   = 0x10000;
          channelstate[0].Position = 0xc000;
          channelstate[1].Volume   = 0x10000;
          channelstate[1].Position = 0x4000;

          // Wait 5 seconds
          
          Delay(5 * TICKS_PER_SECOND);

#if USE_AHI_V4
          Printf("Turning on echo on channel 0.\n");

          // Turn on echo on channel 0, turn off channel 1
          // No error checking here, but you should do that.
          maskeffect.mode[0] = AHIEDM_WET;
          maskeffect.mode[1] = AHIEDM_DRY;
          AHI_SetEffect( &maskeffect, actrl );
#endif

          // Wait 5 seconds
          
          Delay(5 * TICKS_PER_SECOND);

          Printf("Fading away the sound on channel 0... ");
          Flush(Output());

          // Fade away channel 0
          
          channelstate[0].FadeVolume = TRUE;

          while(channelstate[0].FadeVolume) {
            Delay(1);
          }

          Printf("Done!\n");

          // Wait a sec...

          Delay(TICKS_PER_SECOND);

          // Stop sounds
          AHI_ControlAudio(actrl,
              AHIC_Play, FALSE,
              TAG_DONE);


#if USE_AHI_V4
          // Cancel effects

          maskeffect.mask.ahie_Effect = AHIET_DSPMASK | AHIET_CANCEL;
          AHI_SetEffect( &maskeffect, actrl );
          
          echoeffect.ahie_Effect     = AHIET_DSPECHO | AHIET_CANCEL;
          AHI_SetEffect( &echoeffect, actrl );
#endif

        }
      }

      // Ask AHI to unload all loaded samples and free the memory for them 
      for(sample = 0; sample < MAXSAMPLES; sample++) {
        UnloadSample(sample);
      }

    }
    FreeAudio();
  }
  CloseAHI();

  return 0;
}
