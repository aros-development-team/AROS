
#include <config.h>

#include <exec/exec.h>
#include <devices/ahi.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <libraries/ahi_sub.h>
#include <libraries/asl.h>
#include <datatypes/datatypes.h>

#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/ahi_sub.h>
#include <stdlib.h>

#include "DriverData.h"
#include "FileFormats.h"
#include "library.h"

#define dd ((struct FilesaveData*) AudioCtrl->ahiac_DriverData)

void PlaySlaveEntry(void);
void RecSlaveEntry(void);

PROCGW( static, void,  playslaveentry, PlaySlaveEntry );
PROCGW( static, void,  recslaveentry,  RecSlaveEntry );

static const LONG frequency[] =
{
  5513,
  6615,
  8000,     // µ- and A-Law
  9600,     // DAT/5
  11025,    // CD/4
  12000,    // DAT/4
  14700,    // CD/3
  16000,    // DAT/3
  17640,    // CD/2.5
  18900,
  19200,    // DAT/2.5
  22050,    // CD/2
  24000,    // DAT/2
  27429,
  29400,    // CD/1.5
  32000,    // DAT/1.5
  33075,
  37800,
  44056,    // Some kind of video rate
  44100,    // CD
  48000,    // DAT
  88200,    // CD*2
  96000     // DAT*2
};

#define FREQUENCIES (sizeof frequency / sizeof frequency[ 0 ])


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG _AHIsub_AllocAudio(
    struct TagItem *tagList,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;
  char *ext = "";

  if(AslBase == NULL)
  {
    return AHISF_ERROR;
  }

  AudioCtrl->ahiac_DriverData = AllocVec(sizeof(struct FilesaveData),MEMF_CLEAR);
  
  if( dd != NULL )
  {
    dd->fs_AHIsubBase       = AHIsubBase;
    dd->fs_SlaveSignal      = -1;
    dd->fs_MasterSignal     = AllocSignal(-1);
    dd->fs_MasterTask       = (struct Process *) FindTask(NULL);
    dd->fs_RecSlaveSignal   = -1;
    dd->fs_RecMasterSignal  = AllocSignal(-1);
  }
  else
  {
    return AHISF_ERROR;
  }

  if((dd->fs_MasterSignal == -1) || (dd->fs_RecMasterSignal == -1))
  {
    return AHISF_ERROR;
  }

  dd->fs_Format = GetTagData(AHIDB_FileSaveFormat, FORMAT_8SVX, tagList);

  switch(dd->fs_Format)
  {
    case FORMAT_8SVX:
      ext = ".8SVX";
      break;

    case FORMAT_AIFF:
      ext = ".AIFF";
      break;

    case FORMAT_AIFC:
      ext = ".AIFC";
      break;

    case FORMAT_S16:
      break;

    case FORMAT_WAVE:
      ext = ".WAV";
      break;

    default:
      break;
  }

  {
    struct TagItem playtags[] =
    {
      { ASLFR_InitialFile,  (ULONG) ext     },
      { ASLFR_DoSaveMode,   TRUE            },
      { ASLFR_RejectIcons,  TRUE            },
      { ASLFR_TitleText,    (ULONG) LibName },
      { TAG_DONE,           0               }
    };
  
    struct TagItem rectags[] =
    {
      { ASLFR_RejectIcons,  TRUE                            },
      { ASLFR_TitleText,    (ULONG) "Select a sound sample" },
      { TAG_DONE,           0                               }
    };
    
    if(!(dd->fs_FileReq = AllocAslRequest(ASL_FileRequest, playtags)))
    {
      return AHISF_ERROR;
    }

    if(!(dd->fs_RecFileReq = AllocAslRequest(ASL_FileRequest, rectags)))
    {
      return AHISF_ERROR;
    }
  }

  return AHISF_KNOWHIFI|AHISF_KNOWSTEREO|AHISF_CANRECORD|AHISF_MIXING|AHISF_TIMING;
}


