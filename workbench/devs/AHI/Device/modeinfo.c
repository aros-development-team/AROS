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
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include "ahi_def.h"
#include "localize.h"
#include "modeinfo.h"
#include "audioctrl.h"
#include "database.h"
#include "debug.h"
#include "header.h"
#include "misc.h"


// Boolean comparison macros

#define XOR(a,b) ((a && !b) || (!a && b))
#define XNOR(a,b) (! XOR(a,b))


// NUL-terminating string copy

static int
stccpy( char *to, const char *from, int n )
{
  int i = 1;

  if( n == 0 ) return 0;

  while( *from && i < n )
  {
    *to++ = *from++;
    i++;
  }
  *to = '\0';
  
  return i;
}

/******************************************************************************
** DizzyTestAudioID & TestAudioID *********************************************
******************************************************************************/

// tags may be NULL

Fixed DizzyTestAudioID(ULONG id, struct TagItem *tags )
{
  ULONG volume=0,stereo=0,panning=0,hifi=0,pingpong=0,record=0,realtime=0,
        fullduplex=0,bits=0,channels=0,minmix=0,maxmix=0,multichannel=0;
  ULONG total=0,hits=0;
  struct TagItem *tstate, *tag;
  
  if(tags == NULL)
  {
    return (Fixed) 0x10000;
  }

  if(id == AHI_DEFAULT_ID)
  {
    id = AHIBase->ahib_AudioMode;
  }

  AHI_GetAudioAttrs( id, NULL,
                     AHIDB_Volume,       &volume,
                     AHIDB_Stereo,       &stereo,
                     AHIDB_Panning,      &panning,
                     AHIDB_MultiChannel, &multichannel,
                     AHIDB_HiFi,         &hifi,
                     AHIDB_PingPong,     &pingpong,
                     AHIDB_Record,       &record,
                     AHIDB_Bits,         &bits,
                     AHIDB_MaxChannels,  &channels,
                     AHIDB_MinMixFreq,   &minmix,
                     AHIDB_MaxMixFreq,   &maxmix,
                     AHIDB_Realtime,     &realtime,
                     AHIDB_FullDuplex,   &fullduplex,
                     TAG_DONE );

  tstate = tags;

  while ((tag = NextTagItem(&tstate)))
  {
    switch (tag->ti_Tag)
    {
      // Check source mode

      case AHIDB_AudioID:
	// Give two points for this
        total+=2;
        if( ((tag->ti_Data)&0xffff0000) == (id & 0xffff0000) )
          hits+=2;
        break;

      // Boolean tags

      case AHIDB_Volume:
        total++;
        if(XNOR(tag->ti_Data, volume))
          hits++;
        break;

      case AHIDB_Stereo:
        total++;
        if(XNOR(tag->ti_Data, stereo))
          hits++;
        break;
      case AHIDB_Panning:
        total++;
        if(XNOR(tag->ti_Data, panning))
          hits++;
        break;
      case AHIDB_MultiChannel:
        total++;
        if(XNOR(tag->ti_Data, multichannel))
          hits++;
        break;
      case AHIDB_HiFi:
        total++;
        if(XNOR(tag->ti_Data, hifi))
          hits++;
        break;
      case AHIDB_PingPong:
        total++;
        if(XNOR(tag->ti_Data, pingpong))
          hits++;
        break;
      case AHIDB_Record:
        total++;
        if(XNOR(tag->ti_Data, record))
          hits++;
        break;
      case AHIDB_Realtime:
        total++;
        if(XNOR(tag->ti_Data, realtime))
          hits++;
        break;
      case AHIDB_FullDuplex:
        total++;
        if(XNOR(tag->ti_Data, fullduplex))
          hits++;
        break;

      // The rest

      case AHIDB_Bits:
        total++;
        if(tag->ti_Data <= bits)
          hits++;
        break;
      case AHIDB_MaxChannels:
        total++;
        if(tag->ti_Data <= channels )
          hits++;
        break;
      case AHIDB_MinMixFreq:
        total++;
        if(tag->ti_Data >= minmix)
          hits++;
        break;
      case AHIDB_MaxMixFreq:
        total++;
        if(tag->ti_Data <= maxmix)
          hits++;
        break;
    } /* switch */
  } /* while */


  if(total)
    return (Fixed) ((hits<<16)/total);
  else
    return (Fixed) 0x10000;
}

// tags may be NULL

BOOL TestAudioID(ULONG id, struct TagItem *tags )
{
  if(DizzyTestAudioID(id, tags) != 0x10000)
    return FALSE;
  else
    return TRUE;
}


/******************************************************************************
** AHI_GetAudioAttrsA *********************************************************
******************************************************************************/

