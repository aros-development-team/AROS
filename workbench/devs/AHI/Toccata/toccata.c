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

#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <clib/toccata_protos.h>
#include <pragmas/toccata_pragmas.h>

#include <string.h>
#include <stdio.h>

#include "toccata.h"

#define ENVPREFS    "ENV:toccata-emul.prefs"
#define ENVARCPREFS "ENVARC:toccata-emul.prefs"
#define IDCODE      "TOCEMUL"


/* Globals */

#include "version.h"

struct Library        *IFFParseBase = NULL;
struct Library        *UtilityBase  = NULL;
struct Library        *AHIBase      = NULL;
struct ToccataBase    *ToccataBase  = NULL;
struct DosLibrary     *DOSBase      = NULL;

struct Process        *SlaveProcess = NULL;

struct toccataprefs tprefs = {
  IDCODE,

  /* Toccata registers */
  -24, -24,
  -24, -24,
  0, 0,
  0, 0,
  0,
  TMODE_LINEAR_8,
  11025,
  TINPUT_Line,
  FALSE,
  PATDEF_CAPTUREIOPRI,
  PATDEF_CAPTUREBUFFERPRI,
  PATDEF_CAPTUREBLOCKSIZE,
  PATDEF_MAXCAPTUREBLOCKS,
  PATDEF_PLAYBACKIOPRI,
  PATDEF_PLAYBACKBUFFERPRI,
  PATDEF_PLAYBACKBLOCKSIZE,
  PATDEF_PLAYBACKSTARTBLOCKS,
  PATDEF_PLAYBACKBLOCKS,

  /* AHI prefs */
  0x00100006,
  0x00100002,
  0, 0, 0, 0, 0
};

ULONG error = TIOERR_UNKNOWN;






/*
 *  UserLibInit(): Library init
 */

int ASM __UserLibInit (REG(a6) struct Library *libbase)
{
  ToccataBase = (struct ToccataBase *) libbase;

  ToccataBase->tb_BoardAddress = (APTR) 0xBADC0DED;

  if(!(IFFParseBase = OpenLibrary("iffparse.library",37)))
  {
    return 1;
  }

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

	puta4();
  return 0;
}

/*
 *  UserLibCleanup(): Library cleanup
 */

