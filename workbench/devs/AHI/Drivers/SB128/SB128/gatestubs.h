/* Automatically generated header! Do not edit! */

#ifndef _GATEPROTO_AHIsub_H
#define _GATEPROTO_AHIsub_H

#include <dos/dos.h>
#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/ahi_sub.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __NOLIBBASE__
#include <proto/ahi_sub.h>
#undef __NOLIBBASE__
#include <stdarg.h>

struct Library*
gwLibInit(	struct Library* ___library,
	BPTR ___seglist,
	struct ExecIFace* _iface);

struct Library*
gwLibOpen(struct LibraryManagerInterface* _iface);

BPTR
gwLibClose(struct LibraryManagerInterface* _iface);

BPTR
gwLibExpunge(struct LibraryManagerInterface* _iface);

ULONG
gwLibNull(struct LibraryManagerInterface* _iface);

ULONG
gwAHIsub_AllocAudio(struct AHIsubIFace* _iface,
	struct TagItem * ___tagList,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

void
gwAHIsub_FreeAudio(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

void
gwAHIsub_Disable(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

void
gwAHIsub_Enable(struct AHIsubIFace* _iface,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_Start(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_Update(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_Stop(struct AHIsubIFace* _iface,
	ULONG ___Flags,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_SetVol(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	Fixed ___Volume,
	sposition ___Pan,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags);

ULONG
gwAHIsub_SetFreq(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	ULONG ___Freq,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags);

ULONG
gwAHIsub_SetSound(struct AHIsubIFace* _iface,
	UWORD ___Channel,
	UWORD ___Sound,
	ULONG ___Offset,
	LONG ___Length,
	struct AHIAudioCtrlDrv * ___AudioCtrl,
	ULONG ___Flags);

ULONG
gwAHIsub_SetEffect(struct AHIsubIFace* _iface,
	APTR ___Effect,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_LoadSound(struct AHIsubIFace* _iface,
	UWORD ___Sound,
	ULONG ___Type,
	APTR ___Info,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

ULONG
gwAHIsub_UnloadSound(struct AHIsubIFace* _iface,
	UWORD ___Sound,
	struct AHIAudioCtrlDrv * ___Audioctrl);

LONG
gwAHIsub_GetAttr(struct AHIsubIFace* _iface,
	ULONG ___Attribute,
	LONG ___Argument,
	LONG ___DefValue,
	struct TagItem * ___tagList,
	struct AHIAudioCtrlDrv * ___AudioCtrl);

LONG
gwAHIsub_HardwareControl(struct AHIsubIFace* _iface,
	ULONG ___Attribute,
	LONG ___Argument,
	struct AHIAudioCtrlDrv * ___AudioCtrl);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GATEPROTO_AHIsub_H */
