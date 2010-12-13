
#include <exec/exec.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>
#include <libraries/toccata.h>

#include <dos/dos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/ahi_sub.h>
#include <clib/toccata_protos.h>
#include <pragmas/toccata_pragmas.h>

#include <math.h>

#include "toccata.h"

extern void KPrintF(char *fmt,...);

#define dd ((struct toccata *) AudioCtrl->ahiac_DriverData)

//#define PLAYBUFFERSIZE 512        // Size in bytes
//#define RECBUFFERSIZE  512*32     // in bytes

extern char __far _LibID[];
extern char __far _LibName[];

extern void __asm SlaveProcessEntry(void);
extern void __asm CallPlayFunc(void);
extern void __asm RecordFunc(void);
extern void __asm PlayFuncMono(void);
extern void __asm PlayFuncStereo(void);
extern void __asm PlayFuncMono32(void);
extern void __asm PlayFuncStereo32(void);
extern void __asm MixFunc(void);

struct Library        *UtilityBase = NULL;
struct Library        *AHIsubBase  = NULL;
struct ToccataBase    *ToccataBase = NULL;
struct DosLibrary     *DOSBase     = NULL;

LONG INPUTS  = 5;
BOOL In_Use  = FALSE;
BOOL NoTask  = FALSE;
LONG IrqSize = 512;
LONG PLAYBUFFERSIZE = 512;
LONG RECBUFFERSIZE  = 512*32;


LONG fixed2negdbvalue( LONG volume);
LONG fixed2posdbvalue( LONG volume);
BOOL StartPlaying(struct AHIAudioCtrlDrv *AudioCtrl, struct Process *me);
BOOL StartRecording(struct AHIAudioCtrlDrv *AudioCtrl, struct Process *me);

typedef BOOL __asm 
PreTimer_proto(register __a2 struct AHIAudioCtrlDrv* actrl);

typedef void __asm 
PostTimer_proto(register __a2 struct AHIAudioCtrlDrv* actrl);

const static STRPTR Inputs[] =
{
  "Line",
  "Aux1",
  "Mic",
  "Mic +20 dB",
  "Mixer"
};

const static ULONG inputmap[] =
{
  TINPUT_Line,
  TINPUT_Aux1,
  TINPUT_Mic,
  TINPUT_Mic,
  TINPUT_Mix
};

const static ULONG micgainmap[] =
{
  FALSE,
  FALSE,
  FALSE,
  TRUE,
  FALSE
};


int  __saveds __asm __UserLibInit (register __a6 struct Library *libbase)
{
  char prefs[10]="0";

  AHIsubBase = libbase;

  if(!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",37)))
  {
    Alert(AN_Unknown|AG_OpenLib|AO_DOSLib);
    return 1;
  }

  if(!(UtilityBase = OpenLibrary("utility.library",37)))
  {
    Alert(AN_Unknown|AG_OpenLib|AO_UtilityLib);
    return 1;
  }

/*
** "toccata..library" is a modified version of "toccata.library" which calls
** TT_RawInt in a real hardware interrupt instead of a software interrupt.
** The author wishes to remain anonymous, but his hack suits my purposes well.
*/

  ToccataBase = (struct ToccataBase *)OpenLibrary("toccata..library",12);

  if(ToccataBase == NULL)
  {
    ToccataBase = (struct ToccataBase *)OpenLibrary("toccata.library",12);
  }

  if(ToccataBase == NULL)
  {
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)
        OpenLibrary("intuition.library", 37);
    struct EasyStruct req = { sizeof (struct EasyStruct),
        0, _LibName, "Cannot open 'toccata.library' v12", "OK"};

    if(IntuitionBase) {
      	EasyRequest( NULL, &req, NULL, NULL );
      	CloseLibrary((struct Library *) IntuitionBase);
    }
    else
    {
      Alert(AN_Unknown|AG_OpenLib|AO_Unknown);
    }
    // NOTE! Don't fail if Toccata.library couldn't be opened!
  }


  /* Check for DraCoMotion */

  if(ToccataBase &&
     ToccataBase->tb_HardInfo &&
    (ToccataBase->tb_HardInfo->hi_Flags & HIF_DMOTION))
  {
    INPUTS = 1;
  }
  else
  {
    INPUTS = 5;
  }

  if(GetVar("ENV:AHItoccataNoTask", prefs, sizeof prefs, NULL ) != -1)
  {
    if(prefs[0] == '1')
    {
      NoTask = TRUE;
    }
  }

  if(GetVar("ENV:AHItoccataIrqSize", prefs, sizeof prefs, NULL ) != -1)
  {
    if(StrToLong(prefs, &IrqSize) != -1)
    {
      IrqSize = max(IrqSize, 32);
      IrqSize = min(IrqSize, 512);
      switch(IrqSize) {
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
          break;
        default:
          IrqSize = 512;
          break;
      }
    }
  }

  if(GetVar("ENV:AHItoccataPlayBufferSize", prefs, sizeof prefs, NULL ) != -1)
  {
    if(StrToLong(prefs, &PLAYBUFFERSIZE) != -1)
    {
      PLAYBUFFERSIZE = max(PLAYBUFFERSIZE, 512);
      PLAYBUFFERSIZE = min(PLAYBUFFERSIZE, 512*32);
      PLAYBUFFERSIZE = PLAYBUFFERSIZE & 0xfffffe00;
    }
  }

  if(GetVar("ENV:AHItoccataRecordBufferSize", prefs, sizeof prefs, NULL ) != -1)
  {
    if(StrToLong(prefs, &RECBUFFERSIZE) != -1)
    {
      RECBUFFERSIZE = max(RECBUFFERSIZE, 512);
      RECBUFFERSIZE = min(RECBUFFERSIZE, 512*32);
      RECBUFFERSIZE = RECBUFFERSIZE & 0xfffffe00;
    }
  }

  return 0;
}

