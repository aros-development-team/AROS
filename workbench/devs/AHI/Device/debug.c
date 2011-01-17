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

#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "ahi_def.h"
#include "debug.h"

/******************************************************************************
** Support code ***************************************************************
******************************************************************************/

static const char*
GetTagName( Tag tag )
{
  switch( tag )
  {
    case AHIA_AudioID: return "AHIA_AudioID";
    case AHIA_MixFreq: return "AHIA_MixFreq";
    case AHIA_Channels: return "AHIA_Channels";
    case AHIA_Sounds: return "AHIA_Sounds";
    case AHIA_SoundFunc: return "AHIA_SoundFunc";
    case AHIA_PlayerFunc: return "AHIA_PlayerFunc";
    case AHIA_PlayerFreq: return "AHIA_PlayerFreq";
    case AHIA_MinPlayerFreq: return "AHIA_MinPlayerFreq";
    case AHIA_MaxPlayerFreq: return "AHIA_MaxPlayerFreq";
//    case AHIA_PlayerFreqUnit: return "AHIA_PlayerFreqUnit";
    case AHIA_RecordFunc: return "AHIA_RecordFunc";
    case AHIA_UserData: return "AHIA_UserData";
//    case AHIA_ErrorFunc: return "AHIA_ErrorFunc";
    case AHIA_AntiClickSamples: return "AHIA_AntiClickSamples";
    case AHIP_BeginChannel: return "AHIP_BeginChannel";
    case AHIP_EndChannel: return "AHIP_EndChannel";
    case AHIP_Freq: return "AHIP_Freq";
    case AHIP_Vol: return "AHIP_Vol";
    case AHIP_Pan: return "AHIP_Pan";
    case AHIP_Sound: return "AHIP_Sound";
    case AHIP_Offset: return "AHIP_Offset";
    case AHIP_Length: return "AHIP_Length";
    case AHIP_LoopFreq: return "AHIP_LoopFreq";
    case AHIP_LoopVol: return "AHIP_LoopVol";
    case AHIP_LoopPan: return "AHIP_LoopPan";
    case AHIP_LoopSound: return "AHIP_LoopSound";
    case AHIP_LoopOffset: return "AHIP_LoopOffset";
    case AHIP_LoopLength: return "AHIP_LoopLength";
    case AHIC_Play: return "AHIC_Play";
    case AHIC_Record: return "AHIC_Record";
//    case AHIC_PausePlay: return "AHIC_PausePlay";
//    case AHIC_PauseRecord: return "AHIC_PauseRecord";
    case AHIC_MixFreq_Query: return "AHIC_MixFreq_Query";
    case AHIC_Input: return "AHIC_Input";
    case AHIC_Input_Query: return "AHIC_Input_Query";
    case AHIC_Output: return "AHIC_Output";
    case AHIC_Output_Query: return "AHIC_Output_Query";
//    case AHIC_MonitorVolumeLeft: return "AHIC_MonitorVolumeLeft";
//    case AHIC_MonitorVolumeLeft_Query: return "AHIC_MonitorVolumeLeft_Query";
//    case AHIC_MonitorVolumeRight: return "AHIC_MonitorVolumeRight";
//    case AHIC_MonitorVolumeRight_Query: return "AHIC_MonitorVolumeRight_Query";
//    case AHIC_OutputVolumeLeft: return "AHIC_OutputVolumeLeft";
//    case AHIC_OutputVolumeLeft_Query: return "AHIC_OutputVolumeLeft_Query";
//    case AHIC_OutputVolumeRight: return "AHIC_OutputVolumeRight";
//    case AHIC_OutputVolumeRight_Query: return "AHIC_OutputVolumeRight_Query";
//    case AHIC_InputGainLeft: return "AHIC_InputGainLeft";
//    case AHIC_InputGainLeft_Query: return "AHIC_InputGainLeft_Query";
//    case AHIC_InputGainRight: return "AHIC_InputGainRight";
//    case AHIC_InputGainRight_Query: return "AHIC_InputGainRight_Query";
//    case AHIC_PlaySampleFormat: return "AHIC_PlaySampleFormat";
//    case AHIC_PlaySampleFormat_Query: return "AHIC_PlaySampleFormat_Query";
//    case AHIC_RecordSampleFormat: return "AHIC_RecordSampleFormat";
//    case AHIC_RecordSampleFormat_Query: return "AHIC_RecordSampleFormat_Query";
    case AHIDB_AudioID: return "AHIDB_AudioID";
    case AHIDB_BufferLen: return "AHIDB_BufferLen";
    case AHIDB_Driver: return "AHIDB_Driver";
    case AHIDB_Author: return "AHIDB_Author";
    case AHIDB_Copyright: return "AHIDB_Copyright";
    case AHIDB_Version: return "AHIDB_Version";
    case AHIDB_Annotation: return "AHIDB_Annotation";
    case AHIDB_Name: return "AHIDB_Name";
    case AHIDB_Data: return "AHIDB_Data";
    case AHIDB_Flags: return "AHIDB_Flags";
//    case AHIDB_Play: return "AHIDB_Play";
    case AHIDB_Record: return "AHIDB_Record";
//    case AHIDB_Direct: return "AHIDB_Direct";
    case AHIDB_Volume: return "AHIDB_Volume";
    case AHIDB_Stereo: return "AHIDB_Stereo";
    case AHIDB_Panning: return "AHIDB_Panning";
//    case AHIDB_Surround: return "AHIDB_Surround";
    case AHIDB_PingPong: return "AHIDB_PingPong";
    case AHIDB_MultTable: return "AHIDB_MultTable";
    case AHIDB_MaxChannels: return "AHIDB_MaxChannels";
    case AHIDB_MaxPlaySamples: return "AHIDB_MaxPlaySamples";
    case AHIDB_MaxRecordSamples: return "AHIDB_MaxRecordSamples";
    case AHIDB_Bits: return "AHIDB_Bits";
    case AHIDB_HiFi: return "AHIDB_HiFi";
    case AHIDB_Realtime: return "AHIDB_Realtime";
    case AHIDB_FullDuplex: return "AHIDB_FullDuplex";
//    case AHIDB_Accelerated: return "AHIDB_Accelerated";
//    case AHIDB_Available: return "AHIDB_Available";
//    case AHIDB_Hidden: return "AHIDB_Hidden";
    case AHIDB_Frequencies: return "AHIDB_Frequencies";
    case AHIDB_FrequencyArg: return "AHIDB_FrequencyArg";
    case AHIDB_Frequency: return "AHIDB_Frequency";
//    case AHIDB_FrequencyArray: return "AHIDB_FrequencyArray";
    case AHIDB_IndexArg: return "AHIDB_IndexArg";
    case AHIDB_Index: return "AHIDB_Index";
    case AHIDB_Inputs: return "AHIDB_Inputs";
    case AHIDB_InputArg: return "AHIDB_InputArg";
    case AHIDB_Input: return "AHIDB_Input";
//    case AHIDB_InputArray: return "AHIDB_InputArray";
    case AHIDB_Outputs: return "AHIDB_Outputs";
    case AHIDB_OutputArg: return "AHIDB_OutputArg";
    case AHIDB_Output: return "AHIDB_Output";
    case AHIDB_MultiChannel: return "AHIDB_MultiChannel";
//    case AHIDB_OutputArray: return "AHIDB_OutputArray";
//    case AHIDB_MonitorVolumesLeft: return "AHIDB_MonitorVolumesLeft";
//    case AHIDB_MonitorVolumeLeftArg: return "AHIDB_MonitorVolumeLeftArg";
//    case AHIDB_MonitorVolumeLeft: return "AHIDB_MonitorVolumeLeft";
//    case AHIDB_MonitorVolumeLeftArray: return "AHIDB_MonitorVolumeLeftArray";
//    case AHIDB_MonitorVolumesRight: return "AHIDB_MonitorVolumesRight";
//    case AHIDB_MonitorVolumeRightArg: return "AHIDB_MonitorVolumeRightArg";
//    case AHIDB_MonitorVolumeRight: return "AHIDB_MonitorVolumeRight";
//    case AHIDB_MonitorVolumeRightArray: return "AHIDB_MonitorVolumeRightArray";
//    case AHIDB_OutputVolumesLeft: return "AHIDB_OutputVolumesLeft";
//    case AHIDB_OutputVolumeLeftArg: return "AHIDB_OutputVolumeLeftArg";
//    case AHIDB_OutputVolumeLeft: return "AHIDB_OutputVolumeLeft";
//    case AHIDB_OutputVolumeLeftArray: return "AHIDB_OutputVolumeLeftArray";
//    case AHIDB_OutputVolumesRight: return "AHIDB_OutputVolumesRight";
//    case AHIDB_OutputVolumeRightArg: return "AHIDB_OutputVolumeRightArg";
//    case AHIDB_OutputVolumeRight: return "AHIDB_OutputVolumeRight";
//    case AHIDB_OutputVolumeRightArray: return "AHIDB_OutputVolumeRightArray";
//    case AHIDB_InputGainsLeft: return "AHIDB_InputGainsLeft";
//    case AHIDB_InputGainLeftArg: return "AHIDB_InputGainLeftArg";
//    case AHIDB_InputGainLeft: return "AHIDB_InputGainLeft";
//    case AHIDB_InputGainLeftArray: return "AHIDB_InputGainLeftArray";
//    case AHIDB_InputGainsRight: return "AHIDB_InputGainsRight";
//    case AHIDB_InputGainRightArg: return "AHIDB_InputGainRightArg";
//    case AHIDB_InputGainRight: return "AHIDB_InputGainRight";
//    case AHIDB_InputGainRightArray: return "AHIDB_InputGainRightArray";
//    case AHIDB_PlaySampleFormats: return "AHIDB_PlaySampleFormats";
//    case AHIDB_PlaySampleFormatArg: return "AHIDB_PlaySampleFormatArg";
//    case AHIDB_PlaySampleFormat: return "AHIDB_PlaySampleFormat";
//    case AHIDB_PlaySampleFormatArray: return "AHIDB_PlaySampleFormatArray";
//    case AHIDB_RecordSampleFormats: return "AHIDB_RecordSampleFormats";
//    case AHIDB_RecordSampleFormatArg: return "AHIDB_RecordSampleFormatArg";
//    case AHIDB_RecordSampleFormat: return "AHIDB_RecordSampleFormat";
//    case AHIDB_RecordSampleFormatArray: return "AHIDB_RecordSampleFormatArray";
    case AHIB_Dizzy: return "AHIB_Dizzy";
    case AHIR_Window: return "AHIR_Window";
    case AHIR_Screen: return "AHIR_Screen";
    case AHIR_PubScreenName: return "AHIR_PubScreenName";
    case AHIR_PrivateIDCMP: return "AHIR_PrivateIDCMP";
    case AHIR_IntuiMsgFunc: return "AHIR_IntuiMsgFunc";
    case AHIR_SleepWindow: return "AHIR_SleepWindow";
    case AHIR_ObsoleteUserData: return "AHIR_ObsoleteUserData";
    case AHIR_UserData: return "AHIR_UserData";
    case AHIR_TextAttr: return "AHIR_TextAttr";
    case AHIR_Locale: return "AHIR_Locale";
    case AHIR_TitleText: return "AHIR_TitleText";
    case AHIR_PositiveText: return "AHIR_PositiveText";
    case AHIR_NegativeText: return "AHIR_NegativeText";
    case AHIR_InitialLeftEdge: return "AHIR_InitialLeftEdge";
    case AHIR_InitialTopEdge: return "AHIR_InitialTopEdge";
    case AHIR_InitialWidth: return "AHIR_InitialWidth";
    case AHIR_InitialHeight: return "AHIR_InitialHeight";
    case AHIR_InitialAudioID: return "AHIR_InitialAudioID";
    case AHIR_InitialMixFreq: return "AHIR_InitialMixFreq";
    case AHIR_InitialInfoOpened: return "AHIR_InitialInfoOpened";
    case AHIR_InitialInfoLeftEdge: return "AHIR_InitialInfoLeftEdge";
    case AHIR_InitialInfoTopEdge: return "AHIR_InitialInfoTopEdge";
    case AHIR_InitialInfoWidth: return "AHIR_InitialInfoWidth";
    case AHIR_InitialInfoHeight: return "AHIR_InitialInfoHeight";
    case AHIR_DoMixFreq: return "AHIR_DoMixFreq";
    case AHIR_DoDefaultMode: return "AHIR_DoDefaultMode";
//    case AHIR_DoChannels: return "AHIR_DoChannels";
//    case AHIR_DoHidden: return "AHIR_DoHidden";
//    case AHIR_DoDirectModes: return "AHIR_DoDirectModes";
    case AHIR_FilterTags: return "AHIR_FilterTags";
    case AHIR_FilterFunc: return "AHIR_FilterFunc";
    case AHIC_MonitorVolume: return "AHIC_MonitorVolume";
    case AHIC_MonitorVolume_Query: return "AHIC_MonitorVolume_Query";
    case AHIC_InputGain: return "AHIC_InputGain";
    case AHIC_InputGain_Query: return "AHIC_InputGain_Query";
    case AHIC_OutputVolume: return "AHIC_OutputVolume";
    case AHIC_OutputVolume_Query: return "AHIC_OutputVolume_Query";
    case AHIDB_MinMixFreq: return "AHIDB_MinMixFreq";
    case AHIDB_MaxMixFreq: return "AHIDB_MaxMixFreq";
    case AHIDB_MinMonitorVolume: return "AHIDB_MinMonitorVolume";
    case AHIDB_MaxMonitorVolume: return "AHIDB_MaxMonitorVolume";
    case AHIDB_MinInputGain: return "AHIDB_MinInputGain";
    case AHIDB_MaxInputGain: return "AHIDB_MaxInputGain";
    case AHIDB_MinOutputVolume: return "AHIDB_MinOutputVolume";
    case AHIDB_MaxOutputVolume: return "AHIDB_MaxOutputVolume";

    default:
      return "Unknown";
  }
}