/****** ahi.device/AHI_GetAudioAttrsA ***************************************
*
*   NAME
*       AHI_GetAudioAttrsA -- examine an audio mode via a tag list
*       AHI_GetAudioAttrs -- varargs stub for AHI_GetAudioAttrsA()
*
*   SYNOPSIS
*       success = AHI_GetAudioAttrsA( ID, [audioctrl], tags );
*       D0                            D0  A2           A1
*
*       BOOL AHI_GetAudioAttrsA( ULONG, struct AHIAudioCtrl *,
*                                struct TagItem * );
*
*       success = AHI_GetAudioAttrs( ID, [audioctrl], attr1, &result1, ...);
*
*       BOOL AHI_GetAudioAttrs( ULONG, struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       Retrieve information about an audio mode specified by ID or audioctrl
*       according to the tags in the tag list. For each entry in the tag
*       list, ti_Tag identifies the attribute, and ti_Data is mostly a
*       pointer to a LONG (4 bytes) variable where you wish the result to be
*       stored.
*
*   INPUTS
*       ID - An audio mode identifier, AHI_DEFAULT_ID (V4) or AHI_INVALID_ID.
*       audioctrl - A pointer to an AHIAudioCtrl structure, only used if
*           ID equals AHI_INVALID_ID. Set to NULL if not used. If set to
*           NULL when used, this function returns immediately. Always set
*           ID to AHI_INVALID_ID and use audioctrl if you have allocated
*           a valid AHIAudioCtrl structure. Some of the tags return incorrect
*           values otherwise.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIDB_Volume (ULONG *) - TRUE if this mode supports volume changes.
*
*       AHIDB_Stereo (ULONG *) - TRUE if output is in stereo. Unless
*           AHIDB_Panning (see below) is TRUE, all even channels are played
*           to the left and all odd to the right.
*
*       AHIDB_MultiChannel (ULONG *) - TRUE if output is in 7.1 channels.
*
*       AHIDB_Panning (ULONG *) - TRUE if this mode supports stereo panning.
*
*       AHIDB_HiFi (ULONG *) - TRUE if no shortcuts, like pre-division, is
*           used by the mixing routines.
*
*       AHIDB_PingPong (ULONG *) - TRUE if this mode can play samples backwards.
*
*       AHIDB_Record (ULONG *) - TRUE if this mode can record samples.
*
*       AHIDB_FullDuplex (ULONG *) - TRUE if this mode can record and play at
*           the same time.
*
*       AHIDB_Realtime (ULONG *) - Modes which return TRUE for this fulfills
*           two criteria:
*           1) Calls to AHI_SetVol(), AHI_SetFreq() or AHI_SetSound() will be
*              performed within (about) 10 ms if called from a PlayFunc Hook.
*           2) The PlayFunc Hook will be called at the specified frequency.
*           If you don't use AHI's PlayFunc Hook, you must not use modes that
*           are not realtime. (Criterium 2 is not that obvious if you consider
*           a mode that renders the output to disk as a sample.)
*
*       AHIDB_Bits (ULONG *) - The number of output bits (8, 12, 14, 16 etc).
*
*       AHIDB_MaxChannels (ULONG *) - The maximum number of channels this mode
*           can handle.
*
*       AHIDB_MinMixFreq (ULONG *) - The minimum mixing frequency supported.
*
*       AHIDB_MaxMixFreq (ULONG *) - The maximum mixing frequency supported.
*
*       AHIDB_Frequencies (ULONG *) - The number of different sample rates
*           available.
*
*       AHIDB_FrequencyArg (ULONG) - Specifies which frequency
*           AHIDB_Frequency should return (see below). Range is 0 to
*           AHIDB_Frequencies-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Frequency (ULONG *) - Return the frequency associated with the
*           index number specified with AHIDB_FrequencyArg (see above).
*
*       AHIDB_IndexArg (ULONG) - AHIDB_Index will return the index which
*           gives the closest frequency to AHIDB_IndexArg
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Index (ULONG *) - Return the index associated with the frequency
*           specified with AHIDB_IndexArg (see above).
*
*       AHIDB_MaxPlaySamples (ULONG *) - Return the lowest number of sample
*           frames that must be present in memory when AHIST_DYNAMICSAMPLE
*           sounds are used. This number must then be scaled by Fs/Fm, where
*           Fs is the frequency of the sound and Fm is the mixing frequency.
*
*       AHIDB_MaxRecordSamples (ULONG *) - Return the number of sample frames
*           you will receive each time the RecordFunc is called.
*
*       AHIDB_BufferLen (ULONG) - Specifies how many characters will be
*           copied when requesting text attributes. Default is 0, which
*           means that AHIDB_Driver, AHIDB_Name, AHIDB_Author,
*           AHIDB_Copyright, AHIDB_Version and AHIDB_Annotation,
*           AHIDB_Input and AHIDB_Output will do nothing.
*
*       AHIDB_Driver (STRPTR) - Name of driver (excluding path and
*           extension). 
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Name (STRPTR) - Human readable name of this mode.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Author (STRPTR) - Name of driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Copyright (STRPTR) - Driver copyright notice.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen
*
*       AHIDB_Version (STRPTR) - Driver version string.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Annotation (STRPTR) - Annotation by driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_MinMonitorVolume (Fixed *)
*       AHIDB_MaxMonitorVolume (Fixed *) - Lower/upper limit for input
*           monitor volume, see AHI_ControlAudioA(). If both are 0.0,
*           the sound hardware does not have an input monitor feature.
*           If both are same, but not 0.0, the hardware always sends the
*           recorded sound to the outputs (at the given volume). (V2)
*
*       AHIDB_MinInputGain (Fixed *)
*       AHIDB_MaxInputGain (Fixed *) - Lower/upper limit for input gain,
*           see AHI_ControlAudioA(). If both are same, there is no input
*           gain hardware. (V2)
*
*       AHIDB_MinOutputVolume (Fixed *)
*       AHIDB_MaxOutputVolume (Fixed *) - Lower/upper limit for output
*           volume, see AHI_ControlAudioA(). If both are same, the sound
*           card does not have volume control. (V2)
*
*       AHIDB_Inputs (ULONG *) - The number of inputs the sound card has.
*           (V2)
*
*       AHIDB_InputArg (ULONG) - Specifies what AHIDB_Input should return
*           (see below). Range is 0 to AHIDB_Inputs-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Input (STRPTR) - Gives a human readable string describing the
*           input associated with the index specified with AHIDB_InputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       AHIDB_Outputs (ULONG *) - The number of outputs the sound card
*           has. (V2)
*
*       AHIDB_OutputArg (ULONG) - Specifies what AHIDB_Output should return
*           (see below). Range is 0 to AHIDB_Outputs-1 (including)
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Output (STRPTR) - Gives a human readable string describing the
*           output associated with the index specified with AHIDB_OutputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       AHIDB_AudioID (ULONG *) - The ID for this mode. (V4)
*
*       If the requested information cannot be found, the variable will be not
*       be touched.
*
*   RESULT
*       TRUE if everything went well.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       In versions earlier than 3, the tags that filled a string buffer would
*       not NULL-terminate the string on buffer overflows.
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_BestAudioIDA()
*
****************************************************************************
*
*/