void __saveds __asm __UserLibCleanup (register __a6 struct Library *libbase)
{
  if(DOSBase)       { CloseLibrary((struct Library *)DOSBase); DOSBase = NULL; }
  if(UtilityBase)   { CloseLibrary(UtilityBase); UtilityBase = NULL; }
  if(ToccataBase)   { CloseLibrary((struct Library *)ToccataBase); ToccataBase = NULL; }
}

ULONG __asm __saveds intAHIsub_AllocAudio(
    register __a1 struct TagItem *tagList,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  if((ToccataBase == NULL) ||
     (ToccataBase->tb_HardInfo == NULL))
  {
    return AHISF_ERROR;
  }

  // Make sure only there is only one user!
  
  Forbid();
  if(In_Use)
  {
    Permit();
    return AHISF_ERROR;
  }
  In_Use = TRUE;
  Permit();

  if(AudioCtrl->ahiac_DriverData = AllocVec(sizeof(struct toccata),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR))
  {
    dd->t_AHIsubBase    = AHIsubBase;
    dd->t_NoTask        = NoTask;
    dd->t_SlaveSignal   = -1;
    dd->t_PlaySignal    = -1;
    dd->t_RecordSignal  = -1;
    dd->t_MixSignal     = -1;

    dd->t_MasterTask = FindTask(NULL);
    dd->t_MasterSignal = AllocSignal(-1);
    if(dd->t_MasterSignal != -1)
    {
      AudioCtrl->ahiac_MixFreq = T_FindFrequency(AudioCtrl->ahiac_MixFreq);

      Forbid();
      if(dd->t_SlaveProcess = CreateNewProcTags(
          NP_Entry,&SlaveProcessEntry,
          NP_Name,_LibName,
          NP_Priority,127,
          TAG_DONE))
      {
        dd->t_SlaveProcess->pr_Task.tc_UserData = AudioCtrl;
      }
      Permit();

      if(dd->t_SlaveProcess)
      {
        Wait(1L << dd->t_MasterSignal);   // Wait for slave to come alive
        if(dd->t_SlaveProcess != NULL)    // Is slave alive or dead?
        {
          dd->t_Flags |= TF_IAMTHEOWNER;

          return AHISF_KNOWSTEREO|AHISF_KNOWHIFI|AHISF_CANRECORD|
                 AHISF_MIXING|AHISF_TIMING;
        }
      }
    }
  }

  In_Use = FALSE;
  return AHISF_ERROR;
}

void __asm __saveds intAHIsub_FreeAudio(
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  if(AudioCtrl->ahiac_DriverData)
  {
    if(dd->t_SlaveProcess)
    {
      if(dd->t_SlaveSignal != -1)
      {
        Signal((struct Task *)dd->t_SlaveProcess,1L<<dd->t_SlaveSignal); // Kill him!
      }
      Wait(1L<<dd->t_MasterSignal);  // Wait for slave to die
    }

    if(dd->t_Flags & TF_IAMTHEOWNER)
    {
      In_Use = FALSE;
    }

    FreeSignal(dd->t_MasterSignal);
    FreeVec(AudioCtrl->ahiac_DriverData);
    AudioCtrl->ahiac_DriverData = NULL;
  }
}