enum Datatype
{
  dt_Hex,
  dt_Dec,
  dt_Boolean,
  dt_String,
  dt_Fixed
};

static enum Datatype
GetDatatype( Tag tag )
{
  switch( tag )
  {
    case AHIA_AudioID: return dt_Hex;
    case AHIA_MixFreq: return dt_Dec;
    case AHIA_Channels: return dt_Dec;
    case AHIA_Sounds: return dt_Dec;
    case AHIA_SoundFunc: return dt_Hex;
    case AHIA_PlayerFunc: return dt_Hex;
    case AHIA_PlayerFreq: return dt_Fixed;
    case AHIA_MinPlayerFreq: return dt_Fixed;
    case AHIA_MaxPlayerFreq: return dt_Fixed;
//    case AHIA_PlayerFreqUnit: return dt_Hex;
    case AHIA_RecordFunc: return dt_Hex;
    case AHIA_UserData: return dt_Hex;
//    case AHIA_ErrorFunc: return dt_Hex;
    case AHIA_AntiClickSamples: return dt_Dec;
    case AHIP_BeginChannel: return dt_Dec;
    case AHIP_EndChannel: return dt_Dec;
    case AHIP_Freq: return dt_Dec;
    case AHIP_Vol: return dt_Fixed;
    case AHIP_Pan: return dt_Fixed;
    case AHIP_Sound: return dt_Dec;
    case AHIP_Offset: return dt_Hex;
    case AHIP_Length: return dt_Hex;
    case AHIP_LoopFreq: return dt_Dec;
    case AHIP_LoopVol: return dt_Fixed;
    case AHIP_LoopPan: return dt_Fixed;
    case AHIP_LoopSound: return dt_Dec;
    case AHIP_LoopOffset: return dt_Hex;
    case AHIP_LoopLength: return dt_Hex;
    case AHIC_Play: return dt_Boolean;
    case AHIC_Record: return dt_Boolean;
//    case AHIC_PausePlay: return dt_Boolean;
//    case AHIC_PauseRecord: return dt_Boolean;
    case AHIC_MixFreq_Query: return dt_Hex;
    case AHIC_Input: return dt_Dec;
    case AHIC_Input_Query: return dt_Hex;
    case AHIC_Output: return dt_Dec;
    case AHIC_Output_Query: return dt_Hex;
//    case AHIC_MonitorVolumeLeft: return dt_Fixed;
//    case AHIC_MonitorVolumeLeft_Query: return dt_Hex;
//    case AHIC_MonitorVolumeRight: return dt_Fixed;
//    case AHIC_MonitorVolumeRight_Query: return dt_Hex;
//    case AHIC_OutputVolumeLeft: return dt_Fixed;
//    case AHIC_OutputVolumeLeft_Query: return dt_Hex;
//    case AHIC_OutputVolumeRight: return dt_Fixed;
//    case AHIC_OutputVolumeRight_Query: return dt_Hex;
//    case AHIC_InputGainLeft: return dt_Fixed;
//    case AHIC_InputGainLeft_Query: return dt_Hex;
//    case AHIC_InputGainRight: return dt_Fixed;
//    case AHIC_InputGainRight_Query: return dt_Hex;
//    case AHIC_PlaySampleFormat: return dt_Hex;
//    case AHIC_PlaySampleFormat_Query: return dt_Hex;
//    case AHIC_RecordSampleFormat: return dt_Hex;
//    case AHIC_RecordSampleFormat_Query: return dt_Dec;
    case AHIDB_AudioID: return dt_Hex;
    case AHIDB_BufferLen: return dt_Dec;
    case AHIDB_Driver: return dt_Hex;
    case AHIDB_Author: return dt_Hex;
    case AHIDB_Copyright: return dt_Hex;
    case AHIDB_Version: return dt_Hex;
    case AHIDB_Annotation: return dt_Hex;
    case AHIDB_Name: return dt_Hex;
    case AHIDB_Data: return dt_Hex;
    case AHIDB_Flags: return dt_Hex;
//    case AHIDB_Play: return dt_Hex;
    case AHIDB_Record: return dt_Hex;
//    case AHIDB_Direct: return dt_Hex;
    case AHIDB_Volume: return dt_Hex;
    case AHIDB_Stereo: return dt_Hex;
    case AHIDB_Panning: return dt_Hex;
//    case AHIDB_Surround: return dt_Hex;
    case AHIDB_PingPong: return dt_Hex;
    case AHIDB_MultTable: return dt_Hex;
    case AHIDB_MaxChannels: return dt_Hex;
    case AHIDB_MaxPlaySamples: return dt_Hex;
    case AHIDB_MaxRecordSamples: return dt_Hex;
    case AHIDB_Bits: return dt_Hex;
    case AHIDB_HiFi: return dt_Hex;
    case AHIDB_Realtime: return dt_Hex;
    case AHIDB_FullDuplex: return dt_Hex;
//    case AHIDB_Accelerated: return dt_Hex;
//    case AHIDB_Available: return dt_Hex;
//    case AHIDB_Hidden: return dt_Hex;
    case AHIDB_Frequencies: return dt_Hex;
    case AHIDB_FrequencyArg: return dt_Dec;
    case AHIDB_Frequency: return dt_Hex;
//    case AHIDB_FrequencyArray: return dt_Hex;
    case AHIDB_IndexArg: return dt_Dec;
    case AHIDB_Index: return dt_Hex;
    case AHIDB_Inputs: return dt_Hex;
    case AHIDB_InputArg: return dt_Dec;
    case AHIDB_Input: return dt_Hex;
//    case AHIDB_InputArray: return dt_Hex;
    case AHIDB_Outputs: return dt_Hex;
    case AHIDB_OutputArg: return dt_Dec;
    case AHIDB_Output: return dt_Hex;
    case AHIDB_MultiChannel: return dt_Hex;
//    case AHIDB_OutputArray: return dt_Hex;
//    case AHIDB_MonitorVolumesLeft: return dt_Hex;
//    case AHIDB_MonitorVolumeLeftArg: return dt_Dec;
//    case AHIDB_MonitorVolumeLeft: return dt_Hex;
//    case AHIDB_MonitorVolumeLeftArray: return dt_Hex;
//    case AHIDB_MonitorVolumesRight: return dt_Hex;
//    case AHIDB_MonitorVolumeRightArg: return dt_Dec;
//    case AHIDB_MonitorVolumeRight: return dt_Hex;
//    case AHIDB_MonitorVolumeRightArray: return dt_Hex;
//    case AHIDB_OutputVolumesLeft: return dt_Hex;
//    case AHIDB_OutputVolumeLeftArg: return dt_Dec;
//    case AHIDB_OutputVolumeLeft: return dt_Hex;
//    case AHIDB_OutputVolumeLeftArray: return dt_Hex;
//    case AHIDB_OutputVolumesRight: return dt_Hex;
//    case AHIDB_OutputVolumeRightArg: return dt_Dec;
//    case AHIDB_OutputVolumeRight: return dt_Hex;
//    case AHIDB_OutputVolumeRightArray: return dt_Hex;
//    case AHIDB_InputGainsLeft: return dt_Hex;
//    case AHIDB_InputGainLeftArg: return dt_Dec;
//    case AHIDB_InputGainLeft: return dt_Hex;
//    case AHIDB_InputGainLeftArray: return dt_Hex;
//    case AHIDB_InputGainsRight: return dt_Hex;
//    case AHIDB_InputGainRightArg: return dt_Dec;
//    case AHIDB_InputGainRight: return dt_Hex;
//    case AHIDB_InputGainRightArray: return dt_Hex;
//    case AHIDB_PlaySampleFormats: return dt_Hex;
//    case AHIDB_PlaySampleFormatArg: return dt_Dec;
//    case AHIDB_PlaySampleFormat: return dt_Hex;
//    case AHIDB_PlaySampleFormatArray: return dt_Hex;
//    case AHIDB_RecordSampleFormats: return dt_Hex;
//    case AHIDB_RecordSampleFormatArg: return dt_Dec;
//    case AHIDB_RecordSampleFormat: return dt_Hex;
//    case AHIDB_RecordSampleFormatArray: return dt_Hex;
    case AHIB_Dizzy: return dt_Boolean;
    case AHIR_Window: return dt_Hex;
    case AHIR_Screen: return dt_Hex;
    case AHIR_PubScreenName: return dt_String;
    case AHIR_PrivateIDCMP: return dt_Hex;
    case AHIR_IntuiMsgFunc: return dt_Hex;
    case AHIR_SleepWindow: return dt_Boolean;
    case AHIR_ObsoleteUserData: return dt_Hex;
    case AHIR_UserData: return dt_Hex;
    case AHIR_TextAttr: return dt_Hex;
    case AHIR_Locale: return dt_Hex;
    case AHIR_TitleText: return dt_String;
    case AHIR_PositiveText: return dt_String;
    case AHIR_NegativeText: return dt_String;
    case AHIR_InitialLeftEdge: return dt_Dec;
    case AHIR_InitialTopEdge: return dt_Dec;
    case AHIR_InitialWidth: return dt_Dec;
    case AHIR_InitialHeight: return dt_Dec;
    case AHIR_InitialAudioID: return dt_Hex;
    case AHIR_InitialMixFreq: return dt_Dec;
    case AHIR_InitialInfoOpened: return dt_Boolean;
    case AHIR_InitialInfoLeftEdge: return dt_Dec;
    case AHIR_InitialInfoTopEdge: return dt_Dec;
    case AHIR_InitialInfoWidth: return dt_Dec;
    case AHIR_InitialInfoHeight: return dt_Dec;
    case AHIR_DoMixFreq: return dt_Boolean;
    case AHIR_DoDefaultMode: return dt_Boolean;
//    case AHIR_DoChannels: return dt_Boolean;
//    case AHIR_DoHidden: return dt_Boolean;
//    case AHIR_DoDirectModes: return dt_Boolean;
    case AHIR_FilterTags: return dt_Hex;
    case AHIR_FilterFunc: return dt_Hex;
    case AHIC_MonitorVolume: return dt_Fixed;
    case AHIC_MonitorVolume_Query: return dt_Hex;
    case AHIC_InputGain: return dt_Fixed;
    case AHIC_InputGain_Query: return dt_Hex;
    case AHIC_OutputVolume: return dt_Fixed;
    case AHIC_OutputVolume_Query: return dt_Hex;
    case AHIDB_MinMixFreq: return dt_Dec;
    case AHIDB_MaxMixFreq: return dt_Dec;
    case AHIDB_MinMonitorVolume: return dt_Hex;
    case AHIDB_MaxMonitorVolume: return dt_Hex;
    case AHIDB_MinInputGain: return dt_Hex;
    case AHIDB_MaxInputGain: return dt_Hex;
    case AHIDB_MinOutputVolume: return dt_Hex;
    case AHIDB_MaxOutputVolume: return dt_Hex;

    default:
      return dt_Hex;
  }
}