ULONG
_AHI_GetAudioAttrsA( ULONG                    id,
		     struct AHIPrivAudioCtrl* actrl,
		     struct TagItem*          tags,
		     struct AHIBase*          AHIBase )
{
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags,*tag1,*tag2,*tstate=tags;
  ULONG *ptr;
  ULONG stringlen;
  struct Library *AHIsubBase=NULL;
  struct AHIAudioCtrlDrv *audioctrl=NULL;
  BOOL rc=TRUE; // TRUE == _everything_ went well
  struct TagItem idtag[2] = { {AHIA_AudioID, 0} , {TAG_DONE, 0} };

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    Debug_GetAudioAttrsA(id, actrl, tags);
  }

  if((audiodb=LockDatabase()))
  {
    if(id == AHI_INVALID_ID)
    {
      if(!(audioctrl= (struct AHIAudioCtrlDrv*) actrl))
        rc=FALSE;
      else
        idtag[0].ti_Data=((struct AHIPrivAudioCtrl *)actrl)->ahiac_AudioID;
    }
    else
    {
      idtag[0].ti_Data = (id == AHI_DEFAULT_ID ? AHIBase->ahib_AudioMode : id);
      audioctrl=(struct AHIAudioCtrlDrv *)CreateAudioCtrl(idtag);
    }

    if(audioctrl && rc )
    {
      if((dbtags=GetDBTagList(audiodb, idtag[0].ti_Data)))
      {
        stringlen=GetTagData(AHIDB_BufferLen,0,tags);
        if((AHIsubBase=OpenLibrary(((struct AHIPrivAudioCtrl *)audioctrl)->ahiac_DriverName,DriverVersion)))
        {
#ifdef __AMIGAOS4__
          struct AHIsubIFace *IAHIsub;
          if ((IAHIsub = (struct AHIsubIFace *) GetInterface((struct Library *) AHIsubBase, "main", 1, NULL)) != NULL)
          {
#endif

          while((tag1=NextTagItem(&tstate)))
          {
            ptr=(ULONG *)tag1->ti_Data;
            switch(tag1->ti_Tag)
            {
            case AHIDB_Driver:
            case AHIDB_Name:
              if((tag2=FindTagItem(tag1->ti_Tag,dbtags)))
                stccpy((char *)tag1->ti_Data,(char *)tag2->ti_Data,stringlen);
              break;
// Skip these!
            case AHIDB_FrequencyArg:
            case AHIDB_IndexArg:
            case AHIDB_InputArg:
            case AHIDB_OutputArg:
              break;
// Strings
            case AHIDB_Author:
            case AHIDB_Copyright:
            case AHIDB_Version:
            case AHIDB_Annotation:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,0, (IPTR)"",dbtags,audioctrl),stringlen);
              break;
// Input & Output strings
            case AHIDB_Input:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,
                  GetTagData(AHIDB_InputArg,0,tags),
                  (IPTR) GetahiString(msgDefault),dbtags,audioctrl),stringlen);
              break;
            case AHIDB_Output:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,
                  GetTagData(AHIDB_OutputArg,0,tags),
                  (IPTR) GetahiString(msgDefault),dbtags,audioctrl),stringlen);
              break;
// Other
            case AHIDB_Bits:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MaxChannels:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,128,dbtags,audioctrl);
              break;
            case AHIDB_MinMixFreq:
              *ptr=AHIsub_GetAttr(AHIDB_Frequency,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MaxMixFreq:
              *ptr=AHIsub_GetAttr(AHIDB_Frequency,(AHIsub_GetAttr(AHIDB_Frequencies,1,0,dbtags,audioctrl)-1),0,dbtags,audioctrl);
              break;
            case AHIDB_Frequencies:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,1,dbtags,audioctrl);
              break;
            case AHIDB_Frequency:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_FrequencyArg,0,tags),0,dbtags,audioctrl);
              break;
            case AHIDB_Index:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_IndexArg,0,tags),0,dbtags,audioctrl);
              break;
            case AHIDB_MaxPlaySamples:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,audioctrl->ahiac_MaxBuffSamples,dbtags,audioctrl);
              break;
            case AHIDB_MaxRecordSamples:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MinMonitorVolume:
            case AHIDB_MaxMonitorVolume:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0x00000,dbtags,audioctrl);
              break;
            case AHIDB_MinInputGain:
            case AHIDB_MaxInputGain:
            case AHIDB_MinOutputVolume:
            case AHIDB_MaxOutputVolume:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0x10000,dbtags,audioctrl);
              break;
            case AHIDB_Inputs:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_Outputs:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,1,dbtags,audioctrl);
              break;