/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void _AHIsub_FreeAudio(
  struct AHIAudioCtrlDrv *AudioCtrl,
  struct DriverBase*      AHIsubBase )
  
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  if(AudioCtrl->ahiac_DriverData)
  {
    FreeAslRequest(dd->fs_FileReq);
    FreeAslRequest(dd->fs_RecFileReq);
    FreeSignal(dd->fs_MasterSignal);
    FreeSignal(dd->fs_RecMasterSignal);
    FreeVec(AudioCtrl->ahiac_DriverData);
    AudioCtrl->ahiac_DriverData = NULL;
  }
}

/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void
_AHIsub_Disable( struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Forbid();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Permit();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG _AHIsub_Start(
    ULONG Flags,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  AHIsub_Stop(Flags, AudioCtrl);

  if(Flags & AHISF_PLAY)
  {
    ULONG savebufferlength;

    if(!(dd->fs_MixBuffer = AllocVec(AudioCtrl->ahiac_BuffSize, MEMF_ANY)))
      return AHIE_NOMEM;

    dd->fs_SaveBufferSize = AudioCtrl->ahiac_MaxBuffSamples;

    // S16 has two buffers (L/R) instead
    if((AudioCtrl->ahiac_Flags & AHIACF_STEREO) && dd->fs_Format != FORMAT_S16)
    {
      dd->fs_SaveBufferSize <<=1;
    }

    if(dd->fs_SaveBufferSize < SAVEBUFFERSIZE)
    {
      dd->fs_SaveBufferSize = SAVEBUFFERSIZE;
    }

    savebufferlength = dd->fs_SaveBufferSize;


    switch(dd->fs_Format)
    {
      case FORMAT_8SVX:
        break;

      case FORMAT_AIFF:
        savebufferlength <<= 1;
        break;

      case FORMAT_AIFC:
        savebufferlength <<= 1;
        break;

      case FORMAT_S16:
        savebufferlength <<= 1;
        break;

      case FORMAT_WAVE:
        savebufferlength <<= 1;
        break;

      default:
        break;
    }


    if(!(dd->fs_SaveBuffer = AllocVec(savebufferlength, MEMF_ANY)))
    {
      return AHIE_NOMEM;
    }

    if ((AudioCtrl->ahiac_Flags & AHIACF_STEREO) && dd->fs_Format == FORMAT_S16)
    {
      if(!(dd->fs_SaveBuffer2 = AllocVec(savebufferlength, MEMF_ANY)))
      {
        return AHIE_NOMEM;
      }
    }

    if(AslRequest(dd->fs_FileReq,NULL))
    {
      struct TagItem proctags[] =
      {
	{ NP_Entry,     (IPTR) &playslaveentry },
	{ NP_Name,      (IPTR) LibName         },
	{ NP_Priority,  -1                      }, // It's a number cruncher...
	{ NP_StackSize, 10000,                  },
	{ TAG_DONE,     0                       }
      };

      Forbid();

      dd->fs_SlaveTask = CreateNewProc( proctags );
      
      if( dd->fs_SlaveTask != NULL )
      {
        dd->fs_SlaveTask->pr_Task.tc_UserData = AudioCtrl;
      }

      Permit();

      if(dd->fs_SlaveTask)
      {
        Wait(1L<<dd->fs_MasterSignal);  // Wait for slave to come alive
        if(dd->fs_SlaveTask == NULL)    // Is slave alive or dead?
        {
          return AHIE_UNKNOWN;
        }
      }
      else
      {
        return AHIE_NOMEM;
      }
    }
    else
    {
      if(IoErr())
      {
        return AHIE_NOMEM;    //error occured
      }
      else
      {
        return AHIE_ABORTED;  //requester cancelled
      }
    }
  }

  if(Flags & AHISF_RECORD)
  {
    if(!(dd->fs_RecBuffer = AllocVec(RECBUFFERSIZE*4,MEMF_ANY)))
    {
      return AHIE_NOMEM;
    }

    if(AslRequest(dd->fs_RecFileReq,NULL))
    {
      struct TagItem proctags[] =
      {
	{ NP_Entry,     (ULONG) &recslaveentry },
	{ NP_Name,      (ULONG) LibName        },
	{ NP_Priority,  1                      },  // Make it steady...
	{ TAG_DONE,     0                      }
      };

      Delay(TICKS_PER_SECOND);         // Wait for window to close etc...

      Forbid();

      dd->fs_RecSlaveTask = CreateNewProc( proctags );
	
      if( dd->fs_RecSlaveTask != NULL )
      {
        dd->fs_RecSlaveTask->pr_Task.tc_UserData = AudioCtrl;
      }

      Permit();

      if(dd->fs_RecSlaveTask)
      {
        Wait(1L<<dd->fs_RecMasterSignal);  // Wait for slave to come alive
        if(dd->fs_RecSlaveTask == NULL)    // Is slave alive or dead?
        {
          return AHIE_UNKNOWN;
        }
      }
      else
      {
        return AHIE_NOMEM;
      }
    }
    else
    {
      if(IoErr())
      {
        return AHIE_NOMEM;    //error occured
      }
      else
      {
        return AHIE_ABORTED;  //requester cancelled
      }
    }
  }

  return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void _AHIsub_Update(
    ULONG Flags,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void _AHIsub_Stop(
    ULONG Flags,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  if(Flags & AHISF_PLAY)
  {
    if(dd->fs_SlaveTask)
    {
      if(dd->fs_SlaveSignal != -1)
      {
        Signal((struct Task *)dd->fs_SlaveTask,1L<<dd->fs_SlaveSignal); // Kill him!
      }
      Wait(1L<<dd->fs_MasterSignal);  // Wait for slave to die
    }
    FreeVec(dd->fs_MixBuffer);
    dd->fs_MixBuffer = NULL;
    FreeVec(dd->fs_SaveBuffer);
    FreeVec(dd->fs_SaveBuffer2);
    dd->fs_SaveBuffer = NULL;
    dd->fs_SaveBuffer2 = NULL;
  }

  if(Flags & AHISF_RECORD)
  {
    if(dd->fs_RecSlaveTask)
    {
      if(dd->fs_RecSlaveSignal != -1)
      {
        Signal((struct Task *)dd->fs_RecSlaveTask,1L<<dd->fs_RecSlaveSignal); // Kill him!
      }
      Wait(1L<<dd->fs_RecMasterSignal);  // Wait for slave to die
    }
    FreeVec(dd->fs_RecBuffer);
    dd->fs_RecBuffer = NULL;
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

IPTR _AHIsub_GetAttr(
    ULONG Attribute,
    LONG Argument,
    IPTR Default,
    struct TagItem *tagList,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;
  size_t i;

  switch(Attribute)
  {
    case AHIDB_Bits:
      switch (GetTagData(AHIDB_FileSaveFormat,FORMAT_8SVX,tagList))
      {
        case FORMAT_8SVX:
          return 8;

        case FORMAT_AIFF:
          return 16;

        case FORMAT_AIFC:
          return 16;

        case FORMAT_S16:
          return 16;

        case FORMAT_WAVE:
          return 16;

        default:
          return Default;
      }

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) frequency[Argument];

    case AHIDB_Index: // Frequency->Index
      if(Argument <= frequency[0])
      {
        return 0;
      }
      if(Argument >= frequency[FREQUENCIES-1])
      {
        return FREQUENCIES-1;
      }
      for(i = 1;i<FREQUENCIES;i++)
      {
        if(frequency[i]>Argument)
        {
          if( (Argument-frequency[i-1]) < (frequency[i]-Argument) )
          {
            return i-1;
          }
          else
          {
            return i;
          }
        }
      }
      return 0;  // Will not happen

    case AHIDB_Author:
      return (IPTR) "Martin 'Leviticus' Blom";

    case AHIDB_Copyright:
      return (IPTR) "Public Domain";

    case AHIDB_Version:
      return (IPTR) LibIDString;

    case AHIDB_Record:
      return TRUE;

    case AHIDB_FullDuplex:
      return TRUE;

    case AHIDB_MaxRecordSamples:
      return RECBUFFERSIZE;

    case AHIDB_Realtime:
      return FALSE;

    case AHIDB_Inputs:
      return 1;

    case AHIDB_Input:
      return (IPTR) "File";    // We have only one input!

    case AHIDB_Outputs:
      return 1;

    case AHIDB_Output:
      return (IPTR) "File";    // We have only one output!

    default:
      return Default;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG _AHIsub_HardwareControl(
    ULONG attribute,
    LONG argument,
    struct AHIAudioCtrlDrv *AudioCtrl,
    struct DriverBase*      AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  return 0;
}