// tags may be NULL

static void
PrintTagList(struct TagItem *tags)
{
  struct TagItem *tstate;
  struct TagItem *tag;

  if( tags == NULL )
  {
    KPrintF( "No taglist!\n" );
  }
  else
  {
    tstate = tags;

    while( ( tag = NextTagItem( &tstate ) ) )
    {
      switch( GetDatatype( tag->ti_Tag ) )
      {
        case dt_Hex:
          KPrintF( "\n  %30s, 0x%P,", 
                   GetTagName( tag->ti_Tag ), tag->ti_Data );
          break;

        case dt_Dec:
          KPrintF( "\n  %30s, %ld,", 
                   GetTagName( tag->ti_Tag ), tag->ti_Data );
          break;

        case dt_Boolean:
          KPrintF( "\n  %30s, %s,", 
                   GetTagName( tag->ti_Tag ), 
                   tag->ti_Data ? "TRUE" : "FALSE" );
          break;

        case dt_String:
          KPrintF( "\n  %30s, %s,", 
                   GetTagName( tag->ti_Tag ),
		   tag->ti_Data != 0 ? tag->ti_Data : "(null)" );
          break;

        case dt_Fixed:
        {
          KPrintF( "\n  %30s, %ld.%ld,", 
                   GetTagName( tag->ti_Tag ), 
                   tag->ti_Data >> 16,
                   ( ( tag->ti_Data & 0xffff ) * 1000 ) >> 16 );
          break;
        }
      }
    }

    KPrintF("\n  TAG_DONE)");
  }
}