// Booleans that defaults to FALSE
            case AHIDB_Realtime:
            case AHIDB_Record:
            case AHIDB_FullDuplex:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,FALSE,dbtags,audioctrl);
              break;
// Booleans that defaults to TRUE
            case AHIDB_PingPong:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,
				  audioctrl->ahiac_BuffType != AHIST_L7_1,
				  dbtags,audioctrl);
              break;
// Tags from the database.
            default:
              if((tag2=FindTagItem(tag1->ti_Tag,dbtags)))
                *ptr=tag2->ti_Data;
              break;
            }
          }
#ifdef __AMIGAOS4__
          if (IAHIsub) {
             DropInterface((struct Interface *) IAHIsub);
             IAHIsub = NULL;
             }
          }
#endif

        }
        else // no AHIsubBase
          rc=FALSE;
      }
      else // no database taglist
        rc=FALSE;
    }
    else // no valid audioctrl
       rc=FALSE;
    if(id != AHI_INVALID_ID)
      AHIFreeVec(audioctrl);
    if(AHIsubBase)
      CloseLibrary(AHIsubBase);
    UnlockDatabase(audiodb);
  }
  else // unable to lock database
    rc=FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%s\n", rc ? "TRUE" : "FALSE" );
  }

  return (ULONG) rc;
}


/******************************************************************************
** AHI_BestAudioIDA ***********************************************************
******************************************************************************/

