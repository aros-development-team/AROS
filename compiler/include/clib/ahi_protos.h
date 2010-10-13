/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_AHI_PROTOS_H
#define CLIB_AHI_PROTOS_H

/*
**	$VER: ahi_protos.h 5.3.2.2 (02.02.2005)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 1994-2005 Martin Blom
**	    All Rights Reserved
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <utility/tagitem.h>
#include <devices/ahi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Functions for allocating/deallocating and controlling AudioCtrl handles */
struct AHIAudioCtrl * AHI_AllocAudioA(struct TagItem * tagList);
struct AHIAudioCtrl * AHI_AllocAudio(Tag tag1, ...);
void AHI_FreeAudio(struct AHIAudioCtrl * AudioCtrl);
ULONG AHI_ControlAudioA(struct AHIAudioCtrl * AudioCtrl, struct TagItem * tagList);
ULONG AHI_ControlAudio(struct AHIAudioCtrl * AudioCtrl, Tag tag1, ...);

/* Functions to control the synthesizer */
void AHI_SetVol(UWORD Channel, Fixed Volume, sposition Pan, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
void AHI_SetFreq(UWORD Channel, ULONG Freq, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
void AHI_SetSound(UWORD Channel, UWORD Sound, ULONG Offset, LONG Length, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
ULONG AHI_SetEffect(APTR Effect, struct AHIAudioCtrl * AudioCtrl);
ULONG AHI_LoadSound(UWORD Sound, ULONG Type, APTR Info, struct AHIAudioCtrl * AudioCtrl);
void AHI_UnloadSound(UWORD Sound, struct AHIAudioCtrl * Audioctrl);
ULONG AHI_NextAudioID(ULONG Last_ID);

/* Functions to query the audio mode database */
BOOL AHI_GetAudioAttrsA(ULONG ID, struct AHIAudioCtrl * Audioctrl, struct TagItem * tagList);
BOOL AHI_GetAudioAttrs(ULONG ID, struct AHIAudioCtrl * Audioctrl, Tag tag1, ...);
ULONG AHI_BestAudioIDA(struct TagItem * tagList);
ULONG AHI_BestAudioID(Tag tag1, ...);

/* Functions for the audio mode requester */
struct AHIAudioModeRequester * AHI_AllocAudioRequestA(struct TagItem * tagList);
struct AHIAudioModeRequester * AHI_AllocAudioRequest(Tag tag1, ...);
BOOL AHI_AudioRequestA(struct AHIAudioModeRequester * Requester, struct TagItem * tagList);
BOOL AHI_AudioRequest(struct AHIAudioModeRequester * Requester, Tag tag1, ...);
void AHI_FreeAudioRequest(struct AHIAudioModeRequester * Requester);

/*--- functions in V4 or higher ---*/

/* More functions to control the synthesizer */
void AHI_PlayA(struct AHIAudioCtrl * Audioctrl, struct TagItem * tagList);
void AHI_Play(struct AHIAudioCtrl * Audioctrl, Tag tag1, ...);

/* Find out how many bytes a sample frame occupy */
ULONG AHI_SampleFrameSize(ULONG SampleType);

/* Semi-private functions to manage the audio mode database */
ULONG AHI_AddAudioMode(struct TagItem * AHIPrivate);
ULONG AHI_RemoveAudioMode(ULONG AHIPrivate);
ULONG AHI_LoadModeFile(STRPTR AHIPrivate);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_AHI_PROTOS_H */