/******************************************************************************
** Send debug to serial port **************************************************
******************************************************************************/

#ifndef __AMIGAOS4__
#if defined( __AROS__ ) && !defined( __mc68000__ )

#include <aros/asmcall.h>

AROS_UFH2( void,
	   rawputchar_m68k,
	   AROS_UFHA( UBYTE,            c,       D0 ),
	   AROS_UFHA( struct ExecBase*, SysBase, A3 ) )
{
  AROS_USERFUNC_INIT
  RawPutChar( c );
  AROS_USERFUNC_EXIT  
}

#else

static const UWORD rawputchar_m68k[] = 
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};

#endif

void
KPrintFArgs( UBYTE* fmt, 
             ULONG* args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}
#endif


/******************************************************************************
** All functions **************************************************************
******************************************************************************/


void
Debug_AllocAudioA( struct TagItem *tags )
{
  KPrintF("AHI_AllocAudioA(");
  PrintTagList(tags);
}

void
Debug_FreeAudio( struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_FreeAudio(0x%P)\n", audioctrl);
}

void
Debug_KillAudio( void )
{
  KPrintF("AHI_KillAudio()\n");
}

void
Debug_ControlAudioA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_ControlAudioA(0x%P,", audioctrl);
  PrintTagList(tags);
}


