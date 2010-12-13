/* Simple sample player for AHI using the low-level API
   with double buffered AHIST_DYNAMICSAMPLE sounds.

   Usage: DoubleBuffer < [raw sample file]

   the file must be in mono 16 bit signed big endian format sampled
   in 17640 Hz.

   This software is Public Domain. */

#include <devices/ahi.h>
#include <exec/exec.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define EQ ==
#define MINBUFFLEN 10000

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

BYTE signal=-1;

struct AHIAudioModeRequester *req=NULL;
struct AHIAudioCtrl *actrl=NULL;

BOOL DBflag=FALSE;  // double buffer flag
ULONG BufferLen=NULL;

struct AHISampleInfo Sample0 =
{
  AHIST_M16S,
  NULL,
  NULL,
};

struct AHISampleInfo Sample1 =
{
  AHIST_M16S,
  NULL,
  NULL,
};

__asm __saveds ULONG SoundFunc(register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHISoundMessage *smsg)
{
  if(DBflag = !DBflag) // Flip and test
    AHI_SetSound(0,1,0,0,actrl,NULL);
  else
    AHI_SetSound(0,0,0,0,actrl,NULL);
  Signal(actrl->ahiac_UserData,(1L<<signal));
  return NULL;
}

struct Hook SoundHook =
{
  0,0,
  SoundFunc,
  NULL,
  NULL,
};


int main()
{
  ULONG mixfreq,playsamples,readsamples,signals,i;

  if(AHImp=CreateMsgPort())
  {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest)))
    {
      AHIio->ahir_Version = 4;  // Open at least version 4 of 'ahi.device'.
      if(!(AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL)))
      {
        AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;

        if(req=AHI_AllocAudioRequest(
            AHIR_PubScreenName,"",
            AHIR_TitleText,"Select a mode and rate",
            AHIR_InitialMixFreq,17640,
            AHIR_DoMixFreq,TRUE,
            TAG_DONE))
        {
          if(AHI_AudioRequest(req,TAG_DONE))
          {
            if(actrl=AHI_AllocAudio(
                AHIA_AudioID,req->ahiam_AudioID,
                AHIA_MixFreq,req->ahiam_MixFreq,
                AHIA_Channels,1,
                AHIA_Sounds,2,
                AHIA_SoundFunc,&SoundHook,
                AHIA_UserData,FindTask(NULL),
                TAG_DONE))
            {
              AHI_GetAudioAttrs(AHI_INVALID_ID,actrl,
                  AHIDB_MaxPlaySamples,&playsamples,
                  TAG_DONE);
              AHI_ControlAudio(actrl,
                  AHIC_MixFreq_Query,&mixfreq,
                  TAG_DONE);
              BufferLen=playsamples*17640/mixfreq;
              if (BufferLen<MINBUFFLEN)
                BufferLen=MINBUFFLEN;

              Sample0.ahisi_Length=BufferLen;
              Sample1.ahisi_Length=BufferLen;
              Sample0.ahisi_Address=AllocVec(BufferLen*2,MEMF_PUBLIC|MEMF_CLEAR);
              Sample1.ahisi_Address=AllocVec(BufferLen*2,MEMF_PUBLIC|MEMF_CLEAR);

              if(Sample0.ahisi_Address && Sample1.ahisi_Address)
              {
                if((!(AHI_LoadSound(0,AHIST_DYNAMICSAMPLE,&Sample0,actrl))) &&
                   (!(AHI_LoadSound(1,AHIST_DYNAMICSAMPLE,&Sample1,actrl))))
                {
                  if((signal=AllocSignal(-1)) != -1)
                  {
                    if(!(AHI_ControlAudio(actrl,
                        AHIC_Play,TRUE,
                        TAG_DONE)))
                    {
// Everything is set up now. Let's load the buffers and start rockin'...
                      Read(Input(),Sample0.ahisi_Address,BufferLen*2);

// The new AHI_PlayA() function is demonstrated...
                      AHI_Play(actrl,
                        AHIP_BeginChannel,0,
                        AHIP_Freq,17640,
                        AHIP_Vol,0x10000L,
                        AHIP_Pan,0x8000L,
                        AHIP_Sound,0,
                        AHIP_Offset,0,
                        AHIP_Length,0,
                        AHIP_EndChannel,NULL,
                        TAG_DONE);
/* These functions were available in V2, too.
                      AHI_SetFreq(0,17640,actrl,AHISF_IMM);
                      AHI_SetVol(0,0x10000L,0x8000L,actrl,AHISF_IMM);
                      AHI_SetSound(0,0,0,0,actrl,AHISF_IMM);
*/
                      for(;;)
                      {
                        signals=Wait((1L<<signal) | SIGBREAKF_CTRL_C);
                        if(signals & SIGBREAKF_CTRL_C)
                          break;

                        if(DBflag)
                        {
                          readsamples=Read(Input(),Sample1.ahisi_Address,BufferLen*2)/2;
                          if(readsamples<BufferLen)
                          {
                            // Clear rest of buffer
                            for(i=readsamples;i<BufferLen;i++)
                              ((WORD *)Sample1.ahisi_Address)[i]=0;
                            Wait(1L<<signal);
                            // Clear other buffer
                            for(i=0;i<BufferLen;i++)
                              ((WORD *)Sample0.ahisi_Address)[i]=0;
                            break;
                          }
                        }
                        else
                        {
                          readsamples=Read(Input(),Sample0.ahisi_Address,BufferLen*2)/2;
                          if(readsamples<BufferLen)
                          {
                            // Clear rest of buffer
                            for(i=readsamples;i<BufferLen;i++)
                              ((WORD *)Sample0.ahisi_Address)[i]=0;
                            Wait(1L<<signal);
                            // Clear other buffer
                            for(i=0;i<BufferLen;i++)
                              ((WORD *)Sample1.ahisi_Address)[i]=0;
                            break;
                          }
                        }
                        

                      }
                      if(signals & SIGBREAKF_CTRL_C)
                        Printf("***Break\n");
                      else
                        Wait(1L<<signal);   // Wait for half-loaded buffer to finish.

                      AHI_ControlAudio(actrl,
                          AHIC_Play,FALSE,
                          TAG_DONE);
                    }
                    FreeSignal(signal);
                    signal=-1;
                  }
                  
                }
                else
                  Printf("Cannot intialize sample buffers\n");
              }
              else
                Printf("Out of memory.\n");

              FreeVec(Sample0.ahisi_Address);
              FreeVec(Sample1.ahisi_Address);
              Sample0.ahisi_Address=NULL;
              Sample1.ahisi_Address=NULL;
              AHI_FreeAudio(actrl);
              actrl=NULL;
            }
            else
              Printf("Unable to allocate sound hardware\n");
          }
          else
          {
            if(IoErr() EQ NULL)
              Printf("Aborted.\n");
            else if(IoErr() EQ ERROR_NO_MORE_ENTRIES)
              Printf("No available audio modes.\n");
            else if(IoErr() EQ ERROR_NO_FREE_STORE)
              Printf("Out of memory.\n");
          }
          
          AHI_FreeAudioRequest(req);
          req=NULL;
        }
        else
          Printf("Out of memory.\n");

        CloseDevice((struct IORequest *)AHIio);
        AHIDevice=-1; // Good habit, IMHO.
      }
      DeleteIORequest((struct IORequest *)AHIio);
      AHIio=NULL;
    }
    DeleteMsgPort(AHImp);
    AHImp=NULL;
  }
}