ULONG __asm __saveds intAHIsub_Start(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{

  AHIsub_Stop(AHISF_PLAY|AHISF_RECORD,AudioCtrl);       // Only half duplex!

  if(Flags & AHISF_PLAY)
  {

    if(!(dd->t_PlaySoftInt = AllocVec(sizeof(struct Interrupt),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR)))
      return AHIE_NOMEM;

    if(!(dd->t_MixSoftInt = AllocVec(sizeof(struct Interrupt),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR)))
      return AHIE_NOMEM;

    switch(AudioCtrl->ahiac_BuffType)
    {
      case AHIST_M16S:
        dd->t_PlaySoftInt->is_Code = (void (* )())PlayFuncMono;
        dd->t_Mode = TMODE_LINEAR_16;
        dd->t_TocSamples = PLAYBUFFERSIZE>>1;   // Toc. buffer is 16 bit mono
        break;
      case AHIST_S16S:
        dd->t_PlaySoftInt->is_Code = (void (* )())PlayFuncStereo;
        dd->t_Mode = TMODE_LINEAR_16_S;
        dd->t_TocSamples = PLAYBUFFERSIZE>>2;   // Toc. buffer is 16 bit stereo
        break;
      case AHIST_M32S:
        dd->t_PlaySoftInt->is_Code = (void (* )())PlayFuncMono32;
        dd->t_Mode = TMODE_LINEAR_16;
        dd->t_TocSamples = PLAYBUFFERSIZE>>1;   // Toc. buffer is 16 bit mono
        break;
      case AHIST_S32S:
        dd->t_PlaySoftInt->is_Code = (void (* )())PlayFuncStereo32;
        dd->t_Mode = TMODE_LINEAR_16_S;
        dd->t_TocSamples = PLAYBUFFERSIZE>>2;   // Toc. buffer is 16 bit stereo
        break;
      default:
        return AHIE_BADSAMPLETYPE;
    }

    if(!(dd->t_SampBuffer1 = AllocVec(PLAYBUFFERSIZE,
        MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->t_SampBuffer2 = AllocVec(PLAYBUFFERSIZE,
        MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;

    if(!(dd->t_MixBuffer1 = AllocVec(AudioCtrl->ahiac_BuffSize,
        MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->t_MixBuffer2 = AllocVec(AudioCtrl->ahiac_BuffSize,
        MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->t_MixBuffer3 = AllocVec(AudioCtrl->ahiac_BuffSize,
        MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;

    dd->t_PlaySoftInt->is_Node.ln_Type = NT_INTERRUPT;
    dd->t_PlaySoftInt->is_Node.ln_Name = _LibName;
    dd->t_PlaySoftInt->is_Data = AudioCtrl;

    dd->t_MixSoftInt->is_Code = (void (* )())MixFunc;
    dd->t_MixSoftInt->is_Node.ln_Type = NT_INTERRUPT;
    dd->t_MixSoftInt->is_Node.ln_Name = _LibName;
    dd->t_MixSoftInt->is_Data = AudioCtrl;

    Signal((struct Task *)dd->t_SlaveProcess,1L<<dd->t_PlaySignal);
  }

  if(Flags & AHISF_RECORD)
  {
    if(!(dd->t_RecBuffer = AllocVec(RECBUFFERSIZE,MEMF_PUBLIC|MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->t_RecMessage = AllocVec(sizeof(struct AHIRecordMessage),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR)))
      return AHIE_NOMEM;

    dd->t_RecMessage->ahirm_Type = AHIST_S16S;
    dd->t_RecMessage->ahirm_Buffer = dd->t_RecBuffer;

    Signal((struct Task *)dd->t_SlaveProcess,1L<<dd->t_RecordSignal);
  }

  return AHIE_OK;
}

/*
void __asm __saveds __interrupt intAHIsub_Update(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
}
*/

void __asm __saveds intAHIsub_Stop(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  if(Flags & AHISF_PLAY)
  {
    T_Stop(TSF_DONTSAVECACHE);
    dd->t_Flags &= ~TF_ISPLAYING;

// Disable Loopback
    T_SetPartTags(PAT_LoopbackVolume, -64, TAG_DONE);

    FreeVec(dd->t_MixBuffer1);    dd->t_MixBuffer1  = NULL;
    FreeVec(dd->t_MixBuffer2);    dd->t_MixBuffer2  = NULL;
    FreeVec(dd->t_MixBuffer3);    dd->t_MixBuffer3  = NULL;
    FreeVec(dd->t_SampBuffer1);   dd->t_SampBuffer1 = NULL;
    FreeVec(dd->t_SampBuffer2);   dd->t_SampBuffer2 = NULL;
    FreeVec(dd->t_MixSoftInt);    dd->t_MixSoftInt  = NULL;
    FreeVec(dd->t_PlaySoftInt);   dd->t_PlaySoftInt = NULL;
  }

  if(Flags & AHISF_RECORD)
  {
    T_Stop(TSF_DONTSAVECACHE);
    dd->t_Flags &= ~TF_ISRECORDING;

// Disable Loopback
    T_SetPartTags(PAT_LoopbackVolume, -64, TAG_DONE);

    FreeVec(dd->t_RecBuffer);
    dd->t_RecBuffer = NULL;
  }
}



IPTR __asm __saveds intAHIsub_GetAttr(
    register __d0 ULONG Attribute,
    register __d1 LONG Argument,
    register __d2 IPTR Default,
    register __a1 struct TagItem *tagList,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl)
{
  if(ToccataBase == NULL)
  {
    return Default;
  }

  switch(Attribute)
  {
    case AHIDB_Bits:
      return 16;
    case AHIDB_Frequencies:
    {
      ULONG freq = NULL;
      LONG  freqs = 0;
      while(freq = T_NextFrequency(freq))
        freqs++;
      return freqs;
    }
    case AHIDB_Frequency: // Index->Frequency
    {
      ULONG freq = NULL;
      LONG  i;
      for(i = 0; i<=Argument ; i++)
        freq = T_NextFrequency(freq);
      return (LONG) freq;
    }
    case AHIDB_Index: // Frequency->Index
    {
      ULONG freq = NULL,realfreq = T_FindFrequency(Argument);
      LONG  index = 0;
      while((freq = T_NextFrequency(freq)) != realfreq)
        index++;
      return index;
    }
    case AHIDB_Author:
      return (IPTR) "Martin 'Leviticus' Blom";
    case AHIDB_Copyright:
      return (IPTR) "Public Domain";
    case AHIDB_Version:
      return (IPTR) _LibID;
    case AHIDB_Annotation:
      return (IPTR) "Based on code by Pauli Porkka, Peter Kunath and Frank Riffel.";
    case AHIDB_Record:
      return TRUE;
    case AHIDB_FullDuplex:
      return FALSE;
    case AHIDB_Realtime:
        return TRUE;
    case AHIDB_MaxPlaySamples:
      // FIXME: PLAYBUFFERSIZE should actually be converted to samples here.
      // However, it's no disaster, since AHIDB_MaxPlaySamples will just be
      // overy pessimistic.
      return Default+PLAYBUFFERSIZE;
    case AHIDB_MaxRecordSamples:
      return RECBUFFERSIZE>>2;
    case AHIDB_MinMonitorVolume:
      return 0x00000;
    case AHIDB_MaxMonitorVolume:
      if(ToccataBase->tb_HardInfo != NULL &&
	 ToccataBase->tb_HardInfo->hi_Flags & HIF_1845)
      {
        return 0x0000; // Workaround for bug in AD1845
      }
      else
      {
        return 0x10000;
      }
    case AHIDB_MinInputGain:
      return 0x10000;
    case AHIDB_MaxInputGain:
      return 0xd55d0;           // 13.335<<16 == +22.5 dB
    case AHIDB_MinOutputVolume:
      return 0x00000;
    case AHIDB_MaxOutputVolume:
      return 0x10000;
    case AHIDB_Inputs:
      return INPUTS;
    case AHIDB_Input:
      return (IPTR) Inputs[Argument];
    case AHIDB_Outputs:
      return 1;
    case AHIDB_Output:
      return (IPTR) "Line";     // We have only one output!
    default:
      return Default;
  }
}

const static LONG negboundaries[] =
{
  65536,55141,46395,39037,32845,27636,23253,19565,16461,13850,11654,9805,8250,
  6941,5840,4914,4135,3479,2927,2463,2072,1743,1467,1234,1038,873,735,618,520,
  438,368,310,260,219,184,155,130,110,92,77,65,55,46,39,32,27,23,19,16,13,11,9,
  8,6,5,4,4,3,2,2,2,1,1,1,0
};

LONG fixed2negdbvalue( LONG volume)
{
  LONG i = 0;

  while(volume < negboundaries[i])
    i++;
  return(-i);
}

const static LONG posboundaries[] =
{
  65536,77889,92572,110022,130761,155410,184705,219522,260903,
  310084,368536,438005,520570,618699,735326,873936
};

LONG fixed2posdbvalue( LONG volume)
{
  LONG i = 0;

  while((volume >= posboundaries[i+1]) && i<=14)
    i++;
  return(i);
}

LONG __asm __saveds __interrupt intAHIsub_HardwareControl(
    register __d0 ULONG attribute,
    register __d1 LONG argument,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  LONG rc = TRUE;

  if(ToccataBase && ToccataBase->tb_HardInfo)   // Check if hardware is present...
  {
    switch (attribute)
    {
      case AHIC_MonitorVolume:
        if((ToccataBase->tb_HardInfo->hi_Flags & HIF_1845) == 0)
        {
          dd->t_Loopback = argument;
          if(dd->t_Flags & TF_ISRECORDING)
          {
            T_SetPartTags(PAT_LoopbackVolume, fixed2negdbvalue(argument), TAG_DONE);
          }
        }
        break;
      case AHIC_MonitorVolume_Query:
        if(dd->t_Flags & TF_ISRECORDING)
        {
          T_GetPartTags(PAT_LoopbackVolume, &rc, TAG_DONE);
          rc = negboundaries[-rc];
        }
        else
        {
          rc = dd->t_Loopback;
        }
        break;
      case AHIC_InputGain:
        T_SetPartTags(PAT_InputVolumeLeft, fixed2posdbvalue(argument),
                      PAT_InputVolumeRight, fixed2posdbvalue(argument), TAG_DONE);
        break;
      case AHIC_InputGain_Query:
        T_GetPartTags(PAT_InputVolumeLeft, &rc, TAG_DONE);
        rc = negboundaries[rc];
        break;
      case AHIC_OutputVolume:
        T_SetPartTags(PAT_OutputVolumeLeft, fixed2negdbvalue(argument),
                      PAT_OutputVolumeRight, fixed2negdbvalue(argument), TAG_DONE);
        break;
      case AHIC_OutputVolume_Query:
        T_GetPartTags(PAT_OutputVolumeLeft, &rc, TAG_DONE);
        rc = negboundaries[-rc];
        break;
      case AHIC_Input:
        dd->t_Input = argument;
        T_SetPartTags(PAT_Input,   inputmap[argument],
                      PAT_MicGain, micgainmap[argument], TAG_DONE);
        break;
      case AHIC_Input_Query:
        rc = dd->t_Input;
        break;
      case AHIC_Output_Query:
        rc = 0;                           // There is only one output
        break;
      default:
        rc = FALSE;
        break;
    }
  }
  else
  {
    rc = FALSE;
  }
  return rc;
}

// SlaveProcessEntry() sets up a2 and a6. __saveds fixes a5
void __asm __saveds SlaveProcess(register __a2 struct AHIAudioCtrlDrv *AudioCtrl)
{
  struct Process *me = (struct Process *) FindTask(NULL);

  T_SaveSettings(0);                 // Save state

  T_Stop(TSF_DONTSAVECACHE);
  dd->t_Flags &= ~(TF_ISPLAYING | TF_ISRECORDING);

  T_SetPartTags(                     // Reset
      PAT_InputVolumeLeft,    0,
      PAT_InputVolumeRight,   0,
      PAT_OutputVolumeLeft,   0,
      PAT_OutputVolumeRight,  0,
      PAT_LoopbackVolume,     -64,
      PAT_Input,              TINPUT_Line,
//      PAT_Mode,               TMODE_LINEAR_16_S,
//      PAT_Frequency,          AudioCtrl->ahiac_MixFreq,
      TAG_DONE);

  dd->t_SlaveSignal  = AllocSignal(-1);
  dd->t_PlaySignal   = AllocSignal(-1);
  dd->t_RecordSignal = AllocSignal(-1);
  dd->t_MixSignal    = AllocSignal(-1);

  if( (dd->t_SlaveSignal  != -1) &&
      (dd->t_PlaySignal   != -1) &&
      (dd->t_RecordSignal != -1) &&
      (dd->t_MixSignal    != -1))
  {
    // Tell Master we're alive
    Signal(dd->t_MasterTask, 1L << dd->t_MasterSignal);

    for(;;)
    {
      ULONG signalset;
    
      signalset = Wait((1L << dd->t_SlaveSignal)
                     | (1L << dd->t_PlaySignal)
                     | (1L << dd->t_RecordSignal)
                     | (1L << dd->t_MixSignal));

      if(signalset & (1L << dd->t_SlaveSignal))
      {
        // Quit
        break;
      }

      if(signalset & (1L << dd->t_PlaySignal))
      {
        StartPlaying(AudioCtrl, me);
      }

      if(signalset & (1L << dd->t_RecordSignal))
      {
        StartRecording(AudioCtrl, me);
      }

      if(signalset & (1L << dd->t_MixSignal))
      {
        PreTimer_proto*  pretimer  = (PreTimer_proto*)  AudioCtrl->ahiac_PreTimer;
        PostTimer_proto* posttimer = (PostTimer_proto*) AudioCtrl->ahiac_PostTimer;
	BOOL pretimer_rc;

	pretimer_rc = pretimer(AudioCtrl);
	
        CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
	
        if(! pretimer_rc ) {
          CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->t_MixBuffer3);
        }
	
        posttimer(AudioCtrl);
      }
    }
  }

  T_Stop(TSF_DONTSAVECACHE);
  dd->t_Flags &= ~(TF_ISPLAYING | TF_ISRECORDING);

  T_LoadSettings(0);                 // Restore state

  Forbid();
  FreeSignal(dd->t_SlaveSignal);    dd->t_SlaveSignal   = -1;
  FreeSignal(dd->t_PlaySignal);     dd->t_PlaySignal    = -1;
  FreeSignal(dd->t_RecordSignal);   dd->t_RecordSignal  = -1;
  FreeSignal(dd->t_MixSignal);      dd->t_MixSignal     = -1;

  // Tell the Master we're dying
  dd->t_SlaveProcess = NULL;
  Signal((struct Task *)dd->t_MasterTask, 1L << dd->t_MasterSignal);

  // Multitaking will resume when we are dead.
}


BOOL StartPlaying(struct AHIAudioCtrlDrv *AudioCtrl, struct Process *me)
{
  BOOL playing;

  // Disable Loopback
  T_SetPartTags(PAT_LoopbackVolume, -64, TAG_DONE);

  playing = T_RawPlaybackTags(
      TT_ErrorTask,   me,
      TT_ErrorMask,   (1L << dd->t_PlaySignal),
      TT_RawInt,      dd->t_PlaySoftInt,
      TT_Mode,        dd->t_Mode,
      TT_Frequency,   AudioCtrl->ahiac_MixFreq,
      TT_RawBuffer1,  dd->t_SampBuffer1,
      TT_RawBuffer2,  dd->t_SampBuffer2,
      TT_BufferSize,  PLAYBUFFERSIZE,
      TT_RawIrqSize,  IrqSize,
      TAG_DONE);
  
  if(playing)
  {
    dd->t_Flags |= TF_ISPLAYING;
  }

  return playing;
}

BOOL StartRecording(struct AHIAudioCtrlDrv *AudioCtrl, struct Process *me)
{
  BOOL recording;

  if(ToccataBase->tb_HardInfo->hi_Flags & HIF_1845)
  {
    // Disable Loopback (workaround for bug in AD1845)
    T_SetPartTags(PAT_LoopbackVolume, -64, TAG_DONE);
  }
  else 
  {
    // Enable Loopback
    T_SetPartTags(PAT_LoopbackVolume, fixed2negdbvalue(dd->t_Loopback), TAG_DONE);
  }

  recording = T_CaptureTags(
      TT_ErrorTask,   me,
      TT_ErrorMask,   (1L << dd->t_RecordSignal),
      TT_Save,        RecordFunc,
      TT_CBParamA1,   AudioCtrl,
      TT_Mode,        TMODE_LINEAR_16_S,
      TT_Frequency,   AudioCtrl->ahiac_MixFreq,
      TT_BufferSize,  RECBUFFERSIZE,
      TT_Flags,       TTF_READYRETURN,
      TAG_DONE);

  if(recording)
  {
    dd->t_Flags |= TF_ISRECORDING;
  }

  return recording;
}