void
Debug_SetVol( UWORD chan, Fixed vol, sposition pan, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetVol(%ld, 0x%08lx, 0x%08lx, 0x%P, %ld)\n",
      chan & 0xffff, vol, pan, audioctrl, flags);
}

void
Debug_SetFreq( UWORD chan, ULONG freq, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetFreq(%ld, %ld, 0x%P, %ld)\n",
      chan & 0xffff, freq, audioctrl, flags);
}

void
Debug_SetSound( UWORD chan, UWORD sound, ULONG offset, LONG length, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetSound(%ld, %ld, 0x%08lx, 0x%08lx, 0x%P, %ld)\n",
      chan & 0xffff, sound & 0xffff, offset, length, audioctrl, flags);
}

void
Debug_SetEffect( ULONG *effect, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_SetEffect(0x%08lx (Effect 0x%P), 0x%P)\n",
      effect, *effect, audioctrl);
}

void
Debug_LoadSound( UWORD sound, ULONG type, APTR info, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_LoadSound(%ld, %ld, 0x%08lx, 0x%08lx) ", sound, type, (ULONG) info, (ULONG) audioctrl);

  if(type == AHIST_SAMPLE || type == AHIST_DYNAMICSAMPLE)
  {
    struct AHISampleInfo *si = (struct AHISampleInfo *) info;

    KPrintF("[T:0x%08lx A:0x%P L:%ld]", si->ahisi_Type, si->ahisi_Address, si->ahisi_Length);
  }
}