/****** ahi.device/AHI_BestAudioIDA *****************************************
*
*   NAME
*       AHI_BestAudioIDA -- calculate the best ModeID with given parameters
*       AHI_BestAudioID -- varargs stub for AHI_BestAudioIDA()
*
*   SYNOPSIS
*       ID = AHI_BestAudioIDA( tags );
*       D0                     A1
*
*       ULONG AHI_BestAudioIDA( struct TagItem * );
*
*       ID = AHI_BestAudioID( tag1, ... );
*
*       ULONG AHI_BestAudioID( Tag, ... );
*
*   FUNCTION
*       Determines the best AudioID to fit the parameters set in the tag
*       list.
*
*   INPUTS
*       tags - A pointer to a tag list. Only the tags present matter.
*
*   TAGS
*       Many combinations are probably stupid to ask for, like not supporting
*       panning or recording.
*
*       AHIDB_AudioID (ULONG) - The mode must use the same audio hardware
*           as this mode does.
*
*       AHIDB_Volume (BOOL) - If TRUE: mode must support volume changes.
*           If FALSE: mode must not support volume changes.
*
*       AHIDB_Stereo (BOOL) - If TRUE: mode must have stereo output.
*           If FALSE: mode must not have stereo output (=mono).
*
*       AHIDB_MultiChannel (BOOL) - If TRUE: mode must have 7.1 channel output.
*           If FALSE: mode must not have 7.1 channel output (=mono or stereo).
*
*       AHIDB_Panning (BOOL) - If TRUE: mode must support volume panning.
*           If FALSE: mode must not support volume panning. 
*
*       AHIDB_HiFi (BOOL) - If TRUE: mode must have HiFi output.
*           If FALSE: mode must not have HiFi output.
*
*       AHIDB_PingPong (BOOL) - If TRUE: mode must support playing samples
*           backwards. If FALSE: mode must not support playing samples
*           backwards.
*
*       AHIDB_Record (BOOL) - If TRUE: mode must support recording. If FALSE:
*           mode must not support recording.
*
*       AHIDB_Realtime (BOOL) - If TRUE: mode must be realtime. If FALSE:
*           take a wild guess.
*
*       AHIDB_FullDuplex (BOOL) - If TRUE: mode must be able to record and
*           play at the same time.
*
*       AHIDB_Bits (UBYTE) - Mode must have greater or equal number of bits.
*
*       AHIDB_MaxChannels (UWORD) - Mode must have greater or equal number
*           of channels.
*
*       AHIDB_MinMixFreq (ULONG) - Lowest mixing frequency supported must be
*           less or equal.
*
*       AHIDB_MaxMixFreq (ULONG) - Highest mixing frequency must be greater
*           or equal.
*
*       AHIB_Dizzy (struct TagItem *) - This tag points to a second tag list.
*           After all other tags has been tested, the mode that matches these
*           tags best is returned, i.e. the one that has most of the features
*           you ask for, and least of the ones you don't want. Without this
*           second tag list, this function hardly does what its name
*           suggests. (V4)
*
*   RESULT
*       ID - The best AudioID to use or AHI_INVALID_ID if none of the modes
*           in the audio database could meet the requirements.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       Due to a bug in the code that compared the boolean tag values in
*       version 4.158 and earlier, TRUE must be equal to 1. The bug is not
*       present in later revisions.
*
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_GetAudioAttrsA()
*
****************************************************************************
*
*/

ULONG
_AHI_BestAudioIDA( struct TagItem* tags,
		   struct AHIBase* AHIBase )
{
  ULONG id = AHI_INVALID_ID, bestid = 0;
  Fixed score, bestscore = 0;
  struct TagItem *dizzytags;
  static const struct TagItem const_defdizzy[] =
  {
    { AHIDB_Volume,     TRUE },
    { AHIDB_Stereo,     TRUE },
    { AHIDB_MultiChannel, FALSE },
    { AHIDB_Panning,    TRUE },
    { AHIDB_HiFi,       TRUE  },
    { AHIDB_Realtime,   TRUE  },
    // And we don't care about the rest...
    { TAG_DONE,         0     }
  };
  
  const struct TagItem defdizzy[] =
  {
    // Give the user's preferred sound card extra points
    { AHIDB_AudioID,    AHIBase->ahib_AudioMode },
    { TAG_MORE,         (IPTR) &const_defdizzy }
  };

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_BestAudioIDA(tags);
  }

  dizzytags = (struct TagItem *) GetTagData(AHIB_Dizzy, (IPTR) defdizzy,tags);

  while(AHI_INVALID_ID != (id=AHI_NextAudioID(id)))
  {
    if(!TestAudioID(id, tags))
    {
      continue;
    }

    // Check if this id the better than the last one
    score = DizzyTestAudioID(id, dizzytags);
    if(score > bestscore)
    {
      bestscore = score;
      bestid = id;
    }
    else if(score == bestscore)
    {
      if(id > bestid)
      {
        bestid = id;    // Return the highest suitable audio id.
      }
    }
  }

  if(bestid == 0)
  {
    bestid = AHI_INVALID_ID;
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%08lx\n",bestid);
  }

  return bestid;
}

