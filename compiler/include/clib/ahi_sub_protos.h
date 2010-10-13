/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_AHI_SUB_PROTOS_H
#define CLIB_AHI_SUB_PROTOS_H

/*
**	$VER: ahi_sub_protos.h 5.2.2.2 (02.02.2005)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 1994-2005 Martin Blom
**	    All Rights Reserved
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/ahi_sub.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Functions for basic audio input and output */
ULONG AHIsub_AllocAudio(struct TagItem * tagList, struct AHIAudioCtrlDrv * AudioCtrl);
void AHIsub_FreeAudio(struct AHIAudioCtrlDrv * AudioCtrl);
void AHIsub_Disable(struct AHIAudioCtrlDrv * AudioCtrl);
void AHIsub_Enable(struct AHIAudioCtrlDrv * AudioCtrl);
ULONG AHIsub_Start(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);
ULONG AHIsub_Update(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);
ULONG AHIsub_Stop(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);

/* Functions for hardware acceleration */
ULONG AHIsub_SetVol(UWORD Channel, Fixed Volume, sposition Pan, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
ULONG AHIsub_SetFreq(UWORD Channel, ULONG Freq, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
ULONG AHIsub_SetSound(UWORD Channel, UWORD Sound, ULONG Offset, LONG Length, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
ULONG AHIsub_SetEffect(APTR Effect, struct AHIAudioCtrlDrv * AudioCtrl);
ULONG AHIsub_LoadSound(UWORD Sound, ULONG Type, APTR Info, struct AHIAudioCtrlDrv * AudioCtrl);
ULONG AHIsub_UnloadSound(UWORD Sound, struct AHIAudioCtrlDrv * Audioctrl);

/* Functions for driver queries */
LONG AHIsub_GetAttr(ULONG Attribute, LONG Argument, LONG DefValue, struct TagItem * tagList, struct AHIAudioCtrlDrv * AudioCtrl);

/* Functions for controlling the analog mixer etc. */
LONG AHIsub_HardwareControl(ULONG Attribute, LONG Argument, struct AHIAudioCtrlDrv * AudioCtrl);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_AHI_SUB_PROTOS_H */