void
Debug_UnloadSound( UWORD sound, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_UnloadSound(%ld, 0x%P)\n", sound, audioctrl);
}

void
Debug_NextAudioID( ULONG id)
{
  KPrintF("AHI_NextAudioID(0x%08lx)",id);
}

void
Debug_GetAudioAttrsA( ULONG id, struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_GetAudioAttrsA(0x%08lx, 0x%P,",id, audioctrl);
  PrintTagList(tags);
}

void
Debug_BestAudioIDA( struct TagItem *tags )
{
  KPrintF("AHI_BestAudioIDA(");
  PrintTagList(tags);
}

void
Debug_AllocAudioRequestA( struct TagItem *tags )
{
  KPrintF("AHI_AllocAudioRequestA(");
  PrintTagList(tags);
}

void
Debug_AudioRequestA( struct AHIAudioModeRequester *req, struct TagItem *tags )
{
  KPrintF("AHI_AudioRequestA(0x%P,", req);
  PrintTagList(tags);
}

void
Debug_FreeAudioRequest( struct AHIAudioModeRequester *req )
{
  KPrintF("AHI_FreeAudioRequest(0x%P)\n", req);
}

void
Debug_PlayA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_PlayA(0x%P,", audioctrl);
  PrintTagList(tags);
  KPrintF("\n");
}

void
Debug_SampleFrameSize( ULONG sampletype)
{
  KPrintF("AHI_SampleFrameSize(%ld)",sampletype);
}

void
Debug_AddAudioMode(struct TagItem *tags )
{
  KPrintF("AHI_AddAudioMode(");
  PrintTagList(tags);
}

void
Debug_RemoveAudioMode( ULONG id)
{
  KPrintF("AHI_RemoveAudioMode(0x%08lx)",id);
}

void
Debug_LoadModeFile( STRPTR name)
{
  KPrintF("AHI_LoadModeFile(%s)", name);
}
