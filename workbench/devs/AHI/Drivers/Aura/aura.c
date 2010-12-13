/*
** Because of the massive number of interrupts the Aura "soundcard" needs,
** this driver does some *very* ugly things:
**  1) It requires both CIA-B timers.
**  2) It takes complete control over the level 6 interrupt. Make sure
**     nobody else is using it!
**
**  We use Timer A of CIA-B.
**
*/


#include <exec/exec.h>
#include <dos/dos.h>
#include <exec/interrupts.h>
#include <hardware/cia.h>
#include <resources/cia.h>

#include <proto/cia.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/ahi_sub.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "aura.h"

#define EQ ==
#define dd ((struct aura *) AudioCtrl->ahiac_DriverData)

#define RECBUFFERSIZE  2048   // in samples

#define STARTA_OR  CIACRAF_START
#define STOPA_AND  CIACRAF_TODIN | CIACRAF_PBON | CIACRAF_OUTMODE | CIACRAF_SPMODE
#define STOPB_AND  CIACRBF_ALARM | CIACRBF_PBON | CIACRBF_OUTMODE

extern void KPrintF(char *fmt,...);

extern char __far _LibID[];
extern char __far _LibName[];

extern void __asm InstallUglyInterruptHack(register __a1 void (* )(), register __a2 struct aura *);
extern void __asm UninstallUglyInterruptHack(register __a3 struct aura *);
extern void __asm DummyFunc(void);
extern void __asm DummyInt(void);
extern void __asm PlayIntMono(void);
extern void __asm RecordIntMono(void);
extern void __asm PlayAndRecordIntMono(void);
extern void __asm PlayIntStereo(void);
extern void __asm RecordIntStereo(void);
extern void __asm PlayAndRecordIntStereo(void);
extern void __asm SoftFunc(void);

struct ExecBase *SysBase;

const static void *IntCodeTable[] =
{
  DummyInt,
  PlayIntMono,
  RecordIntMono,
  PlayAndRecordIntMono,

  DummyInt,
  PlayIntStereo,
  RecordIntStereo,
  PlayAndRecordIntStereo

};

#define FREQUENCIES 23
const static ULONG frequency[FREQUENCIES] =
{
  4410,         // CD/10
  4800,         // DAT/10
  5513,         // CD/8
  6000,         // DAT/8
  7350,         // CD/6
  8000,         // µ- and A-Law, DAT/6
  9600,         // DAT/5
	11025,        // CD/4
  12000,        // DAT/4
	14700,        // CD/3
  16000,        // DAT/3
	17640,        // CD/2.5
  18900,
	19200,        // DAT/2.5
  22050,        // CD/2
	24000,        // DAT/2
  27429,
	29400,        // CD/1.5
  32000,        // DAT/1.5
	33075,
  37800,
	44100,        // CD
  48000         // DAT
};

struct Library        *UtilityBase = NULL;
struct Library        *AHIsubBase = NULL;
struct CIA            *ciab = (struct CIA *)0xbfd000;
struct CIABase        *ciabbase = NULL;

BOOL InUse = FALSE;

int  __saveds __asm __UserLibInit (register __a6 struct Library *libbase)
{
  SysBase = *(struct ExecBase **)4;
  AHIsubBase=libbase;

  if( ! (UtilityBase=OpenLibrary("utility.library",37)))
  {
    Alert(AN_Unknown|AG_OpenLib|AO_UtilityLib);
    return 1;
  }

  if( ! (ciabbase = OpenResource(CIABNAME)))
  {
    return 1;
  }

  return 0;
}

void __saveds __asm __UserLibCleanup (register __a6 struct Library *libbase)
{
  if(UtilityBase)   { CloseLibrary(UtilityBase); UtilityBase=NULL; }
}