void ASM __UserLibCleanup (REG(a6) struct Library *libbase)
{

  if(IFFParseBase) {
    CloseLibrary(IFFParseBase);
    IFFParseBase = NULL;
  }

  if(DOSBase) {
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  if(UtilityBase) {
    CloseLibrary(UtilityBase);
    UtilityBase = NULL;
  }
}


/*
 *  UserLibOpen(): Called from OpenLibrary()
 */

int ASM __UserLibOpen (REG(a6) struct Library *libbase) {

  if(libbase->lib_OpenCnt == 1) {
    /* Was 0, became 1 */
    SlaveProcess = CreateNewProcTags(
        NP_Entry,     SlaveTaskEntry,
        NP_Name,      _LibName,
        NP_Priority,  3,
        TAG_DONE);

    /* Wait until our slave is ready */
    while(SlaveProcess && !SlaveInitialized) {
      Delay(1);
    }

//	DBG("Loading settings..\n");
//    if(SlaveProcess && SlaveInitialized) {
//      T_LoadSettings(0);
//    }
  }

  if(!SlaveInitialized) {
    return 1;
  }

  return 0;
}


/*
 *  UserLibClose(): Called from CloseLibrary()
 */

void ASM __UserLibClose (REG(a6) struct Library *libbase) {
  if(libbase->lib_OpenCnt == 1) {
    /* Is 1, will become 0 */
    if(SlaveProcess) {
      Signal((struct Task *) SlaveProcess, SIGBREAKF_CTRL_C);
    }
    while(SlaveProcess) {
      Delay(1);
    }
  }

}

/*
 *  hardinfo & fillhardinfo()
 */

static struct HardInfo hardinfo;

void fillhardinfo(void) {
  LONG  freqs    = 1;
  ULONG minfreq  = 0;
  ULONG maxfreq  = 0;
  ULONG id;
  struct AHIAudioCtrl *actrl = NULL;

  if(audioctrl) {
    id = AHI_INVALID_ID;
    actrl = audioctrl;
  }
  else {
    if(tprefs.Mode & TMODEF_STEREO) {
      id = tprefs.StereoMode;
    }
    else {
      id = tprefs.MonoMode;
    }
  }

  AHI_GetAudioAttrs(id, actrl,
      AHIDB_Frequencies, &freqs,
      AHIDB_MinMixFreq,  &minfreq,
      AHIDB_MaxMixFreq,  &maxfreq,
      TAG_DONE);

  hardinfo.hi_Version      = 1;
  hardinfo.hi_Revision     = 0;
  hardinfo.hi_Frequencies  = freqs;
  hardinfo.hi_MinFrequency = minfreq;
  hardinfo.hi_MaxFrequency = maxfreq;
  hardinfo.hi_Flags        = 0;

  ToccataBase->tb_HardInfo = &hardinfo;
}


/*
 *  sendmessage(): Send a message to the slave and wait for reply
 */

static ULONG sendmessage(ULONG id, APTR data) {
  struct MsgPort      *replyport = NULL;
  struct slavemessage *msg = NULL;
  ULONG rc = 0;

  if(msg = AllocVec(sizeof(struct slavemessage), MEMF_PUBLIC | MEMF_CLEAR)) {
    if(replyport = CreateMsgPort()) {
      msg->Msg.mn_ReplyPort = replyport;
      msg->Msg.mn_Length    = sizeof(struct slavemessage);
      msg->ID               = id;
      msg->Data             = data;

      PutMsg(&SlaveProcess->pr_MsgPort, (struct Message *) msg);

      WaitPort(replyport);
      GetMsg(replyport);
      rc = (ULONG) msg->Data;
      DeleteMsgPort(replyport);
      FreeVec(msg);
    }
  }

  return rc;
}


/*
 *  IoErr()
 */

ASM ULONG t_IoErr(void) {
  return error;
}


/*
 *  RawPlayback()
 */

ASM BOOL t_RawPlayback(REG(a0) struct TagItem *tags) {
  return (BOOL) sendmessage(MSG_RAWPLAY, tags);
}


/*
 *  SaveSettings()
 */

static BOOL savesettings(STRPTR name) {
  BOOL rc = FALSE;
  BPTR file;

  file = Open(name, MODE_NEWFILE);
  if(file) {
    if(Write(file, &tprefs, sizeof tprefs) == sizeof tprefs) {
      rc = TRUE;
    }
    Close(file);
  }
  return rc;
}

ASM BOOL t_SaveSettings(REG(d0) ULONG flags) {
  BOOL rc = TRUE;

  if(flags == 1) {
    rc = savesettings(ENVARCPREFS);
  }

  if(rc) {
    rc = savesettings(ENVPREFS);
  }

  return rc;
}


/*
 *  LoadSettings()
 */

static BOOL loadsettings(STRPTR name) {
  BOOL rc = FALSE;
  BPTR file;
  struct toccataprefs tempprefs;

  file = Open(name, MODE_OLDFILE);
  if(file) {
    if(Read(file, &tempprefs, sizeof tempprefs) == sizeof tempprefs) {
      if(strcmp(tempprefs.ID, IDCODE) == 0) {
        memcpy(&tprefs, &tempprefs, sizeof tempprefs);
        rc = TRUE;
      }
    }
    Close(file);
  }
  return rc;
}

ASM BOOL t_LoadSettings(REG(d0) ULONG flags) {
  BOOL rc = FALSE;

  if(flags == 1) {
    rc = loadsettings(ENVARCPREFS);
  }
  else if(flags == 0) {
    rc = loadsettings(ENVPREFS);
  }

  T_SetPartTags(
      PAT_InputVolumeLeft,  tprefs.InputVolumeLeft,
      PAT_InputVolumeRight, tprefs.InputVolumeLeft,
      PAT_OutputVolumeLeft, tprefs.OutputVolumeLeft,
      PAT_OutputVolumeRight,tprefs.OutputVolumeRight,
      PAT_LoopbackVolume,   tprefs.LoopbackVolume,
      PAT_Input,            tprefs.Input,
      PAT_MicGain,          tprefs.MicGain,
      PAT_Mode,             tprefs.Mode,
      PAT_Frequency,        tprefs.Frequency,
      TAG_DONE);

  return rc;
}


/*
 *  Expand()
 */

ASM WORD t_Expand(REG(d0) UBYTE value, REG(d1) ULONG mode) {
  return 0;
}


/*
 *  StartLevel()
 */

ASM BOOL t_StartLevel(REG(a0) struct TagItem *tags) {
  return (BOOL) sendmessage(MSG_LEVELON, tags);
}


/*
 *  Capture()
 */

ASM BOOL t_Capture(REG(a0) struct TagItem *tags) {
  return (BOOL) sendmessage(MSG_RECORD, tags);
}


/*
 *  Playback()
 */

ASM BOOL t_Playback(REG(a0) struct TagItem *tags) {
  return (BOOL) sendmessage(MSG_PLAY, tags);
}


/*
 *  Pause()
 */

ASM void t_Pause(REG(d0) ULONG pause) {
  ULONG p = pause;
  sendmessage(MSG_PAUSE, &p);
}


/*
 *  Stop()
 */

ASM void t_Stop(REG(d0) ULONG flags) {
  ULONG f = flags;
  sendmessage(MSG_STOP, &f);
}


/*
 *  StopLevel()
 */

ASM void t_StopLevel(void) {
  sendmessage(MSG_LEVELOFF, NULL);
}


/*
 *  FindFrequency()
 */

ASM ULONG t_FindFrequency(REG(d0) ULONG frequency) {
  ULONG index = 0;
  ULONG freq  = 0;
  ULONG id;
  struct AHIAudioCtrl *actrl = NULL;

  if(audioctrl) {
    id = AHI_INVALID_ID;
    actrl = audioctrl;
  }
  else {
    if(tprefs.Mode & TMODEF_STEREO) {
      id = tprefs.StereoMode;
    }
    else {
      id = tprefs.MonoMode;
    
    }
  }
  
  AHI_GetAudioAttrs(id, actrl,
      AHIDB_IndexArg, frequency,
      AHIDB_Index,    &index,
      TAG_DONE);

  AHI_GetAudioAttrs(id, actrl,
      AHIDB_FrequencyArg, index,
      AHIDB_Frequency,    &freq,
      TAG_DONE);

  return freq;
}


/*
 *  NextFrequency()
 */

ASM ULONG t_NextFrequency(REG(d0) ULONG frequency) {
  LONG  frequencies = 1;
  ULONG index = 0;
  ULONG freq = 0;
  ULONG id;
  struct AHIAudioCtrl *actrl = NULL;

  if(audioctrl) {
    id = AHI_INVALID_ID;
    actrl = audioctrl;
  }
  else {
    if(tprefs.Mode & TMODEF_STEREO) {
      id = tprefs.StereoMode;
    }
    else {
      id = tprefs.MonoMode;
    }
  }
  
  if(frequency < ToccataBase->tb_HardInfo->hi_MinFrequency) {
    index = 0;  
  }
  else {
    AHI_GetAudioAttrs(id, actrl,
        AHIDB_IndexArg, frequency,
        AHIDB_Index,    &index,
        AHIDB_Frequencies, &frequencies,
        TAG_DONE);

    if(index < (frequencies - 1 )) {
      index++;
    }
    else {
      return 0;
    }
  }

  AHI_GetAudioAttrs(id, actrl,
      AHIDB_FrequencyArg, index,
      AHIDB_Frequency,    &freq,
      TAG_DONE);

  return freq;
}


/*
 *  SetPart()
 */

ASM void t_SetPart(REG(a0) struct TagItem *tags) {
  struct TagItem *tstate;
  struct TagItem *tag;

  BOOL newmode   = FALSE;
  BOOL newhwprop = FALSE;

  tstate = tags;

  while (tag = NextTagItem(&tstate)) {
    switch (tag->ti_Tag) {
      case PAT_MixAux1Left:
        tprefs.MixAux1Left = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_MixAux1Right:
        tprefs.MixAux1Right = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_MixAux2Left:
        tprefs.MixAux2Left = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_MixAux2Right:
        tprefs.MixAux2Right = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_InputVolumeLeft:
        tprefs.InputVolumeLeft = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_InputVolumeRight:
        tprefs.InputVolumeRight = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_OutputVolumeLeft:
        tprefs.OutputVolumeLeft = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_OutputVolumeRight:
        tprefs.OutputVolumeRight = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_LoopbackVolume:
        tprefs.LoopbackVolume = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_Mode:
        tprefs.Mode = tag->ti_Data;
        newmode = TRUE;
        break;
      case PAT_Frequency:
        tprefs.Frequency = tag->ti_Data;
        newmode = TRUE;
        break;
      case PAT_Input:
        tprefs.Input = tag->ti_Data;
        newhwprop = TRUE;
        break;
      case PAT_MicGain:
        tprefs.MicGain = tag->ti_Data;
        newhwprop = TRUE;
        break;

      /* These are unsupported */
      case PAT_CaptureIoPri:
        tprefs.CaptureIoPri = tag->ti_Data;
        break;
      case PAT_CaptureBufferPri:
        tprefs.CaptureBufferPri = tag->ti_Data;
        break;
      case PAT_CaptureBlockSize:
        tprefs.CaptureBlockSize = tag->ti_Data;
        break;
      case PAT_MaxCaptureBlocks:
        tprefs.MaxCaptureBlocks = tag->ti_Data;
        break;
      case PAT_PlaybackIoPri:
        tprefs.PlaybackIoPri = tag->ti_Data;
        break;
      case PAT_PlaybackBufferPri:
        tprefs.PlaybackBufferPri = tag->ti_Data;
        break;
      case PAT_PlaybackBlockSize:
        tprefs.PlaybackBlockSize = tag->ti_Data;
        break;
      case PAT_PlaybackStartBlocks:
        tprefs.PlaybackStartBlocks = tag->ti_Data;
        break;
      case PAT_PlaybackBlocks:
        tprefs.PlaybackBlocks = tag->ti_Data;
        break;
    }
  }

  if(newmode) {
    sendmessage(MSG_MODE, NULL);
  }
  else if(newhwprop) {
    sendmessage(MSG_HWPROP, NULL);
  }
}


/*
 *  GetPart()
 */

ASM void t_GetPart(REG(a0) struct TagItem *tags) {
  struct TagItem *tstate;
  struct TagItem *tag;

  tstate = tags;


  while (tag = NextTagItem(&tstate)) {
    ULONG *dst;
    
    dst = (ULONG *) tag->ti_Data;

    switch (tag->ti_Tag) {
      case PAT_MixAux1Left:
        *dst = tprefs.MixAux1Left;
        break;
      case PAT_MixAux1Right:
        *dst = tprefs.MixAux1Right;
        break;
      case PAT_MixAux2Left:
        *dst = tprefs.MixAux2Left;
        break;
      case PAT_MixAux2Right:
        *dst = tprefs.MixAux2Right;
        break;
      case PAT_InputVolumeLeft:
        *dst = tprefs.InputVolumeLeft;
        break;
      case PAT_InputVolumeRight:
        *dst = tprefs.InputVolumeRight;
        break;
      case PAT_OutputVolumeLeft:
        *dst = tprefs.OutputVolumeLeft;
        break;
      case PAT_OutputVolumeRight:
        *dst = tprefs.OutputVolumeRight;
        break;
      case PAT_LoopbackVolume:
        *dst = tprefs.LoopbackVolume;
        break;
      case PAT_Mode:
        *dst = tprefs.Mode;
        break;
      case PAT_Frequency:
        *dst = tprefs.Frequency;
        break;
      case PAT_Input:
        *dst = tprefs.Input;
        break;
      case PAT_MicGain:
        *dst = tprefs.MicGain;
        break;
      case PAT_CaptureIoPri:
        *dst = tprefs.CaptureIoPri;
        break;
      case PAT_CaptureBufferPri:
        *dst = tprefs.CaptureBufferPri;
        break;
      case PAT_CaptureBlockSize:
        *dst = tprefs.CaptureBlockSize;
        break;
      case PAT_MaxCaptureBlocks:
        *dst = tprefs.MaxCaptureBlocks;
        break;
      case PAT_PlaybackIoPri:
        *dst = tprefs.PlaybackIoPri;
        break;
      case PAT_PlaybackBufferPri:
        *dst = tprefs.PlaybackBufferPri;
        break;
      case PAT_PlaybackBlockSize:
        *dst = tprefs.PlaybackBlockSize;
        break;
      case PAT_PlaybackStartBlocks:
        *dst = tprefs.PlaybackStartBlocks;
        break;
      case PAT_PlaybackBlocks:
        *dst = tprefs.PlaybackBlocks;
        break;
    }
  }
}

/*
 *  Open()
 */

ASM struct TocHandle * t_Open(REG(a0) UBYTE *name, REG(a1) struct TagItem *tags) {
  return NULL;
}


/*
 *  Close()
 */

ASM void t_Close(REG(a0) struct TocHandle *handle) {
}


/*
 *  Play()
 */

ASM BOOL t_Play(REG(a0) struct TocHandle *handle, REG(a1) struct TagItem *tags) {
  return FALSE;
}


/*
 *  Record()
 */

ASM BOOL t_Record(REG(a0) struct TocHandle *handle, REG(a1) struct TagItem *tags) {
  return FALSE;
}

/*
 *  Convert()
 */

ASM void t_Convert(REG(a0) APTR src, REG(a1) APTR dest, REG(d0) ULONG samples,
                 REG(d1) ULONG srcmode, REG(d2) ULONG destmode) {
}


/*
 *  BytesPerSample()
 */

ASM ULONG t_BytesPerSample(REG(d0) ULONG mode) {
  const static ULONG table[] = {
    1,    // TMODE_LINEAR_8
    2,    // TMODE_LINEAR_16
    1,    // TMODE_ALAW
    1,    // TMODE_ULAW
    2,    // TMODE_RAW_16
    0,
    0,
    0,
    2,    // TMODE_LINEAR_8_S
    4,    // TMODE_LINEAR_16_S
    2,    // TMODE_ALAW_S
    2,    // TMODE_ULAW_S
    4,    // TMODE_RAW_16_S
    0,
    0,
    0
  };

  return table[mode];
}




/* No documentation available for the following functions */


/*
 *  OpenFile()
 */

ASM struct MultiSoundHandle * t_OpenFile(REG(a0) UBYTE *name, REG(d0) ULONG flags) {
  return NULL;
}


/*
 *  CloseFile()
 */

ASM void t_CloseFile(REG(a0) struct MultiSoundHandle *handle) {
}


/*
 *  ReadFile()
 */

ASM LONG t_ReadFile(REG(a0) struct MultiSoundHandle *handle,
                    REG(a1) UBYTE *dest, REG(d0) ULONG length) {
  return 0;
}





/* No prototypes available for the following functions... */


/*
 *  WriteSmpte()
 */

ASM ULONG t_WriteSmpte(void) {
  return 0;
}


/*
 *  StopSmpte()
 */

ASM ULONG t_StopSmpte(void) {
  return 0;
}


/*
 *  Reserved1()
 */

ASM ULONG t_Reserved1(void) {
  return 0;
}


/*
 *  Reserved2()
 */

ASM ULONG t_Reserved2(void) {
  return 0;
}


/*
 *  Reserved3()
 */

ASM ULONG t_Reserved3(void) {
  return 0;
}


/*
 *  Reserved4()
 */

ASM ULONG t_Reserved4(void) {
  return 0;
}


/*
 *  Own()
 */

ASM void t_Own(void) {
}


/*
 *  Disown()
 */

ASM void t_Disown(void) {
}


/*
 *  SetReg()
 */

ASM void t_SetReg(void) {
}


/*
 *  GetReg()
 */

ASM ULONG t_GetReg(void) {
  return 0;
}