ULONG __asm __saveds intAHIsub_AllocAudio(
    register __a1 struct TagItem *tagList,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{

  // Only one user at the time!

  Forbid();
  if(InUse)
  {
    Permit();
    return AHISF_ERROR;
  }
  InUse = TRUE;
  Permit();


  if(AudioCtrl->ahiac_DriverData=AllocVec(sizeof(struct aura),MEMF_PUBLIC|MEMF_CLEAR))
  {

    dd->a_CIAperiod=SysBase->ex_EClockFrequency*2/AudioCtrl->ahiac_MixFreq;
    AudioCtrl->ahiac_MixFreq=SysBase->ex_EClockFrequency/dd->a_CIAperiod/2;

    if(dd->a_AuraInt=AllocVec(sizeof(struct Interrupt),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR))
    {
      dd->a_AuraInt->is_Node.ln_Type=NT_INTERRUPT;
      dd->a_AuraInt->is_Node.ln_Name=_LibName;
      dd->a_AuraInt->is_Data=AudioCtrl;
      // It will never be called anyway...
      dd->a_AuraInt->is_Code=(void (* )()) DummyFunc;

      // Allocate Timer A
      if( ! (AddICRVector((struct Library *) ciabbase,CIAICRB_TA,dd->a_AuraInt)))
      {
        dd->a_GotTimerA = TRUE;

        // Allocate Timer B
        if ( ! (AddICRVector((struct Library *)ciabbase,CIAICRB_TB,dd->a_AuraInt)))
        {
          dd->a_GotTimerB = TRUE;

          // Stop both timers, set 02 pulse count-down mode, set continuous mode
          Disable();
          ciab->ciacra &= STOPA_AND;
          ciab->ciacrb &= STOPB_AND;
          Enable();

          InstallUglyInterruptHack(DummyInt, (struct aura *)AudioCtrl->ahiac_DriverData);

          if(dd->a_SoftInt = AllocVec(
              sizeof(struct Interrupt),MEMF_PUBLIC|MEMF_ANY|MEMF_CLEAR))
          {
            dd->a_SoftInt->is_Node.ln_Type=NT_INTERRUPT;
            dd->a_SoftInt->is_Node.ln_Name=_LibName;
            dd->a_SoftInt->is_Data=AudioCtrl;
            dd->a_SoftInt->is_Code=(void (* )()) SoftFunc;

            if(TRUE)         // FIXIT: Check if hardware is present...
            {
              return AHISF_KNOWSTEREO|AHISF_CANRECORD|AHISF_MIXING|AHISF_TIMING;
            }
          }
        }
      }
    }
  }
  return AHISF_ERROR;
}

void __asm __saveds intAHIsub_FreeAudio(
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  if(AudioCtrl->ahiac_DriverData)
  {
    if(dd->a_GotTimerA && dd->a_GotTimerB)
    {
      UninstallUglyInterruptHack((struct aura *)AudioCtrl->ahiac_DriverData);

      if(dd->a_GotTimerA)
      {
        RemICRVector((struct Library *)ciabbase,CIAICRB_TA,dd->a_AuraInt);
      }
      if(dd->a_GotTimerB)
      {
        RemICRVector((struct Library *)ciabbase,CIAICRB_TB,dd->a_AuraInt);
      }
    }
    FreeVec(dd->a_SoftInt);
    FreeVec(dd->a_AuraInt);
    FreeVec(AudioCtrl->ahiac_DriverData);
    AudioCtrl->ahiac_DriverData=NULL;
  }

  InUse = FALSE;
}



ULONG __asm __saveds intAHIsub_Start(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{

  if(Flags & AHISF_PLAY)
  {
    AHIsub_Stop(AHISF_PLAY,AudioCtrl);

    if(!(dd->a_MixBuffer1=AllocVec(AudioCtrl->ahiac_BuffSize,MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;

    if(!(dd->a_MixBuffer2=AllocVec(AudioCtrl->ahiac_BuffSize,MEMF_PUBLIC|MEMF_CLEAR|MEMF_ANY)))
      return AHIE_NOMEM;

    dd->a_Status |= STATUS_PLAY;

    switch(AudioCtrl->ahiac_BuffType)
    {
      case AHIST_M16S:
        dd->a_Status &= ~STATUS_STEREO;
        break;
      case AHIST_S16S:
        dd->a_Status |= STATUS_STEREO;
        break;
      default:
        return AHIE_BADSAMPLETYPE;
    }
    InstallUglyInterruptHack((void (* )())IntCodeTable[dd->a_Status],
        (struct aura *)AudioCtrl->ahiac_DriverData);

/*
    KPrintF("DummyInt: 0x%08lx\n",DummyInt);
    KPrintF("PlayIntMono: 0x%08lx\n",PlayIntMono);
    KPrintF("RecordIntMono: 0x%08lx\n",RecordIntMono);
    KPrintF("PlayAndRecordIntMono: 0x%08lx\n",PlayAndRecordIntMono);

    KPrintF("DummyInt: 0x%08lx\n",DummyInt);
    KPrintF("PlayIntStereo: 0x%08lx\n",PlayIntStereo);
    KPrintF("RecordIntStereo: 0x%08lx\n",RecordIntStereo);
    KPrintF("PlayAndRecordIntStereo: 0x%08lx\n",PlayAndRecordIntStereo);

    KPrintF("Code: 0x%08lx\n",IntCodeTable[dd->a_Status]);
*/
// Start the interval timer - we will start the counter after
// writing the low, and high byte counter values

    ciab->ciatalo = dd->a_CIAperiod & 0xff;
    ciab->ciatahi = dd->a_CIAperiod>>8;

// Turn on start bit

    Disable();
    ciab->ciacra |= STARTA_OR;
    Enable();
  }

  if(Flags & AHISF_RECORD)
  {
    if(!(dd->a_RecBuffer1=AllocVec(RECBUFFERSIZE<<2,MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->a_RecBuffer2=AllocVec(RECBUFFERSIZE<<2,MEMF_ANY)))
      return AHIE_NOMEM;
    if(!(dd->a_RecMessage=AllocVec(sizeof(struct AHIRecordMessage),MEMF_ANY|MEMF_CLEAR)))
      return AHIE_NOMEM;

    dd->a_RecMessage->ahirm_Type=AHIST_S16S;

    dd->a_Status |= STATUS_RECORD;
    InstallUglyInterruptHack((void (* )())IntCodeTable[dd->a_Status],
        (struct aura *)AudioCtrl->ahiac_DriverData);

// Start the interval timer - we will start the counter after
// writing the low, and high byte counter values

    ciab->ciatalo = dd->a_CIAperiod & 0xff;
    ciab->ciatahi = dd->a_CIAperiod>>8;

// Turn on start bit

    Disable();
    ciab->ciacra |= STARTA_OR;
    Enable();
  }

  return AHIE_OK;
}

void __asm __saveds __interrupt intAHIsub_Update(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
}


void __asm __saveds intAHIsub_Stop(
    register __d0 ULONG Flags,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  if(Flags & AHISF_PLAY)
  {
    dd->a_Status &= ~STATUS_PLAY;
    InstallUglyInterruptHack((void (* )())IntCodeTable[dd->a_Status],
        (struct aura *)AudioCtrl->ahiac_DriverData);

    FreeVec(dd->a_MixBuffer1);
    dd->a_MixBuffer1=NULL;
    FreeVec(dd->a_MixBuffer2);
    dd->a_MixBuffer2=NULL;
  }

  if(Flags & AHISF_RECORD)
  {
    dd->a_Status &= ~STATUS_RECORD;
    InstallUglyInterruptHack((void (* )())IntCodeTable[dd->a_Status],
        (struct aura *)AudioCtrl->ahiac_DriverData);

    FreeVec(dd->a_RecBuffer1);
    dd->a_RecBuffer1=NULL;
    FreeVec(dd->a_RecBuffer2);
    dd->a_RecBuffer2=NULL;
    FreeVec(dd->a_RecMessage);
    dd->a_RecMessage=NULL;
  }

  if(!(dd->a_Status & (STATUS_PLAY | STATUS_RECORD)))
  {
    // Stop timer, set 02 pulse count-down mode, set continuous mode
    Disable();
    ciab->ciacra &= STOPA_AND;
    Enable();
  }
}



IPTR __asm __saveds intAHIsub_GetAttr(
    register __d0 ULONG Attribute,
    register __d1 LONG Argument,
    register __d2 IPTR Default,
    register __a1 struct TagItem *tagList,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl)
{
  switch(Attribute)
  {
    case AHIDB_Bits:
      return 12;
    case AHIDB_Frequencies:
      return FREQUENCIES;
    case AHIDB_Frequency: // Index->Frequency
      return (LONG) frequency[Argument];
    case AHIDB_Index: // Frequency->Index
    {
      int i;
      
      if(Argument<=frequency[0])
        return 0;
      if(Argument>=frequency[FREQUENCIES-1])
        return FREQUENCIES-1;
      for(i=1;i<FREQUENCIES;i++)
        if(frequency[i]>Argument)
        {
          if( (Argument-frequency[i-1]) < (frequency[i]-Argument) )
            return i-1;
          else
            return i;
        }
      return 0;  // Will not happen
    }
    case AHIDB_Author:
      return (IPTR) "Martin 'Leviticus' Blom";
    case AHIDB_Copyright:
      return (IPTR) "Public Domain";
    case AHIDB_Version:
      return (IPTR) _LibID;
    case AHIDB_Annotation:
      return (IPTR) "This driver plays some very nasty tricks with the\n"
                    "Level 6 interrupt, and will not work unless it can\n"
                    "get full control over both timer A and B.";
      break;
    case AHIDB_Record:
      return TRUE;
    case AHIDB_FullDuplex:
      return TRUE;
    case AHIDB_Realtime:
      return TRUE;
    case AHIDB_MaxRecordSamples:
      return RECBUFFERSIZE;
    case AHIDB_MinMonitorVolume:
    case AHIDB_MaxMonitorVolume:
      return 0x00000;
    case AHIDB_MinInputGain:
    case AHIDB_MaxInputGain:
    case AHIDB_MinOutputVolume:
    case AHIDB_MaxOutputVolume:
      return 0x10000;
    case AHIDB_Inputs:
    case AHIDB_Outputs:
      return 1;
    case AHIDB_Input:
    case AHIDB_Output:
      return (IPTR) "Line";
    default:
      return Default;
  }
}

LONG __asm __saveds __interrupt intAHIsub_HardwareControl(
    register __d0 ULONG attribute,
    register __d1 LONG argument,
    register __a2 struct AHIAudioCtrlDrv *AudioCtrl )
{
  switch (attribute)
  {
    case AHIC_MonitorVolume_Query:
      return 0;
    case AHIC_InputGain_Query:
    case AHIC_OutputVolume_Query:
      return 0x10000;
    case AHIC_Input_Query:
      return 0;                       // There is only one input
    case AHIC_Output_Query:
      return 0;                       // There is only one output
  }
  return FALSE;
}
